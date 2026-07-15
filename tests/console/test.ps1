# Run all .lg tests in the tests directory and compare output to .expected files.
#
# Usage (from this directory):
#   powershell -ExecutionPolicy Bypass -File test.ps1

$ErrorActionPreference = 'Stop'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$testsDir = Join-Path $scriptDir 'tests'
Set-Location $testsDir

$logoName = 'qlogo.exe'
$candidates = @(
    (Join-Path $scriptDir '..\..\qlogo.exe'),
    (Join-Path $scriptDir '..\..\qlogo\qlogo.exe'),
    (Join-Path $scriptDir '..\..\qlogo\qlogo'),
    (Join-Path $scriptDir '..\..\qlogo\qlogo.exe')
)

$logoPath = $null
foreach ($c in $candidates) {
    $resolved = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($c)
    if (Test-Path -LiteralPath $resolved -PathType Leaf) {
        $logoPath = (Resolve-Path -LiteralPath $resolved).Path
        break
    }
}

if (-not $logoPath) {
    $cmd = Get-Command $logoName -ErrorAction SilentlyContinue
    if (-not $cmd) {
        $cmd = Get-Command 'qlogo' -ErrorAction SilentlyContinue
    }
    if ($cmd) {
        $logoPath = $cmd.Source
    }
}

if (-not $logoPath) {
    Write-Host "Error: could not find executable 'qlogo'."
    exit 1
}

function Normalize-Text([string]$text) {
    return (($text -replace "`r`n", "`n") -replace "`r", "`n")
}

$failed = New-Object System.Collections.Generic.List[string]
$testCount = 0

Get-ChildItem -LiteralPath $testsDir -Filter '*.lg' -File | Sort-Object Name | ForEach-Object {
    $lg = $_.FullName
    $name = $_.Name
    $expectedPath = [IO.Path]::ChangeExtension($lg, '.expected')

    Write-Host $name
    $testCount++

    if (-not (Test-Path -LiteralPath $expectedPath)) {
        Write-Host "  missing expected file: $([IO.Path]::GetFileName($expectedPath))"
        $failed.Add($name) | Out-Null
        return
    }

    $tmpOut = [IO.Path]::GetTempFileName()
    try {
        $argList = "/c `"`"$logoPath`" < `"$lg`" > `"$tmpOut`" 2>&1`""
        $proc = Start-Process -FilePath 'cmd.exe' -ArgumentList $argList -Wait -PassThru -NoNewWindow
        $actual = Normalize-Text ([IO.File]::ReadAllText($tmpOut))
        $expected = Normalize-Text ([IO.File]::ReadAllText($expectedPath))

        if ($actual -ne $expected) {
            # Show a brief diff-like hint (first differing region).
            $aLines = $actual -split "`n", -1
            $eLines = $expected -split "`n", -1
            $max = [Math]::Max($aLines.Length, $eLines.Length)
            for ($i = 0; $i -lt $max; $i++) {
                $a = if ($i -lt $aLines.Length) { $aLines[$i] } else { '<missing>' }
                $e = if ($i -lt $eLines.Length) { $eLines[$i] } else { '<missing>' }
                if ($a -ne $e) {
                    Write-Host "  differ at line $($i + 1):"
                    Write-Host "  - $e"
                    Write-Host "  + $a"
                    break
                }
            }
            $failed.Add($name) | Out-Null
        }
    }
    finally {
        Remove-Item -LiteralPath $tmpOut -ErrorAction SilentlyContinue
    }
}

if ($failed.Count -gt 0) {
    Write-Host ''
    Write-Host '============================'
    Write-Host '==== FAILED TESTS:'
    Write-Host '===='
    foreach ($f in $failed) {
        Write-Host "==== $f"
        Write-Host '===='
    }
    Write-Host '============================'
}

Write-Host "$testCount tests."

if ($failed.Count -gt 0) {
    exit 1
}
exit 0
