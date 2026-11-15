$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
if (Test-Path (Join-Path $PSScriptRoot "..\bin\haeun-vm.exe")) {
  $Root = Resolve-Path (Join-Path $PSScriptRoot "..")
}
$vm = Join-Path $Root "bin\haeun-vm.exe"
if (-not (Test-Path $vm)) { $vm = Join-Path $Root "build\haeun-vm.exe" }
& $vm (Join-Path $Root "images\hello.hxf")
if (-not $?) { & $vm (Join-Path $Root "build\hello.hxf") }
