#!/usr/bin/env python3
# Injects (adds or replaces) a PNG into a Falcon4/FreeFalcon ui95 resource bank
# (.idx + .rsc) under a given image id. Reverse of rsc_extract.py.
#
# Bank format (see ui95/cresmgr.cpp + imagersc.h):
#   .idx: int32 size; int32 version; then `size` bytes of records.
#         image record = ImageHeader (60 bytes):
#           int32 Type(=100); char ID[32]; int32 flags;
#           int16 centerx, centery, w, h; int32 imageoffset; int32 palettesize; int32 paletteoffset
#   .rsc: int32 size; int32 version; then the Data_ blob (size bytes).
#         => a Data_ offset O lives at file offset 8+O. imageoffset/paletteoffset are into Data_.
#   Colors are RGB555 (r<<10 | g<<5 | b). We write the new image as 16-bit (flags=0x02),
#   no palette, so no palette bookkeeping is needed.
#
# We only APPEND new pixel data to the .rsc blob, so existing image offsets stay valid.
# If the id already exists, its record is repointed to the new data (old data is left
# orphaned but harmless). A one-time .bak of each file is made.
#
# Usage: python rsc_pack.py <bank_base> <png> <IMAGE_ID>
#   e.g. python rsc_pack.py "G:/Games/FreeFalcon6/art/uiskin/ff4/win_all" mybg.png WIN_CTLTBL
import sys, os, struct, shutil
from PIL import Image

RSC_16BIT = 0x02
REC = 60

def to555(im):
    im = im.convert("RGB")
    w, h = im.size
    px = im.load()
    out = bytearray(w * h * 2)
    i = 0
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            v = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3)
            struct.pack_into("<H", out, i, v)
            i += 2
    return w, h, bytes(out)

def main():
    base, png, image_id = sys.argv[1], sys.argv[2], sys.argv[3]
    if len(image_id) > 31:
        print("id too long (max 31 chars)"); return

    idx_path, rsc_path = base + ".idx", base + ".rsc"
    for p in (idx_path, rsc_path):
        if not os.path.exists(p + ".bak"):
            shutil.copy2(p, p + ".bak")
            print(f"backup -> {p}.bak")

    with open(idx_path, "rb") as f: idx = bytearray(f.read())
    with open(rsc_path, "rb") as f: rsc = bytearray(f.read())

    isize, iver = struct.unpack_from("<ii", idx, 0)
    dsize, dver = struct.unpack_from("<ii", idx, 0)
    dsize, dver = struct.unpack_from("<ii", rsc, 0)

    w, h, data = to555(Image.open(png))

    # append pixel data to the rsc blob
    new_imgoff = len(rsc) - 8           # offset into Data_ (file len - 8 header)
    rsc += data
    struct.pack_into("<i", rsc, 0, len(rsc) - 8)   # update data size

    # build the new 60-byte record
    rec = bytearray(REC)
    struct.pack_into("<i", rec, 0, 100)                       # Type = image
    idb = image_id.encode("latin1")[:31]
    rec[4:4+len(idb)] = idb                                   # ID[32] (zero padded)
    struct.pack_into("<i", rec, 36, RSC_16BIT)               # flags
    struct.pack_into("<hhhh", rec, 40, 0, 0, w, h)           # centerx, centery, w, h
    struct.pack_into("<iii", rec, 48, new_imgoff, 0, 0)      # imageoffset, palettesize, paletteoffset

    # find existing record with this id (to repoint) else append
    p, end = 8, 8 + isize
    found = -1
    while p + REC <= end:
        if struct.unpack_from("<i", idx, p)[0] != 100:
            break
        nm = idx[p+4:p+36].split(b"\x00")[0].decode("latin1")
        if nm == image_id:
            found = p
            break
        p += REC

    if found >= 0:
        idx[found:found+REC] = rec
        print(f"replaced existing id {image_id} @ idx {found}")
    else:
        # insert before the trailing bytes: append at end of records section
        ins = 8 + isize
        idx[ins:ins] = rec
        struct.pack_into("<i", idx, 0, isize + REC)
        print(f"appended new id {image_id}")

    with open(idx_path, "wb") as f: f.write(idx)
    with open(rsc_path, "wb") as f: f.write(rsc)
    print(f"OK: {image_id}  {w}x{h}  imageoffset={new_imgoff}")

if __name__ == "__main__":
    main()
