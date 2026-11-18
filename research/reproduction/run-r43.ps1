# R4.3 local package runner (same machine). Does not claim third-party success.
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Set-Location $Root
$Log = Join-Path $Root "research\reproduction\r43-run.log"
function Sha([string]$p) {
  if (Test-Path $p) { (Get-FileHash -Algorithm SHA256 $p).Hash.ToLower() } else { $null }
}
$commit = (git rev-parse HEAD)
$dirty = (git status --porcelain | Measure-Object).Count
Write-Host "commit=$commit dirty_files≈$dirty"
if ($dirty -gt 0) { Write-Host "WARNING: working tree dirty; not immutable baseline" }
& make clean 2>&1 | Tee-Object $Log
if ($LASTEXITCODE -ne 0) { throw "make clean failed" }
& make all 2>&1 | Tee-Object $Log -Append
if ($LASTEXITCODE -ne 0) { throw "make all failed" }
& make test 2>&1 | Tee-Object $Log -Append
if ($LASTEXITCODE -ne 0) { throw "make test failed" }
$man = [ordered]@{
  commit = $commit
  dirty_approx = $dirty
  third_party = $false
  hashes = [ordered]@{
    proc_sched = Sha "kernel\process\proc_sched.s"
    proc_policy = Sha "kernel\process\proc_policy.s"
    proc_trap = Sha "kernel\process\proc_trap.s"
    proc_telem = Sha "kernel\process\proc_telem.s"
    proc_fork = Sha "kernel\process\proc_fork.s"
    selection_trace = Sha "research\experiments\r43-selection-trace.json"
    active_apply = Sha "research\experiments\r43-active-apply.json"
  }
}
($man | ConvertTo-Json -Depth 5) | Set-Content -Encoding utf8 (Join-Path $Root "research\reproduction\r43-artifact-manifest.json")
Write-Host "r43 local package run OK"
