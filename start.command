#!/bin/bash
# Build and run the Bangkok Mass Transit Route Finder
# Double-click this file on macOS to run

DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"

echo "Building..."
gcc -Wall -o transit main.c data.c graph.c hashtable.c bst.c
if [ $? -ne 0 ]; then
  echo ""
  echo "Build failed. Press Enter to close."
  read
  exit 1
fi

echo "Done. Starting..."
echo ""
./transit

echo ""
echo "Press Enter to close."
read
