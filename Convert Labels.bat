@echo off
setlocal
cd /d "%~dp0"
if exist "app\Slot-Advanced-Label-Converter-0.3.exe" (
  start "" "app\Slot-Advanced-Label-Converter-0.3.exe"
  exit /b 0
)
where py >nul 2>nul
if errorlevel 1 (
  python tools\convert_labels.py --gui
) else (
  py -3 tools\convert_labels.py --gui
)
if errorlevel 1 (
  echo Install Python with Tk, then run: python -m pip install -r requirements.txt
  pause
)
