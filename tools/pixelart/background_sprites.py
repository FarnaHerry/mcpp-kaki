"""完整远景背景：12 套 480×270 真像素场景，确定性生成，无外部素材。"""

from __future__ import annotations

import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from pixel_lib import Canvas, mix, save_png, sheet  # noqa: E402

W, H = 480, 270
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT_DIR = os.path.join(ROOT, "assets", "textures", "backgrounds")
PREVIEW_DIR = os.path.join(ROOT, "docs", "art")

STYLES = ["dongsheng", "xiniuhe", "nanzhanbu", "beijulu", "tianjie", "difu",
          "yunhai", "dongtian", "cave", "water", "fire", "palace"]

SKY = {
    "dongsheng": ((45, 43, 75, 255), (191, 116, 112, 255)),
    "xiniuhe": ((92, 45, 54, 255), (220, 133, 77, 255)),
    "nanzhanbu": ((78, 128, 161, 255), (210, 189, 145, 255)),
    "beijulu": ((23, 38, 74, 255), (91, 139, 161, 255)),
    "tianjie": ((105, 144, 179, 255), (237, 214, 157, 255)),
    "difu": ((20, 18, 35, 255), (58, 65, 64, 255)),
    "yunhai": ((51, 91, 137, 255), (160, 200, 213, 255)),
    "dongtian": ((37, 67, 78, 255), (122, 178, 162, 255)),
    "cave": ((19, 21, 33, 255), (52, 57, 70, 255)),
    "water": ((12, 38, 67, 255), (35, 100, 119, 255)),
    "fire": ((44, 24, 34, 255), (131, 53, 39, 255)),
    "palace": ((76, 83, 118, 255), (207, 181, 145, 255)),
}


def _gradient(c: Canvas, top, bottom):
    for y in range(H):
        c.hline(0, W - 1, y, mix(top, bottom, y / (H - 1)))


def _mountains(c: Canvas, y: int, color, seed: int, height: int):
    pts = [(0, H), (0, y)]
    for i in range(13):
        x = i * 40
        peak = y - ((i * 17 + seed * 13) % height)
        pts.extend([(x, y), (x + 20, peak)])
    pts.extend([(W, y), (W, H)])
    c.blob(pts, color)


def _cloud(c: Canvas, x: int, y: int, color, scale=1):
    for dx, dy, rx, ry in ((0, 0, 22, 6), (18, -4, 18, 8), (38, 1, 24, 6), (15, 4, 34, 5)):
        c.ellipse(x + dx * scale, y + dy * scale, rx * scale, ry * scale, color)


def _stars(c: Canvas, color, count=34):
    for i in range(count):
        c.px((i * 73 + 19) % W, (i * 31 + 7) % 125, color)


def draw_background(style: str) -> Canvas:
    c = Canvas(W, H)
    top, bottom = SKY[style]
    _gradient(c, top, bottom)

    if style in {"beijulu", "difu", "dongtian", "cave", "water"}:
        _stars(c, (178, 207, 201, 255) if style != "difu" else (105, 157, 111, 255))

    if style == "dongsheng":
        c.ellipse(375, 64, 27, 27, (240, 189, 126, 255))
        for x, y in ((35, 67), (185, 92), (330, 118)):
            _cloud(c, x, y, (207, 156, 151, 255))
        _mountains(c, 179, (99, 78, 102, 255), 2, 65)
        _mountains(c, 210, (66, 65, 79, 255), 5, 55)
        for x in (60, 255, 400):
            c.rect(x, 201, 42, 28, (66, 55, 59, 255))
            c.blob([(x - 6, 202), (x + 21, 188), (x + 48, 202)], (100, 66, 65, 255))
    elif style == "xiniuhe":
        c.ellipse(88, 66, 32, 32, (241, 170, 90, 255))
        _mountains(c, 188, (139, 71, 62, 255), 3, 56)
        for x, w, h in ((22, 70, 60), (174, 86, 87), (342, 96, 66)):
            c.rect(x, 220 - h, w, h, (105, 58, 54, 255))
            c.rect(x - 5, 220 - h, w + 10, 7, (155, 81, 58, 255))
    elif style == "nanzhanbu":
        _cloud(c, 45, 54, (218, 220, 199, 255)); _cloud(c, 310, 85, (218, 220, 199, 255))
        _mountains(c, 188, (118, 133, 125, 255), 4, 48)
        for x in (55, 195, 350):
            c.rect(x + 12, 159, 14, 67, (92, 77, 67, 255))
            for y, w in ((157, 50), (174, 40), (190, 30)):
                c.blob([(x, y), (x + 25, y - 10), (x + w, y), (x + w - 5, y + 4), (x + 4, y + 4)], (130, 79, 59, 255))
    elif style == "beijulu":
        for i in range(5):
            c.line(0, 35 + i * 10, W, 12 + i * 11, (66, 130 + i * 8, 145 + i * 6, 255), 3)
        _mountains(c, 183, (100, 135, 160, 255), 2, 90)
        _mountains(c, 215, (60, 87, 119, 255), 6, 63)
        for i in range(13):
            x = i * 40 + 20
            peak = 183 - ((i * 17 + 26) % 90)
            c.line(x, peak, x + 10, peak + 22, (201, 225, 229, 255), 2)
    elif style == "tianjie":
        c.ellipse(378, 56, 30, 30, (250, 231, 174, 255))
        for x, y in ((0, 75), (130, 117), (300, 145), (40, 200), (330, 218)):
            _cloud(c, x, y, (229, 231, 216, 255))
        c.rect(190, 137, 106, 10, (174, 147, 108, 255))
        for x in (202, 228, 254, 280):
            c.rect(x, 101, 8, 38, (211, 191, 147, 255))
        c.blob([(181, 102), (243, 75), (306, 102)], (187, 125, 86, 255))
    elif style == "difu":
        _mountains(c, 174, (43, 42, 57, 255), 2, 90)
        _mountains(c, 217, (30, 33, 43, 255), 8, 50)
        c.rect(208, 121, 64, 103, (29, 31, 38, 255))
        c.blob([(188, 125), (240, 89), (292, 125)], (47, 48, 52, 255))
        for x, y in ((83, 128), (345, 103), (391, 178), (117, 203)):
            c.ellipse(x, y, 4, 7, (112, 172, 111, 255))
            c.px(x, y - 2, (211, 221, 153, 255))
    elif style == "yunhai":
        for x, y in ((-20, 55), (170, 88), (330, 45), (40, 160), (260, 178), (360, 225)):
            _cloud(c, x, y, (200, 223, 226, 255))
        for x, y in ((90, 145), (350, 125)):
            c.ellipse(x, y, 40, 8, (124, 160, 175, 255))
            c.blob([(x - 32, y), (x + 32, y), (x + 8, y + 32), (x - 8, y + 25)], (83, 112, 132, 255))
    elif style == "dongtian":
        for x, y in ((15, 50), (300, 80), (160, 135)):
            _cloud(c, x, y, (157, 198, 185, 255))
        c.ellipse(240, 160, 132, 24, (82, 126, 103, 255))
        c.blob([(115, 159), (365, 159), (285, 229), (218, 246), (170, 211)], (57, 91, 79, 255))
        c.ellipse(240, 121, 55, 20, (116, 178, 154, 255))
        c.ellipse(240, 121, 43, 14, (75, 127, 116, 255))
    elif style == "cave":
        _mountains(c, 204, (38, 41, 52, 255), 4, 70)
        for i in range(16):
            x = i * 32
            depth = 18 + (i * 19) % 50
            c.blob([(x, 0), (x + 28, 0), (x + 15, depth)], (28, 31, 43, 255))
        for x in (75, 260, 410):
            c.line(x, 170, x + 5, 215, (88, 115, 124, 255), 2)
    elif style == "water":
        for x in (40, 170, 340, 440):
            c.line(x, 0, x + 35, 190, (65, 135, 151, 255), 8)
        for i in range(25):
            x = (i * 61 + 30) % W; y = (i * 43 + 20) % 220
            c.ellipse(x, y, 2 + i % 2, 2 + i % 2, (107, 185, 190, 255))
        _mountains(c, 222, (25, 72, 84, 255), 3, 35)
        for x in (45, 205, 385):
            c.line(x, 225, x - 8, 182, (111, 80, 116, 255), 5)
            c.line(x - 4, 204, x + 9, 190, (151, 105, 126, 255), 3)
    elif style == "fire":
        _mountains(c, 195, (86, 43, 43, 255), 4, 72)
        for x in (45, 165, 300, 420):
            c.blob([(x, 224), (x + 5, 160), (x + 13, 194), (x + 20, 135), (x + 26, 224)], (164, 65, 39, 255))
            c.line(x + 12, 213, x + 19, 158, (231, 133, 57, 255), 3)
    elif style == "palace":
        for x, y in ((0, 78), (310, 64), (90, 181), (340, 206)):
            _cloud(c, x, y, (220, 216, 196, 255))
        c.rect(55, 97, 370, 16, (146, 119, 100, 255))
        for x in (80, 150, 220, 290, 360):
            c.rect(x, 111, 18, 119, (179, 156, 126, 255))
            c.rect(x - 4, 111, 26, 7, (221, 194, 139, 255))
        c.blob([(35, 98), (240, 52), (445, 98)], (138, 88, 76, 255))
    return c


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    images = []
    meta = {}
    for style in STYLES:
        a = draw_background(style).a.copy()
        b = draw_background(style).a.copy()
        assert (a == b).all() and bool((a[..., 3] == 255).all()), style
        from PIL import Image
        img = Image.fromarray(a, mode="RGBA")
        images.append(img)
        save_png(img, os.path.join(OUT_DIR, f"bg_{style}.png"))
        meta[style] = {"texture": f"bg_{style}.png", "size": [W, H]}
    atlas = sheet(images, W, H, 2)
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    save_png(atlas, os.path.join(PREVIEW_DIR, "background_preview.png"))
    manifest = {"generator": "tools/pixelart/background_sprites.py", "styles": meta}
    with open(os.path.join(OUT_DIR, "manifest.json"), "w", encoding="utf-8") as f:
        json.dump(manifest, f, ensure_ascii=False, indent=2, sort_keys=True)
        f.write("\n")
    print(f"[pixelart] {len(STYLES)} 套 480×270 背景 → {os.path.relpath(OUT_DIR, ROOT)}")


if __name__ == "__main__":
    main()
