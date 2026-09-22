cd /d %~dp0

ECHO OFF

set CDKPath=%1
set CFGPath=%2

"%CDKPath%\CSKY\FlashProgrammer\Bins\CSKYFlashProgramerConsole.exe" -f %CFGPath%

IF %ERRORLEVEL% NEQ 0 (
	goto err_exit
)
exit /b 0

:err_exit
exit /b 2
