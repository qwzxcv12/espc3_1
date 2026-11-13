@echo off
title BIEN DICH VA NAP CODE ESP32-C3
color 0B

echo =====================================================================
echo          HE THONG TU DONG BIEN DICH VA NAP FIRMWARE ESP32-C3
echo =====================================================================
echo.

cd /d "%~dp0"

REM 1. Kiem tra Python
python --version >nul 2>&1
if errorlevel 1 goto :no_python

REM 2. Dong bo ma nguon main.cpp sang ESP32C3_RS232.cpp
if exist "main.cpp" copy /Y "main.cpp" "ESP32C3_RS232.cpp" >nul

REM 3. Chay qua trinh build va nap
echo [*] Dang dong bo ma nguon, bien dich tren GitHub va nap vao ESP32-C3...
echo.
python cloud_builder.py
if errorlevel 1 goto :build_failed

color 0A
echo.
echo =====================================================================
echo   [THANH CONG] DA BIEN DICH VA NAP FIRMWARE VAO ESP32-C3 XONG!
echo =====================================================================
echo.
goto :end

:no_python
color 0C
echo.
echo [LOI] Khong tim thay Python trong he thong!
echo Vui long cai dat Python va tich chon "Add Python to PATH".
echo.
goto :end

:build_failed
color 0C
echo.
echo =====================================================================
echo   [THAT BAI] Co loi xay ra trong qua trinh bien dich hoac nap code!
echo   Vui long kiem tra:
echo     1. Cap ket noi USB/COM da cam chac chua?
echo     2. Cong COM6 co bi phan mem nao chiem dung (Serial Monitor) khong?
echo =====================================================================
echo.
goto :end

:end
pause