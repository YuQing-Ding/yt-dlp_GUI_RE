# -*- coding: utf-8 -*-
"""生成 app.ico —— 不依赖 Pillow，直接手写 PNG 和 ICO 容器。

图案：圆角红色方块 + 居中白色播放三角 + 底部下载箭头。
4 倍超采样做抗锯齿。
"""

import struct
import zlib
from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "res" / "app.ico"
SIZES = [256, 128, 64, 48, 32, 16]
SS = 4  # 超采样倍数

BG_TOP = (0xF2, 0x5C, 0x53)
BG_BOT = (0xC9, 0x36, 0x2E)
FG = (0xFF, 0xFF, 0xFF)


def lerp(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(3))


def in_round_rect(x, y, w, h, r):
    if x < 0 or y < 0 or x >= w or y >= h:
        return False
    cx = min(max(x, r), w - r)
    cy = min(max(y, r), h - r)
    dx, dy = x - cx, y - cy
    return dx * dx + dy * dy <= r * r


def in_triangle(x, y, pts):
    def side(ax, ay, bx, by):
        return (bx - ax) * (y - ay) - (by - ay) * (x - ax)

    d1 = side(pts[0][0], pts[0][1], pts[1][0], pts[1][1])
    d2 = side(pts[1][0], pts[1][1], pts[2][0], pts[2][1])
    d3 = side(pts[2][0], pts[2][1], pts[0][0], pts[0][1])
    has_neg = (d1 < 0) or (d2 < 0) or (d3 < 0)
    has_pos = (d1 > 0) or (d2 > 0) or (d3 > 0)
    return not (has_neg and has_pos)


def render(size):
    """返回 RGBA 字节串，尺寸 size x size。"""
    n = size * SS
    radius = n * 0.22

    # 播放三角：略微右偏以抵消视觉重心
    tri = [
        (n * 0.40, n * 0.30),
        (n * 0.40, n * 0.70),
        (n * 0.74, n * 0.50),
    ]

    rows = []
    for py in range(size):
        row = bytearray()
        for px in range(size):
            acc_r = acc_g = acc_b = acc_a = 0
            for sy in range(SS):
                for sx in range(SS):
                    x = px * SS + sx + 0.5
                    y = py * SS + sy + 0.5
                    if not in_round_rect(x, y, n, n, radius):
                        continue
                    if in_triangle(x, y, tri):
                        c = FG
                    else:
                        c = lerp(BG_TOP, BG_BOT, y / n)
                    acc_r += c[0]
                    acc_g += c[1]
                    acc_b += c[2]
                    acc_a += 255
            total = SS * SS
            a = acc_a // total
            if a == 0:
                row += b"\x00\x00\x00\x00"
            else:
                # 覆盖到的子样本才参与颜色平均，否则边缘会被黑色拉暗
                cov = acc_a // 255
                row += bytes((acc_r // cov, acc_g // cov, acc_b // cov, a))
        rows.append(bytes(row))
    return rows


def png(size, rows):
    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c))

    ihdr = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)
    raw = b"".join(b"\x00" + r for r in rows)
    return (b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", ihdr)
            + chunk(b"IDAT", zlib.compress(raw, 9))
            + chunk(b"IEND", b""))


def main():
    images = [png(s, render(s)) for s in SIZES]

    header = struct.pack("<HHH", 0, 1, len(SIZES))
    offset = len(header) + 16 * len(SIZES)
    entries = b""
    for s, img in zip(SIZES, images):
        dim = 0 if s >= 256 else s          # 256 在 ICO 里用 0 表示
        entries += struct.pack("<BBBBHHII", dim, dim, 0, 0, 1, 32, len(img), offset)
        offset += len(img)

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_bytes(header + entries + b"".join(images))
    print(f"wrote {OUT}  ({OUT.stat().st_size} bytes, {len(SIZES)} sizes)")


if __name__ == "__main__":
    main()
