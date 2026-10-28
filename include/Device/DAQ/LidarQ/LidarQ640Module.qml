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
    readonly property DAQDeviceBase family: internals.device
    property var acquisitionParameters
    property int accum: acquisitionParameters.accumDefault
    property url saveDir: acquisitionParameters.saveDirDefault
    property bool save: false
    sections: [connectionSection, acquisitionSection].filter(item => item !== null)
    // 界面只选择本机网卡；其他连接参数固定在型号配置中。
    property int networkInterfaceIndex: internals.connectionParameters.networkInterfaceDefault
    mainPages: [
        Page {
            title: qsTr("功率谱 3D")
            contentItem: SurfaceView {
                id: surface
                specification: internals.chartParameters.plotConfiguration
                Connections {
                    target: internals.device
                    function onSpecReady(values) { surface.updateSpectrum(values) }
                }
            }
        },
        Page {
            title: qsTr("风速 | CNR")
            contentItem: Wind2DView {
                id: wind
                specification: internals.chartParameters.plotConfiguration
                Connections {
                    target: internals.device
                    function onWsReady(values) { wind.updateWS(values) }
                    function onCnrReady(values) { wind.updateCNR(values) }
                }
            }
        },
        Page {
            title: qsTr("风羽图")
            contentItem: WindBarbItem {
                id: barb
                readonly property var spec: internals.chartParameters.plotConfiguration
                configuration: ({ heightBins: spec.rows, maxHeight: spec.rows * spec.gateSpacing,
                    windowSeconds: spec.barbWindow, columnSeconds: spec.barbColumn })
                Connections {
                    target: internals.device
                    function onProfileReady(profile) { barb.appendColumn(profile) }
                }
            }
        }
    ]
    readonly property Section connectionSection: Section {
        title: qsTr("设备连接")
        FieldRow {
            label: qsTr("网卡")
            ChoiceField {
                Layout.fillWidth: true
                model: internals.connectionParameters.networkInterfaceOptions
                tips: internals.connectionParameters.networkInterfaceTips
                value: root.networkInterfaceIndex
                onCommitted: i => root.networkInterfaceIndex = i
            }
        }
        RowLayout {
            Button {
                Layout.fillWidth: true
                text: qsTr("连接")
                onClicked: {
                    if (internals.device.open(root.networkInterfaceIndex))
                        root.logMessage(qsTr("连接请求已提交"))
                }
            }
            Button { Layout.fillWidth: true; text: qsTr("断开"); onClicked: internals.device.close() }
        }
    }
    acquisitionParameters: internals.device.acquisitionParameters
    readonly property Section acquisitionSection: DAQControl {
        device: internals.device
        acquisition: root
        sendParameters: true
    }
    objectName: "LidarQ640Module"
    readonly property string name: internals.device.device.displayName
    readonly property string subtitle: internals.device.device.subtitle
    readonly property bool isOpen: internals.device.device.isOpen
    readonly property bool isLocked: internals.device.device.isLocked
    readonly property string holder: internals.device.device.holder


    QtObject {
        id: internals
        readonly property var chartParameters: device.chartParameters()
        readonly property var connectionParameters: device.connectionParameters
        readonly property LidarQ640x128Device device: root.LidarQ640x128Device
    }
    Connections {
        target: internals.device.device.logSource
        function onLogMessage(text): void { root.logMessage(text) }
    }
}
