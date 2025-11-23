@echo off
set build_path="%1\\build"
cmake -S %1 -B %build_path% -DCMAKE_BUILD_TYPE=%2 -G "Visual Studio 17 2022"
cmake --build %build_path% --config %2

IF not %ERRORLEVEL% EQU 0 exit /b %ERRORLEVEL%

IF "%3"=="-e" %build_path%\\%2\\bin\\examples\\window\\window.exe