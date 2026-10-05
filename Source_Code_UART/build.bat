@echo off
setlocal

set ROOT=%~dp0
if not exist "%ROOT%build" mkdir "%ROOT%build"

set GPP=
for /f "delims=" %%G in ('where.exe g++') do (
	set GPP=%%G
	goto :found_gpp
)

echo g++ was not found on PATH.
exit /b 1

:found_gpp
"%GPP%" -std=c++17 -Wall -Wextra -Wpedantic -I"%ROOT%include" "%ROOT%src\main.cpp" -o "%ROOT%build\UARTSim.exe"
if errorlevel 1 exit /b 1

echo Built %ROOT%build\UARTSim.exe