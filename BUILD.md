# 本地打包指南（Windows）

> 本机环境：Windows + Visual Studio 2022 Community（J 盘）+ Windows 10 SDK。
> 产物为单文件便携版 `.exe`，无需安装任何运行时依赖。

## 一、环境要求（本机已具备，无需额外安装）

| 组件 | 要求 | 本机位置 |
|---|---|---|
| MSVC 工具集 | v143（VS 2022） | `J:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.43.34808` |
| Windows 10 SDK | 10.0（含头文件 / Lib / `rc.exe`） | `C:\Program Files (x86)\Windows Kits\10\`（10.0.22621.0） |
| NuGet 包 | Microsoft.Windows.ImplementationLibrary（WIL，纯头文件库） | 仓库 `packages\` 目录，已就位 |

> 注意：上游仓库的 `.vcxproj` 原本指定 `PlatformToolset=v145`（VS 2026），本仓库已改为 `v143`。
> 若将来从上游重新拉取代码覆盖了 `src\MinimalImageViewer.vcxproj`，需再次把 4 处 `v145` 改回 `v143`。

## 二、打包步骤

### 步骤 1：确认 WIL 包完整（仅首次或目录损坏时需要）

检查该文件是否存在：

```
packages\Microsoft.Windows.ImplementationLibrary.1.0.260126.7\build\native\Microsoft.Windows.ImplementationLibrary.targets
```

若缺失，用仓库自带的 `.nupkg` 就地解压修复（PowerShell）：

```powershell
cd d:\Pycharm_Files\ydem-imageviewer\packages\Microsoft.Windows.ImplementationLibrary.1.0.260126.7
Copy-Item *.nupkg wil.zip -Force
Expand-Archive wil.zip -DestinationPath . -Force
Remove-Item wil.zip
```

### 步骤 2：构建（推荐：PowerShell 一条命令）

在任意 PowerShell 窗口执行（无需“开发者命令提示符”，脚本会自动定位 MSBuild）：

```powershell
# 0) 先结束可能正在后台常驻的查看器进程，避免 .exe 被占用导致打包/链接失败
taskkill /IM MinimalImageViewer.exe /F 2>$null

# 1) 定位并调用 MSBuild 构建
$inst = & 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe' -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
& (Join-Path $inst 'MSBuild\Current\Bin\MSBuild.exe') d:\Pycharm_Files\ydem-imageviewer\src\MinimalImageViewer.vcxproj /p:Configuration=Release /p:Platform=x64 /t:Rebuild /v:minimal
```

或者：打开开始菜单的 **“x64 Native Tools Command Prompt for VS 2022”**，执行：

```bat
cd /d d:\Pycharm_Files\ydem-imageviewer
taskkill /IM MinimalImageViewer.exe /F >nul 2>&1
msbuild src\MinimalImageViewer.vcxproj /p:Configuration=Release /p:Platform=x64
```

> 说明：本应用 Esc / 关闭按钮 / Alt+F4 现在只会“隐藏到后台”（进程常驻），
> 因此打包前必须先用 `taskkill` 结束残留进程，否则 MSBuild 因 `.exe` 被占用而链接失败。

### 步骤 3：取产物

| 平台 | 产物路径 |
|---|---|
| x64（默认） | `src\x64\Release\MinimalImageViewer.exe` |
| x86（可选，`/p:Platform=Win32`） | `src\Release\MinimalImageViewer.exe` |

Release 配置使用 `/MT` 静态链接，`.exe` 为**零依赖单文件**，直接分发即可。
`MinimalImageViewer.pdb` 是调试符号，分发时可不带。

## 三、可选：进一步压缩体积

原生编译约 1.6 MB；如需压到 ~500 KB（README 宣传值），构建后执行：

```bat
upx --lzma src\x64\Release\MinimalImageViewer.exe
```

## 四、常见问题

| 现象 | 原因 / 解决 |
|---|---|
| `Platform Toolset 'v145' must be installed` | vcxproj 被上游覆盖，重新把 `v145` 改为 `v143`（见第一节注意） |
| `EnsureNuGetPackageBuildImports` 报错、找不到 `.targets` | WIL 包目录不完整，执行步骤 1 修复 |
| 打开 `.sln` 提示 `windowsstore` 项目加载失败 | 正常。该 Store 打包目录被上游 `.gitignore` 忽略、未随仓库发布；只打便携版 `.exe` 不受影响，忽略提示即可 |
| `'msbuild' 不是内部或外部命令` | 未使用“x64 Native Tools”终端；改用第二节的 PowerShell 一键命令 |
