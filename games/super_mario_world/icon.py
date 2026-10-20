# Super Mario World: a spotted egg in the grass, tall round hills and clouds
# behind it, under a blue sky. An original drawing, never the game's own
# artwork (AGENTS.md, rule 1). Shapes: see tools/icon.py.
vgrad(0x3C8CE8, 0xBEE6FF)
glow(400, 70, 140, 0xFFF6D0, 0.4)
# tall round-topped hills, back to front
for (x, w, top, c) in ((40, 70, 120, 0x8FC6E8), (130, 80, 70, 0x7AB8E0), (330, 90, 90, 0x7AB8E0), (430, 70, 140, 0x8FC6E8)):
    rect(x, top, w, 384 - top, c, 1, w / 2)
    for (dx, dy) in ((w * 0.3, 40), (w * 0.55, 90), (w * 0.35, 140)):
        ellipse(x + dx, top + dy, 5, 9, shade(c, 1.25), 0.8)
# clouds
def cloud(x, y, s):
    for (dx, dy, r) in ((0, 0, 22), (26, -10, 26), (54, 0, 22), (27, 8, 22)):
        circle(x + dx * s, y + dy * s, r * s, 0xFFFFFF, 0.95)
cloud(60, 70, 1.1)
cloud(250, 120, 0.9)
cloud(400, 210, 1.0)
# rolling grass and a dirt band
poly([(0, 300), (80, 284), (180, 292), (300, 280), (420, 290), (512, 282), (512, 384), (0, 384)], 0x3FAE3A)
poly([(0, 306), (80, 290), (180, 298), (300, 286), (420, 296), (512, 288), (512, 300), (0, 318)], 0x2E8A2C)
rect(0, 336, 512, 48, 0xC88A4A)
for x in range(12, 512, 40):
    circle(x, 356, 4, 0xE0A868)
    circle(x + 20, 372, 4, 0xE0A868)
# bushes
for (x, r) in ((70, 34), (110, 26), (430, 30)):
    circle(x, 300, r, 0x2E9A34)
    circle(x - r * 0.3, 292, r * 0.35, 0x5CC85A, 0.7)
# the egg: white, green spots, a shadow
ellipse(262, 318, 60, 10, 0x1E6A22, 0.5)
ellipse(256, 246, 58, 74, 0xFAFAF2)
ellipse(240, 220, 20, 30, 0xFFFFFF, 0.6)
for (x, y, r) in ((232, 206, 14), (280, 232, 16), (246, 270, 13), (290, 284, 10), (226, 248, 8), (272, 190, 9)):
    circle(x, y, r, 0x3FB848)
