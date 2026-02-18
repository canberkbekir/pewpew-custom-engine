@echo off
setlocal

set MSBUILD="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
set SOLUTION=CBEngine.sln
set PLATFORM=x64
set CONFIG=Debug

:: Parse arguments
:parse_args
if "%~1"=="" goto build
if /i "%~1"=="debug" set CONFIG=Debug& shift& goto parse_args
if /i "%~1"=="release" set CONFIG=Release& shift& goto parse_args
if /i "%~1"=="dist" set CONFIG=Dist& shift& goto parse_args
if /i "%~1"=="clean" set CONFIG=%CONFIG%& set CLEAN=1& shift& goto parse_args
shift
goto parse_args

:build
echo ========================================
echo  Building %SOLUTION%
echo  Configuration: %CONFIG%
echo  Platform: %PLATFORM%
echo ========================================

if defined CLEAN (
    echo Cleaning...
    %MSBUILD% %SOLUTION% /t:Clean /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /verbosity:minimal
)

%MSBUILD% %SOLUTION% /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /verbosity:minimal /m
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ========================================
    echo  BUILD FAILED
    echo ========================================
    exit /b 1
)

echo.
echo ========================================
echo  BUILD SUCCEEDED
echo ========================================
exit /b 0
