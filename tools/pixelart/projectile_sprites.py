"""投射物素材：物理 + 8 元素 × 4 帧，16×16，朝右，由节点 rotation 导向。"""

from __future__ import annotations

import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from pixel_lib import Canvas, fill_region, finish, outline_ring, preview, save_png, sheet, thin_cavities  # noqa: E402

FRAME = 16
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT_DIR = os.path.join(ROOT, "assets", "textures", "projectiles")
PREVIEW_DIR = os.path.join(ROOT, "docs", "art")
OUTLINE = (27, 25, 40, 255)
WHITE = (245, 250, 250, 255)

# key = Element 枚举值：NONE/JIN/MU/SHUI/HUO/TU/LEI/FENG。
ELEMENTS = {
    0: ("physical", (179, 188, 197, 255)),
    1: ("jin", (226, 194, 103, 255)),
    2: ("mu", (95, 181, 100, 255)),
    3: ("shui", (82, 165, 218, 255)),
    4: ("huo", (232, 88, 48, 255)),
    5: ("tu", (172, 128, 72, 255)),
    6: ("lei", (172, 112, 236, 255)),
    7: ("feng", (111, 218, 192, 255)),
}


def draw_projectile(element: int, phase: int) -> Canvas:
    c = Canvas(FRAME, FRAME)
    name, color = ELEMENTS[element]
    if element == 0:
        c.line(3, 8, 12, 8, color, 2)
        c.blob([(13, 8), (9, 4), (10, 8), (9, 12)], WHITE)
        c.line(3, 8, 2, 5 + phase % 2, color, 1)
        c.line(3, 8, 2, 11 - phase % 2, color, 1)
    elif element in {1, 2, 5}:
        # 金属/木/土：有重量的小梭/晶刺。
        c.blob([(13, 8), (8, 4), (3, 6), (2, 9), (8, 11)], color)
        c.line(5, 7, 11, 7, WHITE, 1)
        c.px(3 + phase * 2, 2, color)
    elif element == 6:
        # 雷：折线电弧每帧换形。
        pts = [[(3, 7), (6, 7), (6, 4), (10, 9), (13, 8)],
               [(3, 9), (5, 5), (8, 10), (10, 5), (13, 8)],
               [(3, 6), (6, 10), (8, 5), (11, 10), (13, 8)],
               [(3, 8), (5, 4), (8, 9), (11, 6), (13, 8)]][phase]
        for a, b in zip(pts, pts[1:]):
            c.line(a[0], a[1], b[0], b[1], color, 2)
        c.px(12, 7, WHITE)
    else:
        # 火/水/风：弹头和后方粒子尾迹。
        c.ellipse(9, 8, 4.2, 4.0, color)
        c.ellipse(10, 7, 1.8, 1.8, WHITE)
        for j in range(3):
            x = 5 - j + phase % 2
            y = 6 + ((j + phase) % 3)
            c.px(x, y, color)
        if element == 4:
            c.blob([(6, 8), (4, 5 + phase % 2), (4, 9), (3, 11 - phase % 2), (7, 10)], color)
        elif element == 7:
            c.line(3, 5 + phase % 2, 7, 4, WHITE, 1)
            c.line(2, 11 - phase % 2, 6, 10, WHITE, 1)
    return c


def render(element: int, phase: int):
    c = draw_projectile(element, phase)
    for _ in range(3):
        c.seal_holes(2)
        solid = c.a[..., 3] > 0
        thin = thin_cavities(solid | outline_ring(solid), 2)
        if not thin:
            break
        for comp in thin:
            fill_region(c, comp)
    return finish(c, OUTLINE, light=1.08, dark=0.88, bottom=0.92)


def iter_assets() -> list[dict]:
    return [{"group": ELEMENTS[e][0], "frame": (FRAME, FRAME), "anchor": (8, 15), "air_ok": {"fly"},
             "states": {"fly": [render(e, p) for p in range(4)]}} for e in ELEMENTS]


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    rows = []
    meta = {}
    for row, element in enumerate(ELEMENTS):
        frames = [render(element, p) for p in range(4)]
        for i, img in enumerate(frames):
            save_png(img, os.path.join(OUT_DIR, f"projectile_{ELEMENTS[element][0]}_{i:02d}.png"))
        rows.append(sheet(frames, FRAME, FRAME, 4))
        meta[str(element)] = {"name": ELEMENTS[element][0], "row": row, "count": 4, "fps": 10, "loop": True}
    atlas = sheet(rows, FRAME * 4, FRAME, 1)
    save_png(atlas, os.path.join(OUT_DIR, "projectile_sheet.png"))
    manifest = {"generator": "tools/pixelart/projectile_sprites.py", "frame_size": FRAME,
                "sheet": "projectile_sheet.png", "sheet_cols": 4, "sheet_rows": len(rows), "elements": meta}
    with open(os.path.join(OUT_DIR, "manifest.json"), "w", encoding="utf-8") as f:
        json.dump(manifest, f, ensure_ascii=False, indent=2, sort_keys=True)
        f.write("\n")
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    save_png(preview(atlas, scale=8, fw=FRAME, fh=FRAME), os.path.join(PREVIEW_DIR, "projectile_preview.png"))
    print(f"[pixelart] {len(ELEMENTS)} 类弹体 / 32 帧 → {os.path.relpath(OUT_DIR, ROOT)}")


if __name__ == "__main__":
    main()
