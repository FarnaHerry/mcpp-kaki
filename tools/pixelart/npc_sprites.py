"""NPC 素材：8 类身份 × 4 帧待机动画，32×32，锚点 (16,29)。

身份只负责视觉，不承担交互逻辑；运行期按 visual_id 选择，缺省时回退 villager。
"""

from __future__ import annotations

import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from pixel_lib import Canvas, fill_region, finish, outline_ring, preview, save_png, sheet, thin_cavities  # noqa: E402

FRAME = 32
ANCHOR = (16, 29)
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT_DIR = os.path.join(ROOT, "assets", "textures", "npc")
PREVIEW_DIR = os.path.join(ROOT, "docs", "art")
OUTLINE = (28, 26, 40, 255)
SKIN = (239, 199, 163, 255)
SKIN_D = (195, 143, 119, 255)
HAIR = (43, 39, 53, 255)
WHITE = (240, 241, 235, 255)
GOLD = (219, 178, 79, 255)
JADE = (105, 184, 174, 255)

ROLES = {
    "villager": {"cloth": (116, 145, 153, 255), "dark": (72, 91, 107, 255)},
    "elder": {"cloth": (132, 119, 151, 255), "dark": (78, 68, 99, 255)},
    "innkeeper": {"cloth": (169, 113, 88, 255), "dark": (104, 70, 65, 255)},
    "merchant": {"cloth": (79, 135, 134, 255), "dark": (48, 83, 91, 255)},
    "immortal": {"cloth": (234, 242, 246, 255), "dark": (155, 181, 198, 255)},
    "monk": {"cloth": (190, 133, 57, 255), "dark": (126, 76, 51, 255)},
    "judge": {"cloth": (105, 53, 65, 255), "dark": (48, 45, 61, 255)},
    "emperor": {"cloth": (231, 194, 85, 255), "dark": (140, 85, 52, 255)},
}


def draw_npc(role: str, phase: int) -> Canvas:
    c = Canvas(FRAME, FRAME)
    spec = ROLES[role]
    cloth, dark = spec["cloth"], spec["dark"]
    hand_dy = (0, -1, 1, 1)[phase]
    head_dy = -1 if phase == 2 else 0
    # 鞋与腿：所有帧都固定在锚点，不让待机呼吸变成悬空。
    c.rect(12, 25, 4, 4, dark); c.rect(17, 25, 4, 4, dark)
    c.rect(11, 28, 5, 1, HAIR); c.rect(17, 28, 5, 1, HAIR)
    long_robe = role in {"elder", "immortal", "monk", "judge", "emperor"}
    # 后袖/法器。
    if role == "elder":
        c.line(10, 15, 8, 27, GOLD, 2)
        c.px(8, 13, JADE)
    if role == "immortal":
        c.line(11, 20, 8, 25, WHITE, 2)
        c.line(8, 25, 5, 23, WHITE, 1)
    # 身体：长袍或短衣。
    c.tapered(16, 14, 25 if long_robe else 22, 10, 13 if long_robe else 11, cloth)
    c.tapered(13, 15, 24 if long_robe else 21, 3, 4, dark)
    c.line(20, 15, 23, 20 + hand_dy, cloth, 3)
    c.rect(22, 20 + hand_dy, 2, 2, SKIN)
    c.line(12, 15, 10, 20, dark, 3)
    c.rect(9, 20, 2, 2, SKIN_D)
    c.hline(12, 20, 19, GOLD if role in {"merchant", "emperor"} else dark)
    if role == "innkeeper":
        c.blob([(13, 17), (19, 17), (20, 24), (12, 24)], WHITE)
        c.hline(13, 19, 19, dark)
    if role == "monk":
        c.line(12, 14, 20, 22, GOLD, 3)
        for i in range(4):
            c.px(13 + i * 2, 15 + i * 2, HAIR)
    if role == "judge":
        c.rect(13, 16, 7, 5, GOLD)
        c.px(16, 18, dark)
    if role == "emperor":
        c.vline(16, 15, 24, GOLD)
        c.px(16, 17, JADE); c.px(16, 21, JADE)
    # 头部与脸。
    y = 6 + head_dy
    if role != "monk":
        c.rect(11, y, 10, 7, HAIR)
    c.blob([(13, y + 2), (20, y + 2), (21, y + 5), (18, y + 7), (13, y + 6)], SKIN)
    c.px(20, y + 5, SKIN_D)
    c.px(18, y + 3, HAIR if phase != 3 else dark)
    if role != "monk":
        c.hline(12, 20, y + 1, HAIR)
        c.vline(12, y + 2, y + 6, HAIR)
    if role == "elder":
        c.hline(12, 20, y, WHITE)
        c.line(17, y + 6, 18, y + 11, WHITE, 2)
    elif role == "merchant":
        c.rect(10, y - 1, 12, 2, dark)
        c.hline(13, 19, y - 2, dark)
        c.rect(20, 20, 4, 4, GOLD)
    elif role == "immortal":
        c.rect(13, y - 2, 4, 2, HAIR)
        c.line(12, y - 1, 20, y - 2, GOLD, 1)
        c.px(20, y - 2, JADE)
        c.line(12, y + 5, 10, 21, HAIR, 2)
    elif role == "judge":
        c.rect(10, y - 2, 12, 3, HAIR)
        c.rect(9, y - 1, 3, 5, HAIR); c.rect(20, y - 1, 3, 5, HAIR)
        c.hline(15, 18, y + 6, HAIR)
    elif role == "emperor":
        c.rect(12, y - 3, 9, 3, GOLD)
        for x in (12, 16, 20):
            c.vline(x, y - 3, y + 1, GOLD)
            c.px(x, y + 2, JADE)
    elif role == "villager":
        c.hline(12, 20, y, dark)
    return c


def render(role: str, phase: int):
    c = draw_npc(role, phase)
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
    return [{"group": role, "frame": (FRAME, FRAME), "anchor": ANCHOR, "air_ok": set(),
             "states": {"idle": [render(role, p) for p in range(4)]}} for role in ROLES]


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    rows = []
    meta = {}
    for row, role in enumerate(ROLES):
        frames = [render(role, p) for p in range(4)]
        for i, img in enumerate(frames):
            save_png(img, os.path.join(OUT_DIR, f"npc_{role}_idle_{i:02d}.png"))
        rows.append(sheet(frames, FRAME, FRAME, 4))
        meta[role] = {"row": row, "count": 4, "fps": 4, "loop": True}
    atlas = sheet(rows, FRAME * 4, FRAME, 1)
    save_png(atlas, os.path.join(OUT_DIR, "npc_sheet.png"))
    manifest = {"generator": "tools/pixelart/npc_sprites.py", "frame_size": FRAME,
                "sheet": "npc_sheet.png", "sheet_cols": 4, "sheet_rows": len(ROLES),
                "anchor": {"x": ANCHOR[0], "y": ANCHOR[1]}, "roles": meta}
    with open(os.path.join(OUT_DIR, "manifest.json"), "w", encoding="utf-8") as f:
        json.dump(manifest, f, ensure_ascii=False, indent=2, sort_keys=True)
        f.write("\n")
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    save_png(preview(atlas, scale=6, fw=FRAME, fh=FRAME), os.path.join(PREVIEW_DIR, "npc_preview.png"))
    print(f"[pixelart] {len(ROLES)} 类 NPC / 32 帧 → {os.path.relpath(OUT_DIR, ROOT)}")


if __name__ == "__main__":
    main()
