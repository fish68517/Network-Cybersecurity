$ErrorActionPreference = 'Stop'

$ProjectRoot = 'D:\Acode\Android\complete\MedicalSystemDemo'
$GuiAppDir = Join-Path $ProjectRoot 'gui\app'
$BridgeDir = Join-Path $ProjectRoot 'gui\bridge\x64\Release'
$BuildRoot = Join-Path $ProjectRoot 'gui\build'
$DistRoot = Join-Path $ProjectRoot 'gui\dist'
$PortableDir = Join-Path $ProjectRoot '交付\runtime\MedicalSystemDemo_GUI_Portable_x64'
$ZipPath = Join-Path $ProjectRoot '交付\runtime\MedicalSystemDemo_GUI_Portable_x64.zip'

if (Test-Path $BuildRoot) { Remove-Item -Recurse -Force $BuildRoot }
if (Test-Path $DistRoot) { Remove-Item -Recurse -Force $DistRoot }
if (Test-Path $PortableDir) { Remove-Item -Recurse -Force $PortableDir }
if (Test-Path $ZipPath) { Remove-Item -Force $ZipPath }

pyinstaller --noconfirm --clean --onedir --windowed --name MedicalSystemDemo_GUI --distpath $DistRoot --workpath $BuildRoot --specpath $BuildRoot (Join-Path $GuiAppDir 'main.py')

$GuiDistDir = Join-Path $DistRoot 'MedicalSystemDemo_GUI'
if (-not (Test-Path $GuiDistDir)) {
    throw '未找到 PyInstaller 输出目录。'
}

Copy-Item -Path (Join-Path $BridgeDir 'sgx_gui_bridge.dll') -Destination $GuiDistDir -Force
Copy-Item -Path (Join-Path $BridgeDir 'MedicalSystemDemo.signed.dll') -Destination $GuiDistDir -Force
Copy-Item -Path (Join-Path $BridgeDir 'RemoteAttestation_sp.exe') -Destination $GuiDistDir -Force
Get-ChildItem -Path $BridgeDir -Filter '*.dll' |
    Where-Object { $_.Name -ne 'sgx_gui_bridge.dll' -and $_.Name -ne 'MedicalSystemDemo.signed.dll' } |
    Copy-Item -Destination $GuiDistDir -Force

$RunNote = @'
运行方式：
1. 双击 MedicalSystemDemo_GUI.exe
2. 首次运行点击“初始化系统”；已有密态文件时点击“加载密态状态”
3. 默认管理员账号：admin / admin123
4. 请保持 sgx_gui_bridge.dll、MedicalSystemDemo.signed.dll、RemoteAttestation_sp.exe 以及全部 SGX DLL 与 GUI exe 位于同一目录

关键文件：
- GUI 程序：MedicalSystemDemo_GUI.exe
- Bridge：sgx_gui_bridge.dll
- Enclave：MedicalSystemDemo.signed.dll
- 远程认证服务原型：RemoteAttestation_sp.exe
- 密态状态文件：sealed_system_state.bin
'@
Set-Content -Path (Join-Path $GuiDistDir 'RUN_GUI.txt') -Value $RunNote -Encoding UTF8

Copy-Item -Recurse -Force $GuiDistDir $PortableDir
Compress-Archive -Path (Join-Path $PortableDir '*') -DestinationPath $ZipPath -Force

Write-Host "GUI 便携包已生成: $PortableDir"
Write-Host "GUI 压缩包已生成: $ZipPath"
