pragma ComponentBehavior: Bound
import QtQuick
import Lidar.UI
import Lidar.TaskUI
import Lidar.Charts
import QtQuick.Controls
import QtQuick.Layouts

// 末端页面独持具体型号后端，在这里连接业务、图窗与控制分区。
ModulePage {
    id: root
    readonly property string kind: "device"
    readonly property QtObject family: internals.device // 采集卡族基不是 QObject，交出设备对象本身
    property var acquisitionParameters
    property int accum: acquisitionParameters.accumDefault
    property url saveDir: acquisitionParameters.saveDirDefault
    property bool save: false
    sections: [connectionSection, acquisitionSection].filter(item => item !== null)
    // 界面只选择本机网卡；其他连接参数固定在型号配置中。
    property int networkInterfaceIndex: internals.connectionParameters.networkInterfaceDefault
    mainPages: [
        Page {
            title: qsTr("功率谱")
            contentItem: SurfaceView {
                id: surface
                specification: internals.chartParameters.plotConfiguration
                Connections {
                    target: internals.device
                    function onSpectrumReady(channel, values) {
                        surface.updateSpectrum(values);
                    }
                }
            }
        },
        Page {
            title: qsTr("径向风")
            contentItem: Wind2DView {
                id: wind
                specification: internals.chartParameters.plotConfiguration
                Connections {
                    target: internals.device
                    function onRadialReady(channel, speeds, cnr, calibrated) {
                        wind.updateWS(speeds);
                        wind.updateCNR(cnr);
                    }
                }
            }
        },
        Page {
            title: qsTr("风廓线")
            contentItem: WindBarbItem {
                id: barb
                readonly property var spec: internals.chartParameters.plotConfiguration
                configuration: ({
                        heightBins: spec.rows,
                        maxHeight: spec.rows * spec.gateSpacing,
                        windowSeconds: spec.barbWindow,
                        columnSeconds: spec.barbColumn
                    })
                Connections {
                    target: internals.device
                    function onProfileReady(channel, profile) {
                        barb.appendColumn(profile);
                    }
                }
            }
        }
    ]
    readonly property Section connectionSection: ConnectionSection {
        title: qsTr("设备连接")

        label: qsTr("网卡")
        ChoiceField {
            Layout.fillWidth: true
            model: internals.connectionParameters.networkInterfaceOptions
            tips: internals.connectionParameters.networkInterfaceTips
            value: root.networkInterfaceIndex
            onCommitted: i => root.networkInterfaceIndex = i
        }

        firstAction: Action {
            text: qsTr("连接")
            onTriggered: {
                if (internals.device.open(root.networkInterfaceIndex))
                    root.logMessage(qsTr("连接请求已提交"));
            }
        }
        secondAction: Action {
            text: qsTr("断开")
            onTriggered: internals.device.close()
        }
    }
    acquisitionParameters: internals.device.acquisitionParameters
    readonly property Section acquisitionSection: DAQControl {
        device: internals.device
        acquisition: root
        sendParameters: true
    }
    objectName: "LidarQ450Module"
    readonly property string name: internals.device.displayName
    readonly property string subtitle: internals.device.subtitle
    // 借用公共只读资源接口（即设备对象本身），不另存状态。
    readonly property var deviceState: internals.device

    QtObject {
        id: internals
        readonly property var chartParameters: device.chartParameters()
        readonly property var connectionParameters: device.connectionParameters
        readonly property LidarQ450x200Device device: root.LidarQ450x200Device
    }
    Connections {
        target: internals.device
        function onLogMessage(text): void {
            root.logMessage(text);
        }
    }
}
