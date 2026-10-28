# LidarQDevice

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
