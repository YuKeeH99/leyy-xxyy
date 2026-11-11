@echo off
setlocal
if "%~1"=="" (
    echo Usage: setup_venus.bat ^<ql-sdk-path^>
    exit /b 1
)
set SDK=%~1
set HERE=%~dp0
if not exist "%SDK%\ql-application\threadx" (
    echo SDK path not found: %SDK%\ql-application\threadx
    exit /b 1
)
if not exist "%SDK%\ql-application\threadx\venus" mkdir "%SDK%\ql-application\threadx\venus"
xcopy /E /I /Y "%HERE%..\venus\*" "%SDK%\ql-application\threadx\venus\"
copy /Y "%HERE%app_linkscript.ld" "%SDK%\ql-application\threadx\config\common\app_linkscript.ld"
echo Done.
echo Next: edit %SDK%\ql-application\threadx\Makefile and add venus to COMMPILE_DIRS.
endlocal
