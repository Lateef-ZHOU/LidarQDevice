pragma ComponentBehavior: Bound
import QtQuick
import Lidar.UI
import Lidar.TaskUI
import Lidar.Charts
import QtQuick.Controls

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
    // 待连接表单；编辑不修改 Handle 的运行快照。
    property int networkInterfaceIndex: internals.connectionParameters.networkInterfaceDefault
    property string hostIPv4: internals.connectionParameters.hostIPv4Default
    property int hostPort: internals.connectionParameters.hostPortDefault
    property string deviceIPv4: internals.connectionParameters.deviceIPv4Default
    property int devicePort: internals.connectionParameters.devicePortDefault
    property string deviceMAC: internals.connectionParameters.deviceMACDefault
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
    readonly property Section connectionSection: PcapCard {
        device: internals.device
        settings: root
        parameters: internals.connectionParameters
    }
    acquisitionParameters: internals.device.acquisitionParameters
    readonly property Section acquisitionSection: DAQControl {
        device: internals.device
        acquisition: root
        sendParameters: true
    }
    objectName: "LidarQ450Module"
    readonly property string name: internals.device.device.displayName
    readonly property string subtitle: internals.device.device.subtitle
    readonly property bool isOpen: internals.device.device.isOpen
    readonly property bool isLocked: internals.device.device.isLocked
    readonly property string holder: internals.device.device.holder


    QtObject {
        id: internals
        readonly property var chartParameters: device.chartParameters()
        readonly property var connectionParameters: device.connectionParameters
        readonly property LidarQ450x200Device device: root.LidarQ450x200Device
    }
    Connections {
        target: internals.device.device.logSource
        function onLogMessage(text): void { root.logMessage(text) }
    }
}
