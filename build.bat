@echo off
setlocal enabledelayedexpansion

echo ===============================================================
echo  geotiff-to-raw Unified App Build and Package Script
echo ===============================================================

set "PROJECT_ROOT=%~dp0"
if "%PROJECT_ROOT:~-1%"=="\" set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"

set "BUILD_DIR=%PROJECT_ROOT%\build"
set "DIST_DIR=%PROJECT_ROOT%\dist"

rem --- Packaging Naming Convention: <Project>_<Version>_<OS>_<Arch>.zip ---
set "APP_NAME=geotiff-to-raw"
set "APP_VERSION=v1.0.0"
set "APP_OS=windows"
set "APP_ARCH=x64"
set "ZIP_NAME=%APP_NAME%_%APP_VERSION%_%APP_OS%_%APP_ARCH%.zip"
set "ZIP_PATH=%PROJECT_ROOT%\%ZIP_NAME%"
set "DATA_ZIP_NAME=%APP_NAME%_%APP_VERSION%_sample_data.zip"
set "DATA_ZIP_PATH=%PROJECT_ROOT%\%DATA_ZIP_NAME%"

rem --- 1. Detect ThirdPart environment variable ---
if not defined ThirdPart (
    if defined ThirdParty (
        set "ThirdPart=%ThirdParty%"
    )
)

if not defined ThirdPart (
    if not defined QT_DIR (
        echo [ERROR] Environment variable 'ThirdPart' is not defined and QT_DIR is not set!
        echo Please configure the ThirdPart environment variable (e.g. set ThirdPart=D:\HR\DEV\ThirdParty)
        pause
        exit /b 1
    )
)

rem --- 2. Resolve GDAL and Qt5 paths from ThirdPart ---
if not defined GDAL_INC (
    if defined GDAL_DIR (
        set "GDAL_INC=%GDAL_DIR%\include"
    ) else if defined ThirdPart (
        set "GDAL_INC=%ThirdPart%\gdal111\include"
    )
)

if not defined GDAL_LIB (
    if defined GDAL_DIR (
        set "GDAL_LIB=%GDAL_DIR%\lib\vc140\x64\Release\gdal_i.lib"
    ) else if defined ThirdPart (
        set "GDAL_LIB=%ThirdPart%\gdal111\lib\vc140\x64\Release\gdal_i.lib"
    )
)

if not defined GDAL_DLL (
    if defined GDAL_DIR (
        set "GDAL_DLL=%GDAL_DIR%\bin\x64\gdal111.dll"
    ) else if defined ThirdPart (
        set "GDAL_DLL=%ThirdPart%\gdal111\bin\x64\gdal111.dll"
    )
)

if not defined QT_DIR (
    if defined ThirdPart (
        set "QT_DIR=%ThirdPart%\Qt5.11\vc140\x64"
    )
)

echo.
echo [Path Configuration]
if defined ThirdPart echo   ThirdPart : %ThirdPart%
echo   GDAL_INC  : %GDAL_INC%
echo   GDAL_LIB  : %GDAL_LIB%
echo   GDAL_DLL  : %GDAL_DLL%
echo   QT_DIR    : %QT_DIR%

echo.
echo [1/5] Preparing output directory: %DIST_DIR%
if exist "%DIST_DIR%" rd /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%"
mkdir "%DIST_DIR%\platforms"
mkdir "%DIST_DIR%\styles"
mkdir "%DIST_DIR%\sdk"

echo.
echo [2/5] Running CMake Configuration (x64)...
if exist "%BUILD_DIR%" rd /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"
cmake -B "%BUILD_DIR%" -S "%PROJECT_ROOT%" -A x64 -DGDAL_INCLUDE_DIR="%GDAL_INC%" -DGDAL_LIBRARY="%GDAL_LIB%" -DQT5_INSTALL_DIR="%QT_DIR%"

if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo [3/5] Compiling project (Release)...
cmake --build "%BUILD_DIR%" --config Release

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo [4/5] Copying Executables and Dependencies to dist...

rem 1. Copy Executables
if exist "%BUILD_DIR%\bin\geotiff-to-raw.exe" (
    copy /y "%BUILD_DIR%\bin\geotiff-to-raw.exe" "%DIST_DIR%\" >nul
    echo   [+] Copied: geotiff-to-raw.exe
) else (
    if exist "%BUILD_DIR%\bin\Release\geotiff-to-raw.exe" (
        copy /y "%BUILD_DIR%\bin\Release\geotiff-to-raw.exe" "%DIST_DIR%\" >nul
        echo   [+] Copied: geotiff-to-raw.exe [Release]
    )
)

rem 2. Copy Theme Style QSS
if exist "%PROJECT_ROOT%\resources\themestyle.qss" (
    copy /y "%PROJECT_ROOT%\resources\themestyle.qss" "%DIST_DIR%\" >nul
    echo   [+] Copied Theme File: resources/themestyle.qss
)

rem 3. Copy Engine SDK Files (.h & .cpp)
if exist "%PROJECT_ROOT%\src\RawTerrainReader.h" (
    copy /y "%PROJECT_ROOT%\src\RawTerrainReader.h" "%DIST_DIR%\sdk\" >nul
    copy /y "%PROJECT_ROOT%\src\RawTerrainReader.cpp" "%DIST_DIR%\sdk\" >nul
    echo   [+] Copied Engine SDK Files: sdk/RawTerrainReader.h, sdk/RawTerrainReader.cpp
)

rem 4. Copy GDAL DLL
if exist "%GDAL_DLL%" (
    copy /y "%GDAL_DLL%" "%DIST_DIR%\" >nul
    echo   [+] Copied GDAL DLL: gdal111.dll
) else (
    if exist "%BUILD_DIR%\bin\gdal111.dll" (
        copy /y "%BUILD_DIR%\bin\gdal111.dll" "%DIST_DIR%\" >nul
        echo   [+] Copied GDAL DLL: gdal111.dll [from build/bin]
    ) else (
        echo   [WARNING] GDAL DLL not found: %GDAL_DLL%
    )
)

rem 5. Copy Qt5 Core DLLs
if exist "%QT_DIR%\bin\Qt5Core.dll" (
    copy /y "%QT_DIR%\bin\Qt5Core.dll" "%DIST_DIR%\" >nul
    copy /y "%QT_DIR%\bin\Qt5Gui.dll" "%DIST_DIR%\" >nul
    copy /y "%QT_DIR%\bin\Qt5Widgets.dll" "%DIST_DIR%\" >nul
    echo   [+] Copied Qt5 Core DLLs: Qt5Core.dll, Qt5Gui.dll, Qt5Widgets.dll
) else (
    if exist "%BUILD_DIR%\bin\Qt5Core.dll" (
        copy /y "%BUILD_DIR%\bin\Qt5Core.dll" "%DIST_DIR%\" >nul
        copy /y "%BUILD_DIR%\bin\Qt5Gui.dll" "%DIST_DIR%\" >nul
        copy /y "%BUILD_DIR%\bin\Qt5Widgets.dll" "%DIST_DIR%\" >nul
        echo   [+] Copied Qt5 Core DLLs [from build/bin]
    ) else (
        echo   [WARNING] Qt5 Core DLLs not found!
    )
)

rem 6. Copy Qt5 Platform Plugin
if exist "%QT_DIR%\plugins\platforms\qwindows.dll" (
    copy /y "%QT_DIR%\plugins\platforms\qwindows.dll" "%DIST_DIR%\platforms\" >nul
    echo   [+] Copied Qt5 Platform Plugin: platforms/qwindows.dll
) else (
    if exist "%BUILD_DIR%\bin\platforms\qwindows.dll" (
        copy /y "%BUILD_DIR%\bin\platforms\qwindows.dll" "%DIST_DIR%\platforms\" >nul
        echo   [+] Copied Qt5 Platform Plugin: platforms/qwindows.dll [from build/bin]
    ) else (
        echo   [WARNING] Qt5 platform plugin not found: platforms/qwindows.dll
    )
)

rem 7. Copy Qt5 Style Plugin
if exist "%QT_DIR%\plugins\styles\qwindowsvistastyle.dll" (
    copy /y "%QT_DIR%\plugins\styles\qwindowsvistastyle.dll" "%DIST_DIR%\styles\" >nul
    echo   [+] Copied Qt5 Style Plugin: styles/qwindowsvistastyle.dll
)

rem 8. Copy Documentation and License
if exist "%PROJECT_ROOT%\README.md" (
    copy /y "%PROJECT_ROOT%\README.md" "%DIST_DIR%\" >nul
    echo   [+] Copied Documentation: README.md
)
if exist "%PROJECT_ROOT%\LICENSE" (
    copy /y "%PROJECT_ROOT%\LICENSE" "%DIST_DIR%\" >nul
    echo   [+] Copied License: LICENSE
)

echo.
echo [5/5] Packaging into Portable Release ZIP Archives...
powershell -NoProfile -Command "Compress-Archive -Path '%DIST_DIR%\*' -DestinationPath '%ZIP_PATH%' -Force"
if %ERRORLEVEL% equ 0 (
    echo   [+] Created software release archive: %ZIP_NAME%
) else (
    echo   [WARNING] Failed to create software release archive.
)

rem Check and package sample data if present
if exist "%PROJECT_ROOT%\data\*.bin" (
    powershell -NoProfile -Command "Compress-Archive -Path '%PROJECT_ROOT%\data' -DestinationPath '%DATA_ZIP_PATH%' -Force"
    if !ERRORLEVEL! equ 0 (
        echo   [+] Created sample data archive: %DATA_ZIP_NAME%
    )
)

echo.
echo ===============================================================
echo [SUCCESS] Build and Packaging Completed!
echo Portable Release Directory: %DIST_DIR%
echo Software Release ZIP Archive: %ZIP_PATH%
if exist "%DATA_ZIP_PATH%" echo Sample Data ZIP Archive:     %DATA_ZIP_PATH%
echo You can run dist\geotiff-to-raw.exe directly.
echo ===============================================================
echo.

pause
