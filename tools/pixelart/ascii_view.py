"""ASCII 像素视图（生成器无关）：把素材帧打成字符画，供无图像输入的 agent 目视校验。

消费任何提供 `iter_assets()` 的生成器模块（player_sprites / enemy_sprites），
按模块自己的 PAL 语义色映射字符；亮色（主体）= 空格，故轮廓可读、暗部成块。
`--shaded` 看最终成品（含明暗），默认看扁平形态（无明暗干扰，形状更清楚）。

用法：
  python3 tools/pixelart/ascii_view.py --module enemy_sprites --group boss --state idle
  python3 tools/pixelart/ascii_view.py --module player_sprites --group player --state run
  python3 tools/pixelart/ascii_view.py --module enemy_sprites --list      # 列组/状态
  python3 tools/pixelart/ascii_view.py --png <path>                       # 看已生成的单帧
"""

from __future__ import annotations

import argparse
import importlib
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from PIL import Image  # noqa: E402

# 语义色 → 字符（两套生成器共用一张表；缺的键用默认字符兜底）
GLYPH = {
    "outline": "#",
    # 玩家
    "skin": "o", "skin_d": ",", "hair": "@", "hair_hl": "*",
    "robe": " ", "robe_d": "=", "robe_fold": "-",
    "accent": "+", "accent_d": "~", "sash": "r", "sash_d": "d",
    "boot": "B", "boot_d": "b",
    "blade": "/", "blade_hl": "|", "hilt": "t", "guard": "g",
    # 敌人（中性灰阶：明度越高字符越轻，主体留白）
    "hi": "^", "body": " ", "body_d": "=", "plate": "%", "dark": "x", "eye": "O",
}


def build_chars(mod) -> dict:
    chars = {tuple(mod.OUTLINE): GLYPH["outline"]}
    for name, col in mod.PAL.items():
        chars[tuple(col)] = GLYPH.get(name, "?")
    return chars


def make_ascii(img: Image.Image, chars: dict):
    def nearest(c) -> str:
        best, bd = "?", 1 << 30
        for col, ch in chars.items():
            d = (c[0] - col[0]) ** 2 + (c[1] - col[1]) ** 2 + (c[2] - col[2]) ** 2
            if d < bd:
                bd, best = d, ch
        return best

    px = img.load()
    return ["".join("." if px[x, y][3] == 0 else nearest(px[x, y]) for x in range(img.width))
            for y in range(img.height)]


def print_frames(arts: list[list[str]], title: str) -> None:
    print(f"\n=== {title} ({len(arts)} 帧) ===")
    w = len(arts[0][0])
    print("    " + "".join(f"|{i:<{w - 1}}" for i in range(len(arts))))
    for y in range(len(arts[0])):
        print(f"{y:2d}  " + "".join(" " + a[y] for a in arts))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--module", default="player_sprites")
    ap.add_argument("--group", help="原型组（玩家=player；敌人=melee/ranged/flying/boss）")
    ap.add_argument("--state", help="状态名；缺省看该组全部状态")
    ap.add_argument("--list", action="store_true", help="只列组/状态/帧数")
    ap.add_argument("--shaded", action="store_true", help="按最终成品渲染（含明暗）")
    ap.add_argument("--png", help="直接看一张 PNG")
    args = ap.parse_args()

    if args.png:
        img = Image.open(args.png).convert("RGBA")
        mod = importlib.import_module(args.module)
        for y, row in enumerate(make_ascii(img, build_chars(mod))):
            print(f"{y:2d} {row}")
        return

    mod = importlib.import_module(args.module)
    groups = mod.iter_assets()
    if args.list or args.group is None:
        for g in groups:
            print(f"{g['group']:<10} frame={g['frame']} anchor={g['anchor']} "
                  f"states=" + ", ".join(f"{k}×{len(v)}" for k, v in g["states"].items()))
        if args.list:
            return

    want = [g for g in groups if args.group is None or g["group"] == args.group]
    if not want:
        print(f"[warn] 无此组：{args.group}（可选 {[g['group'] for g in groups]}）")
        return
    chars = build_chars(mod)
    for g in want:
        states = [args.state] if args.state else list(g["states"])
        for st in states:
            if st not in g["states"]:
                print(f"[warn] 无此状态：{st}")
                continue
            arts = [make_ascii(img, chars) for img in g["states"][st]]
            print_frames(arts, f"{g['group']}/{st}")


if __name__ == "__main__":
    main()
