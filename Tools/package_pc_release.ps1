[CmdletBinding()]
param(
    [switch]$SkipSmoke,
    [string]$OutputDirectory = "release"
)

# KIMIA Windows PC package. Run from PowerShell after installing Visual Studio
# 2022, the Windows SDK, CMake and an SDL2 development package:
#   pwsh Tools/package_pc_release.ps1
# The script deliberately packages only the PC runtime; source and toolchains
# never enter the zip.
$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $root

function Invoke-Checked([string]$File, [string[]]$Arguments) {
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$File failed with exit code $LASTEXITCODE"
    }
}

$versionLine = Select-String -Path (Join-Path $root "Engine/Core/include/kimia/Version.h") `
    -Pattern 'KIMIA_ENGINE_VERSION "([0-9.]+)"'
if ($null -eq $versionLine) { throw "Could not read the KIMIA version" }
$version = $versionLine.Matches[0].Groups[1].Value
$buildDirectory = Join-Path $root "out/build/windows-pc-release"
$releaseRoot = (Resolve-Path (Join-Path $root $OutputDirectory) -ErrorAction SilentlyContinue)
if ($null -eq $releaseRoot) {
    New-Item -ItemType Directory -Path (Join-Path $root $OutputDirectory) | Out-Null
    $releaseRoot = Resolve-Path (Join-Path $root $OutputDirectory)
}
$releaseRoot = $releaseRoot.Path
$packageName = "kimia-pc-$version"
$stage = Join-Path $releaseRoot $packageName
$archive = Join-Path $releaseRoot "$packageName.zip"

Write-Host "==> configuring Visual Studio 2022 x64 D3D11 Release"
Invoke-Checked "cmake" @("--preset", "windows-pc-release")
Write-Host "==> building"
Invoke-Checked "cmake" @("--build", "--preset", "windows-pc-release", "--parallel")

if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
if (Test-Path $archive) { Remove-Item -Force $archive }
New-Item -ItemType Directory -Path $stage | Out-Null

Write-Host "==> installing the staged runtime"
Invoke-Checked "cmake" @("--install", $buildDirectory, "--config", "Release", "--prefix", $stage)
New-Item -ItemType Directory -Force -Path (Join-Path $stage "worlds") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $stage "assets") | Out-Null
# Keep the real project content beside the published executable. World files
# store paths relative to this assets root, including spaces in animation
# folders such as `pleyer move`.
$sourceAssets = Join-Path $root "assets"
if (Test-Path $sourceAssets) {
    Copy-Item -Path (Join-Path $sourceAssets "*") -Destination (Join-Path $stage "assets") -Recurse -Force
}

$world = Join-Path $stage "kimia_world.exe"
$smoke = Join-Path $stage "kimia_pc_d3d11_smoke.exe"
if (!(Test-Path $world)) { throw "Install did not produce kimia_world.exe" }
if (!$SkipSmoke) {
    if (!(Test-Path $smoke)) { throw "Install did not produce kimia_pc_d3d11_smoke.exe" }
    Write-Host "==> running 300-frame D3D11 hardware smoke"
    & $smoke --frames 300 --no-vsync
    if ($LASTEXITCODE -ne 0) { throw "D3D11 smoke failed" }
}

$gitCommit = (& git rev-parse --short HEAD).Trim()
@"
engine  $version
target  Windows x64 / Direct3D 11
commit  $gitCommit
built   $([DateTime]::UtcNow.ToString("o"))
"@ | Set-Content -Encoding UTF8 (Join-Path $stage "VERSION.txt")

$manifest = Join-Path $stage "MANIFEST.txt"
$files = Get-ChildItem -Path $stage -Recurse -File | Sort-Object FullName
$lines = foreach ($file in $files) {
    $relative = $file.FullName.Substring($stage.Length) -replace '^[\\/]+', ''
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash.ToLowerInvariant()
    "$hash  $relative"
}
$lines | Set-Content -Encoding UTF8 $manifest

Write-Host "==> creating $archive"
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $archive -CompressionLevel Optimal
$size = (Get-Item $archive).Length
Write-Host ("OK: {0} ({1:N1} MiB)" -f $archive, ($size / 1MB))
