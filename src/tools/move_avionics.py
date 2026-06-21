#!/usr/bin/env python3
# #53 split the axis list into two tabs: move the lower axes (Radar Ant Elev and below) from
# the FLIGHT CONTROLS tab (cluster 10002) into the restored AVIONICS tab (cluster 10003) and
# renumber them from y=130 so each tab fits the grey panel without scrolling.
import re, sys

PATH = sys.argv[1] if len(sys.argv) > 1 else \
    "/mnt/g/Games/FreeFalcon6/art/setup/controladvanced.scf"

# (start, end inclusive, baseY) per avionics row, in display order.
ROWS = [
    (1293, 1391, 130),  # Radar Ant Elev
    (1392, 1491, 180),  # Cursor X
    (1492, 1591, 230),  # Cursor Y
    (1592, 1691, 280),  # Range knob
    (491, 566, 330),    # Field of View
    (568, 643, 380),    # View Zoom
    (1692, 1768, 430),  # HUD brightness
    (1771, 1847, 480),  # Reticle depression
]

def yoff(x):
    if x == 450: return 5
    if x == 451: return 6
    return 21

with open(PATH, "r", encoding="ascii") as f:
    lines = f.readlines()

re_xy      = re.compile(r"^\[XY\]\s+(\d+)\s+(\d+)(.*)$")
re_xywh    = re.compile(r"^\[XYWH\]\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)(.*)$")
re_rev     = re.compile(r"^(\[SETUP\] SETUP_ADVANCED_REVERSE_\w+, C_TYPE_TOGGLE )(\d+) (\d+)(.*)$")
re_cluster = re.compile(r"^\[CLUSTER\]\s+10002\s*$")

for start, end, baseY in ROWS:
    for i in range(start - 1, end):
        ln = lines[i].rstrip("\n")
        if re_cluster.match(ln):
            lines[i] = "[CLUSTER] 10003\n"
            continue
        m = re_xy.match(ln)
        if m:
            lines[i] = "[XY] %s %d%s\n" % (m.group(1), baseY, m.group(3))
            continue
        m = re_xywh.match(ln)
        if m:
            x = int(m.group(1))
            lines[i] = "[XYWH] %d %d %s %s%s\n" % (x, baseY + yoff(x), m.group(3), m.group(4), m.group(5))
            continue
        m = re_rev.match(ln)
        if m:
            lines[i] = "%s%s %d%s\n" % (m.group(1), m.group(2), baseY + 21, m.group(4))
            continue

with open(PATH, "w", encoding="ascii") as f:
    f.writelines(lines)

print("moved %d avionics rows to cluster 10003" % len(ROWS))
