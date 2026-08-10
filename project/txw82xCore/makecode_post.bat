cd /d %~dp0

set dirname=%1
set dirname=%dirname:~0,-4%

if not exist bakup md bakup
if not exist bakup\%dirname% md bakup\%dirname%

copy .\Obj\txw80x.elf bakup\%dirname%\
copy .\Lst\txw80x.map bakup\%dirname%\
copy makedis.bat bakup\%dirname%\
del /q/f loader.bin	
del /q/f code512.bin
del /q/f param.bin
del /q/f program.bin
del /q/f project_merge.bin

