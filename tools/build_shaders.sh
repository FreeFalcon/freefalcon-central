#!/bin/sh
# Artscout - 2026: compile the ffshaders.manifest list to embeddable C++ headers.
#
#   build_shaders.sh <shader_dir> <out_dir> [dxc]
#
# DXC emits the bytecode as a C array itself (-Fh/-Vn), so there is no bin2c step
# and no generated .cpp to keep in sync -- ffshaderblobs.cpp just includes the
# headers. Same manifest drives the Windows build (tools/build_shaders.bat).
set -e

SRC_DIR="${1:?usage: build_shaders.sh <shader_dir> <out_dir> [dxc]}"
OUT_DIR="${2:?usage: build_shaders.sh <shader_dir> <out_dir> [dxc]}"
DXC="${3:-}"

if [ -z "$DXC" ]; then
    if [ -n "$VULKAN_SDK" ] && [ -x "$VULKAN_SDK/bin/dxc" ]; then
        DXC="$VULKAN_SDK/bin/dxc"
    else
        DXC="$(command -v dxc || true)"
    fi
fi

if [ -z "$DXC" ] || [ ! -x "$DXC" ]; then
    echo "build_shaders: dxc not found (install the Vulkan SDK or DirectXShaderCompiler)" >&2
    exit 1
fi

MANIFEST="$SRC_DIR/ffshaders.manifest"
mkdir -p "$OUT_DIR"

# Strip comments/blank lines, then split on '|' and trim each field.
sed -e 's/#.*$//' -e '/^[[:space:]]*$/d' "$MANIFEST" |
while IFS='|' read -r src entry profile target defines var
do
    src=$(printf '%s' "$src" | tr -d ' \r')
    entry=$(printf '%s' "$entry" | tr -d ' \r')
    profile=$(printf '%s' "$profile" | tr -d ' \r')
    target=$(printf '%s' "$target" | tr -d ' \r')
    defines=$(printf '%s' "$defines" | tr -d ' \r')
    var=$(printf '%s' "$var" | tr -d ' \r')
    [ -n "$src" ] || continue

    TARGET_FLAGS=""
    if [ "$target" = "spirv" ]; then
        # vulkan1.2 == SPIR-V 1.5, matching the apiVersion VulkanBackend asks
        # for. A 1.3 target emits SPIR-V 1.6, which a 1.2 device rejects.
        TARGET_FLAGS="-spirv -fspv-target-env=vulkan1.2"
    fi

    DEFINE_FLAGS=""
    if [ -n "$defines" ] && [ "$defines" != "-" ]; then
        DEFINE_FLAGS=$(printf '%s' "$defines" | tr ',' ' ' |
                       sed 's/\([^ ][^ ]*\)/-D \1=1/g')
    fi

    echo "dxc $src [$entry/$profile/$target] -> $var.h"
    # Unquoted on purpose: both hold whole flags that must word-split.
    "$DXC" -T "$profile" -E "$entry" -Fh "$OUT_DIR/$var.h" -Vn "$var" \
           $TARGET_FLAGS $DEFINE_FLAGS "$SRC_DIR/$src"
done
