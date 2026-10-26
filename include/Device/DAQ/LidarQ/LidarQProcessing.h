#pragma once
#include <array>
#include <memory>
#include <QString>
#include <QVector>
#include "Algorithm/Wind/RadialWind.h"
#include "Algorithm/Wind/WindProfile.h"
#include "Acquisition/AcqSaver.h"
#include "Device/DAQ/LidarQ/LidarQRunSide.h"

// 型号帧处理工厂；Emitter 只提供结果信号，不向其读取或写入业务成员。
// 每条处理线程只创建一份；四束环及写位置仅由该闭包维护。
// 任务保证首次 sync 前写满四束，后续同步点仍使用最近四个互异方向。
template<LidarQGeometry Geometry, class Emitter>
auto makeLidarQProcessor(Emitter *self)
{
    return [self, ring_ = std::array<WindProfile::Input, WindProfile::kBeams>{}, ringW_ = 0]
        (const std::shared_ptr<const LidarQCollector<Geometry>> &collector) mutable {
        if (!collector || !collector->settled())
            return; // 只处理完整帧
        const auto &[pose, saveRoot, sessionDir, sync] = collector->context();
        const auto &specArray = collector->constFrame();

        // 目录名由编排层给（一组一个会话目录，同步之后换新的）；文件名来自这一帧的产物。
        const QString fileStem = AcqSaver::fileStem(collector->frameTime());

        // 1. 谱 → 推 GUI → 异步落盘（立刻起线程，不等后续计算）
        // 局部名 specArray：谱数组与"规格"无关，只是本卡的谱恰好也叫 spec，故加后缀避歧义。
        // **发出后本数组只读**：GUI 的 3D 模块直接接住这份份额、不深拷，故之后径向风计算与落盘
        // 都只许读它。
        Q_EMIT self->specReady(specArray);
        if (saveRoot)
            AcqSaver::saveSpec(*saveRoot, sessionDir, fileStem, specArray, QStringLiteral("CDL"));

        // 2. 2D 风速 + CNR（斜率是占位，见 LidarQConfig::Freq2WsSlope——管线真、数值假）
        const auto [ws, cnr] = RadialWind::Calc(specArray, Geometry.row, Geometry.col,
                                                LidarQConfig::Freq2WsSlope, Geometry.freq2WsIntercept());
        Q_EMIT self->wsReady(ws);
        Q_EMIT self->cnrReady(cnr);
        if (saveRoot)
            AcqSaver::saveRadialWind(*saveRoot, sessionDir, fileStem, AcqSaver::timeStem(collector->frameTime()),
                                     {{QStringLiteral("CDL"), {ws, cnr}}});

        // 3. 单通道四束环：本束入环，同步时用最近四束反演。
        ring_[ringW_] = {pose, ws};
        ringW_ = (ringW_ + 1) % WindProfile::kBeams;
        if (!sync)
            return;
        const auto profile = WindProfile::Calc(ring_, LidarQConfig::DBinDist, collector->frameTime()); // 环以 span 交出，只看不拷
        if (!profile)
            return; // 几何退化（含"环里有无效朝向"那一支）
        Q_EMIT self->profileReady(profile);
        if (saveRoot)
            AcqSaver::saveWindProfile(profile, *saveRoot, sessionDir, QStringLiteral("CDL"));
    };
}
