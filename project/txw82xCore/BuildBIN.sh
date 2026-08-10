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

"$SDKTOOLS_EXE" binscript BinScript.BinScript
"$SDKTOOLS_EXE" makecode
"$SDKTOOLS_EXE" crc
cp -fv ./txw82xcore_crc.bin ../txw82xApp/txw82xcore.bin

#crc.exe crc.ini
#BinScript.exe BinScript_Bin2Hex.BinScript

####################################################################################
#动态调整App工程的ld文件
cp -fv ../txw82xApp/utilities/gcc_csky.ld.i ../txw82xApp/utilities/gcc_csky.ld.n
string=$(ls -l|grep txw82xcore_crc.bin| awk '{print $5}')
var=$(printf "%x" "$string")
sed -i "s/CORECODE_SIZE/0x${var}/g" ../txw82xApp/utilities/gcc_csky.ld.n

string=`grep _bss_end Lst/txw82xCore.map`
for var in ${string[@]}
do
	if [[ $var == *"0x200"* ]]; then
		sed -i "s/COREBSS_END/${var}/g" ../txw82xApp/utilities/gcc_csky.ld.n
	fi
done

cp -fv ../txw82xApp/utilities/gcc_csky.ld.n ../txw82xApp/utilities/gcc_csky.ld
###################################################################################

