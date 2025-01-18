#!/bin/bash
set -e

# Resolve the script directory
SOURCE=${BASH_SOURCE[0]}
while [ -L "$SOURCE" ]; do
  DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
  SOURCE=$(readlink "$SOURCE")
  [[ $SOURCE != /* ]] && SOURCE=$DIR/$SOURCE
done
SCRIPT_DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )

# Versioning
VERSION_FILE="$SCRIPT_DIR/../VERSION"
if [ -f "$VERSION_FILE" ]; then
  VERSION=$(cat "$VERSION_FILE")
else
  VERSION=$(git describe --tags --always 2>/dev/null || echo "unknown")
fi
if [ -z "$VERSION" ]; then
  echo "Could not determine version. Please ensure a VERSION file or Git tag exists."
  exit 1
fi

BUILD_DIR="build"
DIST_DIR="wind-dist-$VERSION"
WRT_DIR="$DIST_DIR/wrt"
STD_DIR="$DIST_DIR/std"

echo "Version: $VERSION"
echo "Creating distribution directories..."
mkdir -p "$WRT_DIR"
mkdir -p "$STD_DIR"

echo "Copying runtime files..."
cp -r $SCRIPT_DIR/../src/runtime/* "$WRT_DIR/"
echo "Copying standard library files..."
cp -r $SCRIPT_DIR/../src/std/* "$STD_DIR/"

echo "Configuring the build with CMake..."
cmake -B "$BUILD_DIR" -S $SCRIPT_DIR/.. \
    -DWIND_STD_PATH="std" \
    -DWIND_RUNTIME_PATH="wrt" \
    -DWIND_BUILD_RUNTIME=OFF

echo "Building the compiler..."
cmake --build "$BUILD_DIR"

echo "Organizing distribution..."
cp "$BUILD_DIR/windc" "$DIST_DIR/windc"

echo "Compiling runtime files with windc..."
"$DIST_DIR/windc" \
    "$WRT_DIR/handler.w" \
    "$WRT_DIR/stack.w" \
    "$WRT_DIR/start.w" \
    -ej -o "$WRT_DIR/wrt.o"

echo "Packaging distribution..."
tar -czf "windc-dist-$VERSION.tar.gz" "$DIST_DIR"

echo "Build completed successfully! Distribution is in windc-dist-$VERSION.tar.gz"
