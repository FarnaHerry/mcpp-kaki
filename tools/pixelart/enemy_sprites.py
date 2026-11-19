"""敌人动作素材生成器（design/art-assets.md；承接 player_sprites.py）。

产出（确定性，重跑 bit-identical）：
  assets/textures/enemy/enemy_<archetype>_<state>_<NN>.png   单帧源
  assets/textures/enemy/enemy_sheet.png                      运行期图集（每 (原型,状态) 一行）
  assets/textures/enemy/manifest.json                        帧表 + 原型判定规则
  docs/art/enemy_preview.png                                 ×5 目视大图（.gdignore 不入引擎）

设计要点（与玩家素材的三处不同）：
  * **统一 48×48 帧**：4 个原型体型差很大（飞行小兽 ~16px、Boss ~42px），统一帧格让运行期只有
    一张图集、一份 anchor 规则；空白像素不花钱。
  * **中性可染色调色板**：素材画成**灰阶**（形状靠明度读），引擎按 `Enemy.get_def_color()` 染色
    → 68 个敌人定义（山魈/火魈/赤魈…）共用 4 套素材而仍有色相区分；纯彩素材一染就脏。
  * **原型由 flags 判定**：boss > flying > ranged > melee（data/enemies.json 的 flags，运行期读
    `is_boss/is_flying/is_ranged` 属性），故生成器不需要认识任何具体敌人 id。

状态 = Enemy FSM 映射：idle/patrol/chase → idle/walk，attack/hurt/death 同名；flee→walk，
boss_special→attack（见 enemy_visual.gd 的 STATE_MAP）。

用法：
  python3 tools/pixelart/enemy_sprites.py            # 生成素材
  python3 tools/pixelart/enemy_sprites.py --preview  # 只出预览大图
"""

from __future__ import annotations

import argparse
import json
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from pixel_lib import (Canvas, fill_region, finish, mix, outline_ring, preview,  # noqa: E402
                       save_png, sheet, thin_cavities)
import numpy as np  # noqa: E402

FRAME = 48
CX = 24                      # 帧内中轴
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT_DIR = os.path.join(ROOT, "assets", "textures", "enemy")
PREVIEW_DIR = os.path.join(ROOT, "docs", "art")

# ---------------------------------------------------------------------------
# 中性调色板：全部灰阶（明度读形状），色相交给引擎染色
# ---------------------------------------------------------------------------
OUTLINE = (22, 20, 28, 255)
PAL = {
    "hi": (232, 232, 238, 255),    # 高光/骨刺/牙
    "body": (170, 170, 178, 255),  # 主体
    "body_d": (112, 112, 122, 255),  # 主体暗面
    "plate": (84, 82, 94, 255),    # 甲片/硬壳/暗部
    "dark": (52, 50, 62, 255),     # 内腔/阴影
    "eye": (255, 236, 176, 255),   # 眼（染色后仍偏暖亮）
}

# 各原型的「脚底/最下沿」行（描边落到 +1）与其 FSM 状态表
ARCHETYPES = {
    "melee": {"anchor_y": 41, "sil_bottom": 40},
    "ranged": {"anchor_y": 41, "sil_bottom": 40},
    "flying": {"anchor_y": 37, "sil_bottom": 36},
    "boss": {"anchor_y": 43, "sil_bottom": 42},
}

# 状态 → (帧数, fps, 循环)
STATES = {
    "idle": (4, 5, True),
    "walk": (4, 8, True),
    "attack": (5, 12, False),
    "hurt": (2, 10, True),
    "death": (5, 7, False),
}


def P(**kw):
    """一帧姿势；未给的取静止默认。"""
    p = dict(
        bob=0,             # 全身竖直偏移（负=上）
        lean=0,            # 上身前倾像素
        leg_a=(0, 0),      # 前腿脚位（相对基准）
        leg_b=(0, 0),      # 后腿脚位
        leg_c=(0, 0),      # 四足：第三腿
        leg_d=(0, 0),      # 四足：第四腿
        arm=(0, 0),        # 前爪/持杖手
        arm_b=(0, 0),      # 后爪
        jaw=0,             # 张口/下颚张开
        tail=0,            # 尾摆（横向像素）
        wing=0,            # 翼相位（-1 收 / 0 平 / +1 扬）
        staff=0,           # 法杖/法器抬起
        orb=0,             # 法球亮度档
        eye=True,
        death=0,           # 0 正常 → 1 倒地（逐帧推进）
    )
    p.update(kw)
    return p


# ---------------------------------------------------------------------------
# 状态姿势表（四原型共用骨架参数，差异交给画法）
# ---------------------------------------------------------------------------
def _state_poses(kind: str) -> dict:
    """按原型给出 5 状态姿势。步态/攻击幅度按原型微调。"""
    if kind == "flying":
        return _flying_poses()
    if kind == "boss":
        return _beast_poses()
    return _biped_poses(kind)


def _biped_poses(kind: str) -> dict:
    cast = kind == "ranged"
    reach = 6 if cast else 9
    return {
        # 待机：呼吸 + 爪/杖微动
        # 接地态：bob 恒 0（起伏交给爪/杖/下颚），否则脚会离开锚点行
        "idle": [
            P(),
            P(arm=(1, 0), tail=1, jaw=0),
            P(arm=(0, 1), jaw=1, tail=0),
            P(arm=(-1, 0), tail=-1, jaw=0),
        ],
        # 走：4 帧双腿交替 + 反向摆臂
        # 走：相位差 2 帧，每帧恰有一条腿 dy=0 承重（脚底恒在锚点）
        "walk": [
            P(leg_a=(4, 0), leg_b=(-4, 0), arm=(-2, 0), arm_b=(2, 0)),
            P(leg_a=(2, -2), leg_b=(-1, 0), arm=(-1, -1), arm_b=(1, 0)),
            P(leg_a=(-4, 0), leg_b=(4, 0), arm=(2, 0), arm_b=(-2, 0)),
            P(leg_a=(-1, 0), leg_b=(2, -2), arm=(1, 0), arm_b=(-1, -1)),
        ],
        # 攻击：起手（收）→ 探身 → 命中（爪/杖前伸）→ 收势 → 归位
        "attack": [
            P(lean=-2, arm=(-3, -2), arm_b=(-1, 0), jaw=0, orb=0),
            P(lean=1, arm=(2, -1), arm_b=(0, 1), jaw=1, orb=1),
            P(lean=3, arm=(reach, 1), arm_b=(2, 2), jaw=1, orb=2),
            P(lean=2, arm=(reach - 3, 3), arm_b=(1, 2), jaw=0, orb=1),
            P(lean=0, arm=(1, 1), arm_b=(0, 1), jaw=0, orb=0),
        ],
        # 受击：后仰 + 闭眼
        "hurt": [
            P(lean=-3, bob=-1, eye=False, arm=(-2, -2), arm_b=(-1, -1)),
            P(lean=-1, bob=0, eye=False, arm=(-1, -1), arm_b=(0, 0)),
        ],
        # 死亡：跪 → 塌 → 侧倒（末帧钳制）
        "death": [
            P(lean=-1, bob=1, eye=False, death=1, leg_a=(1, 0), leg_b=(-1, 0)),
            P(lean=2, bob=2, eye=False, death=2, leg_a=(3, 0), leg_b=(-3, 1)),
            P(lean=5, bob=2, eye=False, death=3, leg_a=(6, 1), leg_b=(-5, 1), arm=(-2, 3)),
            P(lean=8, bob=2, eye=False, death=4, leg_a=(9, 2), leg_b=(-6, 2), arm=(-3, 5)),
            P(lean=10, bob=2, eye=False, death=5, leg_a=(11, 3), leg_b=(-7, 3), arm=(-4, 6)),
        ],
    }


def _flying_poses() -> dict:
    """飞行：悬停起伏 + 翼相位，攻击用俯冲啄击。"""
    return {
        "idle": [
            P(bob=0, wing=0, tail=0),
            P(bob=-2, wing=1, tail=1),
            P(bob=0, wing=0, tail=2),
            P(bob=1, wing=-1, tail=1),
        ],
        "walk": [  # 飞行怪没有走：巡游=振翅前移
            P(bob=-1, wing=1, tail=2, lean=1),
            P(bob=1, wing=-1, tail=3, lean=1),
            P(bob=-1, wing=1, tail=4, lean=2),
            P(bob=1, wing=-1, tail=3, lean=1),
        ],
        "attack": [
            P(lean=-2, wing=1, bob=-2, jaw=0),
            P(lean=2, wing=-1, bob=2, jaw=1),
            P(lean=5, wing=-2, bob=4, jaw=1, arm=(3, 2)),
            P(lean=3, wing=0, bob=2, jaw=0, arm=(2, 2)),
            P(lean=0, wing=0, bob=0, jaw=0),
        ],
        "hurt": [
            P(bob=2, wing=-2, eye=False, lean=-3, tail=-2),
            P(bob=0, wing=-1, eye=False, lean=-1, tail=-1),
        ],
        "death": [
            P(bob=1, wing=-2, eye=False, death=1, tail=-3),
            P(bob=2, wing=-1, eye=False, death=2, tail=-4, lean=-2),
            P(bob=3, wing=0, eye=False, death=3, tail=-4, lean=-4),
            P(bob=3, wing=0, eye=False, death=4, tail=-4, lean=-6),
            P(bob=4, wing=0, eye=False, death=5, tail=-5, lean=-8),
        ],
    }


def _beast_poses() -> dict:
    """Boss 四足巨兽：沉重步伐、蓄力冲锋、长尾横扫。"""
    return {
        "idle": [
            P(),
            P(tail=2, jaw=1),
            P(tail=0, jaw=0),
            P(tail=-2, jaw=0),
        ],
        # 四足：对角腿同相，每帧两条腿承重（dy=0），bob 恒 0
        "walk": [
            P(leg_a=(4, 0), leg_c=(-4, 0), leg_b=(-3, -2), leg_d=(3, -2), tail=2),
            P(leg_a=(2, 0), leg_c=(-2, 0), leg_b=(-1, -3), leg_d=(1, -3), tail=3),
            P(leg_a=(-4, 0), leg_c=(4, 0), leg_b=(3, -2), leg_d=(-3, -2), tail=2),
            P(leg_a=(-1, -2), leg_c=(1, -2), leg_b=(2, 0), leg_d=(-2, 0), tail=3),
        ],
        "attack": [
            P(lean=-2, jaw=0, tail=-2, leg_a=(-1, 0), leg_b=(1, 0)),
            P(lean=1, jaw=1, tail=1, leg_a=(0, 0), leg_b=(0, 0)),
            P(lean=3, jaw=1, arm=(5, 2), leg_a=(2, 0), leg_b=(2, 0), tail=3),
            P(lean=3, jaw=1, arm=(3, 3), leg_a=(2, 0), leg_b=(2, 0), tail=4),
            P(lean=1, jaw=0, arm=(0, 1), leg_a=(0, 0), leg_b=(0, 0), tail=2),
        ],
        "hurt": [
            P(lean=-4, bob=-1, eye=False, jaw=1, tail=-3),
            P(lean=-2, bob=0, eye=False, jaw=0, tail=-1),
        ],
        "death": [
            P(lean=0, bob=1, eye=False, death=1, jaw=1, leg_a=(1, 0), leg_c=(-1, 0)),
            P(lean=1, bob=1, eye=False, death=2, jaw=1, leg_a=(4, 0), leg_c=(-4, 0), tail=-3),
            P(lean=2, bob=2, eye=False, death=3, jaw=1, leg_a=(6, 1), leg_c=(-6, 1), tail=-4),
            P(lean=3, bob=2, eye=False, death=4, jaw=1, leg_a=(8, 1), leg_c=(-8, 1), tail=-4),
            P(lean=3, bob=2, eye=False, death=5, jaw=1, leg_a=(9, 1), leg_c=(-9, 1), tail=-4),
        ],
    }


# ---------------------------------------------------------------------------
# 画法：共用骨架（两足） + 三个专用形态
# ---------------------------------------------------------------------------
def _limb(c: Canvas, p0, p1, color, t=3):
    c.line(p0[0], p0[1], p1[0], p1[1], color, t)


def _claw(c: Canvas, x: int, y: int, spread: int, open_: bool):
    """爪/手：3 根指向 +x 的尖爪。"""
    for i in range(3):
        dy = (i - 1) * 2
        c.line(x, y + dy, x + spread + (1 if i == 1 else 0), y + dy + (2 if open_ else 0), PAL["hi"], 1)
    c.rect(x - 2, y - 2, 4, 5, PAL["body_d"])


def draw_biped(c: Canvas, p, kind: str):
    """两足原型：melee=佝偻猛兽（背刺+利爪），ranged=披风术士（兜帽+法杖）。"""
    bob, lean = p["bob"], p["lean"]
    sil = ARCHETYPES[kind]["sil_bottom"]
    hip = (CX - 1, sil - 9 + bob)
    # 腿（后腿先画，压在躯干下）
    foot_b = (CX - 5 + p["leg_b"][0], sil + p["leg_b"][1] + bob)
    foot_a = (CX + 4 + p["leg_a"][0], sil + p["leg_a"][1] + bob)
    _limb(c, (hip[0] - 2, hip[1]), (foot_b[0], foot_b[1] - 1), PAL["body_d"], 4)
    c.rect(foot_b[0] - 3, foot_b[1] - 1, 6, 2, PAL["dark"])
    _limb(c, (hip[0] + 2, hip[1]), (foot_a[0], foot_a[1] - 1), PAL["body"], 4)
    c.rect(foot_a[0] - 3, foot_a[1] - 1, 6, 2, PAL["plate"])

    if kind == "ranged":
        # 披风/袍摆（下宽上窄）
        c.tapered(CX - 1 + lean, hip[1] - 3, sil - 1, 11, 17, PAL["plate"])
        c.tapered(CX - 1 + lean, hip[1] - 3, sil - 2, 9, 14, PAL["body_d"])
    else:
        # 躯干：宽厚、前倾
        c.tapered(CX + lean, hip[1] - 12, hip[1] + 1, 12, 15, PAL["body"])
        c.tapered(CX + lean - 3, hip[1] - 11, hip[1], 4, 5, PAL["body_d"])
        # 背刺
        for i in range(3):
            sx = CX - 6 + lean + i * 3
            sy = hip[1] - 9 + i
            c.line(sx, sy, sx - 2, sy - 4, PAL["plate"], 2)
            c.px(sx - 3, sy - 5, PAL["hi"])

    # 后臂
    sh_b = (CX - 4 + lean, hip[1] - 10)
    h_b = (sh_b[0] - 2 + p["arm_b"][0], sh_b[1] + 6 + p["arm_b"][1])
    _limb(c, sh_b, h_b, PAL["body_d"], 3)
    if kind == "melee":
        _claw(c, h_b[0], h_b[1], 4, False)

    # 头（前伸，带下颚/兜帽）
    hx, hy = CX + 3 + lean, hip[1] - 15
    c.rect(hx - 5, hy - 7, 10, 8, PAL["body"])
    c.rect(hx - 5, hy - 7, 10, 3, PAL["body_d"])
    if kind == "ranged":
        c.rect(hx - 6, hy - 7, 12, 5, PAL["plate"])   # 兜帽
        c.rect(hx - 2, hy - 2, 8, 3, PAL["dark"])     # 兜帽阴影
    else:
        c.rect(hx + 1, hy - 1, 6, 3 + p["jaw"], PAL["body_d"])   # 吻/颚
        c.rect(hx + 4, hy, 5, 2, PAL["hi"])                        # 牙
        for i in range(3):                                          # 角/耳（2px 才不会被描边吃掉）
            c.line(hx - 4 + i * 3, hy - 6, hx - 7 + i * 3, hy - 11, PAL["hi"], 2)
    if p["eye"]:
        c.rect(hx + 2, hy - 4, 2, 2, PAL["eye"])

    # 前臂 + 爪/持杖
    sh_a = (CX + 2 + lean, hip[1] - 10)
    if kind == "ranged" and p["orb"] == 0:
        h_a = (CX + 8, hip[1] - 4)          # 常态：手扶杖身
    else:
        h_a = (sh_a[0] + 3 + min(p["arm"][0], 6), sh_a[1] + 6 + p["arm"][1])
    _limb(c, sh_a, h_a, PAL["body"], 3)
    if kind == "ranged":
        sx = CX + 10 + (1 if p["orb"] else 0)                     # 杖在身前固定位置（不随手飘）
        top = 13 - p["staff"]
        c.line(sx, sil - 2, sx, top, PAL["plate"], 2)             # 杖身
        r = 1.5 + 0.7 * p["orb"]                                  # 法球：小才是球，大就糊脸
        c.ellipse(sx, top - 2, r, r, PAL["eye"] if p["orb"] else PAL["body_d"])
        c.px(sx, top - 2, PAL["hi"])
    else:
        reach = min(p["arm"][0], 6)          # 夹住前伸：48 宽帧右侧只剩 ~6px
        _claw(c, sh_a[0] + 3 + reach, h_a[1], 5 + (1 if p["jaw"] else 0), True)

    # 尾
    if p["tail"]:
        c.line(hip[0] - 6, hip[1] - 2, hip[0] - 10 + p["tail"], hip[1] + 3, PAL["body_d"], 2)


def draw_flying(c: Canvas, p):
    """飞行原形：小兽 + 双翼（翼相位 -2..+1）+ 长尾。"""
    bob, lean = p["bob"], p["lean"]
    sil = ARCHETYPES["flying"]["sil_bottom"]
    by = sil - 6 + bob                      # 躯干中心行
    wing = p["wing"]
    # 翼（先画，压在躯干下）
    for side, dirx in (("up", 1), ("down", 1))[:1]:
        pass
    # 双翼：整块三角 + 2px 前缘（相位改张角/高度）。三根细线在 2px 描边下只会糊成一坨
    for sgn in (-1, 1):
        base = (CX + sgn * 3, by - 1)
        tip = (CX + sgn * (13 + wing), by - 5 - wing * 3)
        mid = (CX + sgn * 7, by + 3 + wing)
        c.blob([base, tip, mid], PAL["body_d"] if sgn < 0 else PAL["body"])
        c.line(base[0], base[1], tip[0], tip[1], PAL["plate"], 2)
    # 躯干
    c.ellipse(CX, by, 6.5, 5.5, PAL["body"])
    c.ellipse(CX - 1, by + 1, 4.5, 3.5, PAL["body_d"])
    # 头 + 喙
    hx = CX + 5 + lean
    c.ellipse(hx, by - 4, 4.5, 4, PAL["body"])
    c.line(hx + 3, by - 4, hx + 8, by - 3 + p["jaw"], PAL["hi"], 2)   # 喙
    if p["eye"]:
        c.rect(hx + 1, by - 6, 2, 2, PAL["eye"])
    # 爪（小型，收在腹下）
    c.line(CX, by + 4, CX + 1 + p["arm"][0], by + 7, PAL["plate"], 2)
    c.line(CX - 3, by + 4, CX - 4 + p["arm_b"][0], by + 7, PAL["plate"], 2)
    # 尾（长，向后）
    tail = max(-5, min(4, p["tail"]))       # 夹住尾摆：越界会被帧边裁平
    tx = CX - 6
    c.line(tx, by, tx - 5 + tail, by + 2 + tail // 2, PAL["body_d"], 2)
    c.line(tx - 5 + tail, by + 2 + tail // 2, tx - 9 + tail, by + 1 + tail, PAL["plate"], 1)


def draw_beast(c: Canvas, p):
    """Boss 四足巨兽：厚重躯干 + 甲背 + 角首 + 长尾。"""
    bob, lean = p["bob"], p["lean"]
    sil = ARCHETYPES["boss"]["sil_bottom"]
    hip = (CX, sil - 12 + bob)
    # 四腿（远侧先画）
    for (fx, fy, col, t) in (
        (CX - 9 + p["leg_c"][0], sil + p["leg_c"][1] + bob, PAL["body_d"], 5),
        (CX + 7 + p["leg_d"][0], sil + p["leg_d"][1] + bob, PAL["body_d"], 5),
    ):
        _limb(c, (hip[0] - 3, hip[1]), (fx, fy - 2), col, t)
        c.rect(fx - 4, fy - 2, 8, 3, PAL["plate"])
    # 躯干（厚，前后等宽）
    c.tapered(CX + lean, hip[1] - 12, hip[1] + 2, 20, 23, PAL["body"])
    c.tapered(CX + lean - 6, hip[1] - 9, hip[1] + 1, 6, 7, PAL["body_d"])
    # 甲背
    for i in range(5):
        sx = CX - 9 + lean + i * 4
        c.line(sx, hip[1] - 12, sx + 2, hip[1] - 17, PAL["plate"], 2)
    tail = max(-4, min(6, p["tail"]))
    if tail:
        c.line(CX - 9, hip[1] - 2, CX - 13 + tail, hip[1] + 2, PAL["body_d"], 3)
        c.line(CX - 13 + tail, hip[1] + 2, CX - 16 + tail, hip[1] - 1, PAL["plate"], 2)
    # 前腿
    for (fx, fy) in (
        (CX - 7 + p["leg_a"][0], sil + p["leg_a"][1] + bob),
        (CX + 9 + p["leg_b"][0], sil + p["leg_b"][1] + bob),
    ):
        _limb(c, (hip[0] + 2, hip[1] + 1), (fx, fy - 2), PAL["body"], 5)
        c.rect(fx - 4, fy - 2, 8, 3, PAL["plate"])
    # 颈 + 头（前伸）
    hx, hy = CX + 9 + lean, hip[1] - 8
    _limb(c, (CX + 3, hip[1] - 10), (hx - 5, hy + 2), PAL["body"], 7)
    c.rect(hx - 5, hy - 4, 13, 9, PAL["body"])
    c.rect(hx + 1, hy + 3, 9, 3 + p["jaw"], PAL["body_d"])       # 颚
    c.rect(hx + 4, hy + 4, 5, 2, PAL["hi"])                       # 牙（夹住右边界）
    for i in range(3):                                             # 双角（短粗，不抢头型）
        c.line(hx - 2 + i * 4, hy - 4, hx - 4 + i * 4, hy - 8 - i, PAL["hi"], 2)
    if p["eye"]:
        c.rect(hx + 2, hy - 2, 3, 2, PAL["eye"])
    # 前爪（攻击时前伸）
    if p["arm"][0]:
        hxp = hx + 4 + min(p["arm"][0], 3)    # 夹住：Boss 头已到 x≈40
        hyp = hy + 6 + p["arm"][1]
        _limb(c, (CX + 3, hip[1] - 6), (hxp, hyp), PAL["body"], 4)
        _claw(c, min(hxp, 36), hyp, 5, True)


DRAW = {"melee": lambda c, p: draw_biped(c, p, "melee"),
        "ranged": lambda c, p: draw_biped(c, p, "ranged"),
        "flying": draw_flying,
        "boss": draw_beast}


def render(pose, archetype: str, flat: bool = False):
    c = Canvas(FRAME, FRAME)
    DRAW[archetype](c, pose)
    # 迭代封缝（与玩家生成器同一套）：描边会把 2px 腔挤成 1px 细缝 → 连描边形态一起查
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
    return finish(c, OUTLINE, light=1.08, dark=0.82, bottom=0.90)


# ---------------------------------------------------------------------------
# 契约：供 validate.py / ascii_view.py 消费（生成器无关）
# ---------------------------------------------------------------------------
def iter_assets() -> list[dict]:
    """每组 = 一个原型；states[state] = [PIL.Image, ...]。"""
    groups = []
    for kind, meta in ARCHETYPES.items():
        states = {}
        for state, (count, _fps, _loop) in STATES.items():
            poses = _state_poses(kind)[state]
            assert len(poses) == count, f"{kind}/{state}: {len(poses)} != {count}"
            states[state] = [render(p, kind) for p in poses]
        groups.append({
            "group": kind,
            "frame": (FRAME, FRAME),
            "anchor": (CX, meta["anchor_y"]),
            # 接地锚点只对「站定」状态强制：飞行原型恒悬空；死亡/受击有位移
            "air_ok": set(STATES) if kind == "flying" else {"hurt", "death"},
            "states": states,
        })
    return groups


def atlas(groups: list[dict], cols: int):
    rows = []
    for g in groups:
        for state in STATES:
            rows.append(sheet(g["states"][state], FRAME, FRAME, cols))
    return sheet(rows, FRAME * cols, FRAME, 1)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview", action="store_true")
    args = ap.parse_args()

    groups = iter_assets()
    cols = max(c for c, _fps, _loop in STATES.values())   # 列数=最大帧数（不是元组长度）
    rows_total = len(ARCHETYPES) * len(STATES)

    if not args.preview:
        os.makedirs(OUT_DIR, exist_ok=True)
        meta = {}
        for g in groups:
            import numpy as _np
            solid = _np.array(g["states"]["idle"][0])[..., 3] > 0
            ys, _xs = _np.nonzero(solid)
            sil_h = int(ys.max()) - int(ys.min()) + 1
            entry = {"anchor": {"x": g["anchor"][0], "y": g["anchor"][1]},
                     "sil_height": sil_h, "states": {}}
            for si, state in enumerate(STATES):
                frames = g["states"][state]
                for i, img in enumerate(frames):
                    save_png(img, os.path.join(OUT_DIR, f"enemy_{g['group']}_{state}_{i:02d}.png"))
                count, fps, loop = STATES[state]
                entry["states"][state] = {
                    "row": list(ARCHETYPES).index(g["group"]) * len(STATES) + si,
                    "count": count, "fps": fps, "loop": loop,
                }
            meta[g["group"]] = entry
        save_png(atlas(groups, cols), os.path.join(OUT_DIR, "enemy_sheet.png"))
        manifest = {
            "generator": "tools/pixelart/enemy_sprites.py",
            "frame_size": FRAME,
            "sheet": "enemy_sheet.png",
            "sheet_cols": cols,
            "sheet_rows": rows_total,
            "sheet_order": [f"{g}/{s}" for g in ARCHETYPES for s in STATES],
            "archetypes": meta,
        }
        with open(os.path.join(OUT_DIR, "manifest.json"), "w", encoding="utf-8") as f:
            json.dump(manifest, f, ensure_ascii=False, indent=2, sort_keys=True)
            f.write("\n")
        total = sum(len(v) for g in groups for v in g["states"].values())
        print(f"[pixelart] {total} 帧 / {len(groups)} 原型 × {len(STATES)} 状态 → {os.path.relpath(OUT_DIR, ROOT)}")

    pv = preview(atlas(groups, cols), scale=5, fw=FRAME, fh=FRAME)
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    save_png(pv, os.path.join(PREVIEW_DIR, "enemy_preview.png"))
    gdignore = os.path.join(PREVIEW_DIR, ".gdignore")
    if not os.path.exists(gdignore):
        open(gdignore, "w", encoding="utf-8").close()
    print(f"[pixelart] 预览 → {os.path.relpath(os.path.join(PREVIEW_DIR, 'enemy_preview.png'), ROOT)}")
    print(f"[pixelart] 原型序：{' '.join(ARCHETYPES)}")


if __name__ == "__main__":
    main()
