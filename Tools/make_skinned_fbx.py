#!/usr/bin/env python3
"""Writes a minimal ASCII FBX with a 2-bone skinned bar plus an animation.

Geometry: a tall box from y=0 to y=2, split at y=1, so the lower ring is
owned by the root bone and the upper ring by the tip bone. Bending the tip
bone must visibly move the top vertices and leave the bottom ones alone.
"""
import sys

# 8 corners of a 1x2x1 bar, bottom ring first (y=0), then top ring (y=2).
verts = [
    (-0.5, 0.0, -0.5), (0.5, 0.0, -0.5), (0.5, 0.0, 0.5), (-0.5, 0.0, 0.5),
    (-0.5, 2.0, -0.5), (0.5, 2.0, -0.5), (0.5, 2.0, 0.5), (-0.5, 2.0, 0.5),
]
# Quads as FBX polygon-vertex-index (last index of each face is ~(-idx-1)).
faces = [
    (0, 1, 2, 3),  # bottom
    (4, 7, 6, 5),  # top
    (0, 4, 5, 1),
    (1, 5, 6, 2),
    (2, 6, 7, 3),
    (3, 7, 4, 0),
]
pvi = []
for f in faces:
    for i, v in enumerate(f):
        pvi.append(v if i < len(f) - 1 else (-v - 1))

def fmt(vals, per=3):
    return ",".join("%.6f" % v for v in vals)

positions = []
for v in verts:
    positions.extend(v)

# Ids
GEOM, MODEL, DEFORMER = 1000, 2000, 3000
BONE_ROOT, BONE_TIP = 4000, 4001
CLUSTER_ROOT, CLUSTER_TIP = 5000, 5001
ANIM_STACK, ANIM_LAYER = 6000, 6001
CURVE_NODE, CURVE_X = 7000, 7001

out = []
w = out.append

w("; FBX 7.4.0 project file — KIMIA test asset (skinned bar)")
w("FBXHeaderExtension:  {")
w("\tFBXHeaderVersion: 1003")
w("\tFBXVersion: 7400")
w("\tCreator: \"KIMIA asset_gen\"")
w("}")
w("GlobalSettings:  {")
w("\tVersion: 1000")
w("\tProperties70:  {")
w("\t\tP: \"UpAxis\", \"int\", \"Integer\", \"\",1")
w("\t\tP: \"UpAxisSign\", \"int\", \"Integer\", \"\",1")
w("\t\tP: \"FrontAxis\", \"int\", \"Integer\", \"\",2")
w("\t\tP: \"FrontAxisSign\", \"int\", \"Integer\", \"\",1")
w("\t\tP: \"CoordAxis\", \"int\", \"Integer\", \"\",0")
w("\t\tP: \"CoordAxisSign\", \"int\", \"Integer\", \"\",1")
w("\t\tP: \"UnitScaleFactor\", \"double\", \"Number\", \"\",100")
w("\t\tP: \"TimeMode\", \"enum\", \"\", \"\",6")
w("\t}")
w("}")

w("Definitions:  {")
w("\tVersion: 100")
w("\tCount: 8")
for t, c in [("GlobalSettings", 1), ("Model", 3), ("Geometry", 1), ("Deformer", 3),
             ("AnimationStack", 1), ("AnimationLayer", 1), ("AnimationCurveNode", 1),
             ("AnimationCurve", 1)]:
    w("\tObjectType: \"%s\" {" % t)
    w("\t\tCount: %d" % c)
    w("\t}")
w("}")

w("Objects:  {")
# --- geometry ---
w("\tGeometry: %d, \"Geometry::Bar\", \"Mesh\" {" % GEOM)
w("\t\tVertices: *%d {" % len(positions))
w("\t\t\ta: %s" % fmt(positions))
w("\t\t} ")
w("\t\tPolygonVertexIndex: *%d {" % len(pvi))
w("\t\t\ta: %s" % ",".join(str(i) for i in pvi))
w("\t\t} ")
w("\t\tGeometryVersion: 124")
w("\t}")

# --- models ---
w("\tModel: %d, \"Model::Bar\", \"Mesh\" {" % MODEL)
w("\t\tVersion: 232")
w("\t\tProperties70:  {")
w("\t\t\tP: \"Lcl Translation\", \"Lcl Translation\", \"\", \"A\",0,0,0")
w("\t\t}")
w("\t}")
w("\tModel: %d, \"Model::BoneRoot\", \"LimbNode\" {" % BONE_ROOT)
w("\t\tVersion: 232")
w("\t\tProperties70:  {")
w("\t\t\tP: \"Lcl Translation\", \"Lcl Translation\", \"\", \"A\",0,0,0")
w("\t\t}")
w("\t}")
# The tip bone sits 1 unit up the bar, in its parent's space.
w("\tModel: %d, \"Model::BoneTip\", \"LimbNode\" {" % BONE_TIP)
w("\t\tVersion: 232")
w("\t\tProperties70:  {")
w("\t\t\tP: \"Lcl Translation\", \"Lcl Translation\", \"\", \"A\",0,1,0")
w("\t\t}")
w("\t}")

# --- skin deformer + clusters ---
w("\tDeformer: %d, \"Deformer::Skin\", \"Skin\" {" % DEFORMER)
w("\t\tVersion: 101")
w("\t\tLink_DeformAcuracy: 50")
w("\t}")

# Root bone owns the bottom ring (indices 0..3).
w("\tDeformer: %d, \"SubDeformer::ClusterRoot\", \"Cluster\" {" % CLUSTER_ROOT)
w("\t\tVersion: 100")
w("\t\tUserData: \"\", \"\"")
w("\t\tIndexes: *4 {")
w("\t\t\ta: 0,1,2,3")
w("\t\t} ")
w("\t\tWeights: *4 {")
w("\t\t\ta: 1,1,1,1")
w("\t\t} ")
w("\t\tTransform: *16 {")
w("\t\t\ta: 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1")
w("\t\t} ")
w("\t\tTransformLink: *16 {")
w("\t\t\ta: 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1")
w("\t\t} ")
w("\t}")

# Tip bone owns the top ring (indices 4..7). Its bind matrix is a
# translation of 1 up the Y axis (column-major, translation in the last row).
w("\tDeformer: %d, \"SubDeformer::ClusterTip\", \"Cluster\" {" % CLUSTER_TIP)
w("\t\tVersion: 100")
w("\t\tUserData: \"\", \"\"")
w("\t\tIndexes: *4 {")
w("\t\t\ta: 4,5,6,7")
w("\t\t} ")
w("\t\tWeights: *4 {")
w("\t\t\ta: 1,1,1,1")
w("\t\t} ")
w("\t\tTransform: *16 {")
w("\t\t\ta: 1,0,0,0,0,1,0,0,0,0,1,0,0,-1,0,1")
w("\t\t} ")
w("\t\tTransformLink: *16 {")
w("\t\t\ta: 1,0,0,0,0,1,0,0,0,0,1,0,0,1,0,1")
w("\t\t} ")
w("\t}")

# --- animation: rotate the tip bone about Z from 0 to 90 degrees ---
w("\tAnimationStack: %d, \"AnimStack::Bend\", \"\" {" % ANIM_STACK)
w("\t\tProperties70:  {")
# FBX time unit: 46186158000 ticks per second. 1 second clip.
w("\t\t\tP: \"LocalStart\", \"KTime\", \"Time\", \"\",0")
w("\t\t\tP: \"LocalStop\", \"KTime\", \"Time\", \"\",46186158000")
w("\t\t}")
w("\t}")
w("\tAnimationLayer: %d, \"AnimLayer::BaseLayer\", \"\" {" % ANIM_LAYER)
w("\t}")
w("\tAnimationCurveNode: %d, \"AnimCurveNode::R\", \"\" {" % CURVE_NODE)
w("\t\tProperties70:  {")
w("\t\t\tP: \"d|X\", \"Number\", \"\", \"A\",0")
w("\t\t\tP: \"d|Y\", \"Number\", \"\", \"A\",0")
w("\t\t\tP: \"d|Z\", \"Number\", \"\", \"A\",0")
w("\t\t}")
w("\t}")
w("\tAnimationCurve: %d, \"AnimCurve::\", \"\" {" % CURVE_X)
w("\t\tDefault: 0")
w("\t\tKeyVer: 4009")
w("\t\tKeyTime: *2 {")
w("\t\t\ta: 0,46186158000")
w("\t\t} ")
w("\t\tKeyValueFloat: *2 {")
w("\t\t\ta: 0,90")
w("\t\t} ")
w("\t\tKeyAttrFlags: *1 {")
w("\t\t\ta: 24840")
w("\t\t} ")
w("\t\tKeyAttrDataFloat: *4 {")
w("\t\t\ta: 0,0,218434821,218434821")
w("\t\t} ")
w("\t\tKeyAttrRefCount: *1 {")
w("\t\t\ta: 2")
w("\t\t} ")
w("\t}")
w("}")

w("Connections:  {")
w("\tC: \"OO\",%d,0" % MODEL)
w("\tC: \"OO\",%d,0" % BONE_ROOT)
w("\tC: \"OO\",%d,%d" % (BONE_TIP, BONE_ROOT))
w("\tC: \"OO\",%d,%d" % (GEOM, MODEL))
w("\tC: \"OO\",%d,%d" % (DEFORMER, GEOM))
w("\tC: \"OO\",%d,%d" % (CLUSTER_ROOT, DEFORMER))
w("\tC: \"OO\",%d,%d" % (CLUSTER_TIP, DEFORMER))
w("\tC: \"OO\",%d,%d" % (BONE_ROOT, CLUSTER_ROOT))
w("\tC: \"OO\",%d,%d" % (BONE_TIP, CLUSTER_TIP))
w("\tC: \"OO\",%d,0" % ANIM_STACK)
w("\tC: \"OO\",%d,%d" % (ANIM_LAYER, ANIM_STACK))
w("\tC: \"OO\",%d,%d" % (CURVE_NODE, ANIM_LAYER))
w("\tC: \"OP\",%d,%d, \"Lcl Rotation\"" % (CURVE_NODE, BONE_TIP))
w("\tC: \"OP\",%d,%d, \"d|Z\"" % (CURVE_X, CURVE_NODE))
w("}")

open(sys.argv[1], "w").write("\n".join(out) + "\n")
print("wrote", sys.argv[1])
