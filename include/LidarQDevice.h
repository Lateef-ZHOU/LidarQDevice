#pragma once
#include <string>
#include <format>
#include "WindBarbItem.h"
#include "SurfaceItem.h"
#include "Wind2DItem.h"
#include <QDateTime>
#include <QtQml/qqmlregistration.h>
#include "DAQChartSpecification.h"
#include <memory>
#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QVariantMap>

#include "DAQFamily/DAQDevice.h"                  // 采集卡族两层（族基 DAQFamily + 族层 DAQDevice）
#include "PcapForm.h"
#include "LidarQRunSide.h"    // 运行侧：帧头布局 + 规格预设 + 收集器

// Qt 只登记具体结果元组；各项仍指向算法生成的原帧。
using LidarQ640Profiles = DAQDevice<PcapHandle, LidarQCollector<LidarQGeometry::Geo640x128>>::Profiles;
using LidarQ640Channels = DAQDevice<PcapHandle, LidarQCollector<LidarQGeometry::Geo640x128>>::Channels;
struct LidarQ640ChannelsQml {
    Q_GADGET
    QML_FOREIGN(LidarQ640Channels)
    QML_ANONYMOUS
};
struct LidarQ640ProfilesQml
{
    Q_GADGET
    QML_FOREIGN(LidarQ640Profiles)
    QML_ANONYMOUS
};

// Qt 只登记具体结果元组；各项仍指向算法生成的原帧。
using LidarQ450Profiles = DAQDevice<PcapHandle, LidarQCollector<LidarQGeometry::Geo450x200>>::Profiles;
using LidarQ450Channels = DAQDevice<PcapHandle, LidarQCollector<LidarQGeometry::Geo450x200>>::Channels;
struct LidarQ450ChannelsQml {
    Q_GADGET
    QML_FOREIGN(LidarQ450Channels)
    QML_ANONYMOUS
};
struct LidarQ450ProfilesQml
{
    Q_GADGET
    QML_FOREIGN(LidarQ450Profiles)
    QML_ANONYMOUS
};

// 国耀两种规格共用模板流程；Qt 属性与信号由非模板末端类声明。
// 末端同时是 Handle、Device 与采集卡族，唯一 QObject 来自 PcapHandle；处理、落盘与风廓线由族层完成。
template <LidarQGeometry Geometry>
class LidarQDevice : public DAQDevice<PcapHandle, LidarQCollector<Geometry>>
{
    using Base = DAQDevice<PcapHandle, LidarQCollector<Geometry>>;
public:
    [[nodiscard]] QVariantMap connectionParameters() const
    {
        return PcapForm::connectionParameters();
    }
    bool openInterface(const int interfaceIndex)
    {
        const auto params = PcapForm::parseParameters(*this, interfaceIndex,
            LidarQConfig::HostIPv4, LidarQConfig::HostPort, LidarQConfig::DeviceIPv4, LidarQConfig::DevicePort,
            LidarQConfig::DeviceMAC, Base::kPcapReadTimeout, Base::kPcapKernelBufferBytes);
        // 末端的 open 槽按名遮住 Handle::open；连接参数交给 Handle 本身。
        return params && PcapHandle::open(*params);
    }

    explicit LidarQDevice(QObject *parent)
        : Base("采集·国耀",
               std::format("{}×{}", Geometry.row, Geometry.col), parent)
    {
        this->setObjectName(QStringLiteral("LidarQ%1").arg(geo_.row));
    }

    ~LidarQDevice() override = default;

    // 型号判据：累积时间须在 国耀卡 的范围内，越界记录原因并返回空；不造 Collector、不发令。
    [[nodiscard]] std::optional<DAQParameters> paramsFor(int accumSec, std::optional<WorldModel> pose,
        std::optional<QDir> saveRoot, QString sessionDir, bool sync) const override
    {
        if (accumSec < LidarQConfig::AccumSecMin || accumSec > LidarQConfig::AccumSecMax)
        {
            Q_EMIT this->logMessage(std::format("累积时间 {} 秒超出本型号范围（{}~{} 秒），未启动", accumSec, LidarQConfig::AccumSecMin, LidarQConfig::AccumSecMax));
            return std::nullopt;
        }
        return DAQParameters(*this, accumSec, std::move(pose), std::move(saveRoot), std::move(sessionDir), sync);
    }
    // 本卡无中止协议，同 Hfnl。
    QList<QByteArray> stopCommands() const override { return {}; }
    // 型号模板共用参数生成；两个末端各自提供 Qt 元对象入口。
    [[nodiscard]] QVariantMap chartParameters() const
    {
        const auto plotConfiguration = DAQChartSpecification::make<LidarQChannelSpec<Geometry>>(
            -static_cast<int>(geo_.wsAbsMax()), static_cast<int>(geo_.wsAbsMax()), LidarQConfig::Plt3dValMax);
        return {{"plotConfiguration", plotConfiguration}};
    }

    [[nodiscard]] QVariantMap acquisitionParameters() const
    { return {{"accumMin", LidarQConfig::AccumSecMin}, {"accumMax", LidarQConfig::AccumSecMax},
              {"accumDefault", LidarQConfig::AccumSecDefault}, {"saveDirDefault", QUrl::fromLocalFile(QDir::currentPath())}}; }

    // 空闲时单独下发累积时间（空令牌：空闲时过，被任务锁着则拒绝）。
    bool sendParams(const int accumSec)
    {
        const auto command = LidarQConfig::accumulationCommand(accumSec);
        return this->send({{command.cbegin(), command.cend()}}, {});
    }
    void startNamedTest(const int accumSec, const bool save, const QUrl &folder)
    {
        Base::startTest(accumSec, save, folder, AcqSaver::sessionDir(
            QDateTime::currentDateTime(), this->objectName() + QStringLiteral("Test")));
    }

private:
    static constexpr LidarQGeometry geo_ = Geometry;
};

class LidarQ450x200Device final
#ifdef Q_MOC_RUN
    : public PcapHandle // 元对象基类：唯一 QObject 来自 Handle，模板层不加 Q_OBJECT。
#else
    : public LidarQDevice<LidarQGeometry::Geo450x200>
#endif
{
    Q_OBJECT
    Q_PROPERTY(std::string displayName MEMBER displayName CONSTANT)
    Q_PROPERTY(std::string subtitle MEMBER subtitle CONSTANT)
    Q_PROPERTY(bool isOpen READ isOpen)
    Q_PROPERTY(QVariant holder READ qmlHolder)
    Q_PROPERTY(QVariantMap connectionParameters READ connectionParameters CONSTANT)
    QML_ELEMENT
    QML_ATTACHED(LidarQ450x200Device)
    QML_UNCREATABLE("由对应 QML 官邸创建和拥有")
    Q_PROPERTY(QVariantMap acquisitionParameters READ acquisitionParameters CONSTANT)
    using Base = LidarQDevice<LidarQGeometry::Geo450x200>;
public:
    static LidarQ450x200Device *qmlAttachedProperties(QObject *owner) { return new LidarQ450x200Device(owner); }
    explicit LidarQ450x200Device(QObject *parent) : Base(parent) {}
    ~LidarQ450x200Device() override { PreDestruction(); }
    Q_INVOKABLE [[nodiscard]] QVariantMap chartParameters() const { return Base::chartParameters(); }
// 族信号按通道下标；本卡只有 0 = CDL。
Q_SIGNALS:
    void lockChanged(HandleCore::ResourcePhase phase) override;
    void channelsReady(const LidarQ450Channels &channels) override;
    void profilesReady(const LidarQ450Profiles &profiles) override;
public Q_SLOTS:
    bool open(const int interfaceIndex) { return openInterface(interfaceIndex); }
    void close() { PcapHandle::close(); }
    bool sendParams(const int accumSec) { return Base::sendParams(accumSec); }
    void startTest(const int accumSec, const bool save, const QUrl &folder) { startNamedTest(accumSec, save, folder); }
    void cancelTest() noexcept { Base::cancelTest(); }
private:
    void PreDestruction() noexcept final
    {
        static_assert(FinalDevice<LidarQ450x200Device, decltype(&LidarQ450x200Device::PreDestruction)>);
        Base::PreDestruction();
    }
};

class LidarQ640x128Device final
#ifdef Q_MOC_RUN
    : public PcapHandle // 元对象基类：唯一 QObject 来自 Handle，模板层不加 Q_OBJECT。
#else
    : public LidarQDevice<LidarQGeometry::Geo640x128>
#endif
{
    Q_OBJECT
    Q_PROPERTY(std::string displayName MEMBER displayName CONSTANT)
    Q_PROPERTY(std::string subtitle MEMBER subtitle CONSTANT)
    Q_PROPERTY(bool isOpen READ isOpen)
    Q_PROPERTY(QVariant holder READ qmlHolder)
    Q_PROPERTY(QVariantMap connectionParameters READ connectionParameters CONSTANT)
    QML_ELEMENT
    QML_ATTACHED(LidarQ640x128Device)
    QML_UNCREATABLE("由对应 QML 官邸创建和拥有")
    Q_PROPERTY(QVariantMap acquisitionParameters READ acquisitionParameters CONSTANT)
    using Base = LidarQDevice<LidarQGeometry::Geo640x128>;
public:
    static LidarQ640x128Device *qmlAttachedProperties(QObject *owner) { return new LidarQ640x128Device(owner); }
    explicit LidarQ640x128Device(QObject *parent) : Base(parent) {}
    ~LidarQ640x128Device() override { PreDestruction(); }
    Q_INVOKABLE [[nodiscard]] QVariantMap chartParameters() const { return Base::chartParameters(); }
// 族信号按通道下标；本卡只有 0 = CDL。
Q_SIGNALS:
    void lockChanged(HandleCore::ResourcePhase phase) override;
    void channelsReady(const LidarQ640Channels &channels) override;
    void profilesReady(const LidarQ640Profiles &profiles) override;
public Q_SLOTS:
    bool open(const int interfaceIndex) { return openInterface(interfaceIndex); }
    void close() { PcapHandle::close(); }
    bool sendParams(const int accumSec) { return Base::sendParams(accumSec); }
    void startTest(const int accumSec, const bool save, const QUrl &folder) { startNamedTest(accumSec, save, folder); }
    void cancelTest() noexcept { Base::cancelTest(); }
private:
    void PreDestruction() noexcept final
    {
        static_assert(FinalDevice<LidarQ640x128Device, decltype(&LidarQ640x128Device::PreDestruction)>);
        Base::PreDestruction();
    }
};

// 末端仅提供 Qt 入口；历史和绘制由 ChartCore 的模板实现。
class LidarQ450WindBarb
#ifdef Q_MOC_RUN
    : public WindBarbChart
#else
    : public WindBarbItem<typename std::tuple_element_t<0, LidarQ450Profiles>::element_type>
#endif
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit LidarQ450WindBarb(QQuickItem *parent = nullptr) : WindBarbItem(parent) {}
    Q_INVOKABLE void appendProfiles(const LidarQ450Profiles &profiles)
    { appendColumn(std::get<0>(profiles)); }
};

// 末端仅提供 Qt 入口；历史和绘制由 ChartCore 的模板实现。
class LidarQ640WindBarb
#ifdef Q_MOC_RUN
    : public WindBarbChart
#else
    : public WindBarbItem<typename std::tuple_element_t<0, LidarQ640Profiles>::element_type>
#endif
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit LidarQ640WindBarb(QQuickItem *parent = nullptr) : WindBarbItem(parent) {}
    Q_INVOKABLE void appendProfiles(const LidarQ640Profiles &profiles)
    { appendColumn(std::get<0>(profiles)); }
};

// 仅适配 Qt 元对象；类型与只读份额直达绘图数值入口。
class LidarQ450Spectrum : public SurfaceItem
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit LidarQ450Spectrum(QQuickItem *parent = nullptr) : SurfaceItem(parent) {}
    Q_INVOKABLE bool setData(const LidarQ450Channels &channels, double low, double high)
    {
        if (!channels) return false;
        return setSpectrum(std::get<0>(*channels).spectrum, low, high);
    }
};

// 仅适配 Qt 元对象；类型与只读份额直达绘图数值入口。
class LidarQ450RadialWind : public Wind2DItem
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit LidarQ450RadialWind(QQuickItem *parent = nullptr) : Wind2DItem(parent) {}
    Q_INVOKABLE void setData(const LidarQ450Channels &channels)
    {
        if (!channels) return;
        setRadialWind(std::get<0>(*channels).radialWind);
    }
};

// 仅适配 Qt 元对象；类型与只读份额直达绘图数值入口。
class LidarQ450CNR : public Wind2DItem
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit LidarQ450CNR(QQuickItem *parent = nullptr) : Wind2DItem(parent) {}
    Q_INVOKABLE void setData(const LidarQ450Channels &channels)
    {
        if (!channels) return;
        setCNR(std::get<0>(*channels).cnr);
    }
};

// 仅适配 Qt 元对象；类型与只读份额直达绘图数值入口。
class LidarQ640Spectrum : public SurfaceItem
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit LidarQ640Spectrum(QQuickItem *parent = nullptr) : SurfaceItem(parent) {}
    Q_INVOKABLE bool setData(const LidarQ640Channels &channels, double low, double high)
    {
        if (!channels) return false;
        return setSpectrum(std::get<0>(*channels).spectrum, low, high);
    }
};

// 仅适配 Qt 元对象；类型与只读份额直达绘图数值入口。
class LidarQ640RadialWind : public Wind2DItem
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit LidarQ640RadialWind(QQuickItem *parent = nullptr) : Wind2DItem(parent) {}
    Q_INVOKABLE void setData(const LidarQ640Channels &channels)
    {
        if (!channels) return;
        setRadialWind(std::get<0>(*channels).radialWind);
    }
};

// 仅适配 Qt 元对象；类型与只读份额直达绘图数值入口。
class LidarQ640CNR : public Wind2DItem
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit LidarQ640CNR(QQuickItem *parent = nullptr) : Wind2DItem(parent) {}
    Q_INVOKABLE void setData(const LidarQ640Channels &channels)
    {
        if (!channels) return;
        setCNR(std::get<0>(*channels).cnr);
    }
};
