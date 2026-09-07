#!/usr/bin/env python3
"""Builds the low-poly street-football pack: assets/street.

Every model is plain boxes/cylinders/tori with flat MTL colors, generated
here so the geometry stays tweakable — edit the numbers below and re-run:

    python3 Tools/make_street_models.py

Layout (kept apart from every other pack, so customizing one file never
touches the rest):
    assets/street/props/<name>.obj + <name>.mtl   goal, cone, tires, bricks, bench
    assets/street/kids/<name>.obj + <name>.mtl     three old-alley kids

Conventions: Y up, feet on y=0, figures face +Z, sizes are real metres.
Faces wind counter-clockwise from outside (the engine's contract).
"""

import math
import os

ROOT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                    "assets", "street")


class Model:
    def __init__(self, name):
        self.name = name
        self.verts = []    # (x, y, z)
        self.normals = []  # (x, y, z)
        self.faces = []    # (material, [v0, v1, ...], n)

    def vert(self, x, y, z):
        self.verts.append((x, y, z))
        return len(self.verts) - 1

    def normal(self, x, y, z):
        self.normals.append((x, y, z))
        return len(self.normals) - 1

    def face(self, mat, corners, normal):
        self.faces.append((mat, list(corners), normal))

    def box(self, cx, cy, cz, sx, sy, sz, mat):
        hx, hy, hz = sx / 2.0, sy / 2.0, sz / 2.0
        c = [self.vert(cx + dx, cy + dy, cz + dz) for dx, dy, dz in
             [(-hx, -hy, -hz), (hx, -hy, -hz), (hx, hy, -hz), (-hx, hy, -hz),
              (-hx, -hy, hz), (hx, -hy, hz), (hx, hy, hz), (-hx, hy, hz)]]
        self.face(mat, [c[4], c[5], c[6], c[7]], self.normal(0, 0, 1))    # front
        self.face(mat, [c[1], c[0], c[3], c[2]], self.normal(0, 0, -1))   # back
        self.face(mat, [c[5], c[1], c[2], c[6]], self.normal(1, 0, 0))    # right
        self.face(mat, [c[0], c[4], c[7], c[3]], self.normal(-1, 0, 0))   # left
        self.face(mat, [c[3], c[7], c[6], c[2]], self.normal(0, 1, 0))    # top
        self.face(mat, [c[0], c[1], c[5], c[4]], self.normal(0, -1, 0))   # bottom

    @staticmethod
    def _rot_axis(axis, x, y, z):
        if axis == "x":  # Y -> X (about Z, det +1, winding kept)
            return (y, -x, z)
        if axis == "z":  # Y -> Z (about X, det +1, winding kept)
            return (x, -z, y)
        return (x, y, z)

    def cylinder(self, cx, cy, cz, r_top, r_bot, h, mat, seg=12, axis="y"):
        def place(x, y, z):
            rx, ry, rz = self._rot_axis(axis, x, y, z)
            return self.vert(cx + rx, cy + ry, cz + rz)

        def direction(x, y, z):
            return self.normal(*self._rot_axis(axis, x, y, z))

        top, bot = [], []
        for i in range(seg):
            a = 2.0 * math.pi * i / seg
            top.append(place(r_top * math.cos(a), h / 2.0, r_top * math.sin(a)))
            bot.append(place(r_bot * math.cos(a), -h / 2.0, r_bot * math.sin(a)))
        for i in range(seg):
            j = (i + 1) % seg
            mid = 2.0 * math.pi * (i + 0.5) / seg
            n = direction(math.cos(mid), 0.0, math.sin(mid))
            self.face(mat, [top[i], top[j], bot[j], bot[i]], n)
        tc = place(0.0, h / 2.0, 0.0)
        bc = place(0.0, -h / 2.0, 0.0)
        for i in range(seg):
            j = (i + 1) % seg
            self.face(mat, [tc, top[j], top[i]], direction(0, 1, 0))
            self.face(mat, [bc, bot[i], bot[j]], direction(0, -1, 0))

    def torus(self, cx, cy, cz, big_r, small_r, mat, seg_u=16, seg_v=10, tilt_deg=0.0):
        tilt = math.radians(tilt_deg)
        ct, st = math.cos(tilt), math.sin(tilt)

        def place(x, y, z):
            ry = y * ct - z * st  # lean about X; det +1, winding kept
            rz = y * st + z * ct
            return self.vert(cx + x, cy + ry, cz + rz)

        def direction(x, y, z):
            return self.normal(x, y * ct - z * st, y * st + z * ct)

        ring = []
        for i in range(seg_u):
            u = 2.0 * math.pi * i / seg_u
            row = []
            for j in range(seg_v):
                v = 2.0 * math.pi * j / seg_v
                r = big_r + small_r * math.cos(v)
                row.append(place(r * math.cos(u), small_r * math.sin(v), r * math.sin(u)))
            ring.append(row)
        for i in range(seg_u):
            ni = (i + 1) % seg_u
            u = 2.0 * math.pi * (i + 0.5) / seg_u
            for j in range(seg_v):
                nj = (j + 1) % seg_v
                v = 2.0 * math.pi * (j + 0.5) / seg_v
                n = direction(math.cos(v) * math.cos(u), math.sin(v),
                              math.cos(v) * math.sin(u))
                self.face(mat, [ring[i][j], ring[i][nj], ring[ni][nj], ring[ni][j]], n)

    def write(self, folder, materials):
        os.makedirs(folder, exist_ok=True)
        obj_path = os.path.join(folder, self.name + ".obj")
        with open(obj_path, "w", encoding="utf-8") as f:
            f.write("# %s — generated by Tools/make_street_models.py (do not hand-edit)\n" % self.name)
            f.write("mtllib %s.mtl\n" % self.name)
            f.write("o %s\n" % self.name)
            for x, y, z in self.verts:
                f.write("v %.6f %.6f %.6f\n" % (x, y, z))
            for x, y, z in self.normals:
                f.write("vn %.6f %.6f %.6f\n" % (x, y, z))
            current = None
            for mat, corners, n in self.faces:
                if mat != current:
                    f.write("usemtl %s\n" % mat)
                    current = mat
                f.write("f %s\n" % " ".join("%d//%d" % (v + 1, n + 1) for v in corners))
        with open(os.path.join(folder, self.name + ".mtl"), "w", encoding="utf-8") as f:
            f.write("# colors for %s — TWEAK THESE to customize (Kd = red green blue, 0..1)\n"
                    % self.name)
            for mat, (r, g, b) in materials.items():
                f.write("\nnewmtl %s\nKd %.6f %.6f %.6f\n" % (mat, r, g, b))
        tris = sum(max(0, len(c) - 2) for _, c, _ in self.faces)
        print("wrote %-28s %4d verts %4d tris %d mats" %
              (os.path.join(os.path.basename(folder), self.name), len(self.verts), tris,
               len(materials)))


# ---------------------------------------------------------------- props ---

def goal_small(folder):
    """A 1.6m street goal frame facing +Z: posts, bar, ground feet."""
    m = Model("goal_small")
    white = "paint"
    m.cylinder(-0.8, 0.5, 0, 0.035, 0.035, 1.0, white)
    m.cylinder(0.8, 0.5, 0, 0.035, 0.035, 1.0, white)
    m.cylinder(0, 1.0, 0, 0.035, 0.035, 1.67, white, axis="x")
    m.cylinder(-0.8, 0.035, -0.25, 0.03, 0.03, 0.55, white, axis="z")
    m.cylinder(0.8, 0.035, -0.25, 0.03, 0.03, 0.55, white, axis="z")
    m.write(folder, {"paint": (0.90, 0.90, 0.92)})


def cone(folder):
    m = Model("cone")
    m.box(0, 0.015, 0, 0.20, 0.03, 0.20, "base")
    m.cylinder(0, 0.14, 0, 0.020, 0.075, 0.22, "body", seg=14)
    m.cylinder(0, 0.13, 0, 0.048, 0.058, 0.05, "band", seg=14)
    m.write(folder, {"base": (0.55, 0.18, 0.06), "body": (0.95, 0.35, 0.08),
                     "band": (0.95, 0.95, 0.95)})


def tire_stack(folder):
    m = Model("tire_stack")
    m.torus(0, 0.055, 0, 0.14, 0.055, "rubber")
    m.torus(0, 0.165, 0, 0.14, 0.055, "rubber")
    m.torus(0, 0.275, 0, 0.14, 0.055, "rubber")
    m.torus(0.30, 0.185, 0.02, 0.14, 0.055, "rubber", tilt_deg=65.0)
    m.write(folder, {"rubber": (0.10, 0.10, 0.11)})


def brick_stack(folder):
    """One mini pile; place two a couple of metres apart for a brick goal."""
    m = Model("brick_stack")
    bw, bh, bd = 0.20, 0.06, 0.10
    rows = [(3, "brick_a"), (2, "brick_b"), (1, "brick_a")]
    for level, (count, mat) in enumerate(rows):
        for i in range(count):
            x = (i - (count - 1) / 2.0) * (bw + 0.005)
            m.box(x, bh / 2.0 + level * (bh + 0.004), 0, bw, bh, bd, mat)
    m.write(folder, {"brick_a": (0.62, 0.28, 0.16), "brick_b": (0.55, 0.24, 0.14)})


def bench(folder):
    m = Model("bench")
    for z in (-0.11, 0.0, 0.11):
        m.box(0, 0.42, z, 1.40, 0.04, 0.09, "wood")
    for y in (0.60, 0.72):
        m.box(0, y, -0.155, 1.40, 0.09, 0.04, "wood")
    for x in (-0.60, 0.60):
        for z in (-0.12, 0.12):
            m.box(x, 0.21, z, 0.05, 0.42, 0.05, "iron")
        m.box(x, 0.10, 0, 0.05, 0.05, 0.29, "iron")
    m.write(folder, {"wood": (0.45, 0.28, 0.15), "iron": (0.15, 0.15, 0.17)})


# ----------------------------------------------------------------- kids ---

def kid(folder, name, shirt, shorts, shoes, skin, hair, striped=None, cap=None,
        tall=1.0, pants=None):
    """An old-alley kid facing +Z, ~1.1m (tall scales Y). striped=(c1, c2)
    makes a banded shirt; cap=(color,) adds a cap; pants colors long legs."""
    m = Model(name)
    y = 0.0
    shoe_h = 0.06
    for x in (-0.07, 0.07):
        m.box(x, shoe_h / 2.0, 0.02, 0.09, shoe_h, 0.16, shoes[0])
    y += shoe_h
    leg_h = 0.30 * tall
    leg_mat = pants if pants else skin[0]
    for x in (-0.07, 0.07):
        m.box(x, y + leg_h / 2.0, 0, 0.10, leg_h, 0.11, leg_mat)
    y += leg_h
    m.box(0, y + 0.08, 0, 0.26, 0.16, 0.17, shorts[0])
    y += 0.16
    torso_h = 0.32 * tall
    if striped:
        bands = 4
        c1, c2 = striped[0], striped[2]
        for i in range(bands):
            m.box(0, y + torso_h * (i + 0.5) / bands, 0, 0.28,
                  torso_h / bands + 0.001, 0.18, c1 if i % 2 == 0 else c2)
    else:
        m.box(0, y + torso_h / 2.0, 0, 0.28, torso_h, 0.18, shirt[0])
    sleeve_y = y + torso_h - 0.05
    for x in (-0.175, 0.175):
        m.box(x, sleeve_y, 0, 0.08, 0.10, 0.09, shirt[0])
        m.box(x, sleeve_y - 0.05 - 0.12 * tall, 0, 0.07, 0.24 * tall, 0.08, skin[0])
    y += torso_h
    m.box(0, y + 0.11, 0, 0.20, 0.22, 0.20, skin[0])  # head
    for x in (-0.05, 0.05):  # eyes
        m.box(x, y + 0.13, 0.10, 0.025, 0.03, 0.012, "eyes")
    mats = {"eyes": (0.08, 0.07, 0.07), shoes[0]: shoes[1], shorts[0]: shorts[1],
            shirt[0]: shirt[1], skin[0]: skin[1]}
    if striped:
        mats[striped[2]] = striped[3]
    if cap:
        mats[cap[0]] = cap[1]
        m.cylinder(0, y + 0.235, 0, 0.115, 0.125, 0.07, cap[0], seg=14)
        m.box(0, y + 0.205, 0.15, 0.16, 0.02, 0.12, cap[0])  # brim
        m.box(0, y + 0.10, -0.105, 0.21, 0.12, 0.03, hair[0])  # hair at the back
        mats[hair[0]] = hair[1]
    else:
        m.box(0, y + 0.19, 0, 0.21, 0.08, 0.21, hair[0])  # hair cap
        m.box(0, y + 0.10, -0.10, 0.21, 0.12, 0.04, hair[0])
        mats[hair[0]] = hair[1]
    m.write(folder, mats)


def main():
    props = os.path.join(ROOT, "props")
    kids = os.path.join(ROOT, "kids")

    goal_small(props)
    cone(props)
    tire_stack(props)
    brick_stack(props)
    bench(props)

    # Ali: the captain -- red/white stripes, red cap, blue shorts.
    kid(kids, "kid_ali", shirt=("shirt", (0.80, 0.12, 0.12)),
        shorts=("shorts", (0.10, 0.20, 0.55)), shoes=("shoes", (0.16, 0.14, 0.13)),
        skin=("skin", (0.85, 0.62, 0.45)), hair=("hair", (0.12, 0.10, 0.10)),
        striped=("shirt", None, "stripe", (0.93, 0.93, 0.93)), cap=("cap", (0.80, 0.12, 0.12)))
    # Reza: barefoot in green, beige shorts.
    kid(kids, "kid_reza", shirt=("shirt", (0.12, 0.45, 0.20)),
        shorts=("shorts", (0.75, 0.68, 0.52)), shoes=("feet", (0.78, 0.55, 0.40)),
        skin=("skin", (0.78, 0.55, 0.40)), hair=("hair", (0.10, 0.09, 0.09)))
    # Hassan: the tall one -- yellow shirt, long dark pants.
    kid(kids, "kid_hassan", shirt=("shirt", (0.90, 0.75, 0.15)),
        shorts=("shorts", (0.10, 0.14, 0.35)), shoes=("shoes", (0.20, 0.18, 0.17)),
        skin=("skin", (0.88, 0.66, 0.50)), hair=("hair", (0.14, 0.11, 0.10)),
        tall=1.15, pants="shorts")


if __name__ == "__main__":
    main()
