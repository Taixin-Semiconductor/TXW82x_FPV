cd /d %~dp0

set dirname=%1
set dirname2=%dirname:~0,-4%

del %dirname2%_PSRAM.bin
del /q/f APP.bin
del /q/f APP_PSRAM.bin

copy %dirname% APP.bin

pin_bin.exe script.cfg
set dirname=%dirname:~0,-4%
set dirname=%dirname%_%date:~0,4%%date:~5,2%%date:~8,2%%time:~0,2%%time:~3,2%%time:~6,2%
if not exist bakup md bakup
if not exist bakup\%dirname% md bakup\%dirname%
copy Lst\* bakup\%dirname%\
copy project.map bakup\%dirname%\
