@echo on
setlocal enabledelayedexpansion

REM Create build and install directories
mkdir be\install
cd be

REM Run CMake to configure the project
cmake ^
    -DFETCH_DEPENDENCIES=ON ^
    -DDEPENDENCIES_ONLY=OFF ^
    -DCMAKE_INSTALL_PREFIX=.\install ^
    -DCMAKE_BUILD_TYPE=Release ^
    ..
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

REM Build the project using all available processors
cmake --build . --target install -- -m
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

endlocal