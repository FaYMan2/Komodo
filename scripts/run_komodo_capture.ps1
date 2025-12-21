param(
    [string]$ExePath = ".\\build\\Release\\komodo.exe",
    [int]$Seconds = 1
)

$resolvedExe = Resolve-Path $ExePath -ErrorAction Stop
$workDir = Split-Path -Parent $resolvedExe.Path

$stdoutPath = Join-Path $workDir "komodo.stdout.txt"
$stderrPath = Join-Path $workDir "komodo.stderr.txt"

Remove-Item $stdoutPath, $stderrPath -ErrorAction SilentlyContinue

$p = Start-Process `
    -FilePath $resolvedExe.Path `
    -WorkingDirectory $workDir `
    -RedirectStandardOutput $stdoutPath `
    -RedirectStandardError $stderrPath `
    -PassThru `
    -NoNewWindow

Start-Sleep -Seconds $Seconds

$alive = -not $p.HasExited
$exitCode = if ($alive) { "" } else { $p.ExitCode }

Write-Host "pid=$($p.Id) alive=$alive exit=$exitCode"
Write-Host "stdout=$stdoutPath"
Write-Host "stderr=$stderrPath"

if (Test-Path $stdoutPath) {
    Write-Host "--- stdout ---"
    Get-Content $stdoutPath
}

if (Test-Path $stderrPath) {
    Write-Host "--- stderr ---"
    Get-Content $stderrPath
}


