"""玩家动作素材生成器（design/art-assets.md §5 落地清单第 1-2 条）。

产出（全部确定性，重跑 bit-identical）：
  assets/textures/player/player_<state>_<NN>.png   单帧源（art-assets.md：先单帧跑通）
  assets/textures/player/player_sheet.png          运行期图集（行=状态，列=帧）
  assets/textures/player/manifest.json             帧表（fps/loop/row/count/anchor）
  docs/art/player_preview.png                      ×6 目视校验大图（.gdignore 不入引擎）

规格：32×32 格子 / 锚点 (16, 29) = 描边底（脚底）/ 面朝右（引擎 flip_h 出朝左）
9 状态 = Player FSM 全部态（idle run jump fall wall_cling dash attack fly meditate）

主角造型：白衣仙子剑仙——墨蓝长发/半挽发髻、玉簪、雪白交领广袖仙衣、
层叠长裙与淡青飘带，背剑/持剑/御剑。只换视觉，不改 FSM、碰撞体或图集契约。

身材分带（面朝右，单位=帧内像素行）：
  2          描边顶
  3-4        半挽发髻/玉簪  5-11 头（大眼/刘海/长鬓发）
  12         颈            13-18 交领短衣/腰封
  19-24      层叠仙裙       25-27 裙摆下露出的鞋履
  27-28      鞋 -> 29 描边底

用法：
  python3 tools/pixelart/player_sprites.py            # 生成素材
  python3 tools/pixelart/player_sprites.py --preview  # 只出预览大图
"""

from __future__ import annotations

import argparse
import json
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from pixel_lib import (Canvas, fill_region, finish, outline_ring, preview,  # noqa: E402
                       save_png, sheet, thin_cavities)
import numpy as np  # noqa: E402

FRAME = 32
FOOT_Y = 28           # 靴底（描边落到 29）
ANCHOR = (16, 29)     # 帧内锚点：描边底 = 站地面上
CX = 16               # 身体中轴
HEAD_TOP, HEAD_BOT = 5, 11
NECK_Y = 12
SHOULDER_Y = 13
TORSO_BOT = 18        # 躯干最后一行的下一行（range 用）
SASH_Y = 17
SKIRT_TOP, SKIRT_BOT = 19, 24
HIP_Y = 23

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT_DIR = os.path.join(ROOT, "assets", "textures", "player")
PREVIEW_DIR = os.path.join(ROOT, "docs", "art")

# ---------------------------------------------------------------------------
# 调色板（白衣仙子：雪白/月白仙衣 + 墨蓝长发 + 淡青玉饰）
# 白色靠冷灰蓝暗面分层，避免一整块白糊掉；眼睛与玉饰以青色呼应。
# ---------------------------------------------------------------------------
OUTLINE = (25, 28, 43, 255)
PAL = {
    "skin": (255, 225, 205, 255),
    "skin_d": (218, 169, 160, 255),
    "blush": (237, 154, 165, 255),
    "hair": (35, 42, 64, 255),
    "hair_hl": (75, 94, 126, 255),
    "hair_shine": (135, 158, 184, 255),
    "robe": (250, 252, 255, 255),       # 雪白上衣/广袖
    "robe_d": (205, 220, 234, 255),     # 月白暗面/背袖
    "robe_fold": (163, 187, 207, 255),
    "accent": (144, 211, 218, 255),     # 淡青云纹/衣缘/玉饰
    "accent_d": (78, 139, 157, 255),
    "skirt": (234, 243, 250, 255),      # 层叠长裙
    "skirt_hl": (255, 255, 255, 255),
    "sash": (133, 190, 202, 255),       # 腰封/发带/飘带
    "sash_d": (72, 125, 146, 255),
    "sock": (205, 220, 234, 255),       # 裙摆下裤袜（白色系）
    "sock_hl": (250, 252, 255, 255),
    "boot": (99, 124, 145, 255),
    "boot_d": (60, 79, 103, 255),
    "eye": (61, 130, 153, 255),
    "eye_hl": (255, 255, 255, 255),
    "blade": (206, 226, 238, 255),
    "blade_hl": (252, 255, 255, 255),
    "hilt": (80, 109, 132, 255),
    "guard": (214, 190, 126, 255),      # 淡金剑镡/玉簪
}

SHOULDER_F = (CX + 2, SHOULDER_Y)
SHOULDER_B = (CX - 3, SHOULDER_Y)
FOOT_F0 = (CX + 3, FOOT_Y)
FOOT_B0 = (CX - 3, FOOT_Y)


# ---------------------------------------------------------------------------
# 姿势数据
# ---------------------------------------------------------------------------
def P(**kw):
    """一帧姿势参数；未给的用站立默认。"""
    p = dict(
        bob=0,            # 全身竖直偏移（负=上）
        lean=0,           # 上身/头前倾像素
        skirt=(0, 0),     # 裙摆 (横向剪切, 额外下沉)
        flare=0,          # 裙摆张开（底宽增量）
        hand_f=(2, 6),    # 前手（相对前肩）
        hand_b=(-3, 6),   # 后手（相对后肩）
        foot_f=(0, 0),    # 前脚（相对 FOOT_F0）
        foot_b=(0, 0),    # 后脚（相对 FOOT_B0）
        head_dy=0,
        sword=None,       # None=背鞘 | ("hand", 角度, 剑长)
        ribbon=0,         # 腰带飘带向后长度
        eye=True,
        mode="stand",     # stand | sit
        board=False,      # 御剑：脚下飞剑
    )
    p.update(kw)
    return p


def _legs_cycle():
    """6 帧跑动：单腿相位轨迹 L，前脚 = L[i]、后脚 = L[i+3]。

    约束：每帧恰有一条腿 dy=0（接地），故脚底恒落在锚点行——跑动是「腿在动」，
    不是整个人上下弹（bob 恒 0，起伏交给腿的收放表达）。
    """
    l = [(5, 0), (2, 0), (-2, 0), (-4, -1), (-2, -3), (1, -4)]
    return l, [(l[(i + 3) % 6]) for i in range(6)]


_RUN_L, _ = _legs_cycle()
_RUN_F = _RUN_L
_RUN_B = [_RUN_L[(i + 3) % 6] for i in range(6)]


def poses() -> dict:
    """状态 → 帧姿势表（顺序 = 图集行序，勿随意改动：manifest 行号依赖它）。"""
    st = {}

    # 待机：呼吸 1px 起伏 + 裙摆微摆 + 发带/衣带轻飘
    st["idle"] = [
        P(bob=0, ribbon=0),
        P(bob=0, skirt=(1, 0), hand_f=(3, 6), ribbon=1),
        P(bob=0, head_dy=-1, skirt=(0, 1), hand_f=(2, 7), hand_b=(-3, 7), ribbon=1, eye=False),
        P(bob=0, skirt=(-1, 0), hand_f=(2, 6), ribbon=0, eye=False),
    ]

    # 跑动：6 帧腿循环（触地/下压/腾越 ×2）+ 手臂反摆 + 前倾
    st["run"] = [
        P(bob=0, lean=1, foot_f=_RUN_F[i], foot_b=_RUN_B[i],
          hand_f=(2 - _RUN_F[i][0] // 2, 5), hand_b=(-3 - _RUN_B[i][0] // 2, 5),
          skirt=(1, 0), ribbon=2 + (i % 3), flare=1)
        for i in range(6)
    ]

    # 起跳：屈膝上收、双臂上扬、裙摆下压
    st["jump"] = [
        P(bob=-1, lean=1, foot_f=(1, -3), foot_b=(-2, -4), hand_f=(3, -3), hand_b=(-3, -2),
          skirt=(0, 1), ribbon=3, flare=1),
        P(bob=-1, lean=1, foot_f=(2, -2), foot_b=(-3, -3), hand_f=(2, -4), hand_b=(-4, -3),
          skirt=(0, 2), ribbon=4, flare=2),
    ]

    # 下落：展腿、扬臂、裙摆上翻
    st["fall"] = [
        P(bob=0, foot_f=(3, 1), foot_b=(-4, -1), hand_f=(4, -1), hand_b=(-4, -1),
          skirt=(-1, -1), ribbon=4, flare=2),
        P(bob=0, foot_f=(3, 0), foot_b=(-5, -2), hand_f=(5, -2), hand_b=(-5, -2),
          skirt=(-1, -2), ribbon=5, flare=3),
    ]

    # 贴墙：背倚左壁（手抓上方、脚蹬下方，身体外弓）
    st["wall_cling"] = [
        P(bob=0, lean=1, hand_b=(-4, -4), hand_f=(3, 2), foot_f=(-7, -6), foot_b=(-6, -1),
          skirt=(-1, 0), ribbon=2),
        P(bob=0, lean=1, hand_b=(-5, -5), hand_f=(4, 2), foot_f=(-7, -5), foot_b=(-5, -2),
          skirt=(-1, 1), ribbon=3),
    ]

    # 冲刺：极度前倾、后腿拖曳、衣摆后掠、剑前指（2 帧交替增强）
    st["dash"] = [
        P(bob=0, lean=2, foot_f=(6, 0), foot_b=(-8, -2), hand_f=(4, 1), hand_b=(-4, 3),
          skirt=(-2, 0), ribbon=6, flare=2, sword=("hand", -15, 11)),
        P(bob=0, lean=2, foot_f=(7, 0), foot_b=(-7, -3), hand_f=(4, 1), hand_b=(-4, 4),
          skirt=(-3, 1), ribbon=7, flare=3, sword=("hand", -22, 11)),
    ]

    # 普攻 5 帧：起手（剑后引）→ 横扫 → 斜劈 → 下压 → 收势
    st["attack"] = [
        P(bob=0, lean=-1, hand_f=(1, -2), hand_b=(-3, 0), foot_f=(-1, 0), foot_b=(-4, 0),
          sword=("hand", -140, 11), ribbon=1),
        P(bob=0, lean=1, hand_f=(3, -1), hand_b=(-3, 3), foot_f=(3, 0), foot_b=(-5, 0),
          sword=("hand", -55, 11), skirt=(1, 0), ribbon=3, flare=1),
        P(bob=0, lean=2, hand_f=(5, 2), hand_b=(-3, 4), foot_f=(6, 0), foot_b=(-6, 0),
          sword=("hand", 55, 11), skirt=(-1, 1), ribbon=4, flare=2, eye=False),
        P(bob=0, lean=2, hand_f=(4, 4), hand_b=(-2, 5), foot_f=(6, 0), foot_b=(-6, 0),
          sword=("hand", 80, 11), skirt=(-2, 2), ribbon=4, flare=2, eye=False),
        P(bob=0, lean=1, hand_f=(3, 4), hand_b=(-2, 5), foot_f=(3, 0), foot_b=(-4, 0),
          sword=("hand", 70, 11), skirt=(-1, 0), ribbon=2, eye=False),
    ]

    # 飞行（御剑）：4 帧悬停浮动，衣带长曳，脚下飞剑
    st["fly"] = [
        P(bob=-1, lean=1, foot_f=(-1, -2), foot_b=(1, -2), hand_f=(5, 0), hand_b=(-4, 0),
          skirt=(-2, -1), ribbon=6, flare=2, board=True),
        P(bob=-1, lean=1, foot_f=(-1, -1), foot_b=(1, -1), hand_f=(6, -1), hand_b=(-5, -1),
          skirt=(-2, -2), ribbon=7, flare=3, board=True),
        P(bob=-1, lean=1, foot_f=(0, -2), foot_b=(2, -2), hand_f=(5, 1), hand_b=(-4, 1),
          skirt=(-2, -1), ribbon=7, flare=2, board=True, eye=False),
        P(bob=-1, lean=1, foot_f=(-1, -1), foot_b=(1, -1), hand_f=(5, 0), hand_b=(-4, 0),
          skirt=(-1, -1), ribbon=6, flare=2, board=True, eye=False),
    ]

    # 打坐：盘膝坐姿（mode=sit 走专用画法）
    st["meditate"] = [
        P(mode="sit", bob=0, ribbon=1),
        P(mode="sit", bob=0, head_dy=-1, lean=1, hand_f=(3, 3), ribbon=2, eye=False),
    ]

    return st


# ---------------------------------------------------------------------------
# 部件绘制
# ---------------------------------------------------------------------------
def draw_head(c: Canvas, x0: int, top: int, ribbon: int, eye: bool):
    """仙子头（面朝右）：垂腰长发 + 半挽小髻 + 玉簪 + 青色大眼。"""
    sway = min(2, ribbon // 2)
    # 长直发先画在头后，垂到腰下；动作幅度只改变发尾，不改变头部读形。
    c.blob([(x0, top - 1), (x0 + 5, top), (x0 + 4, top + 8),
            (x0 + 2 - sway, top + 15), (x0 - 3 - sway, top + 14),
            (x0 - 2, top + 6)], PAL["hair"])
    c.line(x0 + 1, top + 3, x0 - 1 - sway, top + 12, PAL["hair_hl"], 1)
    c.px(x0 + 1, top + 4, PAL["hair_shine"])

    # 半挽小髻 + 横贯玉簪；不要高马尾或大蝴蝶结，保持清冷仙侠轮廓。
    c.rect(x0 + 1, top - 2, 3, 2, PAL["hair"])
    c.px(x0 + 2, top - 2, PAL["hair_hl"])
    c.line(x0 - 1, top - 1, x0 + 5, top - 2, PAL["guard"], 1)
    c.px(x0 + 5, top - 2, PAL["accent"])
    for i in range(2 + ribbon):
        tx, ty = x0 - 2 - i, top + 2 + i // 3
        c.px(tx, ty, PAL["accent"] if i % 2 == 0 else PAL["sash_d"])
        c.px(tx, ty + 1, PAL["sash_d"])

    # 后脑发块与偏圆的脸颊/尖下巴。
    c.rect(x0, top, 8, 8, PAL["hair"])
    c.blob([(x0 + 3, top + 2), (x0 + 8, top + 2), (x0 + 8, top + 5),
            (x0 + 6, top + 7), (x0 + 3, top + 6)], PAL["skin"])
    c.px(x0 + 8, top + 4, PAL["skin"])       # 鼻尖
    c.px(x0 + 7, top + 6, PAL["blush"])
    c.px(x0 + 8, top + 5, PAL["skin_d"])     # 嘴

    # 锯齿刘海 + 垂到肩下的鬓发。
    c.hline(x0 + 1, x0 + 8, top + 1, PAL["hair"])
    c.hline(x0 + 2, x0 + 7, top + 2, PAL["hair"])
    c.px(x0 + 3, top + 3, PAL["hair"])
    c.px(x0 + 5, top + 3, PAL["hair"])
    c.line(x0 + 2, top + 2, x0 + 1, top + 10, PAL["hair"], 2)
    c.px(x0 + 2, top + 8, PAL["hair_hl"])
    c.hline(x0 + 2, x0 + 5, top, PAL["hair_hl"])
    c.px(x0 + 3, top, PAL["hair_shine"])

    if eye:
        # 2×3 动漫眼：深色眼线、青瞳、1px 高光。
        c.rect(x0 + 5, top + 3, 2, 3, PAL["hair"])
        c.px(x0 + 6, top + 4, PAL["eye"])
        c.px(x0 + 5, top + 3, PAL["eye_hl"])
        c.px(x0 + 6, top + 5, PAL["hair_hl"])
    else:
        c.hline(x0 + 5, x0 + 6, top + 4, PAL["hair_hl"])


def draw_arm(c: Canvas, sh, hand, sleeve, cuff, t=3):
    """广袖：上袖宽、袖口收窄，淡青滚边外只露小手。"""
    sx, sy = sh
    hx, hy = hand
    mx = sx + (hx - sx) * 0.62
    my = sy + (hy - sy) * 0.62
    c.line(sx, sy, mx, my, sleeve, 4)
    c.line(mx, my, hx, hy, sleeve, 3)
    cx = hx - (hx - mx) * 0.22
    cy = hy - (hy - my) * 0.22
    c.px(cx, cy, cuff)
    c.rect(hx - 1, hy - 1, 2, 2, PAL["skin"])
    c.px(hx, hy + 1, PAL["skin_d"])


def draw_leg(c: Canvas, hip, foot):
    """白袜裤 + 青灰布鞋；大部分被长裙遮住，跑跳时露出步态。"""
    hx, hy = hip
    fx, fy = foot
    c.line(hx, hy, fx, fy - 1, PAL["sock"], 3)
    c.line(hx, hy, fx, fy - 1, PAL["sock_hl"], 1)
    c.rect(fx - 2, fy - 1, 4, 2, PAL["boot"])
    c.rect(fx - 2, fy, 4, 1, PAL["boot_d"])
    c.px(fx + 1, fy - 1, PAL["accent"])


def draw_back_sword(c: Canvas, bob: int, lean: int):
    """非出剑状态背负长剑：白衣剪影旁保留清晰剑柄/剑镡。"""
    c.line(CX - 5 + lean, 21 + bob, CX + 1 + lean, 8 + bob, PAL["accent_d"], 2)
    c.line(CX - 4 + lean, 20 + bob, CX + 1 + lean, 9 + bob, PAL["robe_d"], 1)
    c.line(CX + 1 + lean, 8 + bob, CX + 3 + lean, 4 + bob, PAL["hilt"], 2)
    c.line(CX - 1 + lean, 7 + bob, CX + 3 + lean, 9 + bob, PAL["guard"], 1)


def draw_hand_sword(c: Canvas, hand, angle_deg: float, length: float):
    """持剑：从手部沿角度出剑（0°=右 / 90°=下 / -90°=上）。"""
    hx, hy = hand
    a = math.radians(angle_deg)
    dx, dy = math.cos(a), math.sin(a)
    px, py = -dy, dx
    c.line(hx - dx * 2, hy - dy * 2, hx, hy, PAL["hilt"], 2)
    c.line(hx - px * 2, hy - py * 2, hx + px * 2, hy + py * 2, PAL["guard"], 1)
    # 剑尖夹进安全区 [1, 30]：宁可短一点也不让帧边裁平（校验器另查贴边）
    lo, hi = 3.0, 28.0
    t = length
    if abs(dx) > 1e-6:
        t = min(t, (hi - hx) / dx if dx > 0 else (lo - hx) / dx)
    if abs(dy) > 1e-6:
        t = min(t, (hi - hy) / dy if dy > 0 else (lo - hy) / dy)
    t = max(2.0, t)
    tx, ty = hx + dx * t, hy + dy * t
    c.line(hx + dx * 2, hy + dy * 2, tx, ty, PAL["blade"], 2)
    c.line(hx + dx * 2 - px, hy + dy * 2 - py, tx - px, ty - py, PAL["blade_hl"], 1)
    c.px(int(round(tx)), int(round(ty)), PAL["blade_hl"])


def draw_board_sword(c: Canvas, y: int, ribbon: int):
    """脚下飞剑（御剑飞行）。"""
    back = 6 + ribbon // 3
    c.line(CX - back, y, CX + 7, y, PAL["blade"], 2)
    c.line(CX - back, y - 1, CX + 7, y - 1, PAL["blade_hl"], 1)
    c.rect(CX + 7, y - 1, 3, 2, PAL["blade"])
    c.rect(CX + 10, y, 1, 1, PAL["blade_hl"])
    c.rect(CX - back - 2, y - 1, 2, 2, PAL["guard"])
    c.line(CX - back - 2, y - 4, CX - back - 2, y - 1, PAL["sash"], 2)


def draw_ribbon(c: Canvas, ribbon: int, base_y: int, wave: int):
    """腰带飘带（向后=左）。"""
    for i in range(ribbon):
        x = CX - 5 - i
        y = base_y + int(round(math.sin((i + wave) * 0.9) * 1.2))
        c.px(x, y, PAL["sash"] if i % 2 == 0 else PAL["sash_d"])
        c.px(x, y + 1, PAL["sash_d"])


def draw_standing(c: Canvas, p):
    bob, lean, ribbon = p["bob"], p["lean"], p["ribbon"]
    kick, drop = p["skirt"]
    sh_f = (SHOULDER_F[0] + lean, SHOULDER_F[1] + bob)
    sh_b = (SHOULDER_B[0] + lean, SHOULDER_B[1] + bob)
    hip = (CX, HIP_Y + bob)
    foot_f = (FOOT_F0[0] + p["foot_f"][0], FOOT_F0[1] + p["foot_f"][1] + bob)
    foot_b = (FOOT_B0[0] + p["foot_b"][0], FOOT_B0[1] + p["foot_b"][1] + bob)
    hand_f = (sh_f[0] + p["hand_f"][0], sh_f[1] + p["hand_f"][1])
    hand_b = (sh_b[0] + p["hand_b"][0], sh_b[1] + p["hand_b"][1])

    # 1) 背剑 + 后裙摆在所有肢体之下，形成白衣剑仙的长轮廓
    if p["sword"] is None and not p["board"]:
        draw_back_sword(c, bob, lean)
    c.blob([(CX - 3 + lean, SASH_Y + bob), (CX + 2 + lean, SKIRT_TOP + bob),
            (CX - 2 + kick, 26 + bob + drop),
            (CX - 7 + kick - p["flare"] // 2, 25 + bob + drop)], PAL["robe_d"])
    c.line(CX - 7 + kick - p["flare"] // 2, 25 + bob + drop,
           CX - 2 + kick, 26 + bob + drop, PAL["accent"], 1)

    # 2) 后臂 / 后腿
    draw_arm(c, sh_b, hand_b, PAL["robe_d"], PAL["accent_d"])
    draw_leg(c, (hip[0] - 2, hip[1]), foot_b)

    # 3) 前腿
    draw_leg(c, (hip[0] + 2, hip[1]), foot_f)

    # 4) 雪白层叠仙裙（冷灰褶线 + 淡青下摆，白色也有层次）
    top, bot = SKIRT_TOP + bob, SKIRT_BOT + bob + drop
    for i, y in enumerate(range(top, bot + 1)):
        t = i / max(1, (bot - top))
        w = 8 + (5 + p["flare"]) * t
        off = int(round(kick * t))
        left = CX - w // 2 + off
        right = CX + w // 2 + off - 1
        c.hline(left, right, y, PAL["skirt"])
        c.px(left + 1, y, PAL["skirt_hl"])
        c.px(CX + off, y, PAL["robe_fold"])
        c.px(right - 1, y, PAL["robe_d"])
    c.hline(CX - 6 + kick, CX + 5 + kick, bot, PAL["accent"])

    # 5) 颈 + 收窄腰线 + 交领白衣 + 淡青腰封/玉佩
    c.rect(CX - 1 + lean, NECK_Y + bob, 5, 1, PAL["skin_d"])
    for i, y in enumerate(range(SHOULDER_Y + bob, TORSO_BOT + bob)):
        t = i / 5.0
        w = 8 - int(round(2.0 * t))
        off = int(round(lean * (1.0 - t)))
        left = CX - w // 2 + off
        c.hline(left, left + w - 1, y, PAL["robe"])
        c.px(left, y, PAL["robe_fold"])
        if i < 3:
            c.px(left + w - 2, y, PAL["robe_d"])
    # 左右交领，不使用水手领/胸结。
    c.line(CX - 3 + lean, SHOULDER_Y + bob, CX + 2 + lean, SASH_Y + bob,
           PAL["accent_d"], 1)
    c.line(CX + 3 + lean, SHOULDER_Y + bob, CX + lean, SHOULDER_Y + 3 + bob,
           PAL["accent"], 1)
    c.px(CX + 2 + lean, SHOULDER_Y + 2 + bob, PAL["robe_fold"])
    c.rect(CX - 3 + lean, SASH_Y + bob, 7, 2, PAL["sash"])
    c.rect(CX - 4 + lean, SASH_Y + bob, 2, 2, PAL["sash_d"])
    c.px(CX + 2 + lean, SASH_Y + bob, PAL["guard"])
    c.px(CX + 2 + lean, SASH_Y + 1 + bob, PAL["accent"])
    # 6) 前臂
    draw_arm(c, sh_f, hand_f, PAL["robe"], PAL["accent"])

    # 7) 头
    draw_head(c, CX - 4 + lean, HEAD_TOP + bob + p["head_dy"], ribbon, p["eye"])

    # 8) 持剑 / 飞剑 / 飘带
    if p["sword"] is not None:
        kind, ang, ln = p["sword"]
        if kind == "hand":
            draw_hand_sword(c, hand_f, ang, ln)
    if p["board"]:
        draw_board_sword(c, FOOT_Y - 1, ribbon)
    if ribbon > 0:
        draw_ribbon(c, ribbon, SASH_Y + 1 + bob, kick + drop)


def draw_sitting(c: Canvas, p):
    """打坐：白裙盘膝 + 广袖结印 + 长剑横于膝。"""
    bob, ribbon, lean = p["bob"], p["ribbon"], p["lean"]
    # 盘膝：两层月白裙料收在蒲团式轮廓内。
    c.ellipse(CX, 25 + bob, 8.0, 2.8, PAL["robe_d"])
    c.ellipse(CX, 24 + bob, 7.0, 2.2, PAL["skirt"])
    c.hline(CX - 7, CX + 6, 25 + bob, PAL["skirt_hl"])
    c.hline(CX - 8, CX + 7, 27 + bob, PAL["boot"])
    # 交领白衣（坐姿）+ 淡青腰封。
    for i, y in enumerate(range(13 + bob, 24 + bob)):
        t = i / 11.0
        w = 8 - int(round(1.2 * t))
        off = int(round(lean * (1.0 - t) * 2))
        c.hline(CX - w // 2 + off, CX + w // 2 + off - 1, y, PAL["robe"])
        c.px(CX - w // 2 + off, y, PAL["robe_fold"])
        c.px(CX + w // 2 + off - 3, y, PAL["robe_d"])
    c.line(CX + 3 + lean, 13 + bob, CX + lean, 16 + bob, PAL["accent"], 1)
    c.line(CX - 3 + lean, 13 + bob, CX + 2 + lean, 20 + bob, PAL["accent_d"], 1)
    c.rect(CX - 3 + lean, 21 + bob, 7, 2, PAL["sash"])
    c.px(CX + 2 + lean, 21 + bob, PAL["guard"])
    # 结印双手（随姿势微调，供打坐呼吸帧用）
    hx, hy = p["hand_f"]
    c.rect(CX - 1 + hx - 2, 22 + bob + hy - 3, 3, 3, PAL["skin"])
    c.rect(CX - 4 + hx - 2, 22 + bob + hy - 3, 3, 3, PAL["skin_d"])
    # 横剑于膝
    c.line(CX - 8, 27 + bob, CX + 6, 27 + bob, PAL["hilt"], 2)
    c.line(CX + 6, 27 + bob, CX + 10, 27 + bob, PAL["blade"], 1)
    c.rect(CX - 10, 26 + bob, 2, 3, PAL["guard"])
    # 头
    draw_head(c, CX - 4 + lean, 6 + bob + p["head_dy"], ribbon, p["eye"])


def render(p, flat: bool = False):
    """flat=True：不描边不调明暗（纯语义色，供 ASCII 审图/调色板检查）。"""
    c = Canvas(FRAME, FRAME)
    if p["mode"] == "sit":
        draw_sitting(c, p)
    else:
        draw_standing(c, p)
    # 迭代封缝：描边本身会把 2px 腔挤成 1px 细缝 → 连描边后的形态一起检查，
    # 直到「成品」不再含细缝（细缝在成品里就是一条黑线/黑点）。
    for _ in range(4):
        c.seal_holes(2)
        solid = c.a[..., 3] > 0
        thin = thin_cavities(solid | outline_ring(solid), 2)
        if not thin:
            break
        for comp in thin:
            fill_region(c, comp)
    if flat:
        return finish(c, OUTLINE, light=1.0, dark=1.0, bottom=1.0, outline_scope="outer")
    return finish(c, OUTLINE, light=1.10, dark=0.80, bottom=0.88)


# ---------------------------------------------------------------------------
# 输出
# ---------------------------------------------------------------------------
FPS = {"idle": 6, "run": 12, "jump": 8, "fall": 8, "wall_cling": 6,
       "dash": 12, "attack": 16, "fly": 8, "meditate": 3}
NON_LOOP = {"attack", "jump"}


def atlas(framed: dict, order: list, cols: int):
    """图集：**每状态一行**（行号 = manifest.states[x].row），不足处留空。

    不做紧凑打包——紧凑排布会让「第 7 个状态」跨行，运行期按 (行,列) 取帧就对不上。
    """
    rows = [sheet(framed[n], FRAME, FRAME, cols) for n in order]
    return sheet(rows, FRAME * cols, FRAME, 1)


# 状态 → 是否豁免「脚底贴锚点」：腾空/贴墙姿态不接地
PLAYER_AIR_OK = {"jump", "fall", "fly", "wall_cling"}


def iter_assets() -> list[dict]:
    """生成器无关契约（validate.py / ascii_view.py 消费）：单组 = 玩家全套状态。"""
    table = poses()
    return [{
        "group": "player",
        "frame": (FRAME, FRAME),
        "anchor": ANCHOR,
        "air_ok": set(PLAYER_AIR_OK),
        "states": {name: [render(p) for p in plist] for name, plist in table.items()},
    }]


def build_all() -> tuple[dict, list, int]:
    table = poses()
    order = list(table.keys())
    cols = max(len(v) for v in table.values())
    framed = {name: [render(p) for p in ps] for name, ps in table.items()}
    return table, framed, cols, order


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview", action="store_true", help="只生成预览大图")
    args = ap.parse_args()

    table, framed, cols, order = build_all()

    if not args.preview:
        os.makedirs(OUT_DIR, exist_ok=True)
        states_meta = {}
        for row, name in enumerate(order):
            for i, img in enumerate(framed[name]):
                save_png(img, os.path.join(OUT_DIR, f"player_{name}_{i:02d}.png"))
            states_meta[name] = {
                "row": row,
                "count": len(framed[name]),
                "fps": FPS.get(name, 8),
                "loop": name not in NON_LOOP,
            }
        save_png(atlas(framed, order, cols), os.path.join(OUT_DIR, "player_sheet.png"))

        manifest = {
            "generator": "tools/pixelart/player_sprites.py",
            "frame_size": FRAME,
            "sheet": "player_sheet.png",
            "sheet_cols": cols,
            "anchor": {"x": ANCHOR[0], "y": ANCHOR[1]},
            "states": states_meta,
        }
        with open(os.path.join(OUT_DIR, "manifest.json"), "w", encoding="utf-8") as f:
            json.dump(manifest, f, ensure_ascii=False, indent=2, sort_keys=True)
            f.write("\n")
        total = sum(len(v) for v in framed.values())
        print(f"[pixelart] {total} 帧 / {len(order)} 状态 → {os.path.relpath(OUT_DIR, ROOT)}")

    pv = preview(atlas(framed, order, cols), scale=6, fw=FRAME, fh=FRAME)
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    save_png(pv, os.path.join(PREVIEW_DIR, "player_preview.png"))
    gdignore = os.path.join(PREVIEW_DIR, ".gdignore")
    if not os.path.exists(gdignore):
        with open(gdignore, "w", encoding="utf-8") as f:
            f.write("")  # Godot 跳过本目录：预览大图不入引擎
    print(f"[pixelart] 预览 → {os.path.relpath(os.path.join(PREVIEW_DIR, 'player_preview.png'), ROOT)}")
    print(f"[pixelart] 状态序：{' '.join(order)}")


if __name__ == "__main__":
    main()
