@echo off
setlocal
cd /d "%~dp0"
where py >nul 2>nul
if errorlevel 1 (
  python tools\build.py --target all %*
) else (
  py -3 tools\build.py --target all %*
)
if errorlevel 1 pause
