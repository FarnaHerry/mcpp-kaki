"""全草药/灵植素材：9 种 ×（生长 4 帧 + 枯萎 1 帧），24×24 真像素。"""

from __future__ import annotations

import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from pixel_lib import Canvas, fill_region, finish, outline_ring, preview, save_png, sheet, thin_cavities  # noqa: E402

FRAME = 24
COLS = 4
ANCHOR = (12, 21)
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT_DIR = os.path.join(ROOT, "assets", "textures", "herbs")
PREVIEW_DIR = os.path.join(ROOT, "docs", "art")
OUTLINE = (28, 31, 39, 255)
GREEN = (73, 145, 81, 255)
GREEN_D = (46, 99, 63, 255)
WHITE = (244, 250, 247, 255)
CYAN = (113, 205, 215, 255)
GOLD = (225, 187, 83, 255)
RED = (213, 77, 65, 255)
PURPLE = (145, 91, 166, 255)
WILT = (114, 96, 72, 255)
WILT_D = (73, 70, 61, 255)

HERBS = ["bing_xin_lian", "chi_yan_hua", "jin_gang_teng", "ju_ling_cao",
         "qian_nian_ling_zhi", "spirit_rice", "wu_dao_cha", "xuan_bing_shen", "zhi_xue_cao"]


def _stem(c: Canvas, sway: int, color=GREEN):
    c.line(12, 20, 12 + sway, 9, color, 2)
    c.line(12, 17, 8 + sway, 13, color, 2)
    c.line(12, 15, 16 + sway, 11, color, 2)


def draw_herb(item_id: str, phase: int, wilted: bool) -> Canvas:
    c = Canvas(FRAME, FRAME)
    sway = (0, 1, 0, -1)[phase % 4]
    if wilted:
        # 枯萎仍有物种轮廓，但整体下垂、去饱和，采集状态一眼可读。
        c.line(12, 20, 15, 14, WILT_D, 2)
        c.line(14, 16, 9, 15, WILT, 2)
        c.line(15, 14, 18, 16, WILT, 2)
        if item_id == "qian_nian_ling_zhi":
            c.ellipse(15, 13, 4.0, 2.0, WILT)
        elif item_id in {"bing_xin_lian", "chi_yan_hua"}:
            c.ellipse(15, 14, 3.0, 2.0, WILT)
        elif item_id == "spirit_rice":
            c.line(14, 14, 17, 10, WILT, 2)
        return c

    if item_id in {"zhi_xue_cao", "ju_ling_cao"}:
        _stem(c, sway)
        color = RED if item_id == "zhi_xue_cao" else CYAN
        for x, y, dx in ((11, 16, -5), (12, 13, 5), (12, 10, -4)):
            c.line(x, y, x + dx + sway, y - 3, GREEN, 3)
            c.px(x + dx + sway, y - 3, color)
        c.px(12 + sway, 7, color)
    elif item_id in {"bing_xin_lian", "chi_yan_hua"}:
        _stem(c, sway, GREEN_D)
        color = (180, 226, 241, 255) if item_id == "bing_xin_lian" else RED
        for dx, dy in ((-3, 0), (3, 0), (0, -3), (0, 2)):
            c.ellipse(12 + sway + dx, 8 + dy, 2.5, 2.3, color)
        c.px(12 + sway, 8, GOLD if item_id == "chi_yan_hua" else WHITE)
        c.ellipse(12, 19, 6.5, 1.8, CYAN)
    elif item_id == "jin_gang_teng":
        c.line(9, 19, 14 + sway, 5, GOLD, 3)
        c.line(15, 20, 9 + sway, 7, GREEN_D, 2)
        for x, y in ((9, 15), (14, 11), (10, 7)):
            c.rect(x, y, 4, 3, GREEN)
            c.px(x + 1, y, GOLD)
    elif item_id == "wu_dao_cha":
        _stem(c, sway, GREEN_D)
        for x, y, dx in ((12, 8, -5), (12, 10, 5), (12, 13, -6), (12, 16, 6)):
            c.line(x, y, x + dx + sway, y - 3, GREEN, 4)
            c.px(x + dx + sway, y - 4, WHITE)
    elif item_id == "qian_nian_ling_zhi":
        c.rect(10, 11, 5, 10, (220, 203, 165, 255))
        c.ellipse(12 + sway, 9, 7.0, 4.3, PURPLE)
        c.hline(6 + sway, 7, 16 + sway, WHITE)
        c.px(9 + sway, 5, GOLD)
        c.ellipse(16, 16, 3.5, 2.3, PURPLE)
        c.rect(15, 16, 2, 5, (220, 203, 165, 255))
    elif item_id == "spirit_rice":
        for dx in (-4, 0, 4):
            c.line(12 + dx, 20, 12 + dx + sway, 6 + abs(dx) // 2, GREEN, 2)
            for j in range(4):
                c.px(12 + dx + sway + (1 if j % 2 else -1), 7 + j + abs(dx) // 2, GOLD)
        c.hline(7, 20, 17, GREEN_D)
    elif item_id == "xuan_bing_shen":
        c.line(12, 10, 10, 19, (190, 222, 228, 255), 3)
        c.line(12, 14, 15, 20, WHITE, 2)
        for dx in (-5, 0, 5):
            c.line(12, 10, 12 + dx + sway, 4 + abs(dx) // 2, CYAN, 3)
        c.px(11, 12, WHITE)
    c.hline(9, 20, 15, GREEN_D)
    return c


def render(item_id: str, phase: int, wilted: bool):
    c = draw_herb(item_id, phase, wilted)
    for _ in range(3):
        c.seal_holes(2)
        solid = c.a[..., 3] > 0
        thin = thin_cavities(solid | outline_ring(solid), 2)
        if not thin:
            break
        for comp in thin:
            fill_region(c, comp)
    return finish(c, OUTLINE, light=1.07, dark=0.86, bottom=0.90)


def iter_assets() -> list[dict]:
    groups = []
    for item_id in HERBS:
        groups.append({"group": item_id, "frame": (FRAME, FRAME), "anchor": ANCHOR, "air_ok": set(),
                       "states": {"grown": [render(item_id, p, False) for p in range(4)],
                                  "wilted": [render(item_id, 0, True)]}})
    return groups


def main():
    groups = iter_assets()
    rows = []
    meta = {}
    for i, g in enumerate(groups):
        item_id = g["group"]
        meta[item_id] = {"grown": {"row": i * 2, "count": 4, "fps": 4, "loop": True},
                         "wilted": {"row": i * 2 + 1, "count": 1, "fps": 1, "loop": False}}
        for state in ("grown", "wilted"):
            frames = g["states"][state]
            for j, img in enumerate(frames):
                save_png(img, os.path.join(OUT_DIR, f"herb_{item_id}_{state}_{j:02d}.png"))
            rows.append(sheet(frames, FRAME, FRAME, COLS))
    atlas = sheet(rows, FRAME * COLS, FRAME, 1)
    os.makedirs(OUT_DIR, exist_ok=True)
    save_png(atlas, os.path.join(OUT_DIR, "herb_sheet.png"))
    manifest = {"generator": "tools/pixelart/herb_sprites.py", "frame_size": FRAME,
                "sheet": "herb_sheet.png", "sheet_cols": COLS, "sheet_rows": len(rows),
                "anchor": {"x": ANCHOR[0], "y": ANCHOR[1]}, "herbs": meta}
    with open(os.path.join(OUT_DIR, "manifest.json"), "w", encoding="utf-8") as f:
        json.dump(manifest, f, ensure_ascii=False, indent=2, sort_keys=True)
        f.write("\n")
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    save_png(preview(atlas, scale=6, fw=FRAME, fh=FRAME), os.path.join(PREVIEW_DIR, "herb_preview.png"))
    print(f"[pixelart] {len(HERBS)} 灵植 / 45 帧 → {os.path.relpath(OUT_DIR, ROOT)}")


if __name__ == "__main__":
    main()
