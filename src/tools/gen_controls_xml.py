#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gen_controls_xml.py  (#53)  — ОФФЛАЙН-генератор файлов настроек управления (цель: уйти от keystrokes.key).

Раскладка (под <config>):
  controls.xml                          — ТОЛЬКО каталог: <function name= label= category=>
  profiles.xml                          — список профайлов + активный
  profiles/default/keyboard.xml         — дефолтные клавиатурные комбинации (из keystrokes.key)
  profiles/default/<GUID>.xml           — кнопки конкретного устройства (GUID), из keystrokes.key
  profiles/default/axismapping.xml      — оси (из бинарного axismapping.dat)

Источники только ЧИТАЮТСЯ: findfunc.cpp, keystrokes.key, axismapping.dat. Сим эти XML только читает.

Запуск:
  python3 gen_controls_xml.py --config /mnt/g/Games/FreeFalcon6/config \
      --findfunc ../sim/siminput/findfunc.cpp \
      --keystrokes /mnt/g/Games/FreeFalcon6/config/keystrokes.key \
      --axismap   /mnt/g/Games/FreeFalcon6/config/axismapping.dat
"""

import argparse
import os
import re
import struct
import sys

# --- BMS-оверрайды меток --------------------------------------------------------------
BMS_LABELS = {
    "SimCMSUp": "CMS Forward", "SimCMSDown": "CMS Aft", "SimCMSLeft": "CMS Left",
    "SimCMSRight": "CMS Right", "SimCMSPress": "CMS Press",
    "SimTMSUp": "TMS Up", "SimTMSDown": "TMS Down", "SimTMSLeft": "TMS Left", "SimTMSRight": "TMS Right",
    "SimDMSUp": "DMS Up", "SimDMSDown": "DMS Down", "SimDMSLeft": "DMS Left", "SimDMSRight": "DMS Right",
    "SimPinkySwitch": "Pinky Switch",
}

USER_FUNC_RE = re.compile(r"USER_FUNCTION\(\s*([A-Za-z_]\w*)\s*\)")
GUID_RE = re.compile(r"GUID=([0-9A-Fa-f]+)")
_COMMS_PREFIX = re.compile(r"^(Wingman|Element|Flight|Radio|AWACS|Tanker)")

# Порядок осей в struct AxisMapping (simio.h) — 23 шт.
AXIS_NAMES = ["Pitch", "Bank", "Yaw", "Throttle", "Throttle2", "BrakeLeft", "BrakeRight",
              "FOV", "PitchTrim", "YawTrim", "BankTrim", "AntElev", "RngKnob", "CursorX",
              "CursorY", "Comm1Vol", "Comm2Vol", "MSLVol", "ThreatVol", "InterComVol",
              "HudBrt", "RetDepr", "Zoom"]


def categorize(name):
    if _COMMS_PREFIX.match(name) or any(k in name for k in ("Comm", "Radio", "AWACS", "Tanker", "Chatter")):
        return "Communications"
    if name.startswith("OTWRadioMenu"):
        return "Communications"
    return ""


def esc(t):
    return (t.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace('"', "&quot;"))


def _num(tok):
    try:
        return int(tok, 16)
    except ValueError:
        return 0


def extract_functions(path):
    names, seen = [], set()
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            if "#define" in line:
                continue
            for m in USER_FUNC_RE.finditer(line.split("//", 1)[0]):
                nm = m.group(1)
                if nm != "a" and nm not in seen:
                    seen.add(nm); names.append(nm)
    return names


def parse_keystrokes(path):
    """label{name->str}, kbd[list], devices{guid->list}, povs[list]."""
    label, kbd, devices, povs = {}, [], {}, []
    if not path or not os.path.isfile(path):
        return label, kbd, devices, povs
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for raw in f:
            s = raw.strip()
            if not s or s[0] in "#;":
                continue
            t = s.split()
            if len(t) < 7:
                continue
            name = t[0]
            buttonId, mouseSide = int(t[1]), int(t[2])
            key2, mod2, key1, mod1 = _num(t[3]), _num(t[4]), _num(t[5]), _num(t[6])
            # keystrokes.key пишет -1 (не назначено) как %#X -> "0XFFFFFFFF". Нормализуем в -1.
            # Оба ключа сохраняем: key2=основная, key1=вторая клавиша аккорда (легаси двухключевые комбо).
            if key2 == 0xFFFFFFFF:
                key2 = -1
            if key1 == 0xFFFFFFFF:
                key1 = -1
            editable = int(t[7]) if len(t) > 7 and re.match(r"^-?\d+$", t[7]) else 1
            q = re.search(r'"([^"]*)"', s)
            if q and name not in label:
                label[name] = q.group(1)
            if key2 == -2:
                g = GUID_RE.search(s)
                if g:
                    devices.setdefault(g.group(1).upper(), []).append(
                        {"function": name, "id": buttonId, "cpbtn": mouseSide})
            elif key2 == -3:
                g = GUID_RE.search(s)
                povs.append({"function": name, "guid": g.group(1).upper() if g else "",
                             "hat": buttonId, "dir": mod2, "cpbtn": mouseSide})
            else:
                kbd.append({"function": name, "k2": key2, "m2": mod2, "k1": key1, "m1": mod1,
                            "cpbtn": buttonId, "editable": editable})
    return label, kbd, devices, povs


def guid_hex(b):
    return "".join("%02X" % x for x in bytearray(b))


def parse_axismapping(path):
    if not path or not os.path.isfile(path):
        return None
    data = open(path, "rb").read()
    need = 4 + 16 + 4 + 256 + len(AXIS_NAMES) * 16
    if len(data) < need:
        return None
    fcd = struct.unpack_from("<i", data, 0)[0]
    tdc = struct.unpack_from("<i", data, 20)[0]
    guids = [guid_hex(data[24 + i * 16: 40 + i * 16]) for i in range(16)]
    axes = []
    base = 280
    for i, nm in enumerate(AXIS_NAMES):
        dev, ax, dz, sat = struct.unpack_from("<iiii", data, base + i * 16)
        axes.append({"name": nm, "device": dev, "axis": ax, "deadzone": dz, "saturation": sat})
    return {"fcd": fcd, "tdc": tdc, "guids": guids, "axes": axes}


def write(path, lines):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description="Generate controls.xml + default profile (#53)")
    ap.add_argument("--findfunc",   default=os.path.join(here, "..", "sim", "siminput", "findfunc.cpp"))
    ap.add_argument("--keystrokes", default="")
    ap.add_argument("--axismap",    default="")
    ap.add_argument("--config",     required=True, help="каталог config игры (куда писать)")
    ap.add_argument("--migrate-devices", action="store_true",
                    help="переносить кнопки устройств в <GUID>.xml (по умолчанию НЕТ)")
    args = ap.parse_args()

    funcs = extract_functions(args.findfunc)
    label, kbd, devices, povs = parse_keystrokes(args.keystrokes)
    axis = parse_axismapping(args.axismap)

    cfg = args.config
    prof = os.path.join(cfg, "profiles", "default")

    # 1) controls.xml — ТОЛЬКО каталог
    cat = ['<?xml version="1.0" encoding="UTF-8"?>', '<controls version="2">', '  <functions>']
    n_bms = n_leg = n_nm = n_cat = 0
    for nm in funcs:
        if nm in BMS_LABELS:
            lbl = BMS_LABELS[nm]; n_bms += 1
        elif nm in label:
            lbl = label[nm]; n_leg += 1
        else:
            lbl = nm; n_nm += 1
        c = categorize(nm)
        if c:
            n_cat += 1
        cat.append('    <function name="%s" label="%s" category="%s"/>' % (esc(nm), esc(lbl), esc(c)))
    cat += ['  </functions>', '</controls>']
    write(os.path.join(cfg, "controls.xml"), cat)

    # 2) profiles.xml
    write(os.path.join(cfg, "profiles.xml"),
          ['<?xml version="1.0" encoding="UTF-8"?>',
           '<profiles active="default">',
           '  <profile name="default" dir="default"/>',
           '</profiles>'])

    # 3) keyboard.xml — дефолтные комбинации. Отрицательные (key=-1 «не назначено», легаси
    # key1=-1 «все клавиши») пишем ДЕСЯТИЧНО, иначе "0x%X" даст 0xFFFFFFFF -> strtol сломается.
    def hx(v):
        return ("0x%X" % v) if v >= 0 else ("%d" % v)

    kb = ['<?xml version="1.0" encoding="UTF-8"?>', '<keyboard version="1">']
    for e in kbd:
        kb.append('  <bind function="%s" k2="%s" m2="%d" k1="%s" m1="%d" cpbtn="%d" editable="%d"/>'
                  % (esc(e["function"]), hx(e["k2"]), e["m2"], hx(e["k1"]), e["m1"],
                     e["cpbtn"], e["editable"]))
    kb.append('</keyboard>')
    write(os.path.join(prof, "keyboard.xml"), kb)

    # 4) <GUID>.xml — кнопки устройств. По умолчанию НЕ мигрируем (чистый кейс назначения с нуля;
    #    save создаст файл при первом бинде). Включить миграцию: --migrate-devices.
    for guid, binds in (devices.items() if args.migrate_devices else []):
        d = ['<?xml version="1.0" encoding="UTF-8"?>', '<device guid="%s" version="1">' % esc(guid)]
        for b in binds:
            d.append('  <button function="%s" id="%d" cpbtn="%d"/>'
                     % (esc(b["function"]), b["id"], b["cpbtn"]))
        # POV с этим GUID
        for p in povs:
            if p["guid"] == guid:
                d.append('  <pov function="%s" hat="%d" dir="%d" cpbtn="%d"/>'
                         % (esc(p["function"]), p["hat"], p["dir"], p["cpbtn"]))
        d.append('</device>')
        write(os.path.join(prof, "%s.xml" % guid), d)

    # 5) axismapping.xml — оси
    if axis:
        ax = ['<?xml version="1.0" encoding="UTF-8"?>',
              '<axismapping version="1" flightControlDevice="%d" totalDeviceCount="%d">'
              % (axis["fcd"], axis["tdc"]), '  <deviceGuids>']
        for i, g in enumerate(axis["guids"]):
            if g != "00000000000000000000000000000000":
                ax.append('    <guid index="%d" value="%s"/>' % (i, g))
        ax.append('  </deviceGuids>')
        ax.append('  <axes>')
        for a in axis["axes"]:
            ax.append('    <axis name="%s" device="%d" axis="%d" deadzone="%d" saturation="%d"/>'
                      % (a["name"], a["device"], a["axis"], a["deadzone"], a["saturation"]))
        ax.append('  </axes>')
        ax.append('</axismapping>')
        write(os.path.join(prof, "axismapping.xml"), ax)

    print("OK config=%s" % cfg)
    print("  controls.xml: %d функций (BMS %d, legacy %d, по имени %d; Communications %d)"
          % (len(funcs), n_bms, n_leg, n_nm, n_cat))
    print("  profiles/default: keyboard %d, устройств %d (%s), POV %d, оси %s"
          % (len(kbd), len(devices), ",".join(devices.keys()) or "-", len(povs),
             "да" if axis else "нет(.dat не найден)"))


if __name__ == "__main__":
    sys.exit(main())
