#!/bin/bash
[ ! -f precompile.exe ] && cp -v ../../../../tools/precompile/Debug/precompile.exe precompile.exe
mkdir -p tmp
precompile.exe ${ProjectPath} $1
exit 0
