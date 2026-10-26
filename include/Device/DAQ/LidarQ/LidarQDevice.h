#pragma once
#include "Device/DAQ/LidarQ/LidarQProcessing.h"
#include <QDateTime>
#include <QtQml/qqmlregistration.h>
#include "UI/Chart/DAQChartSpecification.h"
#include "Acquisition/AcqSaver.h"
#include "Algorithm/Wind/VerticalWindProfile.h"
#include <memory>
#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QVariantMap>

#include "Device/DAQ/DAQDevice.h"                  // 采集卡族两层（族基 DAQDeviceBase + 族层 DAQDevice）
#include "Device/PcapForm.h"
#include "Device/DAQ/LidarQ/LidarQRunSide.h"    // 运行侧：帧头布局 + 规格预设 + 收集器

// 国耀两种规格共用模板流程；Qt 属性与信号由非模板末端类声明。
// 规格贯穿 Collector、处理工厂和设备类型，任务侧继续使用 DAQDeviceBase 组合。
template <LidarQGeometry Geometry>
class LidarQDevice : public DAQDevice<PcapHandle, LidarQCollector<Geometry>>
{
    using Base = DAQDevice<PcapHandle, LidarQCollector<Geometry>>;
public:
    [[nodiscard]] QVariantMap connectionParameters() const
    {
        return PcapForm::connectionParameters(LidarQConfig::HostIPv4, LidarQConfig::HostPort,
                                                LidarQConfig::DeviceIPv4, LidarQConfig::DevicePort, LidarQConfig::DeviceMAC);
    }
    bool open(const int interfaceIndex, const QString &hostIP, const int hostPort,
              const QString &deviceIP, const int devicePort, const QString &deviceMAC)
    {
        const auto params = PcapForm::parseParameters(this->device().logSource(), interfaceIndex, hostIP, hostPort, deviceIP, devicePort,
            deviceMAC, Base::kPcapReadTimeout, Base::kPcapKernelBufferBytes);
        return params && this->device().open(*params);
    }

    explicit LidarQDevice(QObject *parent)
        : Base(QStringLiteral("国耀采集卡"),
               QStringLiteral("网口 · UDP · %1×%2").arg(Geometry.row).arg(Geometry.col), parent)
    {
        static_assert(DeviceLifecycleLayer<LidarQDevice, decltype(&LidarQDevice::PreDestruction)>);
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

protected:

    void PreDestruction() noexcept override { Base::PreDestruction(); }

// 参数下发共用实现；末端同名槽负责 Qt 元对象接入。
public:
    // 空闲时单独下发累积时间（空令牌：空闲时过，被任务锁着则拒绝）。
    bool sendParams(const int accumSec)
    {
        return this->device().send({LidarQConfig::accumulationCommand(accumSec)}, {});
    }

private:
    static constexpr LidarQGeometry geo_ = Geometry;

};

class LidarQ450x200Device final
#ifdef Q_MOC_RUN
    : public DAQDeviceBase // 模板层继承的族级元对象。
#else
    : public LidarQDevice<LidarQGeometry::Geo450x200>
#endif
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap connectionParameters READ connectionParameters CONSTANT)
    QML_ELEMENT
    QML_ATTACHED(LidarQ450x200Device)
    QML_UNCREATABLE("由对应 QML 官邸创建和拥有")
    Q_PROPERTY(QVariantMap acquisitionParameters READ acquisitionParameters CONSTANT)
    using Base = LidarQDevice<LidarQGeometry::Geo450x200>;
public:
    static LidarQ450x200Device *qmlAttachedProperties(QObject *owner) { return new LidarQ450x200Device(owner); }
    explicit LidarQ450x200Device(QObject *parent)
        : Base(parent)
    {
        this->startProcessing();
    }
    ~LidarQ450x200Device() override { PreDestruction(); }
    Q_INVOKABLE [[nodiscard]] QVariantMap chartParameters() const { return Base::chartParameters(); }
Q_SIGNALS:
    void specReady(const QVector<double> &values);
    void wsReady(const QVector<double> &values);
    void cnrReady(const QVector<double> &values);
    void profileReady(VerticalWindProfilePtr profile);
public Q_SLOTS:
    bool open(const int interfaceIndex, const QString &hostIP, const int hostPort,
              const QString &deviceIP, const int devicePort, const QString &deviceMAC)
    { return Base::open(interfaceIndex, hostIP, hostPort, deviceIP, devicePort, deviceMAC); }
    void close() { this->device().close(); }
    bool sendParams(const int accumSec) { return Base::sendParams(accumSec); }
    void startTest(const int accumSec, const bool save, const QUrl &folder)
    {
        Base::startTest(accumSec, save, folder, AcqSaver::sessionDir(
            QDateTime::currentDateTime(), this->objectName() + QStringLiteral("Test")));
    }
    void cancelTest() noexcept { Base::cancelTest(); }
private:
    void run() override { this->consumeFrames(makeLidarQProcessor<LidarQGeometry::Geo450x200>(this)); }
    void PreDestruction() noexcept final
    {
        static_assert(FinalDevice<LidarQ450x200Device, decltype(&LidarQ450x200Device::PreDestruction)>);
        Base::PreDestruction();
    }
};

class LidarQ640x128Device final
#ifdef Q_MOC_RUN
    : public DAQDeviceBase // 模板层继承的族级元对象。
#else
    : public LidarQDevice<LidarQGeometry::Geo640x128>
#endif
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap connectionParameters READ connectionParameters CONSTANT)
    QML_ELEMENT
    QML_ATTACHED(LidarQ640x128Device)
    QML_UNCREATABLE("由对应 QML 官邸创建和拥有")
    Q_PROPERTY(QVariantMap acquisitionParameters READ acquisitionParameters CONSTANT)
    using Base = LidarQDevice<LidarQGeometry::Geo640x128>;
public:
    static LidarQ640x128Device *qmlAttachedProperties(QObject *owner) { return new LidarQ640x128Device(owner); }
    explicit LidarQ640x128Device(QObject *parent)
        : Base(parent)
    {
        this->startProcessing();
    }
    ~LidarQ640x128Device() override { PreDestruction(); }
    Q_INVOKABLE [[nodiscard]] QVariantMap chartParameters() const { return Base::chartParameters(); }
Q_SIGNALS:
    void specReady(const QVector<double> &values);
    void wsReady(const QVector<double> &values);
    void cnrReady(const QVector<double> &values);
    void profileReady(VerticalWindProfilePtr profile);
public Q_SLOTS:
    bool open(const int interfaceIndex, const QString &hostIP, const int hostPort,
              const QString &deviceIP, const int devicePort, const QString &deviceMAC)
    { return Base::open(interfaceIndex, hostIP, hostPort, deviceIP, devicePort, deviceMAC); }
    void close() { this->device().close(); }
    bool sendParams(const int accumSec) { return Base::sendParams(accumSec); }
    void startTest(const int accumSec, const bool save, const QUrl &folder)
    {
        Base::startTest(accumSec, save, folder, AcqSaver::sessionDir(
            QDateTime::currentDateTime(), this->objectName() + QStringLiteral("Test")));
    }
    void cancelTest() noexcept { Base::cancelTest(); }
private:
    void run() override { this->consumeFrames(makeLidarQProcessor<LidarQGeometry::Geo640x128>(this)); }
    void PreDestruction() noexcept final
    {
        static_assert(FinalDevice<LidarQ640x128Device, decltype(&LidarQ640x128Device::PreDestruction)>);
        Base::PreDestruction();
    }
};
