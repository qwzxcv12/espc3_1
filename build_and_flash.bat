@echo off
chcp 65001 >nul
title BIÊN DỊCH VÀ NẠP CODE ESP32-C3 (CLOUD BUILDER)
color 0B

echo =====================================================================
echo          HỆ THỐNG TỰ ĐỘNG BIÊN DỊCH VÀ NẠP FIRMWARE ESP32-C3        
echo =====================================================================
echo.

cd /d "%~dp0"

REM 1. Kiểm tra môi trường Python
python --version >nul 2>&1
if errorlevel 1 (
    color 0C
    echo [LỖI] Không tìm thấy Python trong hệ thống!
    echo Vui lòng cài đặt Python và tích chọn "Add Python to PATH".
    echo.
    pause
    exit /b 1
)

REM 2. Đồng bộ mã nguồn giữa main.cpp và ESP32C3_RS232.cpp
if exist "main.cpp" (
    copy /Y "main.cpp" "ESP32C3_RS232.cpp" >nul
)

REM 3. Chạy quá trình build đám mây và nạp code qua cổng COM
echo [*] Bắt đầu đồng bộ mã nguồn, biên dịch đám mây và nạp vào ESP32-C3...
echo.
python cloud_builder.py

if errorlevel 1 (
    color 0C
    echo.
    echo =====================================================================
    echo   [THẤT BẠI] Có lỗi xảy ra trong quá trình biên dịch hoặc nạp code!
    echo   Vui lòng kiểm tra lại cáp kết nối USB hoặc đóng Serial Monitor nếu mở.
    echo =====================================================================
) else (
    color 0A
    echo.
    echo =====================================================================
    echo   [THÀNH CÔNG] ĐÃ BIÊN DỊCH VÀ NẠP FIRMWARE VÀO ESP32-C3 XONG!      
    echo =====================================================================
)

echo.
pause
