"""像素素材绘制库（design/art-assets.md 落地用）。

设计约束（勿破）：
- **确定性**：同参数必然产出 bit-identical 的 PNG（无浮点随机、无时间戳）。
  素材入库后任何人重跑生成器，git diff 必须为空——否则素材不可复现。
- **真像素**：所有绘制落在整数像素格；不做缩放/抗锯齿（模糊交给显示端）。
  输出 PNG 一律 RGBA 无损（art-assets.md §1）。
- **统一收尾**：`finish()` 提供描边 + 明暗三件套（右上暗/上缘亮/下缘沉），
  保证同一套素材轮廓风格一致（AI/手工后续接管线也照此收尾）。

颜色一律用 (r, g, b, a) 0..255 元组；alpha=0 视作透明。
"""

from __future__ import annotations

import math
import os

import numpy as np
from PIL import Image

Color = tuple  # (r, g, b, a)


def rgba(c: Color, a: int | None = None) -> Color:
    """把 (r,g,b) 或 (r,g,b,a) 规范成 4 元组；a 给定则覆盖 alpha。"""
    if len(c) == 3:
        c = (c[0], c[1], c[2], 255)
    if a is not None:
        c = (c[0], c[1], c[2], a)
    return tuple(int(v) for v in c)


def shade(c: Color, f: float) -> Color:
    """按倍率调亮/调暗（保持 alpha）。"""
    c = rgba(c)
    return (
        max(0, min(255, int(round(c[0] * f)))),
        max(0, min(255, int(round(c[1] * f)))),
        max(0, min(255, int(round(c[2] * f)))),
        c[3],
    )


def mix(c0: Color, c1: Color, t: float) -> Color:
    """线性混色 t=0 → c0，t=1 → c1。"""
    a, b = rgba(c0), rgba(c1)
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(4))


# ----------------------------------------------------------------------------
# 画布
# ----------------------------------------------------------------------------
class Canvas:
    """整数像素画布；写入即覆盖（后画压前画），None = 透明。"""

    def __init__(self, w: int, h: int):
        self.w = int(w)
        self.h = int(h)
        self.a = np.zeros((self.h, self.w, 4), dtype=np.uint8)

    # ---- 基础 ----
    def px(self, x: int, y: int, c: Color) -> None:
        x, y = int(round(x)), int(round(y))
        if 0 <= x < self.w and 0 <= y < self.h:
            self.a[y, x] = rgba(c)

    def get(self, x: int, y: int):
        if 0 <= x < self.w and 0 <= y < self.h:
            return tuple(int(v) for v in self.a[y, x])
        return None

    def opaque(self, x: int, y: int) -> bool:
        return 0 <= x < self.w and 0 <= y < self.h and self.a[y, x, 3] > 0

    def rect(self, x: int, y: int, w: int, h: int, c: Color) -> None:
        x0, y0 = max(0, int(x)), max(0, int(y))
        x1, y1 = min(self.w, int(x) + int(w)), min(self.h, int(y) + int(h))
        if x1 > x0 and y1 > y0:
            self.a[y0:y1, x0:x1] = rgba(c)

    def hline(self, x0: int, x1: int, y: int, c: Color) -> None:
        if x1 < x0:
            x0, x1 = x1, x0
        self.rect(x0, y, x1 - x0 + 1, 1, c)

    def vline(self, x: int, y0: int, y1: int, c: Color) -> None:
        if y1 < y0:
            y0, y1 = y1, y0
        self.rect(x, y0, 1, y1 - y0 + 1, c)

    def line(self, x0: float, y0: float, x1: float, y1: float, c: Color, t: int = 1) -> None:
        """Bresenham 直线，t = 方形笔刷边长（1..n）。"""
        x0, y0, x1, y1 = int(round(x0)), int(round(y0)), int(round(x1)), int(round(y1))
        dx, dy = abs(x1 - x0), abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx - dy
        half = t // 2
        while True:
            if t <= 1:
                self.px(x0, y0, c)
            else:
                self.rect(x0 - half, y0 - half, t, t, c)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                x0 += sx
            if e2 < dx:
                err += dx
                y0 += sy

    def ellipse(self, cx: float, cy: float, rx: float, ry: float, c: Color) -> None:
        """实心椭圆（像素中心判定）。"""
        if rx <= 0 or ry <= 0:
            return
        x0, x1 = int(math.floor(cx - rx)), int(math.ceil(cx + rx))
        y0, y1 = int(math.floor(cy - ry)), int(math.ceil(cy + ry))
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                nx = (x + 0.5 - cx) / rx
                ny = (y + 0.5 - cy) / ry
                if nx * nx + ny * ny <= 1.0:
                    self.px(x, y, c)

    # ---- 形状糖（角色部件用）----
    def tapered(self, cx: float, y0: int, y1: int, w0: float, w1: float, c: Color) -> None:
        """上下宽窄渐变的竖条（袍摆/袖口/锥形躯干）。cx = 中心线，w = 该行像素宽。"""
        if y1 < y0:
            y0, y1 = y1, y0
        span = max(1, y1 - y0)
        for y in range(y0, y1 + 1):
            t = (y - y0) / span
            w = w0 + (w1 - w0) * t
            half = w / 2.0
            self.hline(int(round(cx - half)), int(round(cx + half - 1)), y, c)

    def blob(self, pts, c: Color) -> None:
        """按行扫描多边形填充（pts = [(x,y), ...]，整数/浮点皆可）。"""
        if len(pts) < 3:
            return
        ys = [p[1] for p in pts]
        for y in range(int(math.floor(min(ys))), int(math.ceil(max(ys))) + 1):
            xs = []
            yc = y + 0.5
            for i in range(len(pts)):
                x0, y0 = pts[i]
                x1, y1 = pts[(i + 1) % len(pts)]
                if (y0 <= yc < y1) or (y1 <= yc < y0):
                    t = (yc - y0) / (y1 - y0)
                    xs.append(x0 + (x1 - x0) * t)
            if not xs:
                continue
            self.hline(int(round(min(xs))), int(round(max(xs))) - 1, y, c)

    def copy(self) -> "Canvas":
        out = Canvas(self.w, self.h)
        out.a = self.a.copy()
        return out

    def seal_holes(self, min_thickness: int = 2) -> int:
        """封掉细缝（长宽任一方向 < min_thickness 的封闭腔）；返回封掉的像素数。

        ≥2×2 的腔是正常肢体间隙（手臂与躯干之间），保留——背景该透出来。
        """
        n = 0
        for comp in thin_cavities(self.a[..., 3] > 0, min_thickness):
            fill_region(self, comp)
            n += len(comp)
        return n

    def stamp(self, other: "Canvas", ox: int, oy: int) -> None:
        """把另一张画布贴上来（alpha>0 的像素覆盖），偏移 ox/oy。"""
        for y in range(other.h):
            for x in range(other.w):
                px = other.a[y, x]
                if px[3] > 0:
                    self.px(x + ox, y + oy, tuple(int(v) for v in px))


# ----------------------------------------------------------------------------
# 收尾：明暗 + 描边
# ----------------------------------------------------------------------------

def outline_ring(solid: np.ndarray) -> np.ndarray:
    """finish() 会落描边的像素集合（4 邻域贴着实心的透明像素）。"""
    return _true_neighbors(solid) & ~solid


def thin_cavities(solid: np.ndarray, min_thickness: int = 2) -> list[list[tuple[int, int]]]:
    """封闭腔中「长宽任一方向 < min_thickness」的块（收尾后会成黑线/黑点）。"""
    out = []
    for comp in holes(solid, conn=8):
        xs = [c[0] for c in comp]
        ys = [c[1] for c in comp]
        if min(max(xs) - min(xs) + 1, max(ys) - min(ys) + 1) < min_thickness:
            out.append(comp)
    return out


def fill_region(c: "Canvas", comp: list[tuple[int, int]]) -> None:
    """用腔体边界邻色众数填实一块区域。"""
    solid = c.a[..., 3] > 0
    votes: dict = {}
    for cx, cy in comp:
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                nx, ny = cx + dx, cy + dy
                if 0 <= nx < c.w and 0 <= ny < c.h and solid[ny, nx]:
                    key = tuple(int(v) for v in c.a[ny, nx])
                    votes[key] = votes.get(key, 0) + 1
    if not votes:
        return
    fill = max(votes.items(), key=lambda kv: kv[1])[0]
    for cx, cy in comp:
        c.a[cy, cx] = fill


def finish(
    c: Canvas,
    outline: Color,
    *,
    light: float = 1.16,
    dark: float = 0.78,
    bottom: float = 0.88,
    outline_scope: str = "all",
) -> Image.Image:
    """统一收尾：右上暗（光自左上）/ 上缘亮 / 下缘沉 → 描边。

    outline_scope:
      "all"   —— 所有贴着实心的透明像素（外轮廓 + 肢体间的内轮廓如腋下/胯下），
                 像素画常规做法；要求几何上不留 1px 封闭缝（校验器把关）。
      "outer" —— 只描外部轮廓（封闭空洞保持透明且无描边）。
    """
    a = c.a.copy()
    solid = a[..., 3] > 0

    # 带 1px 透明边的邻域掩码（越界视作透明）
    pad = np.zeros((c.h + 2, c.w + 2), dtype=bool)
    pad[1:-1, 1:-1] = solid
    up = pad[:-2, 1:-1]
    down = pad[2:, 1:-1]
    left = pad[1:-1, :-2]
    right = pad[1:-1, 2:]

    factor = np.ones((c.h, c.w), dtype=np.float32)
    factor = np.where(solid & ~up, factor * light, factor)      # 上缘受光
    factor = np.where(solid & ~right, factor * dark, factor)    # 右缘背光
    factor = np.where(solid & ~down, factor * bottom, factor)   # 下缘沉
    factor = np.where(solid & ~left, factor * 0.96, factor)     # 左缘极轻压（防死白）

    rgb = a[..., :3].astype(np.float32) * factor[..., None]
    a[..., :3] = np.clip(rgb, 0, 255).astype(np.uint8)

    near = _true_neighbors(solid) & ~solid
    if outline_scope == "outer":
        near &= flood_outside(solid)
    a[near] = rgba(outline)

    return Image.fromarray(a, mode="RGBA")


def flood_outside(solid: np.ndarray, conn: int = 4) -> np.ndarray:
    """从画布四边泛洪（穿透明像素），返回「外部连通」掩码。

    conn=8 时对角相接也算连通——用于判定「封闭腔」：像素画里对角捏出来的
    1px 缝不是洞，只是收尾描边会把它点成一颗暗像素（正常的肢体分隔）。
    """
    h, w = solid.shape
    reach = np.zeros((h, w), dtype=bool)
    reach[0, :] = ~solid[0, :]
    reach[-1, :] = ~solid[-1, :]
    reach[:, 0] |= ~solid[:, 0]
    reach[:, -1] |= ~solid[:, -1]
    for _ in range(h + w):  # 最坏情况下逐像素推进
        grown = reach.copy()
        grown[1:, :] |= reach[:-1, :]
        grown[:-1, :] |= reach[1:, :]
        grown[:, 1:] |= reach[:, :-1]
        grown[:, :-1] |= reach[:, 1:]
        if conn == 8:
            grown[1:, 1:] |= reach[:-1, :-1]
            grown[1:, :-1] |= reach[:-1, 1:]
            grown[:-1, 1:] |= reach[1:, :-1]
            grown[:-1, :-1] |= reach[1:, 1:]
        grown &= ~solid
        if np.array_equal(grown, reach):
            break
        reach = grown
    return reach


def holes(solid: np.ndarray, conn: int = 8) -> list[list[tuple[int, int]]]:
    """返回所有封闭腔（透明且不与画布边界连通的连通块），像素坐标列表。"""
    h, w = solid.shape
    inside = ~solid & ~flood_outside(solid, conn=conn)
    seen = np.zeros((h, w), dtype=bool)
    out = []
    for y in range(h):
        for x in range(w):
            if not inside[y, x] or seen[y, x]:
                continue
            stack, comp = [(x, y)], []
            seen[y, x] = True
            while stack:
                cx, cy = stack.pop()
                comp.append((cx, cy))
                for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                    if 0 <= nx < w and 0 <= ny < h and inside[ny, nx] and not seen[ny, nx]:
                        seen[ny, nx] = True
                        stack.append((nx, ny))
            out.append(comp)
    return out


def _true_neighbors(solid: np.ndarray) -> np.ndarray:
    """非环绕版本的 4 邻域掩码（与 roll 版配合做交集，消掉边界环绕伪描边）。"""
    h, w = solid.shape
    near = np.zeros((h, w), dtype=bool)
    near[1:, :] |= solid[:-1, :]
    near[:-1, :] |= solid[1:, :]
    near[:, 1:] |= solid[:, :-1]
    near[:, :-1] |= solid[:, 1:]
    return near


# ----------------------------------------------------------------------------
# 输出
# ----------------------------------------------------------------------------
def save_png(img: Image.Image, path: str) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    img.save(path, format="PNG", optimize=True)


def sheet(frames: list[Image.Image], fw: int, fh: int, cols: int) -> Image.Image:
    """按 cols 列排布图集（行优先，末行不足留空）。"""
    rows = max(1, math.ceil(len(frames) / cols))
    out = Image.new("RGBA", (fw * cols, fh * rows), (0, 0, 0, 0))
    for i, f in enumerate(frames):
        out.paste(f, ((i % cols) * fw, (i // cols) * fh))
    return out


def preview(sheet_img: Image.Image, scale: int = 6, bg=(46, 44, 58, 255),
            grid: bool = True, fw: int = 32, fh: int = 32) -> Image.Image:
    """放大预览（Nearest，供人眼/多模态审图）；可选画格线。"""
    out = Image.new("RGBA", sheet_img.size, bg)
    out.alpha_composite(sheet_img)
    if grid:
        px = out.load()
        for x in range(0, out.width, fw):
            for y in range(out.height):
                r, g, b, a = px[x, y]
                px[x, y] = (min(255, r + 28), min(255, g + 28), min(255, b + 34), a)
        for y in range(0, out.height, fh):
            for x in range(out.width):
                r, g, b, a = px[x, y]
                px[x, y] = (min(255, r + 28), min(255, g + 28), min(255, b + 34), a)
    return out.resize((out.width * scale, out.height * scale), Image.NEAREST)
