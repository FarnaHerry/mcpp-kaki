"""可平铺地形材质：12 种 16×16  tile + 4×3 图集。无透明、无随机、无时间戳。"""

from __future__ import annotations

import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from pixel_lib import Canvas, preview, save_png, sheet  # noqa: E402

SIZE = 16
COLS = 4
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT_DIR = os.path.join(ROOT, "assets", "textures", "terrain")
PREVIEW_DIR = os.path.join(ROOT, "docs", "art")

STYLES = {
    "grass": ((64, 79, 52, 255), (111, 143, 72, 255), (85, 101, 59, 255)),
    "dirt": ((102, 76, 57, 255), (142, 108, 70, 255), (77, 60, 51, 255)),
    "stone": ((83, 88, 103, 255), (132, 139, 151, 255), (57, 61, 78, 255)),
    "bamboo": ((49, 86, 59, 255), (93, 139, 73, 255), (34, 62, 51, 255)),
    "fire": ((82, 46, 47, 255), (166, 71, 49, 255), (216, 108, 52, 255)),
    "ice": ((113, 158, 180, 255), (190, 225, 234, 255), (75, 119, 153, 255)),
    "sand": ((154, 126, 78, 255), (202, 171, 105, 255), (115, 94, 66, 255)),
    "cloud": ((172, 196, 214, 255), (230, 240, 244, 255), (130, 160, 191, 255)),
    "underworld": ((50, 43, 63, 255), (82, 68, 94, 255), (91, 124, 83, 255)),
    "heavenly": ((194, 199, 201, 255), (240, 233, 207, 255), (154, 145, 135, 255)),
    "water": ((37, 71, 100, 255), (61, 123, 148, 255), (91, 165, 178, 255)),
    "wood": ((104, 71, 55, 255), (151, 106, 67, 255), (70, 54, 50, 255)),
}

# 固定细节点，不用随机数；同一函数可复现任意 tile。
DOTS = ((2, 6), (7, 4), (12, 8), (4, 12), (10, 14), (14, 2))


def render(style: str):
    base, hi, dark = STYLES[style]
    c = Canvas(SIZE, SIZE)
    c.rect(0, 0, SIZE, SIZE, base)
    c.hline(0, 15, 0, hi)
    c.hline(0, 15, 1, hi)
    for i, (x, y) in enumerate(DOTS):
        c.px(x, y, hi if i % 2 == 0 else dark)
        if i % 3 == 0:
            c.px(x + 1, y, dark)
    if style in {"grass", "bamboo"}:
        for x in (2, 6, 11, 14):
            c.px(x, 0, dark)
            c.px(x + 1 if x < 14 else 0, 1, hi)
    elif style in {"stone", "heavenly", "underworld"}:
        c.line(1, 8, 5, 9, dark, 1); c.line(10, 3, 14, 4, dark, 1)
        c.px(5, 10, hi)
    elif style == "fire":
        c.line(2, 10, 5, 7, hi, 1); c.line(5, 7, 7, 9, dark, 1)
        c.line(11, 14, 14, 11, hi, 1)
    elif style == "ice":
        c.line(3, 4, 7, 8, hi, 1); c.line(7, 8, 5, 13, hi, 1)
        c.line(7, 8, 12, 6, dark, 1)
    elif style in {"water", "cloud"}:
        for y in (5, 11):
            for x in (1, 8):
                c.hline(x, x + 5, y, hi)
                c.px(x + 2, y + 1, dark)
    elif style == "wood":
        c.hline(0, 15, 5, dark); c.hline(0, 15, 12, dark)
        c.vline(6, 0, 4, dark); c.vline(11, 6, 11, dark)
    elif style == "sand":
        c.hline(2, 7, 7, hi); c.hline(9, 14, 12, dark)
    return c


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    styles = list(STYLES)
    images = []
    meta = {}
    for i, style in enumerate(styles):
        img_a = render(style).a.copy()
        img_b = render(style).a.copy()
        assert (img_a == img_b).all(), f"非确定性 tile: {style}"
        from PIL import Image
        img = Image.fromarray(img_a, mode="RGBA")
        assert bool((img_a[..., 3] == 255).all()), f"tile 不得透明: {style}"
        images.append(img)
        save_png(img, os.path.join(OUT_DIR, f"tile_{style}.png"))
        meta[style] = {"col": i % COLS, "row": i // COLS, "texture": f"tile_{style}.png"}
    atlas = sheet(images, SIZE, SIZE, COLS)
    save_png(atlas, os.path.join(OUT_DIR, "terrain_sheet.png"))
    manifest = {"generator": "tools/pixelart/terrain_sprites.py", "tile_size": SIZE,
                "sheet": "terrain_sheet.png", "sheet_cols": COLS, "sheet_rows": (len(styles) + COLS - 1) // COLS,
                "styles": meta}
    with open(os.path.join(OUT_DIR, "manifest.json"), "w", encoding="utf-8") as f:
        json.dump(manifest, f, ensure_ascii=False, indent=2, sort_keys=True)
        f.write("\n")
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    save_png(preview(atlas, scale=12, grid=True, fw=SIZE, fh=SIZE), os.path.join(PREVIEW_DIR, "terrain_preview.png"))
    print(f"[pixelart] {len(styles)} 种可平铺地形 → {os.path.relpath(OUT_DIR, ROOT)}")


if __name__ == "__main__":
    main()
