@echo off
rem Kulustur'u GitHub'a yukler (depo + indirilebilir oyun). Tarayicida GitHub girisi ister.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\github_yukle.ps1"
echo.
pause
