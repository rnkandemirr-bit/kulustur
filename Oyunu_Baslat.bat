@echo off
rem Kulustur oyununu pencere modunda baslatir
start "" "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0kulustur.uproject" -game -windowed -ResX=1600 -ResY=900
