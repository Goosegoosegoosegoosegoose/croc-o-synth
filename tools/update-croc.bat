@echo off
rem Croc-O-Synth: get the newest build from GitHub and install it on the FM-1.
rem Double-click this file with the FM-1 plugged in by USB and switched on.
setlocal
cd /d "%~dp0"
echo.
echo  Getting the newest Croc-O-Synth build from GitHub...
powershell -NoProfile -Command "$ErrorActionPreference='Stop'; $ProgressPreference='SilentlyContinue'; Invoke-WebRequest 'https://github.com/Goosegoosegoosegoosegoose/croc-o-synth/releases/latest/download/croc-o-synth-install.zip' -OutFile 'croc-o-synth-install.zip'; if (Test-Path 'croc-o-synth-install') { Remove-Item 'croc-o-synth-install' -Recurse -Force }; Expand-Archive 'croc-o-synth-install.zip' -DestinationPath 'croc-o-synth-install'"
if errorlevel 1 (
  echo.
  echo  The download did not work. Check the internet connection and try again.
  pause
  exit /b 1
)
cd croc-o-synth-install
echo.
echo  This build:
type BUILD.txt
echo.
echo  Installing on the FM-1. Leave it plugged in and switched on until this says done.
echo.
py fm1_install.py crocosynth.fwsc
echo.
pause
