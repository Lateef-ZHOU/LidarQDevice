#pragma once
#include <QDateTime>
#include <QtQml/qqmlregistration.h>
#include "DAQChartSpecification.h"
#include <memory>
#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QVariantMap>

#include "DAQDevice.h"                  // 采集卡族两层（族基 DAQDeviceBase + 族层 DAQDevice）
#include "PcapForm.h"
#include "LidarQRunSide.h"    // 运行侧：帧头布局 + 规格预设 + 收集器

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
        : Base(QStringLiteral("采集·国耀"),
               QStringLiteral("%1×%2").arg(Geometry.row).arg(Geometry.col), parent)
    {
        this->setObjectName(QStringLiteral("LidarQ%1").arg(geo_.row));
    }

    ~LidarQDevice() override = default;

    // 型号模板共用参数生成；两个末端各自提供 Qt 元对象入口。
    [[nodiscard]] QVariantMap chartParameters() const
    {
        const auto plotConfiguration = DAQChartSpecification::make(
            geo_.row, geo_.col, LidarQConfig::DBinDist,
            -static_cast<int>(geo_.wsAbsMax()), static_cast<int>(geo_.wsAbsMax()), LidarQConfig::Plt3dValMax);
        return {{"plotConfiguration", plotConfiguration}};
    }

    [[nodiscard]] QVariantMap acquisitionParameters() const
    { return {{"accumMin", LidarQConfig::AccumSecMin}, {"accumMax", LidarQConfig::AccumSecMax},
              {"accumDefault", LidarQConfig::AccumSecDefault}, {"saveDirDefault", QUrl::fromLocalFile(QDir::currentPath())}}; }

    // 空闲时单独下发累积时间（空令牌：空闲时过，被任务锁着则拒绝）。
    bool sendParams(const int accumSec)
    {
        return this->send({LidarQConfig::accumulationCommand(accumSec)}, {});
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
    Q_PROPERTY(QString displayName MEMBER displayName CONSTANT)
    Q_PROPERTY(QString subtitle MEMBER subtitle CONSTANT)
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
    void spectrumReady(int channel, const QVector<double> &values) override;
    void radialReady(int channel, const QVector<double> &ranges, const QVector<double> &speeds,
                     const QVector<double> &cnr, bool calibrated) override;
    void profileReady(int channel, NonuniformVerticalWindProfilePtr profile) override;
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
    Q_PROPERTY(QString displayName MEMBER displayName CONSTANT)
    Q_PROPERTY(QString subtitle MEMBER subtitle CONSTANT)
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
    void spectrumReady(int channel, const QVector<double> &values) override;
    void radialReady(int channel, const QVector<double> &ranges, const QVector<double> &speeds,
                     const QVector<double> &cnr, bool calibrated) override;
    void profileReady(int channel, NonuniformVerticalWindProfilePtr profile) override;
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
