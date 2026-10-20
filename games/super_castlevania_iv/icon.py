# Super Castlevania IV: a castle on a crag under a full moon, bats about
vgrad(0x1A0A1E, 0x7A1E2A)
glow(330, 120, 200, 0xFF9A6A, 0.35)
circle(330, 120, 74, 0xF6E6C8)
circle(310, 104, 12, 0xE2CDA8, 0.8)
circle(352, 140, 16, 0xE2CDA8, 0.7)
circle(338, 92, 7, 0xE2CDA8, 0.7)
# clouds across the moon
ellipse(300, 170, 120, 10, 0x3A1424, 0.8)
ellipse(390, 150, 80, 7, 0x3A1424, 0.7)
# the crag
poly([(120, 384), (160, 300), (200, 270), (250, 262), (300, 250), (350, 266), (390, 300), (430, 384)], 0x150812)
# the castle: keep, towers with pointed roofs, battlements
C = 0x0E050C
rect(210, 170, 140, 92, C)
for x in range(210, 350, 20):
    rect(x, 162, 12, 10, C)
for (x, w, top, roof) in ((180, 34, 120, 40), (255, 50, 70, 60), (346, 30, 140, 34), (225, 22, 140, 30)):
    rect(x, top, w, 262 - top, C)
    poly([(x - 4, top), (x + w / 2, top - roof), (x + w + 4, top)], C)
for (x, y) in ((268, 110), (290, 110), (192, 160), (230, 200), (300, 190), (356, 180), (262, 220)):
    rect(x, y, 6, 12, 0xFFB050, 1, 2)
    glow(x + 3, y + 6, 14, 0xFF8A30, 0.5)
# bats
def bat(x, y, s):
    poly([(x, y), (x - 6 * s, y - 5 * s), (x - 12 * s, y - 2 * s), (x - 18 * s, y - 7 * s), (x - 14 * s, y + 2 * s),
          (x - 6 * s, y + 1 * s), (x, y + 4 * s), (x + 6 * s, y + 1 * s), (x + 14 * s, y + 2 * s), (x + 18 * s, y - 7 * s),
          (x + 12 * s, y - 2 * s), (x + 6 * s, y - 5 * s)], 0x0B0408)
for (x, y, s) in ((120, 90, 1.6), (160, 140, 1.0), (420, 60, 1.2), (450, 200, 0.9), (90, 200, 0.8), (330, 120, 1.3)):
    bat(x, y, s)
# a candle stand in front, lit
rect(64, 270, 8, 90, 0x3A2418)
rect(54, 262, 28, 8, 0x5A3A22, 1, 3)
glow(68, 246, 40, 0xFFB050, 0.7)
poly([(68, 228), (60, 254), (68, 262), (76, 254)], 0xFFD36A)
rect(0, 352, 512, 32, 0x0A0408)
