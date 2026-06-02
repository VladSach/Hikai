#!/usr/bin/env bash
set -euo pipefail

SOURCE_DIR="src"
BUILD_DIR="build"
OUT_DIR="bin"
LIB_DIR="lib/Linux"

INCLUDE_DIRS="-Isrc -Isrc/vendor -Isrc/hkstl"
DEFINES="-DHKDEBUG -DHKDLL_OUT"
LIBS=(
    "-ldxcompiler"
    "-lassimp"
    "-lvulkan"

    "-lX11"
    "-lXrandr"
)

# SONAME
DLL_NAME="libhikai.so"

EXCLUDED_FILES=(
    "$SOURCE_DIR/vendor/imgui/imgui_impl_win32.h"
    "$SOURCE_DIR/vendor/imgui/imgui_impl_win32.cpp"

    "$SOURCE_DIR/platform/console/win_console.cpp"
    "$SOURCE_DIR/platform/debug/win_debug.cpp"
    "$SOURCE_DIR/platform/filesystem/win_filesystem.cpp"
    "$SOURCE_DIR/platform/window/win_window.cpp"
    "$SOURCE_DIR/platform/specs/win_spec.cpp"
)

# Compiler flags
COMPILER_FLAGS="-std=c++17 -Wall -Wextra -g -fPIC"
COMPILER="clang++"

# Linker flags
LINKER_FLAGS="-Wl,-rpath,"$PWD/$LIB_DIR""

mkdir -p "$BUILD_DIR" "$OUT_DIR"

needs_recompile() {
    src="$1"
    obj="$2"

    # needs compile
    if [[ ! -f "$obj" ]]; then
        return 0
    fi

    # newer source
    if [[ "$src" -nt "$obj" ]]; then
        return 0
    fi

    return 1
}

# Compile each .cpp file
while IFS= read -r -d '' src; do
    obj="$BUILD_DIR/$(basename "${src%.*}").o"

    if needs_recompile "$src" "$obj"; then
        echo "Compiling $src"
        $COMPILER $DEFINES $COMPILER_FLAGS $INCLUDE_DIRS -c "$src" -o "$obj"
    fi
done < <(find "$SOURCE_DIR" -type f -name "*.cpp" \
        $(printf '! -path %s ' ${EXCLUDED_FILES[@]}) \
        -print0)

# Link .o files
echo "Linking $DLL_NAME"
$COMPILER -shared $LINKER_FLAGS "$BUILD_DIR"/*.o \
          -L"$LIB_DIR" ${LIBS[@]} -o "$OUT_DIR/$DLL_NAME"
