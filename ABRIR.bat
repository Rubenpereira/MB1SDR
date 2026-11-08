@echo off
cd /d "%~dp0pronto"
if not exist MB1SDR.exe (
  echo Nao achei pronto\MB1SDR.exe - rode COMPILAR.bat primeiro.
  pause
  exit /b 1
)
start "" MB1SDR.exe
