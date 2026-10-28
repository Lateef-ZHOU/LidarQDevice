#pragma once
#include <algorithm>
#include <cstring> // std::memcmp（帧头核验）
#include <optional>
#include <QByteArray>
#include <QList>
#include <QVector>
#include <QtEndian> // qFromBigEndian（帧号/包序字段解析）
#include <QtGlobal> // quint8/quint16（帧头字段）

#include "Device/DAQ/DAQCollector.h" // DAQCollector（族收集器基）

// ============================================================
// LidarQRunSide.h — 国耀采集卡型号夹的**运行侧**：协议常量 + 形状值类 + 收集器。
// 文件名不叫 Config：Config 该是纯常量店，而本文件还装着两个有行为的类；名随其实。
// 内部的 `namespace LidarQConfig` 那一段确实只是常量店。
// 官邸单独住 LidarQDevice.h（分家理由同 Hfnl：重依赖只挂官邸一侧）。
//
// 本卡与 Hfnl 的分歧：**有两种物理规格**（450×200 / 640×128 探测阵列），行×列×包长三数
// 互相牵制（包长要让 行×列×2 整除，两卡各配了不同的 UDP 包长），故装不进裸常量表——用
// LidarQGeometry 值类装一份规格，两枚预设作为 Collector/Device/处理工厂 的模板参数，型号不混用。
//
// 帧头 8 字节布局是协议级不变量（两卡共用，与规格无关）：
//   [0..3] 魔数 {0x47,0x59,0x4c,0x5a}
//   [4..5] 帧号 FrameID（大端 quint16）
//   [6..7] 包序 PacketIndex（大端 quint16，1-based，[1, packetsPerFrame()]）
// ============================================================
// 协议常量（编译期；两卡共用，与规格无关）。**只开这一次**：预设不在此处——它们是
// LidarQGeometry 的值，做成该类的静态成员，遂不必为了"常量在结构体前、预设在结构体后"
// 把本 namespace 劈成两半。
namespace LidarQConfig
{
inline constexpr quint8 HeaderSign[4] = {0x47, 0x59, 0x4c, 0x5a};
inline constexpr int HeaderSize = 8; // 4 魔数 + 2 帧号 + 2 包序
inline constexpr int FrameIdOffset = 4;
inline constexpr int PacketIndexOffset = 6;

// ── 参数配置命令字（0xAA 报文的第二字节；数值位一律大端 quint16）──────────────
// **本店只编码这一个**：累积时间。参考工程 DAQManager::SendDAQParamsConfig 发的
// `0xAA 0x01 <大端 quint16 秒>`，也是**它全工程唯一的下行报文**（450×200 与 640×128 两份
// 部署工程逐字节相同）。
inline constexpr quint8 CmdAccumulationTime = 0x01;
// 累积时间输入范围（型号页直读）：报文值字段是大端 quint16，故上限 65535 s；下限 1 s、默认 10 s 是界面取值。
inline constexpr int AccumSecDefault = 10;
inline constexpr int AccumSecMin = 1;
inline constexpr int AccumSecMax = 65535;

// UI 和自动采集共用协议编码；超限输入在型号边界钳到可表达范围。
[[nodiscard]] inline QByteArray accumulationCommand(int seconds)
{
    const auto value = static_cast<quint16>(std::clamp(seconds, AccumSecMin, AccumSecMax));
    const char bytes[] = {'\xAA', static_cast<char>(CmdAccumulationTime),
                          static_cast<char>(value >> 8), static_cast<char>(value & 0xFF)};
    return QByteArray(bytes, sizeof bytes);
}


// **只实现累积时间这一个参数**（用户定）。本卡的文档里还有两个量——
//     增益   0x02，默认 35，范围 0~100（范围待确认）
//     最大值 0x03，默认 65535，范围 0~65535
// ——不实现：实用性不高、甚至带 bug，实际场景里从未调整过；写进代码只会多两个没人动的旋钮和
// 一条没人走的下行路。真要用时照上面两行补。
//
// **两张采集卡的参数词汇只有同一个量：累积时间**，而实现方式各不相同——
//   · 国实卡：卡上叫"雷达曝光时长"，**本该编进启动命令**（功能预留，卡未实现）；
//   · 国耀卡：走本店这条独立的 0xAA 报文，与启动无关（本卡常发流，压根没有"启动"）。
// 同一个量、两种通路：collectorFor(accumSec, context) 创建收集器；国耀的累积时间由 Collector::buildStart 随每次请求下发。

// ── 物理换算（频率 bin → 风速、距离门 → 距离）────────────────────────────────
// **两条都是占位**（用户定：先按 1 写，把代码打通，实测 / 问到再回填）。参考工程
// 一个物理常量都没有——它的风速图实现是空壳，压根不算风速，故无值可抄。真值的形态看国实卡的
// HfnlConfig::homFreq2WsSlope（采样率 / FFT 点数 × 波长 / 2）与 homDBinDist——本卡的对应两数需要采样率、
// FFT 点数、激光波长、门宽才能算出。
inline constexpr double Freq2WsSlope = 1.0; // **占位**：bin → m/s 的斜率，待实测常量回填
inline constexpr double DBinDist = 1.0;     // **占位**：米/距离门，待实测回填（2D 距离轴 + 风廓线高度都用它）

// 两种规格共用固定网络参数；设备不回应 ARP，显式 MAC 使 Handle 直接组装 UDP。
inline constexpr auto HostIPv4 = "192.168.1.102";
inline constexpr int HostPort = 57568;
inline constexpr auto DeviceIPv4 = "192.168.1.10";
inline constexpr int DevicePort = 57568;
inline constexpr char DeviceMAC[] = "00:11:22:33:44:55";

// ── 绘图显示范围 ────────────────────────────────────────────────────────────
// 3D 功率谱值域：本卡谱元素是 quint16，物理上界就是 65535——**这一条不是占位**，是确定值。
inline constexpr qint64 Plt3dValMax = 65535;
} // namespace LidarQConfig

// ── LidarQGeometry：一张卡的固定规格，作为模板参数区分两种设备。──
struct LidarQGeometry
{
    int row = 0;        // 探测阵列行数
    int col = 0;        // 探测阵列列数
    int packetSize = 0; // 本卡的 UDP 包总长（含帧头；两卡各配了能整除的值，非共用常数）

    [[nodiscard]] constexpr int bytesPerFFT() const noexcept
    {
        return row * col * static_cast<int>(sizeof(quint16));
    }
    [[nodiscard]] constexpr int bytesPerPacket() const noexcept { return packetSize - LidarQConfig::HeaderSize; }
    // 现算不存：派生量只有一份账，改行/列/包长不会有第二处忘记同步。
    [[nodiscard]] constexpr int packetsPerFrame() const noexcept { return bytesPerFFT() / bytesPerPacket(); }

    // ── 物理/显示派生量（现算不存，同上：换 Freq2WsSlope / DBinDist 只改常量店一处）──
    // 零多普勒落在**中心 bin**——这是任何 FFT 多普勒谱的结构事实，不是待测常数，故截距由
    // 斜率推出（形制承自 HfnlConfig::homFreq2WsIntercept）。遂占位期
    // 只有斜率一个数是假的，真斜率一到，截距与量程自动跟对。
    [[nodiscard]] constexpr double freq2WsIntercept() const noexcept
    {
        return -col * LidarQConfig::Freq2WsSlope / 2.0;
    }
    // 风速显示量程：谱只有 col 个 bin，风速必落在 ±(col/2·slope) 内——量程随斜率自适应，
    // 不硬编 ±60（国实卡那两个数是真斜率下的物理范围，占位期照抄会把整条曲线画到框外）。
    [[nodiscard]] constexpr double wsAbsMax() const noexcept
    {
        return col * LidarQConfig::Freq2WsSlope / 2.0;
    }

    // ── 两张卡的预设：**本型的值，故做本型的静态成员**（用法 LidarQGeometry::Geo450x200）。
    //    数值来自各自部署工程的 GlobalParams.h 实测，均整除：
    //      450×200 → 180000B/帧，1200B/包（1208−8），150 包/帧；
    //      640×128 → 163840B/帧，1280B/包（1288−8），128 包/帧。
    //    类内声明 + 类外定义：自身类型在类内尚不完整，定不了值。
    static const LidarQGeometry Geo450x200;
    static const LidarQGeometry Geo640x128;
};

inline constexpr LidarQGeometry LidarQGeometry::Geo450x200{.row = 450, .col = 200, .packetSize = 1208};
inline constexpr LidarQGeometry LidarQGeometry::Geo640x128{.row = 640, .col = 128, .packetSize = 1288};

// ── LidarQCollector：一次尝试的收集器（单缓冲，以功率谱 FrameID 区分常发流中的帧）──
// FrameID 变化就放弃未齐的旧帧并重置计数，PacketIndex 决定本帧内的位置；收满一帧即交付。
// 参考实现用的是 Head/Middle/Tail 状态机 + 双缓冲 + 收完一帧后滞后一帧确认——那是为"设备连续
// 吐流、host 不能错过任何一帧"而设计的容错。本官邸的消费模型是"一次 run 收一帧"（外层协程
// while(true) 循环即"再收下一帧"），双缓冲买的容错用不上，只留分配代价——故只有一个缓冲。
// 也不设逐包命中位图：**用计数代替**，收满 packetsPerFrame() 个包即齐；重复包（同一 idx 到达
// 两次）会让计数虚高，但 UDP 同包重发概率不存在，真要防就得引回位图，那份开销正是要避免的
// （用户定：接受）。
// **换帧不清缓冲**：正常通路上"计数收满"即"每个分片位置都被本帧写过"，那一趟 fill 白跑。
// 代价是把无位图这一注**押得更实**：重复包凑满计数时，没收到的那片读到的不是确定的 0（一眼
// 可辨的空带），而是上一帧同位置的谱值——像真数据的陈旧数据，更难当场看出。运行缓冲在 makeSelf 中整帧
// 零填一次，故首帧之前不存在未初始化内存。
template <LidarQGeometry Geometry>
class LidarQCollector final : public DAQCollector
{
public:
    // 原型只保存配置，不分配整帧数组；运行实例由 makeSelf 创建。
    [[nodiscard]] static std::shared_ptr<const LidarQCollector> createPrototype(
        const int accumSec = LidarQConfig::AccumSecDefault, DAQContext context = {})
    {
        return std::shared_ptr<const LidarQCollector>(new const LidarQCollector(accumSec, std::move(context)));
    }

    [[nodiscard]] const QVector<double> &constFrame() const
    {
        static const QVector<double> empty;
        return settled() ? frame_ : empty;
    }

    // 本斗的数据**此刻齐不齐**：计数收满即齐。
    // **本卡是"为什么问状态而不是履历"的现成反例**：新帧来了 `onFeed` 只清计数**不清缓冲**
    // （正常通路上那趟 fill 是纯浪费）。于是万一计数真回退而新包正在落位，`frame_` 就是新旧混合的
    // 一帧；此时若拿"盖过章"当判据，它仍会喊"齐了"，worker 读到的是**撕裂的一帧**。按计数判则
    // 老老实实说"没齐"，数据到不了 worker。**两种判据的失效方向是反的**：状态谓词偏保守，履历
    // 标记偏危险。
    // （今天两者恒一致——判齐当场 CAS 摘桩，此后不再进料，缓冲遂被冻住。但那是靠外部不变量
    //  兜着，而按状态判连这条都不必倚赖。）
    [[nodiscard]] bool settled() const override { return count_ == geo_.packetsPerFrame(); }

    // 可变点：吃一口料。长度 / 魔数 / 包序任一不合规即丢弃、继续收。帧号变了（含首包：
    // frameId_ 出生态为空，天然判"变了"）即**弃旧启新**：计数归零，本帧从头收起；缓冲不清——
    // 理由与代价见类注。**齐没齐由 settled() 答，本口不答**（皮会再问一次）。
private: // NVI 可变点：派生只实现、不调用（公开的是基类那张非虚的皮 feed）
    void onFeed(const QByteArray &raw, const RxTime at) override
    {
        if (raw.size() != geo_.packetSize)
            return; // 长度不对：不认
        if (std::memcmp(raw.constData(), LidarQConfig::HeaderSign, sizeof(LidarQConfig::HeaderSign)) != 0)
            return; // 魔数不对：不认
        const auto *bytes = reinterpret_cast<const uchar *>(raw.constData());
        const quint16 frameId = qFromBigEndian<quint16>(bytes + LidarQConfig::FrameIdOffset);
        const quint16 packetIndex = qFromBigEndian<quint16>(bytes + LidarQConfig::PacketIndexOffset);
        if (packetIndex < 1 || packetIndex > geo_.packetsPerFrame())
            return; // 包序越界：不认

        if (!frameId_ || frameId != *frameId_) // 新帧（含首包）：弃旧启新
        {
            frameId_ = frameId;
            count_ = 0; // **只清计数，不清缓冲**：交付的前提是计数收满，收满即每个分片位置都被
                        // 本帧写过，旧值无一幸存——那一趟 fill 在正常通路上是纯浪费。
        }
        // **首包盖章**（用户定）：本卡的首包认**包序 1**，不认"缓冲空不空"——缓冲是定长且不清的
        // （见上），根本没有"空"这个态；而包序是协议自己给的帧内定位，比任何我们自己攒的状态都
        // 可靠。用 at 不现读系统时间：at 是网卡驱动在收包中断里盖的章，是本链路最靠前的一份观测；
        // 而收到首包时这一帧在设备侧早已成形，末包只是搬完它。判据：**每份观测都只会比事件晚，
        // 那就取最早的那一份。**
        // 代价明码：包序 1 丢了本帧就凑不齐（count_ 收不满），章的有无不影响任何结论。
        if (packetIndex == 1)
            frameTime_ = at;
        constexpr int elementsPerPacket = geo_.bytesPerPacket() / static_cast<int>(sizeof(quint16));
        double *const out = frame_.data() + (packetIndex - 1) * elementsPerPacket;
        const char *const payload = raw.constData() + LidarQConfig::HeaderSize;
        // 载荷保持主机序 quint16；按未对齐地址读取，不依赖 QByteArray 视图的起始地址。
        for (int i = 0; i < elementsPerPacket; ++i)
            out[i] = qFromUnaligned<quint16>(payload + i * sizeof(quint16));
        ++count_;
    }

public: // 恢复：本口之后仍是收集器的公开面

    // ── 指示区：命令 / 时限 / 造下一只（[主]，纯函数纪律）──
    // 每次 run 先下发本次累积时间；常发流仍按 FrameID 收齐一帧，无新增启动/停止协议。
    [[nodiscard]] QList<QByteArray> buildStart() const override
    { return {LidarQConfig::accumulationCommand(accumSec_)}; }
    // buildStop 不覆写（走基类默认的空表）：本卡无中止协议，同 Hfnl。
    // 常发流不设看门狗：参数下发不是重启一帧的命令，未知帧率下按猜测时限换斗可能反复丢弃在收帧。
    // 参数发送失败由 Device 拒收；成功提交后等收满、取消或断联。
    [[nodiscard]] int timeoutMs() const override { return NoWatchdogMs; }
    // 同参数、独立且已分配的初始收集区；只读取原对象的不可变配置。
    [[nodiscard]] std::shared_ptr<LidarQCollector> makeSelf() const
    {
        auto next = std::shared_ptr<LidarQCollector>(new LidarQCollector(accumSec_, context()));
        next->frame_.resize(geo_.row * geo_.col);
        return next;
    }

private:
    explicit LidarQCollector(const int accumSec, DAQContext context)
        : DAQCollector(std::move(context)), accumSec_(std::clamp(accumSec, LidarQConfig::AccumSecMin, LidarQConfig::AccumSecMax))
    {
        static_assert(Geometry.row > 0 && Geometry.col > 0 && Geometry.bytesPerPacket() > 0);
        static_assert(Geometry.bytesPerPacket() % sizeof(quint16) == 0);
        static_assert(Geometry.bytesPerFFT() % Geometry.bytesPerPacket() == 0);
    }

    const int accumSec_;
    static constexpr LidarQGeometry geo_ = Geometry; // 规格进入类型，运行中不再保存一份

    // ── 收集区（[仅收包线程] 可写）──
    std::optional<quint16> frameId_; // 当前正在收的帧号；空 = 还没见过任何帧
    QVector<double> frame_;
    int count_ = 0;                  // 已收分片数（不设位图，重复包会虚高，见类注）
};

using LidarQ450x200Collector = LidarQCollector<LidarQGeometry::Geo450x200>;
using LidarQ640x128Collector = LidarQCollector<LidarQGeometry::Geo640x128>;
Q_DECLARE_METATYPE(std::shared_ptr<const LidarQ450x200Collector>)
Q_DECLARE_METATYPE(std::shared_ptr<const LidarQ640x128Collector>)
