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
    call :set_default
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

:set_default
echo.
echo 正在注册为默认看图软件...
set "EXE_PATH=%~dp0src\x64\Release\MinimalImageViewer.exe"
set "PROGID=MinimalImageViewer.1"

if not exist "%EXE_PATH%" (
    echo [跳过] 未找到 Release 产物：%EXE_PATH%
    goto :eof
)

rem 先清除可能存在的旧关联，再用 PowerShell 写入（避免 cmd.exe 引号陷阱）
reg delete "HKCU\Software\Classes\%PROGID%" /f >nul 2>&1

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$exe='%EXE_PATH%';$p='%PROGID%';$q=[char]34;$k='HKCU:\Software\Classes\'+$p;" ^
  "New-Item -Path $k -Force|Out-Null;Set-ItemProperty -Path $k -Name '(default)' -Value 'Minimal Image Viewer';" ^
  "New-Item -Path ($k+'\DefaultIcon') -Force|Out-Null;Set-ItemProperty -Path ($k+'\DefaultIcon') -Name '(default)' -Value ($exe+',0');" ^
  "New-Item -Path ($k+'\shell\open\command') -Force|Out-Null;Set-ItemProperty -Path ($k+'\shell\open\command') -Name '(default)' -Value ($q+$exe+$q+' '+$q+'%%1'+$q);" ^
  "$exts='.jpg','.jpeg','.png','.bmp','.gif','.tiff','.tif','.ico','.webp','.heic','.heif','.avif','.cr2','.cr3','.nef','.dng','.arw','.orf','.rw2','.svg','.qoi','.hdr','.tga','.psd','.ppm','.pgm','.pbm','.pnm','.pic';" ^
  "foreach($e in $exts){$ek='HKCU:\Software\Classes\'+$e;New-Item -Path $ek -Force|Out-Null;Set-ItemProperty -Path $ek -Name '(default)' -Value $p;$uc='HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\'+$e+'\UserChoice';if(Test-Path $uc){Remove-Item -Path $uc -Force -Recurse -ErrorAction SilentlyContinue}}"

echo [完成] 已将图片扩展名关联到 MinimalImageViewer
goto :eof
