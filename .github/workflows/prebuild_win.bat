@echo on
setlocal enabledelayedexpansion

REM Create build and install directories
mkdir be\install
cd be

REM Read the architecture parameter
REM Check the first command-line parameter
if "%1" == "AMD64" (
    set ARCH=x64
) else (
    set ARCH=Win32
)

REM Run CMake to configure the project
cmake ^
    -DFETCH_DEPENDENCIES=ON ^
    -DDEPENDENCIES_ONLY=OFF ^
    -DCMAKE_INSTALL_PREFIX=.\install ^
    -DCMAKE_BUILD_TYPE=Release ^
    -A %ARCH% ^
    ..
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

REM Build the project using all available processors
cmake --build . --config Release --target install -- -m
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

endlocal