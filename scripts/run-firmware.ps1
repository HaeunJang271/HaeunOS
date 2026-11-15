$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
if (Test-Path (Join-Path $PSScriptRoot "..\bin\haeun-vm.exe")) {
  $Root = Resolve-Path (Join-Path $PSScriptRoot "..")
}
$vm = Join-Path $Root "bin\haeun-vm.exe"
if (-not (Test-Path $vm)) { $vm = Join-Path $Root "build\haeun-vm.exe" }
$fw = Join-Path $Root "images\boot.hfw"
$disk = Join-Path $Root "images\kernel_disk.raw"
if (-not (Test-Path $fw)) { $fw = Join-Path $Root "build\boot.hfw" }
if (-not (Test-Path $disk)) { $disk = Join-Path $Root "build\kernel_disk.raw" }
& $vm --firmware $fw --disk $disk
