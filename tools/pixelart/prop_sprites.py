"""交互设施素材：10 类 prop / 44 帧，统一 32×48，地面锚点 (16,44)。"""

from __future__ import annotations

import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from pixel_lib import Canvas, fill_region, finish, outline_ring, preview, save_png, sheet, thin_cavities  # noqa: E402

FRAME = 48
W = 32
ANCHOR = (16, 44)
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT_DIR = os.path.join(ROOT, "assets", "textures", "props")
PREVIEW_DIR = os.path.join(ROOT, "docs", "art")
OUTLINE = (27, 27, 40, 255)
STONE = (104, 111, 129, 255)
STONE_D = (62, 66, 85, 255)
WHITE = (236, 245, 245, 255)
CYAN = (100, 207, 215, 255)
GOLD = (222, 181, 80, 255)
WOOD = (139, 91, 62, 255)
WOOD_D = (83, 57, 52, 255)
RED = (202, 76, 65, 255)
PURPLE = (132, 89, 172, 255)
GREEN = (91, 158, 100, 255)

PROPS = ["portal", "storage_chest", "teleport_array", "refine_spot", "meditate_spot",
         "pill_lab", "expand_monument", "jlz_eye", "scene_gate", "cond_portal"]


def _spark(c: Canvas, phase: int, color=CYAN):
    x, y = ((3, 16), (27, 12), (26, 29), (4, 27))[phase]
    c.px(x, y, color)
    if phase % 2:
        c.px(x, y - 1, color)


def _frame(c: Canvas, color=STONE):
    c.rect(4, 7, 5, 37, color); c.rect(23, 7, 5, 37, color)
    c.rect(4, 7, 24, 5, color)
    c.rect(5, 9, 3, 34, STONE_D); c.rect(24, 9, 3, 34, STONE_D)
    c.hline(6, 25, 8, GOLD)


def draw_prop(prop: str, state: str, phase: int) -> Canvas:
    c = Canvas(W, FRAME)
    if prop == "portal":
        _frame(c)
        c.rect(9, 12, 14, 31, (42, 63, 91, 255))
        for j in range(7):
            x = 10 + ((j * 4 + phase * 2) % 12)
            y = 15 + j * 4
            c.rect(x, y, 3, 2, CYAN if j % 2 else PURPLE)
        _spark(c, phase, WHITE)
        c.px(3 + phase * 2, 5 - phase % 2, CYAN)
    elif prop == "storage_chest":
        c.rect(5, 25, 22, 18, WOOD)
        c.rect(4, 23, 24, 7, WOOD_D)
        c.rect(6, 24, 20, 4, WOOD)
        c.vline(7, 25, 42, GOLD); c.vline(24, 25, 42, GOLD)
        c.rect(14, 29, 5, 7, GOLD)
        c.px(16, 32, WOOD_D)
        c.hline(5, 26, 43, WOOD_D)
        _spark(c, phase, GOLD)
        c.px(3 + phase * 2, 21 - phase % 2, GOLD)
    elif prop == "teleport_array":
        active = state == "active"
        glow = CYAN if active else STONE_D
        c.ellipse(16, 41, 13.0, 3.0, STONE_D)
        c.ellipse(16, 40, 11.0, 2.2, glow)
        c.blob([(10, 38), (9, 16), (13, 8), (20, 8), (23, 16), (21, 38)], STONE)
        c.blob([(12, 35), (12, 17), (16, 12), (19, 17), (19, 35)], glow)
        c.line(13, 22 + phase % 2, 19, 18 + phase % 2, WHITE if active else STONE_D, 2)
        c.line(13, 27, 19, 30 - phase % 2, glow, 2)
        _spark(c, phase, CYAN if active else STONE)
    elif prop == "refine_spot":
        c.ellipse(16, 41, 12.0, 3.0, STONE_D)
        c.rect(8, 32, 16, 10, STONE)
        c.rect(5, 29, 22, 5, STONE)
        c.hline(9, 23, 36, GOLD)
        c.blob([(16, 29), (11, 22), (15, 23), (16, 14 + phase), (21, 23), (19, 29)], RED)
        c.blob([(16, 27), (14, 22), (18, 20 + phase), (19, 27)], GOLD)
        _spark(c, phase, GOLD)
    elif prop == "meditate_spot":
        c.ellipse(16, 40, 12.0, 3.0, PURPLE)
        c.ellipse(16, 37, 10.0, 3.0, (104, 88, 137, 255))
        c.hline(8, 24, 37, GOLD)
        c.hline(8, 24, 43, PURPLE)
        c.rect(23, 25, 4, 11, WOOD)
        c.line(25, 25, 24, 17, GOLD, 1)
        c.px(23 - phase % 2, 14 - phase, WHITE)
        c.px(26 + phase % 2, 11 - phase, CYAN)
        _spark(c, phase, WHITE)
    elif prop == "pill_lab":
        c.rect(9, 36, 14, 7, STONE_D)
        c.hline(9, 23, 43, STONE_D)
        c.ellipse(16, 26, 10.0, 12.0, (120, 88, 69, 255))
        c.rect(7, 19, 18, 4, GOLD)
        c.ellipse(16, 18, 8.0, 3.0, STONE_D)
        c.rect(11, 13, 10, 5, GOLD)
        c.rect(12, 28, 8, 7, STONE_D)
        c.blob([(16, 33), (12, 29), (15, 25 - phase % 2), (17, 28), (20, 25), (19, 32)], RED)
        c.px(16, 29, GOLD)
        _spark(c, phase, GOLD)
    elif prop == "expand_monument":
        c.ellipse(16, 41, 12.0, 3.0, STONE_D)
        c.blob([(8, 39), (9, 12), (13, 7), (22, 9), (24, 39)], STONE)
        c.rect(11, 15, 11, 18, STONE_D)
        c.rect(15, 18, 2, 12, GREEN); c.rect(12, 23, 12, 2, GREEN)
        c.px(16, 23 + phase, CYAN)
        _spark(c, phase, GREEN)
    elif prop == "jlz_eye":
        c.ellipse(16, 40, 13.0, 3.0, STONE_D)
        c.hline(8, 24, 43, STONE_D)
        c.ellipse(16, 36, 11.0, 5.0, PURPLE)
        c.ellipse(16, 35, 7.0, 3.0, CYAN)
        c.ellipse(16, 35, 2.5, 2.0, WHITE)
        for j in range(4):
            a = (j + phase * 0.5) * 1.570796
            import math
            c.px(round(16 + math.cos(a) * 10), round(34 + math.sin(a) * 5), GOLD)
        c.px(16, 23 - phase, CYAN)
    elif prop == "scene_gate":
        _frame(c, PURPLE)
        c.rect(9, 12, 14, 31, (48, 40, 68, 255))
        for j in range(5):
            c.px(11 + ((j * 3 + phase) % 10), 16 + j * 5, PURPLE)
        c.rect(7, 4, 18, 4, GOLD)
        _spark(c, phase, PURPLE)
        c.px(4 + phase * 3, 2, PURPLE)
    elif prop == "cond_portal":
        _frame(c, GOLD)
        c.rect(9, 12, 14, 31, (87, 77, 100, 255))
        c.rect(13, 24, 7, 8, GOLD)
        c.px(16, 27, STONE_D)
        c.line(11, 16 + phase, 21, 16 + phase, WHITE, 1)
        _spark(c, phase, GOLD)
        c.px(3 + phase * 2, 5 - phase % 2, GOLD)
    return c


def render(prop: str, state: str, phase: int):
    c = draw_prop(prop, state, phase)
    for _ in range(3):
        c.seal_holes(2)
        solid = c.a[..., 3] > 0
        thin = thin_cavities(solid | outline_ring(solid), 2)
        if not thin:
            break
        for comp in thin:
            fill_region(c, comp)
    return finish(c, OUTLINE, light=1.07, dark=0.86, bottom=0.90)


def states_for(prop: str) -> list[str]:
    return ["inactive", "active"] if prop == "teleport_array" else ["idle"]


def iter_assets() -> list[dict]:
    return [{"group": p, "frame": (W, FRAME), "anchor": ANCHOR, "air_ok": set(),
             "states": {s: [render(p, s, i) for i in range(4)] for s in states_for(p)}} for p in PROPS]


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    rows = []
    meta = {}
    row = 0
    for prop in PROPS:
        meta[prop] = {}
        for state in states_for(prop):
            frames = [render(prop, state, i) for i in range(4)]
            for i, img in enumerate(frames):
                save_png(img, os.path.join(OUT_DIR, f"prop_{prop}_{state}_{i:02d}.png"))
            rows.append(sheet(frames, W, FRAME, 4))
            meta[prop][state] = {"row": row, "count": 4, "fps": 5, "loop": True}
            row += 1
    atlas = sheet(rows, W * 4, FRAME, 1)
    save_png(atlas, os.path.join(OUT_DIR, "prop_sheet.png"))
    manifest = {"generator": "tools/pixelart/prop_sprites.py", "frame_size": [W, FRAME],
                "sheet": "prop_sheet.png", "sheet_cols": 4, "sheet_rows": len(rows),
                "anchor": {"x": ANCHOR[0], "y": ANCHOR[1]}, "props": meta}
    with open(os.path.join(OUT_DIR, "manifest.json"), "w", encoding="utf-8") as f:
        json.dump(manifest, f, ensure_ascii=False, indent=2, sort_keys=True)
        f.write("\n")
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    save_png(preview(atlas, scale=5, fw=W, fh=FRAME), os.path.join(PREVIEW_DIR, "prop_preview.png"))
    print(f"[pixelart] {len(PROPS)} 类设施 / {len(rows) * 4} 帧 → {os.path.relpath(OUT_DIR, ROOT)}")


if __name__ == "__main__":
    main()
