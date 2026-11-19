"""全物品图标生成器：data/items.json 的每个 id 都有独立 16×16 像素图标。

产出：assets/textures/items/item_<id>.png、item_sheet.png、manifest.json；
docs/art/item_preview.png 为 ×8 预览。图标顺序按 id 排序，manifest 记录格子坐标。
确定性：同参数重跑 bit-identical；可用 validate.py --module item_sprites 校验。
"""

from __future__ import annotations

import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from pixel_lib import Canvas, fill_region, finish, outline_ring, preview, save_png, sheet, thin_cavities  # noqa: E402

FRAME = 16
COLS = 7
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT_DIR = os.path.join(ROOT, "assets", "textures", "items")
PREVIEW_DIR = os.path.join(ROOT, "docs", "art")
OUTLINE = (30, 27, 43, 255)
WHITE = (246, 248, 244, 255)
PAPER = (235, 220, 181, 255)
GOLD = (232, 190, 83, 255)
INK = (62, 62, 80, 255)
GREEN = (83, 160, 91, 255)
CYAN = (91, 190, 207, 255)
RED = (211, 76, 68, 255)
PURPLE = (145, 91, 176, 255)


def _pill(c: Canvas, color, mark=WHITE):
    c.ellipse(8, 8.5, 4.5, 4.5, color)
    c.hline(4, 12, 8, INK)
    c.hline(5, 11, 9, mark)
    c.px(6, 5, WHITE)
    c.px(7, 5, WHITE)


def _stone(c: Canvas, color, hi=WHITE):
    c.blob([(8, 2), (12, 6), (11, 12), (7, 14), (3, 9), (5, 4)], color)
    c.blob([(8, 3), (9, 7), (7, 12), (5, 8)], hi)
    c.line(9, 7, 12, 6, INK, 1)
    c.line(9, 7, 10, 11, INK, 1)


def _sword(c: Canvas, color=CYAN, ornate=False):
    c.line(4, 12, 11, 4, color, 2)
    c.line(5, 11, 12, 3, WHITE, 1)
    c.px(12, 3, WHITE)
    c.line(3, 8, 8, 13, GOLD, 2 if ornate else 1)
    c.line(3, 13, 5, 10, INK, 2)
    c.px(2, 13, GOLD if ornate else RED)


def _robe(c: Canvas):
    c.blob([(5, 3), (8, 5), (11, 3), (14, 7), (12, 10), (11, 8),
            (12, 13), (4, 13), (5, 8), (4, 10), (2, 7)], (181, 211, 225, 255))
    c.line(5, 4, 9, 9, WHITE, 2)
    c.line(11, 4, 8, 8, CYAN, 1)
    c.hline(5, 11, 10, INK)
    c.hline(5, 11, 12, CYAN)


def _scroll(c: Canvas, sigil=PURPLE, kind="seal"):
    c.rect(4, 3, 8, 11, PAPER)
    c.rect(3, 2, 10, 2, WHITE)
    c.rect(3, 12, 10, 2, WHITE)
    c.vline(3, 3, 13, GOLD)
    c.vline(12, 3, 13, GOLD)
    if kind == "bagua":
        c.rect(6, 5, 5, 5, INK)
        c.px(7, 6, WHITE); c.px(9, 8, WHITE)
        c.px(8, 7, GOLD)
    elif kind == "rope":
        c.line(6, 5, 10, 10, GOLD, 2)
        c.line(10, 5, 6, 10, sigil, 1)
    elif kind == "wind":
        c.ellipse(8, 7, 2.8, 2.8, CYAN)
        c.hline(5, 11, 10, sigil)
    elif kind == "clone":
        c.rect(5, 5, 3, 5, sigil)
        c.rect(9, 5, 3, 5, INK)
        c.px(6, 6, WHITE); c.px(10, 6, WHITE)
    else:
        c.rect(6, 5, 5, 5, sigil)
        c.px(8, 7, WHITE)
        c.hline(6, 11, 10, INK)


def _herb(c: Canvas, color, kind="leaf"):
    c.line(8, 13, 8, 6, GREEN, 2)
    if kind == "lotus":
        for dx, dy in ((-3, 0), (3, 0), (0, -2), (0, 1)):
            c.ellipse(8 + dx, 6 + dy, 2.4, 2.2, color)
        c.px(8, 6, GOLD)
        c.hline(4, 12, 12, CYAN)
    elif kind == "vine":
        c.line(5, 13, 10, 3, color, 2)
        c.line(11, 13, 6, 4, GOLD, 1)
        c.rect(4, 5, 3, 3, GREEN); c.rect(10, 8, 3, 3, GREEN)
    elif kind == "tea":
        for x, y, d in ((8, 5, -1), (8, 8, 1), (8, 11, -1)):
            c.line(x, y, x + d * 4, y - 2, color, 3)
            c.px(x + d * 5, y - 3, WHITE)
    elif kind == "lingzhi":
        c.rect(7, 8, 3, 6, PAPER)
        c.ellipse(8, 6, 5.5, 3.4, color)
        c.hline(4, 5, 11, WHITE)
        c.px(6, 4, GOLD)
    elif kind == "ginseng":
        c.line(8, 7, 6, 12, color, 3)
        c.line(8, 9, 11, 12, color, 2)
        c.line(8, 6, 4, 3, CYAN, 2)
        c.line(8, 6, 12, 3, CYAN, 2)
        c.px(7, 8, WHITE)
    else:
        c.line(8, 8, 3, 5, color, 3)
        c.line(8, 10, 12, 6, color, 3)
        c.line(8, 6, 10, 3, color, 2)
        c.px(4, 4, WHITE)


def _fruit(c: Canvas, color, kind="peach"):
    if kind == "renshen":
        c.ellipse(8, 9, 3.8, 4.8, color)
        c.px(6, 8, WHITE); c.px(10, 10, GOLD)
        c.line(8, 5, 5, 3, GREEN, 2)
        c.line(8, 5, 11, 3, GREEN, 2)
    else:
        c.ellipse(8, 9, 5.0, 4.5, color)
        c.px(8, 5, RED)
        c.line(8, 5, 11, 3, GREEN, 2)
        c.px(6, 7, WHITE)
        if kind == "pantao":
            c.line(5, 10, 11, 7, GOLD, 1)
            c.px(10, 11, PURPLE)


def _food(c: Canvas, kind):
    if kind == "dry":
        c.blob([(4, 4), (11, 3), (13, 9), (9, 13), (3, 10)], (190, 139, 78, 255))
        c.line(5, 5, 10, 9, PAPER, 2)
        c.px(10, 4, WHITE)
        return
    c.ellipse(8, 7, 5.2, 3.2, WHITE if kind == "brown" else GOLD)
    c.blob([(3, 8), (13, 8), (11, 13), (5, 13)], (155, 92, 62, 255) if kind == "brown" else CYAN)
    c.hline(5, 11, 11, PAPER)
    if kind == "spirit":
        c.px(8, 4, WHITE); c.px(11, 6, WHITE)


def _orb(c: Canvas, color, kind="orb"):
    if kind == "pearl":
        c.blob([(2, 10), (5, 5), (8, 9), (11, 4), (14, 10), (8, 13)], PURPLE)
    if kind == "sarira":
        c.blob([(8, 2), (11, 6), (8, 5), (5, 7)], GOLD)
    c.ellipse(8, 8, 4.5, 4.5, color)
    c.ellipse(7, 7, 2.0, 2.0, WHITE)
    if kind == "lei":
        c.line(9, 4, 6, 8, GOLD, 2)
        c.line(6, 8, 10, 8, GOLD, 1)
        c.line(10, 8, 7, 12, GOLD, 2)


def draw_icon(item_id: str) -> Canvas:
    c = Canvas(FRAME, FRAME)
    pills = {
        "healing_pill": RED, "qi_pill": CYAN, "foundation_pill": GOLD,
        "bing_xin_dan": (154, 219, 237, 255), "chi_yan_dan": (229, 95, 55, 255),
        "jin_gang_dan": (191, 159, 78, 255), "wu_dao_dan": PURPLE,
        "da_huan_dan": (224, 94, 123, 255), "xuan_long_dan": (93, 145, 180, 255),
        "xuan_ming_dan": (74, 87, 139, 255),
    }
    if item_id in pills:
        _pill(c, pills[item_id], GOLD if item_id in {"foundation_pill", "xuan_long_dan", "xuan_ming_dan"} else WHITE)
    elif item_id.startswith("spirit_stone"):
        tier = {"spirit_stone": (113, 156, 143, 255), "spirit_stone_mid": CYAN,
                "spirit_stone_high": PURPLE, "spirit_stone_peak": GOLD}[item_id]
        _stone(c, tier)
    elif item_id in {"flying_sword", "iron_sword", "qing_feng_gu_jian"}:
        _sword(c, {"flying_sword": CYAN, "iron_sword": (157, 167, 177, 255),
                   "qing_feng_gu_jian": (90, 185, 139, 255)}[item_id], item_id != "iron_sword")
    elif item_id == "ding_hai_shen_zhen":
        c.line(8, 3, 8, 12, RED, 3)
        c.vline(8, 3, 12, GOLD)
        c.hline(5, 3, 11, GOLD); c.hline(5, 12, 11, GOLD)
    elif item_id == "protect_robe":
        _robe(c)
    elif item_id in {"shen_wai_can_juan", "pu_ti_xin_fa_juan", "ba_gua_lu", "kun_xian_sheng", "ding_feng_zhu"}:
        kind = {"shen_wai_can_juan": "clone", "ba_gua_lu": "bagua", "kun_xian_sheng": "rope",
                "ding_feng_zhu": "wind"}.get(item_id, "seal")
        _scroll(c, GREEN if item_id == "pu_ti_xin_fa_juan" else PURPLE, kind)
    elif item_id in {"zhi_xue_cao", "ju_ling_cao"}:
        _herb(c, RED if item_id == "zhi_xue_cao" else CYAN)
    elif item_id == "bing_xin_lian":
        _herb(c, (191, 231, 246, 255), "lotus")
    elif item_id == "chi_yan_hua":
        _herb(c, RED, "lotus")
    elif item_id == "jin_gang_teng":
        _herb(c, GOLD, "vine")
    elif item_id == "wu_dao_cha":
        _herb(c, GREEN, "tea")
    elif item_id == "qian_nian_ling_zhi":
        _herb(c, PURPLE, "lingzhi")
    elif item_id == "xuan_bing_shen":
        _herb(c, (176, 220, 235, 255), "ginseng")
    elif item_id in {"brown_rice", "dry_ration", "spirit_rice"}:
        _food(c, {"brown_rice": "brown", "dry_ration": "dry", "spirit_rice": "spirit"}[item_id])
    elif item_id in {"xian_tao", "ren_shen_guo", "pan_tao"}:
        _fruit(c, {"xian_tao": (242, 143, 154, 255), "ren_shen_guo": (237, 210, 143, 255),
                   "pan_tao": (249, 170, 151, 255)}[item_id],
               "renshen" if item_id == "ren_shen_guo" else ("pantao" if item_id == "pan_tao" else "peach"))
    elif item_id == "ba_jiao_shan":
        c.blob([(7, 10), (2, 5), (5, 2), (12, 3), (14, 7)], GREEN)
        c.line(7, 9, 12, 13, GOLD, 2)
        c.line(7, 9, 4, 4, CYAN, 1); c.line(7, 9, 11, 4, CYAN, 1)
    elif item_id == "long_gu":
        c.line(4, 11, 11, 4, WHITE, 3)
        c.ellipse(4, 11, 2.4, 2.4, WHITE); c.ellipse(11, 4, 2.4, 2.4, WHITE)
        c.line(7, 8, 10, 11, CYAN, 1)
    elif item_id == "xuan_bing_sui":
        _stone(c, (152, 216, 241, 255), CYAN)
        c.px(4, 3, WHITE); c.px(12, 12, WHITE)
    elif item_id in {"bi_shui_zhu", "qian_nian_zhen_zhu", "she_li_zi", "li_huo_zhu", "ding_hai_zhu"}:
        _orb(c, {"bi_shui_zhu": CYAN, "qian_nian_zhen_zhu": WHITE, "she_li_zi": GOLD,
                 "li_huo_zhu": RED, "ding_hai_zhu": (91, 140, 221, 255)}[item_id],
             "pearl" if item_id == "qian_nian_zhen_zhu" else ("sarira" if item_id == "she_li_zi" else "orb"))
    elif item_id == "lei_wen_pei":
        _orb(c, (107, 180, 160, 255), "lei")
        c.rect(7, 2, 3, 2, GOLD)
    elif item_id == "zhan_tan_gong_de_xiang":
        c.line(5, 12, 6, 4, GOLD, 1); c.line(8, 12, 8, 3, RED, 1); c.line(11, 12, 10, 4, GOLD, 1)
        c.line(6, 4, 4, 2, WHITE, 1); c.line(10, 4, 12, 2, WHITE, 1)
        c.rect(3, 12, 10, 2, PURPLE)
    elif item_id == "dan_zha":
        c.ellipse(6, 9, 3.2, 3.0, INK); c.ellipse(10, 9, 3.0, 3.5, (74, 65, 62, 255))
        c.px(5, 7, (126, 105, 83, 255)); c.px(10, 7, RED)
    else:
        raise KeyError(f"未覆盖的物品图标: {item_id}")
    return c


def render(item_id: str):
    c = draw_icon(item_id)
    for _ in range(3):
        c.seal_holes(2)
        solid = c.a[..., 3] > 0
        thin = thin_cavities(solid | outline_ring(solid), 2)
        if not thin:
            break
        for comp in thin:
            fill_region(c, comp)
    return finish(c, OUTLINE, light=1.06, dark=0.88, bottom=0.92)


def item_ids() -> list[str]:
    with open(os.path.join(ROOT, "data", "items.json"), encoding="utf-8") as f:
        return sorted(row["id"] for row in json.load(f))


def iter_assets() -> list[dict]:
    return [{"group": "items", "frame": (FRAME, FRAME), "anchor": (8, 15),
             "air_ok": set(item_ids()),
             "states": {item_id: [render(item_id)] for item_id in item_ids()}}]


def main():
    ids = item_ids()
    images = [render(item_id) for item_id in ids]
    os.makedirs(OUT_DIR, exist_ok=True)
    entries = {}
    for i, (item_id, img) in enumerate(zip(ids, images)):
        save_png(img, os.path.join(OUT_DIR, f"item_{item_id}.png"))
        entries[item_id] = {"col": i % COLS, "row": i // COLS}
    atlas = sheet(images, FRAME, FRAME, COLS)
    save_png(atlas, os.path.join(OUT_DIR, "item_sheet.png"))
    manifest = {"generator": "tools/pixelart/item_sprites.py", "frame_size": FRAME,
                "sheet": "item_sheet.png", "sheet_cols": COLS, "sheet_rows": (len(ids) + COLS - 1) // COLS,
                "items": entries}
    with open(os.path.join(OUT_DIR, "manifest.json"), "w", encoding="utf-8") as f:
        json.dump(manifest, f, ensure_ascii=False, indent=2, sort_keys=True)
        f.write("\n")
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    save_png(preview(atlas, scale=8, fw=FRAME, fh=FRAME), os.path.join(PREVIEW_DIR, "item_preview.png"))
    print(f"[pixelart] {len(ids)} 物品图标 → {os.path.relpath(OUT_DIR, ROOT)}")


if __name__ == "__main__":
    main()
