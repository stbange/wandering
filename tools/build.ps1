param([ValidateSet('Build','Test','Run','Coverage','Package')] [string]$Mode='Build')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$cmake=Join-Path $root '.deps/cmake/cmake-4.5.0-windows-x86_64/bin/cmake.exe'
$ctest=Join-Path $root '.deps/cmake/cmake-4.5.0-windows-x86_64/bin/ctest.exe'
if (-not (Test-Path -LiteralPath $cmake)) { throw 'Run tools/bootstrap-windows.ps1 first, or use generic CMake instructions.' }
$originalPath=$env:PATH
$env:PATH=(Join-Path $root '.deps/w64devkit/bin')+';'+$env:PATH
Push-Location $root
try {
    $build=if($Mode -eq 'Coverage'){'build-core'}else{'build'}
    $options=@('-S','.', '-B',$build, '-G','MinGW Makefiles', '-DWC_LUA_SOURCE=.deps/lua-5.4.8', '-DCMAKE_BUILD_TYPE=Debug')
    if($Mode -eq 'Coverage'){$options+=@('-DWC_BUILD_GAME=OFF','-DWC_COVERAGE=ON')}
    else {$options+='-DSDL3_DIR=.deps/sdl/SDL3-3.4.20/x86_64-w64-mingw32/lib/cmake/SDL3'}
    & $cmake @options; if($LASTEXITCODE -ne 0){throw 'Configure failed'}
    & $cmake --build $build --parallel; if($LASTEXITCODE -ne 0){throw 'Build failed'}
    if($Mode -in @('Test','Coverage','Package')) {
        & $ctest --test-dir $build --output-on-failure; if($LASTEXITCODE -ne 0){throw 'Tests failed'}
    }
    if($Mode -eq 'Coverage') { python tools/coverage.py $build --gcov .deps/w64devkit/bin/gcov.exe; if($LASTEXITCODE -ne 0){throw 'Coverage gate failed'} }
    if($Mode -eq 'Run'){ & "$root/build/wandering.exe" }
    if($Mode -eq 'Package') {
        & $cmake --install build --prefix dist/wandering; if($LASTEXITCODE -ne 0){throw 'Install failed'}
        Compress-Archive -Path dist/wandering/* -DestinationPath dist/wandering-0.1.0-Windows.zip -Force
        Write-Host 'Packaged dist/wandering-0.1.0-Windows.zip'
    }
} finally {Pop-Location;$env:PATH=$originalPath}
