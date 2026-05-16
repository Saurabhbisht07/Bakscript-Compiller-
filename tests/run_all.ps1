param(
    [string]$Binary = "bakscript.exe",
    [string]$TestsDir = "main",
    [switch]$IncludeInteractive
)

$ErrorActionPreference = "Stop"

# Resolve paths relative to script location, not caller's current directory.
$scriptDir = $PSScriptRoot
$repoRoot = Split-Path -Path $scriptDir -Parent

if ([System.IO.Path]::IsPathRooted($Binary)) {
    $binaryPath = $Binary
} else {
    $binaryPath = Join-Path $repoRoot $Binary
}

if ([System.IO.Path]::IsPathRooted($TestsDir)) {
    $testsPath = $TestsDir
} else {
    $testsPath = Join-Path $scriptDir $TestsDir
}

if (-not (Test-Path $binaryPath)) {
    Write-Host "Binary not found: $binaryPath" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $testsPath)) {
    Write-Host "Tests directory not found: $testsPath" -ForegroundColor Red
    exit 1
}

$files = Get-ChildItem -Path $testsPath -Filter *.bak | Sort-Object Name
if ($files.Count -eq 0) {
    Write-Host "No .bak files found in $testsPath" -ForegroundColor Yellow
    exit 0
}

$passed = 0
$failed = 0

foreach ($file in $files) {
    if (-not $IncludeInteractive -and $file.Name -eq "user_input.bak") {
        Write-Host ("`n=== " + $file.Name + " ===") -ForegroundColor Cyan
        Write-Host "SKIP (interactive test)" -ForegroundColor Yellow
        continue
    }

    Write-Host ("`n=== " + $file.Name + " ===") -ForegroundColor Cyan
    & $binaryPath $file.FullName
    if ($LASTEXITCODE -eq 0) {
        Write-Host "PASS" -ForegroundColor Green
        $passed++
    } else {
        Write-Host ("FAIL (exit " + $LASTEXITCODE + ")") -ForegroundColor Red
        $failed++
    }
}

Write-Host "`n------------------------"
Write-Host ("Passed: " + $passed)
Write-Host ("Failed: " + $failed)
Write-Host "------------------------"

if ($failed -gt 0) {
    exit 1
}

exit 0
