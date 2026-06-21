#!/usr/bin/env python3
# #53 one-shot helper: renumber the AXIS SETUP rows in controladvanced.scf so each axis
# occupies one clean 50px row, fitting the grey panel with vertical scroll for the overflow.
# Y is derived from the control's X column (deterministic), which also fixes the reverse
# toggles that had drifted to wrong Y values. Run once; safe to re-run (idempotent).
import re, sys

PATH = sys.argv[1] if len(sys.argv) > 1 else \
    "/mnt/g/Games/FreeFalcon6/art/setup/controladvanced.scf"

# (1-based start, end inclusive, baseY=label row Y). Ranges are the per-axis row blocks.
ROWS = [
    (795, 893, 130),    # Rudder
    (895, 959, 180),    # Throttle
    (962, 1026, 230),   # Throttle2
    (1106, 1196, 280),  # Pitch (Pitch Deadzone + Pitch Axis)
    (1197, 1287, 330),  # Bank  (Bank Deadzone + Bank Axis)
    (1029, 1104, 380),  # Brake
    (1325, 1423, 430),  # Ant Elev
    (1424, 1523, 480),  # Cursor X
    (1524, 1623, 530),  # Cursor Y
    (1624, 1723, 580),  # Range knob
    (491, 564, 630),    # FOV
    (568, 641, 680),    # Zoom
    (1724, 1802, 730),  # HUD brightness
    (1803, 1882, 780),  # Reticle depression
]

def yoff(x):
    if x == 450: return 5    # value box
    if x == 451: return 6    # value line
    return 21                # bg line / listbox / reverse button

with open(PATH, "r", encoding="ascii") as f:
    lines = f.readlines()

re_xy   = re.compile(r"^\[XY\]\s+(\d+)\s+(\d+)(.*)$")
re_xywh = re.compile(r"^\[XYWH\]\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)(.*)$")
re_rev  = re.compile(r"^(\[SETUP\] SETUP_ADVANCED_REVERSE_\w+, C_TYPE_TOGGLE )(\d+) (\d+)(.*)$")

for start, end, baseY in ROWS:
    for i in range(start - 1, end):
        ln = lines[i].rstrip("\n")
        m = re_xy.match(ln)
        if m:                                   # label text -> baseY
            lines[i] = "[XY] %s %d%s\n" % (m.group(1), baseY, m.group(3))
            continue
        m = re_xywh.match(ln)
        if m:
            x = int(m.group(1))
            lines[i] = "[XYWH] %d %d %s %s%s\n" % (x, baseY + yoff(x), m.group(3), m.group(4), m.group(5))
            continue
        m = re_rev.match(ln)
        if m:                                   # reverse toggle -> 790 baseY+21
            lines[i] = "%s%s %d%s\n" % (m.group(1), m.group(2), baseY + 21, m.group(4))
            continue

with open(PATH, "w", encoding="ascii") as f:
    f.writelines(lines)

print("renumbered %d axis rows" % len(ROWS))
