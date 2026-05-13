@echo off
:: Build and run the Bangkok Mass Transit Route Finder

cd /d "%~dp0"

echo Building...
gcc -Wall -o transit.exe main.c data.c graph.c hashtable.c bst.c
if %errorlevel% neq 0 (
  echo.
  echo Build failed.
  pause
  exit /b 1
)

echo Done. Starting...
echo.
transit.exe

echo.
pause
