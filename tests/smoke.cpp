#include "LidarQDevice.h"
#include <optional>
#include <vector>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlListReference>
#include <QtQml/QQmlExtensionPlugin>
#include <QDebug>
#include <memory>
#include <QPointer>
#include "DeviceBase.h"
#include "DAQFamily/DAQDevice.h"
Q_IMPORT_QML_PLUGIN(LidarUIPlugin)
Q_IMPORT_QML_PLUGIN(LidarChartsPlugin)
Q_IMPORT_QML_PLUGIN(LidarTaskUIPlugin)
Q_IMPORT_QML_PLUGIN(LidarLidarQDevicePlugin)
struct TestKey : DAQParamsKey { TestKey() {} };
template<LidarQGeometry Geometry>
bool verifyDecodedArray()
{
    LidarQCollector<Geometry> collector(DAQParameters(TestKey{}, 1, {}, {}, {}, false));
    for (int index = 1; index <= Geometry.packetsPerFrame(); ++index) {
        std::vector<unsigned char> packet(LidarQConfig::HeaderSize + Geometry.bytesPerPacket());
        std::memcpy(packet.data(), LidarQConfig::HeaderSign, 4);
        qToBigEndian<quint16>(7, packet.data() + LidarQConfig::FrameIdOffset);
        qToBigEndian<quint16>(static_cast<quint16>(index), packet.data() + LidarQConfig::PacketIndexOffset);
        for (int offset = 0; offset < Geometry.bytesPerPacket(); offset += 2)
            qToUnaligned<quint16>(static_cast<quint16>(index), packet.data() + LidarQConfig::HeaderSize + offset);
        if (collector.feed(packet, {}) != (index == Geometry.packetsPerFrame())) return false;
    }
    const auto &spectrum = *std::get<0>(collector.channels()).spectrum;
    const auto raw = [&](std::size_t i) { return static_cast<double>(i * 2 / Geometry.bytesPerPacket() + 1); };
    const auto columns = static_cast<std::size_t>(Geometry.col);
    for (std::size_t i = 0; i < spectrum.size(); ++i)
        if (spectrum[i] != raw(i) - raw(i % columns)) return false; // 本底 = 第一门同频点
    return spectrum.size() == static_cast<std::size_t>(Geometry.row) * Geometry.col;
}
template<class Channels, std::size_t... I>
Channels sampleChannels(std::index_sequence<I...>)
{
    const auto channel = []<class C>() {
        using S = std::remove_const_t<typename decltype(C::spectrum)::element_type>;
        auto spectrum = std::make_unique<S>();
        std::fill(spectrum->begin(), spectrum->end(), 1.0);
        return C(std::move(spectrum));
    };
    return std::make_shared<typename Channels::element_type>(channel.template operator()<std::tuple_element_t<I, typename Channels::element_type>>()...);
}
int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    if (!verifyDecodedArray<LidarQGeometry::Geo450x200>() || !verifyDecodedArray<LidarQGeometry::Geo640x128>()) return 20;
    QQmlEngine engine;
    bool warned = false;
    QObject::connect(&engine, &QQmlEngine::warnings, &engine, [&](const auto &) { warned = true; });
    {
        QQmlComponent component(&engine);
        component.setData("import Lidar.LidarQ\nLidarQ450Module {  }", QUrl());
        std::unique_ptr<QObject> page(component.create());
        if (!page) { qCritical() << component.errors(); return 1; }
        const QQmlListReference charts(page.get(), "mainPages");
        const QQmlListReference sections(page.get(), "sections");
        if (charts.count() < 1 || sections.count() < 1) return 2;
        if (page->property("name").toString().isEmpty()) return 3;
        QObject *backend = nullptr;
        int backends = 0;
        for (auto *child : page->children()) {
            if (QByteArray(child->metaObject()->className()) == "LidarQ450x200Device") {
                backend = child;
                ++backends;
            }
        }
        if (backends != 1 || backend->parent() != page.get()) return 5;
        // 设备与采集卡族是同一个对象：family 即后端本身，可交叉转换到纯 C++ 族基。
        if (page->property("family").value<QObject *>() != backend || !dynamic_cast<DAQFamily *>(backend)) return 7;
        // 族信号经页面接线交给图窗；参数类型与图窗入口不符时 QML 会报警告（warned）。
        for (int channel = 0; channel < 1; ++channel) {
            Q_EMIT qobject_cast<LidarQ450x200Device *>(backend)->channelsReady(sampleChannels<LidarQ450x200Device::Channels>(std::make_index_sequence<std::tuple_size_v<typename LidarQ450x200Device::Channels::element_type>>{}));
            LidarQ450Profiles profiles;
            std::apply([&](auto &...p) {
                ((p = std::make_shared<const std::remove_const_t<typename std::decay_t<decltype(p)>::element_type>>(
                    std::chrono::system_clock::now(), 30.0)), ...);
            }, profiles);
            Q_EMIT qobject_cast<LidarQ450x200Device *>(backend)->profilesReady(profiles);
        }
        const QPointer<QObject> lifetime(backend);
        app.processEvents();
        page.reset();
        if (lifetime) return 6;
    }
    {
        QQmlComponent component(&engine);
        component.setData("import Lidar.LidarQ\nLidarQ640Module {  }", QUrl());
        std::unique_ptr<QObject> page(component.create());
        if (!page) { qCritical() << component.errors(); return 1; }
        const QQmlListReference charts(page.get(), "mainPages");
        const QQmlListReference sections(page.get(), "sections");
        if (charts.count() < 1 || sections.count() < 1) return 2;
        if (page->property("name").toString().isEmpty()) return 3;
        QObject *backend = nullptr;
        int backends = 0;
        for (auto *child : page->children()) {
            if (QByteArray(child->metaObject()->className()) == "LidarQ640x128Device") {
                backend = child;
                ++backends;
            }
        }
        if (backends != 1 || backend->parent() != page.get()) return 5;
        // 设备与采集卡族是同一个对象：family 即后端本身，可交叉转换到纯 C++ 族基。
        if (page->property("family").value<QObject *>() != backend || !dynamic_cast<DAQFamily *>(backend)) return 7;
        // 族信号经页面接线交给图窗；参数类型与图窗入口不符时 QML 会报警告（warned）。
        for (int channel = 0; channel < 1; ++channel) {
            Q_EMIT qobject_cast<LidarQ640x128Device *>(backend)->channelsReady(sampleChannels<LidarQ640x128Device::Channels>(std::make_index_sequence<std::tuple_size_v<typename LidarQ640x128Device::Channels::element_type>>{}));
            LidarQ640Profiles profiles;
            std::apply([&](auto &...p) {
                ((p = std::make_shared<const std::remove_const_t<typename std::decay_t<decltype(p)>::element_type>>(
                    std::chrono::system_clock::now(), 30.0)), ...);
            }, profiles);
            Q_EMIT qobject_cast<LidarQ640x128Device *>(backend)->profilesReady(profiles);
        }
        const QPointer<QObject> lifetime(backend);
        app.processEvents();
        page.reset();
        if (lifetime) return 6;
    }
    return warned ? 4 : 0;
}
