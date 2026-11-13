$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$deps = Join-Path $root '.deps'
New-Item -ItemType Directory -Path $deps -Force | Out-Null
$downloads = @(
    @('https://github.com/skeeto/w64devkit/releases/download/v2.11.0/w64devkit-x64-2.11.0.7z.exe', 'w64devkit-x64-2.11.0.7z.exe', '54c2bb9962e8a30c8270e34c5efc3d5fee579f53087fa7d435b95fbba2af7b2b'),
    @('https://github.com/libsdl-org/SDL/releases/download/release-3.4.20/SDL3-devel-3.4.20-mingw.zip', 'SDL3-devel-3.4.20-mingw.zip', '5d321d3d2d499cfabad3023921e4b56bbec03558060f4e195ce626d200126442'),
    @('https://github.com/Kitware/CMake/releases/download/v4.5.0/cmake-4.5.0-windows-x86_64.zip', 'cmake-4.5.0-windows-x86_64.zip', '301b1728317ccd84e984b02e366a8293735bc5ac618757d3d8f58b578f062411'),
    @('https://www.lua.org/ftp/lua-5.4.8.tar.gz', 'lua-5.4.8.tar.gz', '4f18ddae154e793e46eeab727c59ef1c0c0c2b744e7b94219710d76f530629ae')
)
foreach ($entry in $downloads) {
    $file = Join-Path $deps $entry[1]
    if (-not (Test-Path -LiteralPath $file)) { Invoke-WebRequest -Uri $entry[0] -OutFile $file }
    if ((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry[2]) { throw "Checksum mismatch: $file" }
}
$seven = Get-Command 7z -ErrorAction SilentlyContinue
if (-not $seven) { $seven = Get-Item 'C:\Program Files\7-Zip\7z.exe' -ErrorAction SilentlyContinue }
if (-not (Test-Path (Join-Path $deps 'w64devkit/bin/gcc.exe'))) {
    if (-not $seven) { throw 'Install 7-Zip or extract the verified w64devkit archive into .deps/w64devkit.' }
    & $seven.Source x (Join-Path $deps $downloads[0][1]) "-o$deps" -y -bso0 -bsp0
    if ($LASTEXITCODE -ne 0) { throw 'Compiler extraction failed' }
}
if (-not (Test-Path (Join-Path $deps 'sdl/SDL3-3.4.20'))) { Expand-Archive -LiteralPath (Join-Path $deps $downloads[1][1]) -DestinationPath (Join-Path $deps 'sdl') }
if (-not (Test-Path (Join-Path $deps 'cmake/cmake-4.5.0-windows-x86_64'))) { Expand-Archive -LiteralPath (Join-Path $deps $downloads[2][1]) -DestinationPath (Join-Path $deps 'cmake') }
if (-not (Test-Path (Join-Path $deps 'lua-5.4.8'))) { tar -xzf (Join-Path $deps $downloads[3][1]) -C $deps; if ($LASTEXITCODE -ne 0) { throw 'Lua extraction failed' } }
Write-Host 'Portable toolchain ready. Run ./tools/build.ps1 -Mode Test'
