$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
if (Test-Path (Join-Path $PSScriptRoot "..\bin\haeun-vm.exe")) {
  $Root = Resolve-Path (Join-Path $PSScriptRoot "..")
}
$vm = Join-Path $Root "bin\haeun-vm.exe"
if (-not (Test-Path $vm)) { $vm = Join-Path $Root "build\haeun-vm.exe" }
$img = Join-Path $Root "images\process_apps.hxf"
if (-not (Test-Path $img)) { $img = Join-Path $Root "build\process_apps.hxf" }
# Apps mode loads build/app_*.hxf relative to CWD — run from repo or copy apps beside CWD.
Push-Location $Root
try {
  if (Test-Path "images\app_calc.hxf") {
    New-Item -ItemType Directory -Force -Path "build" | Out-Null
    Copy-Item "images\app_*.hxf" "build\" -Force
  }
  & $vm $img --apps
} finally {
  Pop-Location
}
