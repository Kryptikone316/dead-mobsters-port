#!/bin/bash
# SPDX-License-Identifier: Unlicense
# Builds fxi.mipsel, the Fenix 0.80 interpreter for the SF3500 (MIPS32r2, little endian), and the port's
# launcher.mipsel into build-sf3500/.
#
# Needs the SF3500 MIPS toolchain, cmake, make and three upstream tarballs in $DEPS (see README.md):
#
#   SF3500_TC=/path/to/toolchain/host  ./build_sf3500.sh
#
# Environment:
#   SF3500_TC   toolchain "host" directory: bin/mips-mti-linux-gnu-gcc and mipsel-buildroot-linux-gnu/sysroot (required)
#   DEPS        folder with SDL2-2.0.14.tar.gz, SDL2_mixer-2.6.3.tar.gz, libxmp-lite-4.6.0.tar.gz (default ./deps)
#   OUT         build folder (default ./build-sf3500)
set -eu

HERE="$(cd "$(dirname "$0")" && pwd)"
: "${SF3500_TC:?set SF3500_TC to the toolchain host directory}"
DEPS="${DEPS:-$HERE/deps}"
OUT="${OUT:-$HERE/build-sf3500}"
JOBS="${JOBS:-$(nproc)}"
export SF3500_TC
SYSROOT="$SF3500_TC/mipsel-buildroot-linux-gnu/sysroot"
LIBDIR="$SYSROOT/mipsel-r2-hard/usr/lib"
CC="$SF3500_TC/bin/mips-mti-linux-gnu-gcc"
STRIP="$SF3500_TC/bin/mips-mti-linux-gnu-strip"
TOOLCHAIN="$HERE/sf3500/cmake/sf3500-toolchain.cmake"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"

die() { echo "build_sf3500.sh: $*" >&2; exit 1; }
[ -x "$CC" ] || die "no mips-mti-linux-gnu-gcc in $SF3500_TC/bin"

# Checks an upstream tarball against its known checksum.
need_tar() {
    local file="$DEPS/$1" sum="$2"
    [ -f "$file" ] || die "missing $file (see README.md)"
    echo "$sum  $file" | sha256sum -c --quiet - || die "$1 does not match the expected checksum"
}
need_tar SDL2-2.0.14.tar.gz       d8215b571a581be1332d2106f8036fcb03d12a70bae01e20f424976d275432bc
need_tar SDL2_mixer-2.6.3.tar.gz  7a6ba86a478648ce617e3a5e9277181bc67f7ce9876605eea6affd4a0d6eea8f
need_tar libxmp-lite-4.6.0.tar.gz 71a93eb0119824bcc56eca95db154d1cdf30304b33d89a4732de6fef8a2c6f38

SDL="$OUT/prefix-sdl2"; XMP="$OUT/prefix-xmp"; MIX="$OUT/prefix-mixer"

# 1. SDL2 with the SF3500 patch (audio pacing for the dummy device, divide-by-zero guard), static.
#    Only its audio core is used: video and input are done by shim/fxsdl.c.
if [ ! -f "$SDL/lib/libSDL2.a" ]; then
    echo "== SDL2"
    rm -rf "$OUT/sdl2-src" "$OUT/sdl2-build"; mkdir -p "$OUT/sdl2-src" "$OUT/sdl2-build"
    tar xzf "$DEPS/SDL2-2.0.14.tar.gz" -C "$OUT/sdl2-src" --strip-components=1
    (cd "$OUT/sdl2-src" && patch -p1 -s < "$HERE/sf3500/patches/sdl2-2.0.14-sf3500.patch")
    (cd "$OUT/sdl2-build" && cmake "$OUT/sdl2-src" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" -DCMAKE_INSTALL_PREFIX="$SDL" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_STATIC_PIC=OFF -DSDL_TEST=OFF \
        -DSDL_HAPTIC=OFF -DSDL_POWER=OFF -DSDL_SENSOR=OFF -DPKG_CONFIG_EXECUTABLE=/nonexistent/pkg-config \
        -DVIDEO_KMSDRM=OFF -DVIDEO_KMSDRM_SHARED=OFF -DALSA=OFF -DPULSEAUDIO=OFF -DJACK=OFF \
        -DESD=OFF -DARTS=OFF -DNAS=OFF -DSNDIO=OFF -DVIDEO_WAYLAND=OFF -DVIDEO_X11=OFF \
        -DVIDEO_OPENGL=OFF -DVIDEO_OPENGLES=OFF -DVIDEO_VULKAN=OFF -DVIDEO_RPI=OFF \
        -DVIDEO_VIVANTE=OFF -DVIDEO_DIRECTFB=OFF > cmake.log 2>&1 && make -j"$JOBS" > make.log 2>&1 && make install > install.log 2>&1) \
        || die "SDL2 build failed, see $OUT/sdl2-build/*.log"
fi

# 2. libxmp-lite (SDL2_mixer's module support; the game itself has only WAV sounds), static. Its installed CMake config has a typo that stops it being found.
if [ ! -f "$XMP/lib/libxmp-lite.a" ]; then
    echo "== libxmp-lite"
    rm -rf "$OUT/xmp-src" "$OUT/xmp-build"; mkdir -p "$OUT/xmp-src" "$OUT/xmp-build"
    tar xzf "$DEPS/libxmp-lite-4.6.0.tar.gz" -C "$OUT/xmp-src" --strip-components=1
    (cd "$OUT/xmp-build" && cmake "$OUT/xmp-src" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" -DCMAKE_INSTALL_PREFIX="$XMP" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DBUILD_SHARED=OFF -DCMAKE_BUILD_TYPE=Release > cmake.log 2>&1 \
        && make -j"$JOBS" > make.log 2>&1 && make install > install.log 2>&1) || die "libxmp-lite build failed, see $OUT/xmp-build/*.log"
    sed -i 's/CMAKE_CURRENT_LISTDIR/CMAKE_CURRENT_LIST_DIR/g' "$XMP/lib/cmake/libxmp-lite/libxmp-lite-config.cmake"
fi

# 3. SDL2_mixer with libxmp-lite for the XM music.
if [ ! -f "$MIX/lib/libSDL2_mixer.a" ]; then
    echo "== SDL2_mixer"
    rm -rf "$OUT/mixer-src" "$OUT/mixer-build"; mkdir -p "$OUT/mixer-src" "$OUT/mixer-build"
    tar xzf "$DEPS/SDL2_mixer-2.6.3.tar.gz" -C "$OUT/mixer-src" --strip-components=1
    (cd "$OUT/mixer-src" && patch -p1 -s < "$HERE/sf3500/patches/sdl2-mixer-2.6.3-xmp-lite.patch")
    (cd "$OUT/mixer-build" && cmake "$OUT/mixer-src" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" -DCMAKE_INSTALL_PREFIX="$MIX" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON -Dlibxmp-lite_DIR="$XMP/lib/cmake/libxmp-lite" \
        -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DSDL2MIXER_VENDORED=ON -DSDL2MIXER_DEPS_SHARED=OFF \
        -DSDL2MIXER_VORBIS=STB -DSDL2MIXER_FLAC=OFF -DSDL2MIXER_MP3=OFF -DSDL2MIXER_MIDI=OFF -DSDL2MIXER_OPUS=OFF \
        -DSDL2MIXER_MOD=ON -DSDL2MIXER_MOD_MODPLUG=OFF -DSDL2MIXER_MOD_XMP=ON -DSDL2MIXER_MOD_XMP_LITE=ON -DSDL2MIXER_MOD_XMP_SHARED=OFF \
        -DSDL2MIXER_CMD=OFF -DSDL2MIXER_WAVE=ON -DSDL2MIXER_SAMPLES=OFF -DSDL2MIXER_INSTALL=ON \
        -DSDL2_DIR="$SDL/lib/cmake/SDL2" -DSDL2_INCLUDE_DIR="$SDL/include/SDL2" -DSDL2_LIBRARY="$SDL/lib/libSDL2.a" > cmake.log 2>&1 \
        && make -j"$JOBS" > make.log 2>&1 && make install > install.log 2>&1) || die "SDL2_mixer build failed, see $OUT/mixer-build/*.log"
fi

# 4. The interpreter. Fixed __DATE__/__TIME__ (the version banner uses them) so the same sources give the same bytes.
echo "== fxi"
mkdir -p "$OUT/obj"
CFLAGS="--sysroot=$SYSROOT -mips32r2 -mtune=74kc -mhard-float -O2 -fno-strict-aliasing -w -DLINUX -DTARGET_linux"
CFLAGS="$CFLAGS -Wno-builtin-macro-redefined -D__DATE__=\"Nov_2026\" -D__TIME__=\"00:00:00\""
FX="$HERE/fenix080"
INC_FXI="-I$HERE/shim -I$FX/fxi/inc -I$FX/include"
objs=""
cd "$HERE"   # sources by relative path: the compiler embeds no checkout path
for f in main dcbr instance strings i_main i_debug i_func g_main g_maps g_blit g_texts g_pal g_draw g_systexts g_console g_scroll g_flic g_mode7 img_png img_pcx sound pathfind fnc_exports; do
    $CC $CFLAGS -std=gnu89 -I shim -I fenix080/fxi/inc -I fenix080/include -c "fenix080/fxi/src/$f.c" -o "$OUT/obj/$f.o"; objs="$objs $OUT/obj/$f.o"
done
for f in files xctype; do
    $CC $CFLAGS -std=gnu89 -I shim -I fenix080/fxi/inc -I fenix080/include -c "fenix080/common/$f.c" -o "$OUT/obj/$f.o"; objs="$objs $OUT/obj/$f.o"
done
# what the port knows about this game's bytecode; built against fxi's own headers
$CC $CFLAGS -std=gnu89 -I shim -I fenix080/fxi/inc -I fenix080/include -c shim/mobsters.c -o "$OUT/obj/mobsters.o"; objs="$objs $OUT/obj/mobsters.o"
# settings shared with the launcher
$CC $CFLAGS -std=gnu11 -I shim -c shim/dm_settings.c -o "$OUT/obj/dm_settings.o"; objs="$objs $OUT/obj/dm_settings.o"
# the shim: the video/input side sees the SDL 1.2 look-alike, the audio side sees real SDL2
$CC $CFLAGS -std=gnu11 -I shim -c shim/fxsdl.c -o "$OUT/obj/fxsdl.o"; objs="$objs $OUT/obj/fxsdl.o"
$CC $CFLAGS -std=gnu11 -I shim -I"$SDL/include" -I"$MIX/include/SDL2" -c shim/fxsdl_audio.c -o "$OUT/obj/fxsdl_audio.o"; objs="$objs $OUT/obj/fxsdl_audio.o"
for f in hwdisp hwaudio joykey; do
    $CC $CFLAGS -std=gnu11 -c "sf3500/$f.c" -o "$OUT/obj/$f.o"; objs="$objs $OUT/obj/$f.o"
done
$CC $CFLAGS -rdynamic -o "$OUT/fxi" $objs \
    "$MIX/lib/libSDL2_mixer.a" "$SDL/lib/libSDL2.a" "$XMP/lib/libxmp-lite.a" \
    "$LIBDIR/libpng.so" "$LIBDIR/libz.so" -lm -ldl -lpthread
"$STRIP" -o "$OUT/fxi.mipsel" "$OUT/fxi"

# 5. The launcher, on the same display, sound and pad layer.
echo "== launcher"
$CC $CFLAGS -std=gnu11 -I shim -I"$SDL/include" -I"$MIX/include/SDL2" -c launcher/launcher.c -o "$OUT/obj/launcher.o"
$CC $CFLAGS -o "$OUT/launcher" "$OUT/obj/launcher.o" "$OUT/obj/dm_settings.o" "$OUT/obj/hwdisp.o" "$OUT/obj/hwaudio.o" "$OUT/obj/joykey.o" \
    "$MIX/lib/libSDL2_mixer.a" "$SDL/lib/libSDL2.a" "$XMP/lib/libxmp-lite.a" -lm -ldl -lpthread
"$STRIP" -o "$OUT/launcher.mipsel" "$OUT/launcher"
echo; echo "Built: $(ls -l "$OUT/fxi.mipsel")"; echo "Built: $(ls -l "$OUT/launcher.mipsel")"
