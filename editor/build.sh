#!/usr/bin/env bash
set -e

# -------------------------------------------------
# Directories
# -------------------------------------------------
SOURCE_DIR="src"
BUILD_DIR="build"
OUT_DIR="bin"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

CXX="clang++"
EXE_NAME="editor"

# -------------------------------------------------
# Includes / Libs / Defines
# -------------------------------------------------
INCLUDE_DIRS=(
    "-Isrc"
    "-I../engine/src"
    "-I../engine/src/vendor"
    "-I../engine/src/hkstl"
)

LIBS=(
    "-L../engine/bin"
    "-lhikai"
)

DEFINES=(
    "-D HKDEBUG"
)

COMPILER_FLAGS=(
    "-Wall"
    "-Wextra"
    "-std=c++17" # Specifies c++ version
    "-g"
)

# Linker flags
LINKER_FLAGS="-Wl,-rpath,"$PWD/../engine/bin""

# Create output dirs
mkdir -p "$BUILD_DIR"
mkdir -p "$OUT_DIR"

# Compile sources (incremental)
OBJ_FILES=()

while IFS= read -r -d '' source; do
    obj="$BUILD_DIR/$(basename "${source%.cpp}.o")"
    OBJ_FILES+=("$obj")

    if [[ ! -f "$obj" ]]; then
        echo "Compiling $source"
        "$CXX" "${DEFINES[@]}" "${COMPILER_FLAGS[@]}" \
              "${INCLUDE_DIRS[@]}" \
              -c "$source" -o "$obj"
        continue
    fi

    src_time=$(stat -c %Y "$source" 2>/dev/null || stat -f %m "$source")
    obj_time=$(stat -c %Y "$obj"    2>/dev/null || stat -f %m "$obj")

    if (( src_time > obj_time )); then
        echo "Recompiling $source"
        "$CXX" "${DEFINES[@]}" "${COMPILER_FLAGS[@]}" \
              "${INCLUDE_DIRS[@]}" \
              -c "$source" -o "$obj"
    fi
done < <(find "$SOURCE_DIR" -name "*.cpp" -print0)

# Link
echo "Linking $EXE_NAME"

"$CXX" -g $LINKER_FLAGS -o "$OUT_DIR/$EXE_NAME" \
    "${OBJ_FILES[@]}" "${LIBS[@]}"

echo "Build successful"
