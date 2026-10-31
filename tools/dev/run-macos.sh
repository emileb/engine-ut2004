#!/bin/sh
# Runs the macOS build in a UT2004 folder: run-macos.sh <game folder> [gl|gl4es] [engine arguments...]
# gl4es draws through ANGLE: ANGLE_DIR names the folder holding libEGL.dylib and libGLESv2.dylib.
set -e
GAME="$1"; shift || true
RENDERER="${1:-gl}"; [ $# -gt 0 ] && shift
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="${BUILD:-$REPO/build-macos}"
SYS="$GAME/System"
[ -d "$SYS" ] || { echo "no System folder in $GAME" >&2; exit 1; }

ln -sf "$BUILD/Source/SDLLaunch/ut2004.bin" "$SYS/ut2004.bin"
ln -sf "$BUILD/Source/SDLLaunch/libopenal.so" "$SYS/libopenal.so"
ln -sf "$BUILD/Source/UCC/ucc-bin" "$SYS/ucc-bin"
# The port's boot inis replace the retail Windows ones; UT2004.ini and User.ini are derived from them.
for f in Default.ini DefUser.ini; do
	cmp -s "$REPO/Engine/Config/$f" "$SYS/$f" || cp "$REPO/Engine/Config/$f" "$SYS/$f"
done

case "$RENDERER" in
gl)    SWITCH=-OPENGLRENDERER ;;
gl4es)
	[ -f "$ANGLE_DIR/libEGL.dylib" ] || { echo "set ANGLE_DIR to the folder with libEGL.dylib and libGLESv2.dylib" >&2; exit 1; }
	export SDL_VIDEO_EGL_DRIVER="$ANGLE_DIR/libEGL.dylib" SDL_VIDEO_GL_DRIVER="$ANGLE_DIR/libGLESv2.dylib"
	SWITCH=-GL4ESRENDERER ;;
*)     echo "renderer is gl or gl4es" >&2; exit 1 ;;
esac

cd "$SYS"
# No CPU-speed probe off x86 (as on Android): CPUSPEED is only reported.
exec ./ut2004.bin CPUSPEED=3000 $SWITCH "$@"
