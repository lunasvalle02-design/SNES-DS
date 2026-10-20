"""Draws a game's icon for the game list: games/<id>/icon.py describes it with
the shapes below, and this turns it into games/<id>/icon.png beside it.

    python tools/icon.py games/<id>/icon.py

The icons are original drawings (shapes, gradients, glows), never a game's
own artwork: no logos, box art or anything taken from a ROM (AGENTS.md, rule 1).
Plain Python, no libraries; anti-aliased by covering each pixel with 4
sub-rows, exact across.

A description runs with these in scope:
    W, H                        the picture's size (512 x 384, the grid's 4:3)
    vgrad(top, bottom)          the background, a vertical gradient
    poly(points, color, a=1)    a filled polygon (non-zero winding)
    circle(x, y, r, color, a=1), ellipse(x, y, rx, ry, color, a=1, rot=0)
    rect(x, y, w, h, color, a=1, r=0)       r: rounded corners
    glow(x, y, r, color, a=1)   a soft light (added), fading to nothing at r
    star(x, y, r, color, a=1, points=4, inner=0.25)
    line(x0, y0, x1, y1, width, color, a=1)
    stroke(points, width, color, a=1, closed=False)
    rgb(0xRRGGBB) or (r, g, b) for colors; shade(color, k) darker/lighter
"""
import math
import os
import struct
import sys
import zlib

W, H = 512, 384
SUB = 4
buf = [[0.0, 0.0, 0.0] for _ in range(W * H)]


def rgb(c):
    if isinstance(c, tuple):
        return c
    return ((c >> 16) & 255, (c >> 8) & 255, c & 255)


def shade(c, k):
    c = rgb(c)
    if k >= 1:
        return tuple(min(255, int(v + (255 - v) * (k - 1))) for v in c)
    return tuple(int(v * k) for v in c)


def blend(i, c, a):
    if a <= 0:
        return
    p = buf[i]
    for j in range(3):
        if p[j] > 255:
            p[j] = 255.0
    if a > 1:
        a = 1
    p[0] += (c[0] - p[0]) * a
    p[1] += (c[1] - p[1]) * a
    p[2] += (c[2] - p[2]) * a


def vgrad(top, bottom):
    t, b = rgb(top), rgb(bottom)
    for y in range(H):
        k = y / (H - 1)
        c = [t[j] + (b[j] - t[j]) * k for j in range(3)]
        for x in range(W):
            buf[y * W + x] = list(c)


def poly(points, color, a=1.0):
    c = rgb(color)
    ys = [p[1] for p in points]
    y0 = max(0, int(math.floor(min(ys))))
    y1 = min(H - 1, int(math.ceil(max(ys))))
    n = len(points)
    for py in range(y0, y1 + 1):
        cov = {}
        for s in range(SUB):
            sy = py + (s + 0.5) / SUB
            xs = []
            for i in range(n):
                (ax, ay), (bx, by) = points[i], points[(i + 1) % n]
                if ay == by:
                    continue
                if (ay <= sy < by) or (by <= sy < ay):
                    t = (sy - ay) / (by - ay)
                    xs.append((ax + (bx - ax) * t, 1 if by > ay else -1))
            xs.sort()
            wind = 0
            for k in range(len(xs) - 1):
                wind += xs[k][1]
                if wind == 0:
                    continue
                l, r = max(0.0, xs[k][0]), min(float(W), xs[k + 1][0])
                if r <= l:
                    continue
                for px in range(int(l), min(W - 1, int(math.ceil(r)) - 1) + 1):
                    o = min(r, px + 1) - max(l, px)
                    if o > 0:
                        cov[px] = cov.get(px, 0.0) + o / SUB
        for px, v in cov.items():
            blend(py * W + px, c, v * a)


def ellipse(x, y, rx, ry, color, a=1.0, rot=0.0):
    n = max(24, int((rx + ry) * 0.8))
    cr, sr = math.cos(rot), math.sin(rot)
    pts = []
    for i in range(n):
        t = 2 * math.pi * i / n
        ex, ey = math.cos(t) * rx, math.sin(t) * ry
        pts.append((x + ex * cr - ey * sr, y + ex * sr + ey * cr))
    poly(pts, color, a)


def circle(x, y, r, color, a=1.0):
    ellipse(x, y, r, r, color, a)


def rect(x, y, w, h, color, a=1.0, r=0):
    if r <= 0:
        poly([(x, y), (x + w, y), (x + w, y + h), (x, y + h)], color, a)
        return
    pts = []
    for cx, cy, t0 in ((x + w - r, y + r, -90), (x + w - r, y + h - r, 0), (x + r, y + h - r, 90), (x + r, y + r, 180)):
        for i in range(9):
            t = math.radians(t0 + i * 90 / 8)
            pts.append((cx + math.cos(t) * r, cy + math.sin(t) * r))
    poly(pts, color, a)


def glow(x, y, r, color, a=1.0):
    c = rgb(color)
    for py in range(max(0, int(y - r)), min(H, int(y + r) + 1)):
        for px in range(max(0, int(x - r)), min(W, int(x + r) + 1)):
            d = math.hypot(px + 0.5 - x, py + 0.5 - y) / r
            if d < 1:
                k = a * (1 - d) ** 2
                q = buf[py * W + px]
                q[0] += c[0] * k
                q[1] += c[1] * k
                q[2] += c[2] * k


def star(x, y, r, color, a=1.0, points=4, inner=0.25):
    pts = []
    for i in range(points * 2):
        t = math.pi * i / points - math.pi / 2
        rr = r if i % 2 == 0 else r * inner
        pts.append((x + math.cos(t) * rr, y + math.sin(t) * rr))
    poly(pts, color, a)


def line(x0, y0, x1, y1, width, color, a=1.0):
    dx, dy = x1 - x0, y1 - y0
    d = math.hypot(dx, dy) or 1
    nx, ny = -dy / d * width / 2, dx / d * width / 2
    poly([(x0 + nx, y0 + ny), (x1 + nx, y1 + ny), (x1 - nx, y1 - ny), (x0 - nx, y0 - ny)], color, a)


def stroke(points, width, color, a=1.0, closed=False):
    """a polyline drawn as one shape (no darker joints when a < 1)"""
    pts = list(points) + ([points[0]] if closed else [])
    left, right = [], []
    for i, (px, py) in enumerate(pts):
        ax, ay = pts[max(0, i - 1)]
        bx, by = pts[min(len(pts) - 1, i + 1)]
        dx, dy = bx - ax, by - ay
        d = math.hypot(dx, dy) or 1
        nx, ny = -dy / d * width / 2, dx / d * width / 2
        left.append((px + nx, py + ny))
        right.append((px - nx, py - ny))
    poly(left + right[::-1], color, a)


def write_png(path):
    raw = bytearray()
    for y in range(H):
        raw.append(0)
        for x in range(W):
            p = buf[y * W + x]
            raw += bytes(max(0, min(255, int(v + 0.5))) for v in p)

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
    tmp = path + ".tmp"
    with open(tmp, "wb") as f:
        f.write(png)
    os.replace(tmp, path)


def main():
    if len(sys.argv) != 2:
        sys.exit("usage: python tools/icon.py games/<id>/icon.py")
    src = sys.argv[1]
    scope = {k: v for k, v in globals().items() if not k.startswith("_")}
    with open(src, encoding="utf-8") as f:
        exec(compile(f.read(), src, "exec"), scope)
    out = os.path.join(os.path.dirname(src), "icon.png")
    write_png(out)
    print("wrote", out)


if __name__ == "__main__":
    main()
