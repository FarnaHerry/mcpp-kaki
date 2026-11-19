"""动作素材校验器（生成器无关）：把「像素素材该有的约束」变成断言。

对任何提供 `iter_assets()` 契约的生成器模块生效（player_sprites / enemy_sprites）：
    iter_assets() -> [ { group, frame=(w,h), anchor=(x,y), air_ok={state,...},
                         states={state: [PIL.Image, ...]} }, ... ]

检查项（退出码非 0 = 不合格）：
  1. 无细缝      —— 封闭腔中长宽任一方向 < 2px 的块（收尾后会成黑线/黑点）
  2. 不贴画布边  —— 素材须留 1px 描边余量
  3. 锚点精确    —— 接地状态（不在 air_ok）的描边底必须落在 anchor.y
  4. 帧间有位移  —— 相邻帧像素必须变化
  5. 循环态首末帧不同 —— 防「6 帧实则 3 帧」
  6. 确定性      —— 同参数重复渲染逐字节一致
  7. 用色统计    —— 报告每状态成品用色数

用法：python3 tools/pixelart/validate.py [--module enemy_sprites] [--verbose]
"""

from __future__ import annotations

import argparse
import hashlib
import importlib
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from pixel_lib import holes  # noqa: E402


def to_solid(img) -> np.ndarray:
    return np.array(img)[..., 3] > 0


def check_frame(img, label: str, anchor_y: int | None, verbose: bool) -> list[str]:
    errs = []
    solid = to_solid(img)
    h, w = solid.shape

    thin = []
    for comp in holes(solid, conn=8):
        xs = [c[0] for c in comp]
        ys = [c[1] for c in comp]
        if min(max(xs) - min(xs) + 1, max(ys) - min(ys) + 1) < 2:
            thin.append(comp)
    if thin:
        errs.append(f"{label} 细缝未封 ×{len(thin)}（如 {max(thin, key=len)[0]}）→ 成品会成黑线/黑点")

    ys, xs = np.nonzero(solid)
    edge = [p for p in zip(xs, ys) if p[0] in (0, w - 1) or p[1] in (0, h - 1)]
    if edge:
        errs.append(f"{label} 贴画布边（{edge[0]}）→ 描边会被裁")

    if anchor_y is not None:
        bottom = int(ys.max())
        if bottom != anchor_y:
            errs.append(f"{label} 脚底行={bottom}，期望 {anchor_y}")

    if verbose:
        print(f"    {label} 高 {int(ys.max()) - int(ys.min()) + 1}px "
              f"x {int(xs.min())}..{int(xs.max())} 不透明 {int(solid.sum())}")
    return errs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--module", default="player_sprites", help="提供 iter_assets() 的生成器模块名")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    mod = importlib.import_module(args.module)
    groups = mod.iter_assets()
    again = mod.iter_assets()          # 确定性对照
    errs: list[str] = []

    print(f"生成器：{args.module}  原型组 {len(groups)}")
    print("组/状态              帧  锚点   帧间位移(px)            用色")
    for gi, g in enumerate(groups):
        anchor = g["anchor"]
        raw_ok = g["air_ok"]
        air_ok = set(raw_ok) if isinstance(raw_ok, (set, list, tuple)) else (
            set(g["states"]) if raw_ok else set())
        for state, frames in g["states"].items():
            label = f"{g['group']}/{state}"
            solids = [to_solid(f) for f in frames]
            anchor_y = None if state in air_ok else anchor[1]
            for i, f in enumerate(frames):
                errs += check_frame(f, f"{label}[{i}]", anchor_y, args.verbose)
            diffs = [int((solids[i] ^ solids[i + 1]).sum()) for i in range(len(solids) - 1)]
            if any(d == 0 for d in diffs):
                errs.append(f"{label} 存在完全相同相邻帧：{diffs}")
            if len(solids) > 2 and np.array_equal(solids[0], solids[-1]):
                errs.append(f"{label} 首末帧完全相同（帧数虚高）")
            cols = len({tuple(v) for f in frames for v in np.array(f)[to_solid(f)]})
            print(f"{label:<20}{len(frames):>3}  {str(anchor_y or '豁免'):<6} {str(diffs):<22} {cols}")
        # 确定性：逐帧比对两次渲染
        ref = again[gi]
        for state, frames in g["states"].items():
            for i, (a, b) in enumerate(zip(frames, ref["states"][state])):
                if hashlib.sha256(a.tobytes()).digest() != hashlib.sha256(b.tobytes()).digest():
                    errs.append(f"{g['group']}/{state}[{i}] 非确定性渲染")

    if errs:
        print(f"\n[FAIL] {len(errs)} 项不合格：")
        for e in errs[:40]:
            print("  -", e)
        if len(errs) > 40:
            print(f"  … 另有 {len(errs) - 40} 项")
        return 1
    total = sum(len(v) for g in groups for v in g["states"].values())
    print(f"\n[OK] {total} 帧 / {len(groups)} 组全部通过（细缝/贴边/锚点/位移/首末帧/确定性）")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
