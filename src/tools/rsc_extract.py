#!/usr/bin/env python3
# Extracts images from a Falcon4/FreeFalcon ui95 resource bank (.idx + .rsc) to PNG.
#
# Format (from ui95/cresmgr.cpp + imagersc.h):
#   .idx: int32 size; int32 version; then `size` bytes of records.
#         each image record = ImageHeader (60 bytes):
#           int32 Type(=100); char ID[32]; int32 flags;
#           int16 centerx, centery, w, h; int32 imageoffset; int32 palettesize; int32 paletteoffset
#   .rsc: int32 size; int32 version; then the Data_ blob.
#         => file offset of any Data_ offset O is (8 + O).
#   Colors are RGB555 (r<<10 | g<<5 | b), 5 bits each (ConvertToScreen early-outs for 10/5/0).
#   flags: 0x01 = 8-bit (palettesize WORD palette @ paletteoffset; pixels = w*h byte indices)
#          0x02 = 16-bit (pixels = w*h WORDs @ imageoffset)
#          0x40000000 = use color key (index 0 / first palette entry treated as transparent)
#
# Usage: python rsc_extract.py <bank_path_without_ext> <out_dir> [NAME1 NAME2 ...]
#   e.g. python rsc_extract.py "G:/Games/FreeFalcon6/art/uiskin/ff4/win_all" out WIN_SETUP WIN_CTLADV
#   no NAMEs => extract everything.
import sys, os, struct
from PIL import Image

RSC_8BIT = 0x01
RSC_16BIT = 0x02
RSC_COLORKEY = 0x40000000

def c555(w):
    r = (w >> 10) & 0x1F
    g = (w >> 5) & 0x1F
    b = w & 0x1F
    return ((r * 255) // 31, (g * 255) // 31, (b * 255) // 31)

def main():
    base, outdir = sys.argv[1], sys.argv[2]
    wanted = set(n.upper() for n in sys.argv[3:])
    os.makedirs(outdir, exist_ok=True)

    with open(base + ".idx", "rb") as f:
        idx = f.read()
    with open(base + ".rsc", "rb") as f:
        rsc = f.read()

    isize, iver = struct.unpack_from("<ii", idx, 0)
    p = 8
    end = 8 + isize
    count = 0
    while p + 60 <= end:
        typ = struct.unpack_from("<i", idx, p)[0]
        if typ != 100:
            print(f"non-image record type {typ} at {p}; stopping")
            break
        name = idx[p+4:p+36].split(b"\x00")[0].decode("latin1")
        flags, = struct.unpack_from("<i", idx, p+36)
        # field offsets: centerx@40 centery@42 w@44 h@46
        cx, cy, w, h = struct.unpack_from("<hhhh", idx, p+40)
        imgoff, palsize, paloff = struct.unpack_from("<iii", idx, p+48)
        p += 60

        if wanted and name.upper() not in wanted:
            continue
        if w <= 0 or h <= 0:
            print(f"skip {name}: w={w} h={h}")
            continue

        img = Image.new("RGBA", (w, h))
        px = img.load()
        base_off = 8
        if flags & RSC_8BIT:
            pal_fo = base_off + paloff
            pal = [c555(struct.unpack_from("<H", rsc, pal_fo + 2*i)[0]) for i in range(palsize)]
            data_fo = base_off + imgoff
            for y in range(h):
                for x in range(w):
                    idxv = rsc[data_fo + y*w + x]
                    r, g, b = pal[idxv] if idxv < len(pal) else (0, 0, 0)
                    a = 0 if (flags & RSC_COLORKEY and idxv == 0) else 255
                    px[x, y] = (r, g, b, a)
        elif flags & RSC_16BIT:
            data_fo = base_off + imgoff
            for y in range(h):
                for x in range(w):
                    w16 = struct.unpack_from("<H", rsc, data_fo + 2*(y*w + x))[0]
                    r, g, b = c555(w16)
                    a = 0 if (flags & RSC_COLORKEY and w16 == 0) else 255
                    px[x, y] = (r, g, b, a)
        else:
            print(f"skip {name}: unknown depth flags={flags:#x}")
            continue

        safe = "".join(ch if ch.isalnum() or ch in "_-" else "_" for ch in name)
        img.save(os.path.join(outdir, safe + ".png"))
        count += 1
        print(f"{name}  {w}x{h}  flags={flags:#x}")

    print(f"done: {count} image(s) -> {outdir}")

if __name__ == "__main__":
    main()
