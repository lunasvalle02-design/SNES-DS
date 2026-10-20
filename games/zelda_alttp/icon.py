# A Link to the Past: a sword resting in a stone in a forest clearing, at dawn
vgrad(0x16324A, 0xE9A65A)
glow(256, 250, 260, 0xFFE2A0, 0.55)
glow(256, 230, 110, 0xFFF6DA, 0.45)
# light rays
for k in range(-4, 5):
    t = k * 0.2
    poly([(256, 120), (256 + math.sin(t - 0.04) * 520, 120 + math.cos(t - 0.04) * 520),
          (256 + math.sin(t + 0.04) * 520, 120 + math.cos(t + 0.04) * 520)], 0xFFF3C8, 0.10)
# far hills and trees
poly([(0, 270)] + [(x, 262 - 26 * math.sin(x / 70.0)) for x in range(0, 513, 16)] + [(512, 384), (0, 384)], 0x3E6B3C)
for x in list(range(-10, 140, 34)) + list(range(380, 540, 34)):
    h = 90 + (x * 37 % 40)
    poly([(x, 290 - h), (x - 30, 300), (x + 30, 300)], 0x24472A)
    poly([(x, 270 - h), (x - 22, 250 - h + 60), (x + 22, 250 - h + 60)], 0x2C5632)
# clearing
poly([(0, 330)] + [(x, 312 - 14 * math.sin(x / 90.0 + 1)) for x in range(0, 513, 16)] + [(512, 384), (0, 384)], 0x5C8F42)
ellipse(256, 330, 120, 22, 0x75A852, 0.8)
# the stone
poly([(196, 336), (212, 300), (300, 300), (316, 336)], 0x8C8F98)
poly([(212, 300), (300, 300), (290, 290), (222, 290)], 0xB7BAC3)
rect(206, 334, 100, 10, 0x6B6E77, 1, 4)
# the sword: blade, guard, grip, pommel
glow(256, 190, 90, 0xDDF2FF, 0.55)
poly([(244, 292), (244, 160), (256, 136), (268, 160), (268, 292)], 0xE7EEF5)
poly([(256, 136), (268, 160), (268, 292), (256, 292)], 0xAFC0D2)
line(256, 150, 256, 290, 2, 0xFFFFFF, 0.7)
poly([(206, 166), (222, 154), (290, 154), (306, 166), (290, 176), (222, 176)], 0x2F5FB8)
poly([(206, 166), (222, 154), (290, 154), (306, 166)], 0x4F86E0)
circle(256, 165, 7, 0xF2C94C)
rect(249, 108, 14, 48, 0x5A3A8C, 1, 4)
for yy in range(114, 154, 8):
    line(249, yy, 263, yy + 4, 2, 0x7C58B6)
circle(256, 104, 11, 0x2F5FB8)
circle(256, 104, 5, 0xF2C94C)
# sparkles
for (sx, sy, r) in ((178, 150, 9), (338, 128, 12), (320, 214, 7), (196, 236, 6)):
    star(sx, sy, r, 0xFFFFFF, 0.9)
