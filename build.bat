@echo off
setlocal enabledelayedexpansion

echo ===============================================================
echo  TiffToRawGui Unified App Build and Package Script
echo ===============================================================

set "PROJECT_ROOT=%~dp0"
if "%PROJECT_ROOT:~-1%"=="\" set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"

set "BUILD_DIR=%PROJECT_ROOT%\build"
set "DIST_DIR=%PROJECT_ROOT%\dist"

if not defined GDAL_INC set "GDAL_INC=D:/HR/DEV/ThirdParty/gdal111/include"
if not defined GDAL_LIB set "GDAL_LIB=D:/HR/DEV/ThirdParty/gdal111/lib/vc140/x64/Release/gdal_i.lib"
if not defined GDAL_DLL set "GDAL_DLL=D:\HR\DEV\ThirdParty\gdal111\bin\x64\gdal111.dll"
if not defined QT_DIR set "QT_DIR=D:/HR/DEV/ThirdParty/Qt5.11/vc140/x64"

echo.
echo [1/4] Preparing output directory: %DIST_DIR%
if exist "%DIST_DIR%" rd /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%"
mkdir "%DIST_DIR%\platforms"
mkdir "%DIST_DIR%\sdk"

echo.
echo [2/4] Running CMake Configuration (x64)...
if exist "%BUILD_DIR%" rd /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"
cmake -B "%BUILD_DIR%" -S "%PROJECT_ROOT%" -A x64 -DGDAL_INCLUDE_DIR="%GDAL_INC%" -DGDAL_LIBRARY="%GDAL_LIB%" -DQT5_INSTALL_DIR="%QT_DIR%"

if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo [3/4] Compiling project (Release)...
cmake --build "%BUILD_DIR%" --config Release

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo [4/4] Copying Executables and Dependencies to dist...

rem 1. Copy Executables
if exist "%BUILD_DIR%\bin\TiffToRawGui.exe" (
    copy /y "%BUILD_DIR%\bin\TiffToRawGui.exe" "%DIST_DIR%\" >nul
    echo   [+] Copied: TiffToRawGui.exe
) else (
    if exist "%BUILD_DIR%\bin\Release\TiffToRawGui.exe" (
        copy /y "%BUILD_DIR%\bin\Release\TiffToRawGui.exe" "%DIST_DIR%\" >nul
        echo   [+] Copied: TiffToRawGui.exe [Release]
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

echo.
echo ===============================================================
echo [SUCCESS] One-click Build and Package Completed!
echo Output Directory: %DIST_DIR%
echo You can run dist\TiffToRawGui.exe directly.
echo ===============================================================
echo.

pause
