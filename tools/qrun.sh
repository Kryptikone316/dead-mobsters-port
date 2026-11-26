#!/bin/bash
# qrun.sh <rundir> [args...]: run fxi.mipsel (or $QRUN_BIN, e.g. launcher.mipsel) in <rundir> under
# qemu-user (via the host), env from the caller
RD="$(cd "$1" && pwd)"; shift
: "${SF3500_TC:?set SF3500_TC to the toolchain host directory}"
SYS="$SF3500_TC/mipsel-buildroot-linux-gnu/sysroot"
envs=()
for v in SF3500_KEYSCRIPT SF3500_SHOT_PREFIX SF3500_SHOT_FRAMES SF3500_QUIT_FRAME SF3500_PAUSE_TEST_POLLS SF3500_STAGE; do [ -n "${!v:-}" ] && envs+=("--env=$v=${!v}"); done
if command -v flatpak-spawn >/dev/null 2>&1; then   # inside a Flatpak sandbox, qemu runs on the host
    exec flatpak-spawn --host --directory="$RD" "${envs[@]}" qemu-mipsel-static -L "$SYS" "./${QRUN_BIN:-fxi.mipsel}" "$@"
fi
cd "$RD" && exec qemu-mipsel-static -L "$SYS" "./${QRUN_BIN:-fxi.mipsel}" "$@"
