#!/bin/bash
SDKTOOLS_EXE=./sdktools.exe
if [ ! -f "$SDKTOOLS_EXE" ]; then
	cp -v ../../../../../tools/sdkTools/sdktools-windows-amd64.exe "$SDKTOOLS_EXE" || exit 1
fi
mkdir -p tmp
"$SDKTOOLS_EXE" precompile ${ProjectPath} "$1"
exit 0
