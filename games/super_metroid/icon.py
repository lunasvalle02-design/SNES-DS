# Super Metroid: a gunship coming down through the storm onto an alien planet
import random
random.seed(7)
vgrad(0x0A0820, 0x3A1A4A)
# stars
for _ in range(90):
    x, y = random.uniform(0, W), random.uniform(0, 230)
    circle(x, y, random.uniform(0.6, 1.6), 0xFFFFFF, random.uniform(0.4, 1))
for (sx, sy) in ((70, 50), (420, 80), (300, 30)):
    star(sx, sy, 7, 0xDDE8FF, 0.9)
# the planet, a crescent lit from the left
glow(390, 120, 150, 0x6A5BFF, 0.35)
circle(390, 120, 86, 0x5B3F8F)
circle(372, 112, 80, 0x8C6CC4)
circle(410, 128, 78, 0x5B3F8F)
for (cx, cy, r) in ((350, 90, 10), (340, 140, 7), (372, 160, 9)):
    circle(cx, cy, r, 0x4E357D, 0.8)
# storm clouds
for (cx, cy, rx) in ((80, 230, 140), (260, 245, 170), (460, 228, 130)):
    ellipse(cx, cy, rx, 26, 0x4A2E63, 0.85)
    ellipse(cx + 20, cy - 8, rx * 0.7, 16, 0x5E3D7A, 0.7)
# the ship: a wide hull, a canopy, thrusters
sx, sy = 196, 150
glow(sx, sy + 40, 70, 0xFFB347, 0.5)
poly([(sx - 70, sy + 6), (sx - 40, sy - 14), (sx + 40, sy - 14), (sx + 70, sy + 6), (sx + 46, sy + 20), (sx - 46, sy + 20)], 0xE58A1F)
poly([(sx - 70, sy + 6), (sx - 40, sy - 14), (sx + 40, sy - 14), (sx + 70, sy + 6)], 0xFFB04A)
ellipse(sx, sy - 14, 30, 16, 0x38E0B0)
ellipse(sx - 8, sy - 19, 12, 5, 0xC8FFF0, 0.8)
rect(sx - 52, sy + 18, 22, 10, 0x8A4A12, 1, 3)
rect(sx + 30, sy + 18, 22, 10, 0x8A4A12, 1, 3)
for tx in (sx - 41, sx + 41):
    poly([(tx - 9, sy + 28), (tx + 9, sy + 28), (tx, sy + 62)], 0xFFE08A, 0.85)
    glow(tx, sy + 36, 22, 0xFFF3C0, 0.8)
# rocks and spires
poly([(0, 300), (40, 262), (70, 290), (110, 230), (140, 286), (190, 270), (230, 300), (290, 262), (330, 296),
      (380, 240), (410, 282), (460, 256), (512, 290), (512, 384), (0, 384)], 0x2A1636)
poly([(0, 340), (60, 310), (120, 332), (200, 300), (260, 336), (340, 310), (420, 338), (512, 312), (512, 384), (0, 384)], 0x1A0D24)
for (x, h) in ((110, 120), (380, 104), (470, 70)):
    poly([(x - 10, 300), (x, 300 - h), (x + 10, 300)], 0x3A2050)
# glowing plants
for (x, y) in ((60, 336), (150, 322), (300, 330), (440, 334)):
    glow(x, y, 26, 0x3CFFB0, 0.6)
    circle(x, y, 4, 0xB8FFE6)
