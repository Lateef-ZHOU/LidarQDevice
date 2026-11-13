#include "LidarQDevice.h"
#include "WindField.h"
#include <optional>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlListReference>
#include <QtQml/QQmlExtensionPlugin>
#include <QDebug>
#include <memory>
#include <QPointer>
#include "DeviceBase.h"
#include "DAQDevice.h"
Q_IMPORT_QML_PLUGIN(LidarUIPlugin)
Q_IMPORT_QML_PLUGIN(LidarChartsPlugin)
Q_IMPORT_QML_PLUGIN(LidarTaskUIPlugin)
Q_IMPORT_QML_PLUGIN(LidarLidarQDevicePlugin)
int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
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
            const auto gates = std::make_shared<const LinearArray<double>>(0, 60, std::vector<double>{1, 2});
            Q_EMIT dynamic_cast<DAQFamily *>(backend)->spectrumReady(channel, std::make_shared<const LinearArray2D<double>>(2, 2, 0, 60, 0, 1, std::vector<double>{1, 2, 3, 4}));
            Q_EMIT dynamic_cast<DAQFamily *>(backend)->radialReady(channel, gates, gates);
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
            const auto gates = std::make_shared<const LinearArray<double>>(0, 60, std::vector<double>{1, 2});
            Q_EMIT dynamic_cast<DAQFamily *>(backend)->spectrumReady(channel, std::make_shared<const LinearArray2D<double>>(2, 2, 0, 60, 0, 1, std::vector<double>{1, 2, 3, 4}));
            Q_EMIT dynamic_cast<DAQFamily *>(backend)->radialReady(channel, gates, gates);
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
