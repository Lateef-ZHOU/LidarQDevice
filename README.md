# LidarQDevice

2026-11-09 用户定（DAQ 第二步）：Collector 的 createPrototype（只读原型）改为 `create(const DAQParameters&)`，按请求参数造完整实例并备齐收集区；makeSelf 只供看门狗重试，复用 create。

2026-11-09 用户定：删除处理线程、中间 Core 与型号处理 lambda；末端直接继承 `DAQDevice<HandleT, CollectorT>`，唯一 QObject 来自 Handle。Collector 继承族基的通道数组，型号只给通道规格并按协议整理功率谱，收齐末包调用 complete() 由族基在收包线程做标准处理；显示、落盘（每帧一个 JSON）与风廓线由族层完成。页面改接族信号 spectrumReady/radialReady/profileReady（按通道下标），family 改为 QtObject，删除 device.device 自引用。单路 CDL；换算常量仍是占位（频点 i 记作 i MHz、系数 Freq2WsSlope、零点在中心频点，门距 DBinDist），结果与原占位一致。删除 LidarQProcessing.h 与未再使用的 freq2WsIntercept。验证：独立构建零警告，CTest 1/1。

2026-11-03 用户定：网口连接区仅保留网卡选择和连接/断开，删除 IP、端口等固定参数展示及对应 QML 参数字段；实际连接配置、校验、ARP 和收发关闭行为保持。

Windows 验证：通过主程序本地源码覆盖增量构建，五个网口官邸 qmllint 目标成功；仍有 QObject 属性类型的既有静态提示。八秒离屏冒烟通过，仅既有字体提示；未进行硬件操作或 Linux 验证。证据：LiDAR_DataACQ/build/build-network-ui.log、smoke-network-ui.json。

2026-11-03 上传接口：官邸仍拥有唯一具体 Core；Core 继承 `Device<HandleT, CollectorT>`，Device 直接继承 Handle 并重写 onData。Core 只提供 Qt 属性、锁信号与末端预析构；不增设上传回调、状态缓存或逐包信号转发。协议、Collector 与 UI 逻辑保持。

两种规格的采集图窗均命名为 `功率谱`、`径向风`、`风廓线`；径向风页继续包含风速和 CNR。仅统一显示名称，不改页面顺序、历史或处理/保存接口。


2026-10-28 用户定：450×200 与 640×128 两个官邸的连接区都只选择网卡，`open(interfaceIndex)` 使用 LidarQConfig 固定参数：本机 `192.168.1.102:57568`，设备 `192.168.1.10:57568`，设备 MAC `00:11:22:33:44:55`。采集卡不回复 ARP，因此显式提供 MAC，Handle 直接组装 UDP；不改共享 Handle 或铭辰的 ARP 路径。QML 不再保存或编辑这五项设置，累积、采集和保存设置不变。

本轮 Windows Qt 6.11.2 Release、两页面 qmllint 及既有 450×200 页面冒烟 1/1 通过，无编译/QML 警告；证据为 `build/build-fixed-connection.log`、`build/test-fixed-connection.log`。未连接实物，不重复 Linux/协议全套，仅本地提交。

LiDAR 的独立官邸：具体 C++ 后端、Collector/协议/处理逻辑及其 QML 页面在同一仓库维护。来自 LiDAR_DataACQ master `5cc6d65`，本次分仓不改变运行行为。

## 边界与接入

- QML 模块：`Lidar.LidarQ`；页面：`LidarQ450Module`, `LidarQ640Module`。
- CMake 目标：`Lidar::LidarQDevice`；静态插件：`LidarLidarQDeviceplugin`，最终可执行文件显式 `Q_IMPORT_QML_PLUGIN(LidarLidarQDevicePlugin)`，并导入 TaskUI/UI/Charts 的静态插件。
- 页面以 attached 对象拥有唯一具体后端，后端不能在 QML 直接创建。设备通过只读 `family` 借出族引用；任务页面只接收族列表，不依赖设备官邸。
- 依赖由 `cmake/Dependencies.cmake` 固定到提交；可用标准 FetchContent 源码覆盖做联调。多个官邸并用时，先开启所有需要的 HandleCore/TaskCore 可选项，再配置共享库，避免已创建目标后补开模块。
- 需要 libpcap >= 1.10.0；Windows 使用 Npcap SDK 与已安装的 Npcap 运行时，Linux 使用 libpcap。驱动不随仓库分发。

## 构建与验证

C++20、CMake >= 3.28、Qt 6.11.2（Quick/Qml/GuiPrivate/ShaderTools）。Windows 使用 MSVC x64；Linux 使用 GCC，Qt 与原生依赖由环境提供。

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="<Qt前缀;需要的原生依赖前缀>"
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target LidarLidarQDevice_qmllint
```

独立测试逐头编译、创建真实页面并销毁后端；不连接实物设备。硬件收发仍需对应设备实测。主程序负责跨官邸仿真、任务及取消链回归。

## 许可

MIT，Copyright (c) 2026 周涛（ZHOU TAO）。Qt、libserialport、libpcap/Npcap 等依赖各自授权，MIT 不替代这些许可；详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。本仓库不包含第三方 SDK、运行库或驱动。

CI：托管 Linux 自动验证；Windows 原生依赖使用预装 SDK/运行时的自托管入口，默认跳过不代表通过。本机 Windows 验证记录与主程序分仓记录一起保存。

本轮本机验证（2026-10-28）：Windows Qt 6.11.2 Release 构建、CMake qmllint 和既有页面冒烟 1/1 通过；未连接实物。日志：build/build-connection-summary.log、build/test-connection-summary.log。两页均编译/静态检查，既有运行冒烟实例化 450×200。

## 资源阶段通知（2026-11-02）

连接采用 HandleCore 的 `connectionChanged(ResourcePhase)`；主动关闭前在所属线程同步发 `AboutToRelease`，完成事件用于回读现状。页面借出已有 `deviceState` 机制对象，不再保存 isOpen/isLocked/holder 的 QML 转交属性；标签通过连接/锁完成阶段回读，不恢复通用 statusChanged。固定依赖已同步；本轮提交尚未推送，验证使用显式本地源码覆盖。


2026-11-03 验证：Windows Qt 6.11.2 主程序 Release 编译、15 组 QML 检查、跨官邸生命周期和三任务回归 7/7 通过。接口联调使用相邻源码覆盖，新依赖固定为本地提交，尚未推送；未连接实物设备或更新板卡。

## 源码目录（2026-11-03）

公开文件直接放在 `include/`，引用时使用文件名，不再携带原单仓库的业务目录前缀。CMake 目标名称、C++ 类型和 QML 模块名保持不变；使用方须同步更新头文件路径及固定依赖版本。

目录整理验证（2026-11-03）：Windows：公开头独立编译、页面测试 1/1 及主程序构建通过。本轮通过本地源码覆盖验证，固定依赖同步更新到本地提交；尚未推送，不代表已验证远端拉取。

## 2026-11-04：设备日志直接接线（用户定）

删除具体设备的 logSource 自引用属性，QML 直接监听现有设备对象的 logMessage/connectionChanged；设备失败日志直接调用继承自 Handle 的信号。协议、收发、重连和关闭顺序不变。 本轮仅本地整理，未推送；Windows 集成验证记录见主程序维护操作。

## 2026-11-04：设备身份与占用查询（用户定）

设备名称/副标题由构造传入 DeviceBase 的公开 const 成员，Qt 通过 MEMBER CONSTANT 直接暴露；占用查询只保留 holder，Qt 用无效 QVariant 表示空闲、字符串表示占用，删除 isLocked 属性。既有连接、重连、收发和取消逻辑保持。

本轮仅本地整理；集成验证记录见主程序 docs/maintenance/validation.md，不推送。

## 2026-11-04：进料在消费端取值（用户定）

Collector 的 onFeed 接收 optional<QByteArray> 的 const 引用，由本型号解析端取出字节。空输入由 DeviceCore 公共 feed 拦截；帧号、包序和收齐规则不变。

本轮仅本地整理，不推送；集成验证见主程序维护记录。


### 2026-11-05 公共视觉与连接布局

两种型号的连接区使用 UICore ConnectionSection；型号构参、枚举、命令及日志不变。 本轮仅本地整理，未推送；Windows 集成验证记录见主程序维护操作。


### 2026-11-05 连接间距、累积默认值与径向风页面整理

两种国耀型号的默认累积时间统一为1秒；显式输入、范围和编辑完成提交行为保持。

主程序 Windows Release 增量构建、八秒桌面冒烟及四窗口装配检查通过；运行 stderr 为空，保留既有 CDL5/Qt 生成代码 C4702 编译警告。仅本地验证，未连接硬件、未推送。
