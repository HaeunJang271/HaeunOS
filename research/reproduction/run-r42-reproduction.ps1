# R4.2 separate-directory clean build (Windows).
# NOTE: Much of R3.x–R4.x is uncommitted vs origin/main; git worktree from HEAD
# would omit sources. This copies the current working tree (excluding build/) into
# a sibling directory and rebuilds there. Third-party independent lab: NOT CLAIMED.
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$Work = Join-Path (Split-Path $Root -Parent) ("HaeunOS-r42-repro-" + $Stamp)
$OutDir = Join-Path $Root "research\reproduction"
$EnvJson = Join-Path $OutDir "r42-environment.json"
$ManJson = Join-Path $OutDir "r42-manifest.json"
$Log = Join-Path $OutDir "r42-reproduction-run.log"

function Sha256File([string]$Path) {
  if (-not (Test-Path $Path)) { return $null }
  return (Get-FileHash -Algorithm SHA256 -Path $Path).Hash.ToLower()
}

New-Item -ItemType Directory -Force -Path $Work | Out-Null
Write-Host "Copying working tree to $Work (exclude build)"
& robocopy $Root $Work /E /XD build .git /NFL /NDL /NJH /NJS /nc /ns /np
# robocopy exit 0-7 are success-ish
if ($LASTEXITCODE -ge 8) { throw "robocopy failed: $LASTEXITCODE" }

Push-Location $Root
$commit = (git rev-parse HEAD 2>$null)
if (-not $commit) { $commit = "unknown" }
$dirty = (git status --porcelain 2>$null | Measure-Object).Count
Pop-Location

$cc = (cc --version 2>&1 | Select-Object -First 1 | Out-String).Trim()
$py = (python --version 2>&1 | Out-String).Trim()
$envObj = [ordered]@{
  timestamp_local = (Get-Date).ToString("o")
  reproduction_kind = "separate_directory_working_tree_copy"
  git_worktree_from_HEAD = "NOT_USED_sources_uncommitted"
  third_party_independent = $false
  git_commit_at_copy = $commit
  dirty_file_count_approx = $dirty
  work_dir = $Work
  os = [System.Environment]::OSVersion.VersionString
  compiler = $cc
  python = $py
}
($envObj | ConvertTo-Json -Depth 4) | Set-Content -Encoding utf8 $EnvJson

Push-Location $Work
try {
  & make clean 2>&1 | Tee-Object -FilePath $Log
  if ($LASTEXITCODE -ne 0) { throw "make clean failed" }
  & make all 2>&1 | Tee-Object -FilePath $Log -Append
  if ($LASTEXITCODE -ne 0) { throw "make all failed" }
  & make build/r12-images.stamp build/r22-images.stamp build/r30-images.stamp 2>&1 | Tee-Object -FilePath $Log -Append
  & make build/r42-validation-tests.exe build/r41-validation-tests.exe 2>&1 | Tee-Object -FilePath $Log -Append
  if ($LASTEXITCODE -ne 0) { throw "make r42 binary failed" }
  New-Item -ItemType Directory -Force -Path "research\experiments" | Out-Null
  & .\build\r42-validation-tests.exe build 2>&1 | Tee-Object -FilePath $Log -Append
  $r42 = $LASTEXITCODE
  & .\build\r41-validation-tests.exe build smoke 2>&1 | Tee-Object -FilePath $Log -Append
  $r41 = $LASTEXITCODE

  $manifest = [ordered]@{
    git_commit_at_copy = $commit
    work_dir = $Work
    exit_r42 = $r42
    exit_r41_smoke = $r41
    hashes = [ordered]@{
      proc_sched = Sha256File "kernel\process\proc_sched.s"
      proc_policy = Sha256File "kernel\process\proc_policy.s"
      proc_trap = Sha256File "kernel\process\proc_trap.s"
      proc_telem = Sha256File "kernel\process\proc_telem.s"
      proc_fork = Sha256File "kernel\process\proc_fork.s"
      r42_profile_json = Sha256File "research\experiments\r42-profile-verification.json"
      r42_perf_json = Sha256File "research\experiments\r42-performance-results.json"
      r42_stats_json = Sha256File "research\experiments\r42-statistics.json"
    }
    note = "Separate directory clean build on same host. Not third-party. Git HEAD worktree omitted because R4 sources are uncommitted."
  }
  New-Item -ItemType Directory -Force -Path (Join-Path $Root "research\experiments") | Out-Null
  Copy-Item "research\experiments\r42-*.json" (Join-Path $Root "research\experiments\") -Force
  ($manifest | ConvertTo-Json -Depth 5) | Set-Content -Encoding utf8 $ManJson
  if ($r42 -ne 0) { throw "r42 validation failed: $r42" }
  Write-Host "R4.2 separate-dir reproduction finished exit_r42=$r42 exit_r41_smoke=$r41"
} finally {
  Pop-Location
}
