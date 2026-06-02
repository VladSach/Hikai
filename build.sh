#!/usr/bin/env bash
set -e

echo "============================"
echo "        Hikai Build"
echo "============================"

echo "Building engine..."
echo "----------------------------"

pushd engine > /dev/null
./build.sh
popd > /dev/null
echo "----------------------------"

echo "Building editor..."
echo "----------------------------"

pushd editor > /dev/null
./build.sh
popd > /dev/null
echo "----------------------------"

mkdir -p bin

echo "Copying editor..."
cp -f editor/bin/editor bin/

echo "============================"
echo "Build successful"
