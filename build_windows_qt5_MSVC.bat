@echo off

REM Date     Version   Author                Changes

echo ************************************
echo ***     DLT Can                  ***
echo ************************************
echo ************************************
echo ***         Configuration        ***
echo ************************************

set NAME=DLTCan
set TARGET_NAME=%NAME%

rem parameter of this batch script can be either x86 or x86_amd64
if "%ARCHITECTURE%"=="" (
    if "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
        set ARCHITECTURE=x86_amd64
    ) else (
        set ARCHITECTURE=x86
    )

    set USE_ARCH_PARAM=false
    if "%1" NEQ "" (
        if "%1"=="x86" set USE_ARCH_PARAM=true
        if "%1"=="x86_amd64" set USE_ARCH_PARAM=true
    )
    if "!USE_ARCH_PARAM!"=="true" set ARCHITECTURE=%1
)

echo Target architecture is %ARCHITECTURE%

echo *** Setting up environment ***

if "%QTDIR%"=="" (
    if "%ARCHITECTURE%"=="x86_amd64" (
        set QTDIR=C:\Qt\5.15.2\msvc2019_64
    ) else (set QTDIR=C:\Qt\5.15.2\msvc2019)
)

if "%MSVC_DIR%"=="" set MSVC_DIR=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build


set PATH=%QTDIR%\bin;%MSVC_DIR%;%MSVC_DIR%\bin;%PATH%
set QTSDK=%QTDIR%

if '%WORKSPACE%'=='' (
    if '%INSTALLATION_DIR%'=='' (
        set INSTALLATION_DIR=c:\%TARGET_NAME%
    )

    set SOURCE_DIR=%CD%
    set BUILD_DIR=%CD%\build\release
) else (
    if '%INSTALLATION_DIR%'=='' (
        set INSTALLATION_DIR=%WORKSPACE%\build\dist\%TARGET_NAME%
    )

    set SOURCE_DIR=%WORKSPACE%
    set BUILD_DIR=%WORKSPACE%\build\release
)

echo ************************************
echo * QTDIR     = %QTDIR%
echo * QTSDK     = %QTSDK%
echo * QWT_DIR   = %QWT_DIR%
echo * MSVC_DIR  = %MSVC_DIR%
echo * PATH      = %PATH%
echo * INSTALLATION_DIR = %INSTALLATION_DIR%
echo * SOURCE_DIR         = %SOURCE_DIR%
echo * BUILD_DIR          = %BUILD_DIR%
echo ************************************

if exist build (
echo ************************************
echo ***  Delete old build Directory  ***
echo ************************************

    rmdir /s /q build
    if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

)

echo ************************************
echo ***  Configure MSVC environment  ***
echo ************************************

call vcvarsall.bat %ARCHITECTURE%
if %ERRORLEVEL% NEQ 0 goto error
echo configuring was successful

if exist %INSTALLATION_DIR% (
echo ************************************
echo ***   Delete old Directory       ***
echo ************************************

    rmdir /s /q %INSTALLATION_DIR%
    if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER
)

echo ************************************
echo ***       Build                  ***
echo ************************************

mkdir build
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

cd build
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

qmake ../%NAME%.pro
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

nmake
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

echo ************************************
echo ***         Create SDK           ***
echo ************************************

echo *** Create directories %INSTALLATION_DIR% ***

if not exist %INSTALLATION_DIR% mkdir %INSTALLATION_DIR%
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

mkdir %INSTALLATION_DIR%\platforms
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

mkdir %INSTALLATION_DIR%\doc
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

mkdir %INSTALLATION_DIR%\arduino
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

mkdir %INSTALLATION_DIR%\arduino\WemosD1MiniCAN
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

mkdir %INSTALLATION_DIR%\arduino\WemosD1R1CANDiymore
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

mkdir %INSTALLATION_DIR%\arduino\WemosD1R1CANKeyestudio
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

echo *** Copy files ***
copy %QTDIR%\bin\Qt5Core.dll %INSTALLATION_DIR%
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

copy %QTDIR%\bin\Qt5Gui.dll %INSTALLATION_DIR%
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

copy %QTDIR%\bin\Qt5Network.dll %INSTALLATION_DIR%
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

copy %QTDIR%\bin\Qt5Svg.dll %INSTALLATION_DIR%
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

copy %QTDIR%\bin\Qt5Widgets.dll %INSTALLATION_DIR%
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

copy %QTDIR%\bin\Qt5PrintSupport.dll %INSTALLATION_DIR%
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

copy %QTDIR%\bin\Qt5Xml.dll %INSTALLATION_DIR%
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

copy %QTDIR%\bin\Qt5OpenGL.dll %INSTALLATION_DIR%
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

copy %QTDIR%\bin\Qt5SerialPort.dll %INSTALLATION_DIR%
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

copy %QTDIR%\plugins\platforms\qwindows.dll %INSTALLATION_DIR%\platforms
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

copy %SOURCE_DIR%\arduino\WemosD1MiniCAN\WemosD1MiniCAN.ino %INSTALLATION_DIR%\arduino\WemosD1MiniCAN
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

copy %SOURCE_DIR%\arduino\WemosD1R1CANDiymore\WemosD1R1CANDiymore.ino %INSTALLATION_DIR%\arduino\WemosD1R1CANDiymore
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

copy %SOURCE_DIR%\arduino\WemosD1R1CANKeyestudio\WemosD1R1CANKeyestudio.ino %INSTALLATION_DIR%\arduino\WemosD1R1CANKeyestudio
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

copy %BUILD_DIR%\%NAME%.exe %INSTALLATION_DIR%
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

copy %SOURCE_DIR%\README.md %INSTALLATION_DIR%
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

copy %SOURCE_DIR%\LICENSE %INSTALLATION_DIR%
if %ERRORLEVEL% NEQ 0 GOTO ERROR_HANDLER

GOTO QUIT

:ERROR_HANDLER
echo ####################################
echo ###       ERROR occured          ###
echo ####################################
set /p name= Continue
exit 1


:QUIT
echo ************************************
echo ***       SUCCESS finish         ***
echo ************************************
echo Installed in: %INSTALLATION_DIR%
set /p name= Continue
exit 0
