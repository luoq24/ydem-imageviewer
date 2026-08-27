@echo off
chcp 65001 >nul
cd /d "%~dp0"

set "MSBUILD="

rem 按常见安装位置依次查找 MSBuild.exe（VS2022 / VS2019，三种版本）
call :TryMSBuild "%ProgramFiles%\Microsoft Visual Studio\2022"
call :TryMSBuild "%ProgramFiles(x86)%\Microsoft Visual Studio\2022"
call :TryMSBuild "J:\Program Files\Microsoft Visual Studio\2022"
call :TryMSBuild "%ProgramFiles%\Microsoft Visual Studio\2019"
call :TryMSBuild "%ProgramFiles(x86)%\Microsoft Visual Studio\2019"
call :TryMSBuild "J:\Program Files\Microsoft Visual Studio\2019"

if defined MSBUILD goto build
echo [错误] 未找到 MSBuild，请确认已安装 Visual Studio 及"使用 C++ 的桌面开发"组件。
echo.
pause
exit /b 1

:build
echo 使用 MSBuild: %MSBUILD%
"%MSBUILD%" src\MinimalImageViewer.vcxproj /p:Configuration=Release /p:Platform=x64 /m /nologo
set "BUILD_RESULT=%ERRORLEVEL%"
echo.
if "%BUILD_RESULT%"=="0" (
    echo [完成] 打包成功，产物位于 src\x64\Release\MinimalImageViewer.exe
) else (
    echo [失败] 打包出错，错误码：%BUILD_RESULT%
)
echo.
pause
exit /b %BUILD_RESULT%

:TryMSBuild
rem %~1 为 Visual Studio 安装根目录，依次检查 Community / Professional / Enterprise
for %%e in (Community Professional Enterprise) do if not defined MSBUILD if exist "%~1\%%e\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%~1\%%e\MSBuild\Current\Bin\MSBuild.exe"
goto :eof
