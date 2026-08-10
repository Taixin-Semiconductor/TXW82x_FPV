#!/bin/bash

if [ -e tmp ]; then
files=$(ls tmp)
for f in $files
do
	ff=$(cat tmp/$f)
	mv -f $ff.bak $ff
done
rm -rf tmp
fi

cp  ./Obj/*.elf project.elf
cp  ./Lst/*.map project.map
cp  ./Obj/*.ihex project.hex



if [ -e parameter.bincfg ]; then
cp -f ./parameter.bincfg parameter.cfg
fi
if [ -e parameter_ui.setcfg ]; then
cp -f ./parameter_ui.setcfg parameter_ui.cfg
fi



SDKTOOLS_EXE=./sdktools.exe
if [ ! -f "$SDKTOOLS_EXE" ]; then
	cp -v ../../../../../tools/sdkTools/sdktools-windows-amd64.exe "$SDKTOOLS_EXE" || exit 1
fi

cp parameter.bincfg parameter.cfg
#[ ! -f loader.bin ] && cp ../../../../sdk/chip/txw81x/loader.bin loader.bin

"$SDKTOOLS_EXE" crc crc_check_corebin.ini
if [ $? -ne 0 ]; then
	echo "!!!txw82xcore code is distroyed!"
	exit $?
else
	"$SDKTOOLS_EXE" binscript BinScript.BinScript
	"$SDKTOOLS_EXE" makecode
	#crc.exe crc.ini
	#BinScript.exe BinScript_Bin2Hex.BinScript
fi
