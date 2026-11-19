"""战斗 FX：8 类 40 帧 32×32——剑气/旋风/升龙/突进/闪现/增益/护盾/剑扇。"""

from __future__ import annotations

import json
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from pixel_lib import Canvas, fill_region, finish, outline_ring, preview, save_png, sheet, thin_cavities  # noqa: E402

FRAME = 32
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT_DIR = os.path.join(ROOT, "assets", "textures", "fx")
PREVIEW_DIR = os.path.join(ROOT, "docs", "art")
OUTLINE = (75, 139, 160, 255)
CYAN = (184, 232, 237, 230)
WHITE = (248, 255, 252, 245)
GOLD = (235, 204, 113, 235)
PURPLE = (179, 151, 224, 220)
ARC = [(148, 205, 215, 120), (184, 232, 237, 200), (239, 253, 251, 255),
       (163, 218, 226, 205), (119, 182, 203, 120)]
RANGES = [(-155, -105), (-115, -45), (-65, 10), (-15, 55), (35, 105)]
EFFECTS = {"slash": (5, 16), "aoe": (6, 14), "rising": (5, 14), "lunge": (4, 16),
           "blink": (5, 14), "buff": (5, 10), "invuln": (5, 10), "fan": (5, 14)}


def draw(effect: str, frame: int) -> Canvas:
    c = Canvas(FRAME, FRAME)
    if effect == "slash":
        a0, a1 = RANGES[frame]
        outer, inner = [], []
        for i in range(15):
            a = math.radians(a0 + (a1 - a0) * i / 14)
            outer.append((16 + math.cos(a) * 13, 16 + math.sin(a) * 13))
            inner.append((16 + math.cos(a) * 9, 16 + math.sin(a) * 9))
        c.blob(outer + list(reversed(inner)), ARC[frame])
        mid = outer[7]
        c.px(mid[0], mid[1], WHITE)
    elif effect == "aoe":
        r = 3 + frame * 1.8
        c.ellipse(16, 16, r, r, CYAN)
        if r > 3:
            c.ellipse(16, 16, r - 2.2, r - 2.2, (0, 0, 0, 0))
        c.px(16 + r, 16, WHITE)
        c.px(16 - r, 16, WHITE)
    elif effect == "rising":
        top = 25 - frame * 5
        c.blob([(16, top), (11 - frame // 2, 27), (16, 24), (21 + frame // 2, 27)], CYAN)
        c.line(16, top + 2, 16, 24, WHITE, 2)
        c.px(10 + frame * 2, 22 - frame * 3, GOLD)
    elif effect == "lunge":
        length = 7 + frame * 5
        for j, y in enumerate((7, 13, 19, 25)):
            c.hline(3 + j % 2, min(28, 3 + length + j * 2), y, CYAN if j % 2 else WHITE)
        c.px(27, 12 + frame * 2, GOLD)
    elif effect == "blink":
        d = 10 - frame * 2
        c.ellipse(16 - d, 17, 4.0, 6.0, PURPLE)
        c.ellipse(16 + d, 15, 4.0, 6.0, CYAN)
        c.px(16, 6 + frame * 3, WHITE)
        c.px(8 + frame * 3, 25, GOLD)
    elif effect == "buff":
        r = 5 + frame * 2
        c.ellipse(16, 20, r, 3.0, GOLD)
        for j in range(4):
            c.px(8 + j * 5, 20 - frame * 3 - (j % 2) * 2, CYAN if j % 2 else WHITE)
        c.px(16, 8 + frame, GOLD)
    elif effect == "invuln":
        c.ellipse(16, 16, 12.0, 12.0, GOLD)
        c.ellipse(16, 16, 10.0, 10.0, (0, 0, 0, 0))
        for j in range(4):
            a = (j + frame * 0.35) * math.pi / 2
            c.px(round(16 + math.cos(a) * 12), round(16 + math.sin(a) * 12), WHITE)
    elif effect == "fan":
        for j in range(3):
            a = math.radians(-35 + j * 35 + (frame - 2) * 8)
            sx, sy = 9, 25
            tx, ty = sx + math.cos(a) * (13 + frame), sy + math.sin(a) * (13 + frame)
            tx, ty = max(3, min(28, tx)), max(3, min(28, ty))
            c.line(sx, sy, tx, ty, CYAN, 2)
            c.px(tx, ty, WHITE)
        c.rect(6, 24, 5, 3, GOLD)
    return c


def render(effect: str, frame: int):
    c = draw(effect, frame)
    for _ in range(3):
        c.seal_holes(2)
        solid = c.a[..., 3] > 0
        thin = thin_cavities(solid | outline_ring(solid), 2)
        if not thin:
            break
        for comp in thin:
            fill_region(c, comp)
    return finish(c, OUTLINE, light=1.03, dark=0.96, bottom=0.98)


def iter_assets() -> list[dict]:
    return [{"group": name, "frame": (FRAME, FRAME), "anchor": (16, 31), "air_ok": {"play"},
             "states": {"play": [render(name, i) for i in range(EFFECTS[name][0])]}} for name in EFFECTS]


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    rows = []
    meta = {}
    for row, (name, (count, fps)) in enumerate(EFFECTS.items()):
        frames = [render(name, i) for i in range(count)]
        for i, img in enumerate(frames):
            save_png(img, os.path.join(OUT_DIR, f"fx_{name}_{i:02d}.png"))
        rows.append(sheet(frames, FRAME, FRAME, 6))
        meta[name] = {"row": row, "count": count, "fps": fps, "loop": False}
    atlas = sheet(rows, FRAME * 6, FRAME, 1)
    save_png(atlas, os.path.join(OUT_DIR, "fx_sheet.png"))
    manifest = {"generator": "tools/pixelart/fx_sprites.py", "frame_size": FRAME,
                "sheet": "fx_sheet.png", "sheet_cols": 6, "effects": meta}
    with open(os.path.join(OUT_DIR, "manifest.json"), "w", encoding="utf-8") as f:
        json.dump(manifest, f, ensure_ascii=False, indent=2, sort_keys=True)
        f.write("\n")
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    save_png(preview(atlas, scale=6, fw=FRAME, fh=FRAME), os.path.join(PREVIEW_DIR, "fx_preview.png"))
    print(f"[pixelart] {len(EFFECTS)} 类战斗 FX / 40 帧 → assets/textures/fx")


if __name__ == "__main__":
    main()
