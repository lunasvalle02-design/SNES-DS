# Secret of Mana: a great tree on a far hill at dawn, and a rusty sword in a
# stone by a stream in front. An original drawing, never the game's own
# artwork (AGENTS.md, rule 1). Make the picture with:
# python tools/icon.py games/secret_of_mana/icon.py
vgrad(0x2B3E7A, 0xF6B36A)
glow(330, 150, 220, 0xFFE6B0, 0.45)
# the far hill and the great tree on it
poly([(0, 250), (120, 215), (250, 190), (380, 200), (512, 230), (512, 384), (0, 384)], 0x4E7A4A)
rect(318, 92, 22, 110, 0x5A3A26)
poly([(318, 200), (300, 210), (358, 210), (340, 200)], 0x5A3A26)
for (x, y, r) in ((329, 70, 62), (282, 96, 44), (378, 96, 46), (300, 56, 36), (360, 54, 38), (329, 34, 34)):
    circle(x, y, r, 0x3F8A4E)
for (x, y, r) in ((316, 60, 26), (356, 82, 22), (292, 92, 18), (340, 36, 18)):
    circle(x, y, r, 0x6DB45E, 0.8)
glow(329, 70, 120, 0xC8FFB0, 0.25)
# motes of light about the tree
for (x, y, r) in ((250, 60, 5), (410, 40, 4), (420, 120, 5), (240, 130, 4), (300, 150, 3), (380, 160, 3)):
    star(x, y, r * 2, 0xFFF6D0, 0.9)
# the near meadow and the stream
poly([(0, 290), (180, 270), (512, 285), (512, 384), (0, 384)], 0x3C6A34)
poly([(0, 384), (60, 330), (150, 305), (260, 300), (330, 312), (300, 340), (210, 384)], 0x5C9AC4)
stroke([(70, 350), (140, 322), (220, 314)], 3, 0xCDEBFF, 0.7)
stroke([(170, 360), (240, 330), (290, 322)], 3, 0xCDEBFF, 0.6)
# the stone, and the sword in it
rect(151, 230, 12, 66, 0xC9CFD6)
rect(155, 232, 3, 64, 0xF2F6FA, 0.7)
rect(134, 220, 46, 10, 0x8C5A2A, 1, 3)
rect(152, 186, 10, 34, 0x6A3E1E)
circle(157, 182, 8, 0xD8A030)
glow(157, 210, 70, 0xFFF0C0, 0.4)
poly([(120, 330), (128, 300), (150, 288), (178, 292), (192, 312), (188, 334), (150, 342)], 0x8A8478)
poly([(132, 304), (150, 292), (172, 296), (160, 304)], 0xB2AC9E, 0.8)
# flowers in the grass
for (x, y, c) in ((40, 300, 0xF2D04A), (90, 292, 0xE88AB0), (420, 310, 0xF2D04A), (470, 330, 0xE88AB0), (370, 350, 0xF2D04A)):
    circle(x, y, 5, c)
