#!/usr/bin/env bash
# Artscout - 2026 (#104, Linux Ф1): error-driven compile harness. Compiles project .cpp files against the Win32
# shim with clang to drive shim growth. Usage: ./linux-compile-check.sh <file.cpp> [more.cpp ...]
# Run from src/ (or pass absolute paths). Not a build system -- a per-file syntax check for the Ф1 shim loop.
set -u
R="$(cd "$(dirname "$0")/.." && pwd)"     # -> src/
INCS=(
  -I "$R/platform/win32shim"   # the shim FIRST so <windows.h>/<io.h>/<tchar.h>/<mmsystem.h>/<dinput.h> resolve to it
  -I "$R" -I "$R/include"
  -I "$R/graphics/include" -I "$R/graphics"
  -I "$R/campaign/include" -I "$R/codelib/include" -I "$R/falclib/include"
  -I "$R/sim/include" -I "$R/ui/include"
  -I "$R/ui95" -I "$R/ui95_ext" -I "$R/vu2/include" -I "$R/tools/lists"
)
# -fms-extensions + -fms-compatibility: accept the MSVC-isms the codebase leans on (forward enum, __declspec, etc.)
# The GCC_ATOMIC -D block re-injects the macros that -fms-compatibility drops but libstdc++'s <atomic> needs
# (clang -dM without ms-compat prints them; all 2 except TEST_AND_SET=1). Without these, atomic_base.h fails to
# compile. (An alternative is -stdlib=libc++, but that needs libc++-dev; the -D route keeps libstdc++.)
FLAGS=(-std=c++17 -fsyntax-only -fms-extensions -fms-compatibility -fms-compatibility-version=19.20 -w -Wno-register -Wno-writable-strings
  -D__GCC_ATOMIC_BOOL_LOCK_FREE=2 -D__GCC_ATOMIC_CHAR_LOCK_FREE=2 -D__GCC_ATOMIC_CHAR16_T_LOCK_FREE=2
  -D__GCC_ATOMIC_CHAR32_T_LOCK_FREE=2 -D__GCC_ATOMIC_WCHAR_T_LOCK_FREE=2 -D__GCC_ATOMIC_SHORT_LOCK_FREE=2
  -D__GCC_ATOMIC_INT_LOCK_FREE=2 -D__GCC_ATOMIC_LONG_LOCK_FREE=2 -D__GCC_ATOMIC_LLONG_LOCK_FREE=2
  -D__GCC_ATOMIC_POINTER_LOCK_FREE=2 -D__GCC_ATOMIC_TEST_AND_SET_TRUEVAL=1)

pass=0; fail=0
for f in "$@"; do
  [ -f "$f" ] || { echo "  MISS $f"; continue; }
  err=$(clang++ "${FLAGS[@]}" "${INCS[@]}" "$f" 2>&1 | grep -m1 'error:')
  if [ -z "$err" ]; then pass=$((pass+1)); echo "  OK   $f";
  else fail=$((fail+1)); echo "  FAIL $f :: $(echo "$err" | sed 's#.*/##' | cut -c1-80)"; fi
done
echo "--- pass=$pass fail=$fail ---"
