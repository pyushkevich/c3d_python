@echo on
setlocal enabledelayedexpansion

REM Create build and install directories
mkdir be\install
cd be

REM Run CMake to configure the project
cmake ^
    -DFETCH_DEPENDENCIES=ON ^
    -DDEPENDENCIES_ONLY=ON ^
    -DCMAKE_INSTALL_PREFIX=.\install ^
    ..

REM Build the project using all available processors
cmake --build . --target install -- /j %NUMBER_OF_PROCESSORS%

endlocal