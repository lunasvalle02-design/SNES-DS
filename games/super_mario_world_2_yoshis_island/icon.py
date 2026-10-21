# Super Mario World 2: Yoshi's Island: a storybook island in pastels, round
# crayon hills, a big flower in front and a trail of spotted eggs, under a
# paper-cream sky. An original drawing, never the game's own artwork
# (AGENTS.md, rule 1). Shapes: see tools/icon.py.
vgrad(0xFFE9C4, 0xBFE4F2)
glow(110, 70, 150, 0xFFF8E6, 0.5)
# clouds, soft and round
def cloud(x, y, s):
    for (dx, dy, r) in ((0, 0, 20), (24, -12, 24), (50, 0, 20), (25, 6, 20)):
        circle(x + dx * s, y + dy * s, r * s, 0xFFFFFF, 0.9)
cloud(330, 60, 1.1)
cloud(60, 130, 0.8)
# the hills, back to front, with crayon hatching
for (x, y, rx, ry, c) in ((120, 250, 150, 120, 0xA6D9A0), (390, 240, 160, 140, 0x9ED2C8), (260, 280, 170, 110, 0x86C97E)):
    ellipse(x, y, rx, ry, c)
    for k in range(-5, 6):
        line(x + k * rx / 6 - 18, y - ry * 0.6, x + k * rx / 6 + 18, y - ry * 0.2, 3, shade(c, 0.85), 0.35)
# the sea at the island's foot
rect(0, 318, 512, 66, 0x7CC4E8)
for x in range(0, 512, 48):
    stroke([(x, 334), (x + 12, 328), (x + 24, 334)], 3, 0xE8F6FF, 0.8)
    stroke([(x + 24, 356), (x + 36, 350), (x + 48, 356)], 3, 0xE8F6FF, 0.6)
# the shore
poly([(0, 316), (120, 300), (260, 310), (400, 298), (512, 306), (512, 326), (0, 330)], 0xF2D49A)
# the flower: five round petals, a sunny middle, a stem and two leaves
line(400, 236, 404, 312, 7, 0x3E9A44)
ellipse(384, 292, 20, 9, 0x3E9A44, 1, -0.5)
ellipse(422, 286, 20, 9, 0x3E9A44, 1, 0.5)
for i in range(5):
    a = i * 2 * math.pi / 5 - math.pi / 2
    circle(400 + 34 * math.cos(a), 200 + 34 * math.sin(a), 26, 0xFF8FB4)
circle(400, 200, 26, 0xFFE15A)
circle(392, 192, 9, 0xFFF4B0, 0.7)
# the trail of eggs, smaller toward the back, each with its spots
for (x, y, s, spot) in ((90, 294, 0.6, 0x5CC85A), (160, 288, 0.75, 0xF2C230), (245, 280, 0.95, 0xE8584A)):
    ellipse(x, y + 34 * s, 34 * s, 6 * s, 0x2E6A30, 0.35)
    ellipse(x, y, 30 * s, 38 * s, 0xFFFDF4)
    for (dx, dy, r) in ((-10, -14, 8), (12, -2, 9), (-4, 14, 7)):
        circle(x + dx * s, y + dy * s, r * s, spot)
