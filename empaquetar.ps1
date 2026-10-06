<#
.SYNOPSIS
    Compila y empaqueta un release portable de Riemann para Windows.

.DESCRIPTION
    - Descarga (si faltan) los runtimes: Python embebido, JRE Temurin y JavaFX SDK.
    - Compila el ejecutable C++ (CMake + MinGW, enlazado estatico).
    - Compila la interfaz Java (Maven).
    - Ensambla la carpeta de release copiando solo lo necesario.
    - Empaqueta las DLLs que suelen faltar en otros equipos (freeglut para PyOpenGL
      y el runtime VC++), de modo que el release corra en cualquier Windows 10/11.
    - Recorta el Python embebido (quita modulos y paquetes no usados).
    - Genera un .zip portable.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\empaquetar.ps1

.EXAMPLE
    # Solo re-empaquetar sin recompilar ni descargar
    powershell -ExecutionPolicy Bypass -File .\empaquetar.ps1 -SkipCpp -SkipJava -SkipDownload

.EXAMPLE
    # Indicar manualmente una DLL de freeglut (si no esta en el PATH)
    powershell -ExecutionPolicy Bypass -File .\empaquetar.ps1 -FreeGlutDll C:\tools\freeglut.dll
#>
[CmdletBinding()]
param(
    [string]$PythonEmbedDir   = "python-3.13.9-embed-amd64",
    [string]$PythonVersion    = "3.13.9",
    [string]$JavaDir          = "java",
    [string]$JavaFxDir        = "javaFx",
    [string]$JavaFxVersion    = "21.0.9",
    [string]$JavaFeatureVersion = "21",
    [string]$OutputDir        = "release",
    [string]$ZipPath          = "Riemann-portable.zip",
    [string]$CmakeGenerator   = "MinGW Makefiles",
    [string]$PipPython        = "python",
    [string]$Requirements     = "requirements.txt",
    [string]$FreeGlutDll      = "",
    [switch]$SkipCpp,
    [switch]$SkipJava,
    [switch]$SkipTrim,
    [switch]$SkipDownload,
    [switch]$NoZip
)

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
if (-not $root) { $root = (Get-Location).Path }
Set-Location $root

function Info($m) { Write-Host "[*] $m" -ForegroundColor Cyan }
function Ok($m)   { Write-Host "[+] $m" -ForegroundColor Green }
function Warn($m) { Write-Host "[!] $m" -ForegroundColor Yellow }
function Fail($m) { Write-Host "[x] $m" -ForegroundColor Red; exit 1 }
function Require-Path($p) { if (-not (Test-Path $p)) { Fail "No existe: $p" } }
function Require-Tool($name) {
    if (-not (Get-Command $name -ErrorAction SilentlyContinue)) {
        Fail "No se encontro '$name' en el PATH."
    }
}

function Download-File([string]$url, [string]$dest) {
    if (Test-Path $dest) { return }
    Info "Descargando $url"
    New-Item -ItemType Directory -Force -Path (Split-Path $dest) | Out-Null
    $old = $ProgressPreference
    $ProgressPreference = "SilentlyContinue"
    try {
        Invoke-WebRequest -Uri $url -OutFile $dest -UseBasicParsing
    }
    finally {
        $ProgressPreference = $old
    }
    if (-not (Test-Path $dest)) { Fail "Fallo la descarga: $url" }
    Ok "Descargado: $dest"
}

function Expand-ZipClean([string]$zip, [string]$dest) {
    if (Test-Path $dest) { Remove-Item $dest -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    Expand-Archive -LiteralPath $zip -DestinationPath $dest -Force
}

# ---------------------------------------------------------------- Runtimes
function Ensure-PythonEmbed {
    if (Test-Path $PythonEmbedDir) {
        Ok "Python embebido ya presente: $PythonEmbedDir"
        return
    }
    if ($SkipDownload) { Fail "No existe $PythonEmbedDir y -SkipDownload esta activo." }
    $url = "https://www.python.org/ftp/python/$PythonVersion/python-$PythonVersion-embed-amd64.zip"
    $tmp = Join-Path $env:TEMP "python-$PythonVersion-embed-amd64.zip"
    Download-File $url $tmp
    Expand-ZipClean $tmp $PythonEmbedDir
    Ok "Python $PythonVersion extraido en $PythonEmbedDir"
}

function Enable-PythonSite([string]$pyDir) {
    $pth = Get-ChildItem $pyDir -Filter "python*._pth" -File -ErrorAction SilentlyContinue |
           Select-Object -First 1
    $stdlibZip = Get-ChildItem $pyDir -Filter "python*.zip" -File -ErrorAction SilentlyContinue |
                 Select-Object -First 1
    $lines = @()
    if ($stdlibZip) { $lines += $stdlibZip.Name }
    $lines += "."
    $lines += "Lib\site-packages"
    $lines += "import site"

    if ($pth) {
        Set-Content -Path $pth.FullName -Value $lines -Encoding ASCII
        Ok "site-packages habilitado en $($pth.Name)"
    }
    else {
        $name = "python" + ($PythonVersion -replace '\.', '') + "._pth"
        Set-Content -Path (Join-Path $pyDir $name) -Value $lines -Encoding ASCII
        Ok "Creado $name con site-packages"
    }
}

function Install-PythonDeps([string]$pyDir) {
    $site = Join-Path $pyDir "Lib/site-packages"
    New-Item -ItemType Directory -Force -Path $site | Out-Null
    if (-not (Test-Path $Requirements)) {
        Warn "No se encontro '$Requirements'; se omiten las dependencias de Python."
        return
    }
    Info "Instalando dependencias Python (numpy, PyOpenGL) en $site ..."
    & $PipPython -m pip install --disable-pip-version-check --no-warn-script-location `
        --only-binary=:all: --target $site -r $Requirements
    if ($LASTEXITCODE -ne 0) { Fail "Fallo instalando las dependencias de Python." }
    Ok "Dependencias Python instaladas."
}

function Ensure-JavaRuntime {
    if (Test-Path (Join-Path $JavaDir "bin/java.exe")) {
        Ok "JRE ya presente en $JavaDir"
        return
    }
    if ($SkipDownload) { Fail "No existe $JavaDir y -SkipDownload esta activo." }
    $url = "https://api.adoptium.net/v3/binary/latest/$JavaFeatureVersion/ga/windows/x64/jre/hotspot/normal/eclipse"
    $tmp = Join-Path $env:TEMP "temurin-jre-$JavaFeatureVersion.zip"
    Download-File $url $tmp
    $extract = Join-Path $env:TEMP "temurin-jre-$JavaFeatureVersion-extract"
    Expand-ZipClean $tmp $extract
    $inner = Get-ChildItem $extract -Directory | Select-Object -First 1
    if (-not $inner) { Fail "No se pudo extraer el JRE de $tmp" }
    if (Test-Path $JavaDir) { Remove-Item $JavaDir -Recurse -Force }
    Move-Item $inner.FullName $JavaDir
    Ok "JRE Temurin $JavaFeatureVersion instalado en $JavaDir"
}

function Ensure-JavaFx {
    if (Test-Path (Join-Path $JavaFxDir "lib")) {
        Ok "JavaFX ya presente en $JavaFxDir/lib"
        return
    }
    if ($SkipDownload) { Fail "No existe $JavaFxDir y -SkipDownload esta activo." }
    $url = "https://download2.gluonhq.com/openjfx/$JavaFxVersion/openjfx-${JavaFxVersion}_windows-x64_bin-sdk.zip"
    $tmp = Join-Path $env:TEMP "openjfx-$JavaFxVersion-sdk.zip"
    Download-File $url $tmp
    $extract = Join-Path $env:TEMP "openjfx-$JavaFxVersion-extract"
    Expand-ZipClean $tmp $extract
    $inner = Get-ChildItem $extract -Directory | Select-Object -First 1
    if (-not $inner -or -not (Test-Path (Join-Path $inner.FullName "lib"))) {
        Fail "Estructura inesperada del SDK JavaFX en $extract"
    }
    if (Test-Path $JavaFxDir) { Remove-Item $JavaFxDir -Recurse -Force }
    New-Item -ItemType Directory -Force -Path (Join-Path $JavaFxDir "lib") | Out-Null
    Copy-Item (Join-Path $inner.FullName "lib/*") (Join-Path $JavaFxDir "lib") -Recurse -Force
    Ok "JavaFX $JavaFxVersion SDK instalado en $JavaFxDir/lib"
}

# Empaqueta freeglut con el nombre exacto que busca PyOpenGL en Windows x64:
# freeglut64.vc14.dll (ver OpenGL/platform/win32.py). Se copia junto a python.exe
# y en site-packages/OpenGL/DLLS para que PyOpenGL lo encuentre.
function Ensure-FreeGlut([string]$pyDir) {
    $dll = $FreeGlutDll
    $runtimeDir = ""
    if (-not $dll) {
        $gcc = (Get-Command gcc -ErrorAction SilentlyContinue).Source
        if ($gcc) {
            $dir = Split-Path $gcc
            foreach ($pat in @("freeglut.dll", "freeglut*.dll", "*freeglut*.dll")) {
                $cand = Get-ChildItem $dir -Filter $pat -File -ErrorAction SilentlyContinue |
                        Select-Object -First 1
                if ($cand) { $dll = $cand.FullName; $runtimeDir = $dir; break }
            }
        }
    }
    if (-not $dll) {
        foreach ($p in ($env:PATH -split ';')) {
            if ([string]::IsNullOrWhiteSpace($p)) { continue }
            $cand = Get-ChildItem $p -Filter "*freeglut*.dll" -File -ErrorAction SilentlyContinue |
                    Select-Object -First 1
            if ($cand) { $dll = $cand.FullName; $runtimeDir = $p; break }
        }
    }
    if (-not $dll -or -not (Test-Path $dll)) {
        Warn "No se encontro freeglut.dll. Graficadora.py puede fallar con 'DLL load failed'. Usa -FreeGlutDll <ruta>."
        return
    }
    $dllsDir = Join-Path $pyDir "Lib/site-packages/OpenGL/DLLS"
    New-Item -ItemType Directory -Force -Path $dllsDir | Out-Null
    foreach ($name in @("freeglut64.vc14.dll", "freeglut.dll", "glut32.dll")) {
        Copy-Item $dll (Join-Path $dllsDir $name) -Force
        Copy-Item $dll (Join-Path $pyDir $name) -Force
    }
    if ($runtimeDir) {
        foreach ($rt in @("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")) {
            $rp = Join-Path $runtimeDir $rt
            if (Test-Path $rp) {
                Copy-Item $rp (Join-Path $pyDir $rt) -Force
                Copy-Item $rp (Join-Path $dllsDir $rt) -Force
            }
        }
    }
    Ok "freeglut empaquetado como freeglut64.vc14.dll"
}

# Copia el runtime VC++ (msvcp140/vcruntime140) a la raiz del release. Python y
# Java suelen traerlo; si no, se toma de System32. Evita "DLL load failed" en
# equipos sin Visual C++ Redistributable instalado.
function Bundle-VCRuntime([string]$outDir) {
    $names = @(
        "msvcp140.dll", "msvcp140_1.dll", "msvcp140_2.dll",
        "vcruntime140.dll", "vcruntime140_1.dll", "concrt140.dll"
    )
    $sources = @(
        (Join-Path $outDir (Split-Path $PythonEmbedDir -Leaf)),
        (Join-Path (Join-Path $outDir $JavaDir) "bin"),
        (Join-Path $env:SystemRoot "System32")
    )
    $copied = 0
    foreach ($n in $names) {
        if (Test-Path (Join-Path $outDir $n)) { continue }
        foreach ($s in $sources) {
            $p = Join-Path $s $n
            if (Test-Path $p) {
                Copy-Item $p (Join-Path $outDir $n) -Force
                $copied++
                break
            }
        }
    }
    Ok "Runtime VC++ empaquetado ($copied DLLs)."
}

function Trim-Python([string]$pyDir) {
    Info "Recortando Python embebido en '$pyDir' ..."

    # 1) Ejecutables y DLLs de modulos que no se usan
    $files = @(
        "pythonw.exe",
        "_tkinter.pyd", "tcl86t.dll", "tk86t.dll",
        "_sqlite3.pyd", "sqlite3.dll",
        "_ssl.pyd", "_hashlib.pyd", "libcrypto-3-x64.dll", "libssl-3-x64.dll",
        "_lzma.pyd", "_bz2.pyd", "_decimal.pyd",
        "winsound.pyd", "_msi.pyd", "_overlapped.pyd"
    )
    foreach ($f in $files) {
        $p = Join-Path $pyDir $f
        if (Test-Path $p) { Remove-Item $p -Force -ErrorAction SilentlyContinue }
    }
    Get-ChildItem $pyDir -Filter "*.pdb" -File -ErrorAction SilentlyContinue |
        Remove-Item -Force -ErrorAction SilentlyContinue

    # 2) Carpetas del stdlib extraido
    $libDirs = @(
        "test", "tests", "tkinter", "tcl", "idlelib", "lib2to3", "distutils",
        "ensurepip", "unittest", "asyncio", "multiprocessing", "concurrent",
        "sqlite3", "xmlrpc", "pydoc_data", "curses", "turtledemo", "venv",
        "site-packages\pip", "site-packages\setuptools", "site-packages\pkg_resources",
        "site-packages\wheel"
    )
    foreach ($d in $libDirs) {
        $p = Join-Path $pyDir "Lib/$d"
        if (Test-Path $p) { Remove-Item $p -Recurse -Force -ErrorAction SilentlyContinue }
    }
    Get-ChildItem (Join-Path $pyDir "Lib/site-packages") -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^(pip|setuptools|wheel|pkg_resources)' } |
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue

    # 3) __pycache__, .pyc y metadatos (miles de archivos pequenos)
    Get-ChildItem $pyDir -Recurse -Directory -Filter "__pycache__" -ErrorAction SilentlyContinue |
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
    Get-ChildItem $pyDir -Recurse -Directory -Filter "*.dist-info" -ErrorAction SilentlyContinue |
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
    Get-ChildItem $pyDir -Recurse -File -Include "*.pyc", "*.pyo" -ErrorAction SilentlyContinue |
        Remove-Item -Force -ErrorAction SilentlyContinue

    # 4) Reconstruir python3xx.zip quitando modulos pesados
    $zip = Get-ChildItem $pyDir -Filter "python*.zip" -File -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($zip) {
        try {
            Add-Type -AssemblyName System.IO.Compression
            Add-Type -AssemblyName System.IO.Compression.FileSystem

            $exclude = @(
                "test/", "tests/", "ensurepip/", "idlelib/", "lib2to3/",
                "distutils/", "tkinter/", "unittest/", "turtledemo/", "venv/",
                "__pycache__/"
            )
            $tmp = "$($zip.FullName).tmp"
            if (Test-Path $tmp) { Remove-Item $tmp -Force }

            $src = [System.IO.Compression.ZipFile]::OpenRead($zip.FullName)
            $dst = [System.IO.Compression.ZipFile]::Open($tmp, [System.IO.Compression.ZipArchiveMode]::Create)
            $kept = 0; $skipped = 0
            foreach ($e in $src.Entries) {
                $name = $e.FullName -replace '\\', '/'
                $skip = $false
                foreach ($p in $exclude) { if ($name.StartsWith($p)) { $skip = $true; break } }
                if ($skip) { $skipped++; continue }
                $ne = $dst.CreateEntry($e.FullName, [System.IO.Compression.CompressionLevel]::Optimal)
                $is = $e.Open(); $os = $ne.Open()
                $is.CopyTo($os); $os.Dispose(); $is.Dispose()
                $kept++
            }
            $dst.Dispose(); $src.Dispose()
            Move-Item $tmp $zip.FullName -Force
            Ok "python zip: $kept entradas conservadas, $skipped eliminadas"
        }
        catch {
            Warn "No se pudo recortar $($zip.Name): $($_.Exception.Message)"
        }
    }

    Ok "Python recortado."
}

# ---------------------------------------------------------------- Runtimes
Ensure-PythonEmbed
Enable-PythonSite $PythonEmbedDir
Install-PythonDeps $PythonEmbedDir
Ensure-FreeGlut $PythonEmbedDir
Ensure-JavaRuntime
Ensure-JavaFx

# ---------------------------------------------------------------- Compilar C++
if (-not $SkipCpp) {
    Require-Tool "cmake"
    Info "Compilando C++ (CMake, Release, enlazado estatico)..."
    if (Test-Path "build") { Remove-Item "build" -Recurse -Force }
    cmake -S . -B build -G $CmakeGenerator -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) { Fail "Fallo la configuracion de CMake (generador: $CmakeGenerator)." }
    cmake --build build --config Release
    if ($LASTEXITCODE -ne 0) { Fail "Fallo la compilacion de CMake." }
}
else {
    Warn "Compilacion C++ omitida (-SkipCpp)."
}

$exe = Get-ChildItem -Path "bin" -Recurse -Filter "Riemann.exe" -File -ErrorAction SilentlyContinue |
       Select-Object -First 1
if (-not $exe) { Fail "No se encontro Riemann.exe. Compila primero o revisa la carpeta bin/." }
Ok "Ejecutable: $($exe.FullName)"

# ---------------------------------------------------------------- Compilar Java
if (-not $SkipJava) {
    Require-Tool "mvn"
    Info "Compilando Interfaz Java (Maven)..."
    mvn -f Interfaz/pom.xml clean package
    if ($LASTEXITCODE -ne 0) { Fail "Fallo 'mvn package'." }
}
else {
    Warn "Compilacion Java omitida (-SkipJava)."
}

$jar = Get-ChildItem "Interfaz/target" -Filter "Interfaz*.jar" -File -ErrorAction SilentlyContinue |
       Select-Object -First 1
if (-not $jar) { Fail "No se encontro el jar de la interfaz en Interfaz/target." }
$depDir = "Interfaz/target/dependency"
if (-not (Test-Path $depDir)) { Warn "No existe $depDir; el jar se copiara sin dependencias." }

# ---------------------------------------------------------------- Ensamblar
Info "Ensamblando release en '$OutputDir' ..."
if (Test-Path $OutputDir) { Remove-Item $OutputDir -Recurse -Force }
New-Item -ItemType Directory -Path $OutputDir | Out-Null

Copy-Item $exe.FullName (Join-Path $OutputDir "Riemann.exe") -Force
Copy-Item "Graficadora.py" (Join-Path $OutputDir "Graficadora.py") -Force

Require-Path $PythonEmbedDir
Copy-Item $PythonEmbedDir (Join-Path $OutputDir (Split-Path $PythonEmbedDir -Leaf)) -Recurse -Force

Require-Path $JavaDir
Copy-Item $JavaDir (Join-Path $OutputDir (Split-Path $JavaDir -Leaf)) -Recurse -Force

Require-Path $JavaFxDir
Copy-Item $JavaFxDir (Join-Path $OutputDir (Split-Path $JavaFxDir -Leaf)) -Recurse -Force

# Interfaz: solo jar + dependencias + datos de entrada
$targetOut = Join-Path $OutputDir "Interfaz/target"
$depOut    = Join-Path $targetOut "dependency"
New-Item -ItemType Directory -Force -Path $depOut | Out-Null
Copy-Item $jar.FullName (Join-Path $targetOut $jar.Name) -Force
if (Test-Path $depDir) {
    Copy-Item "$depDir\*" $depOut -Recurse -Force -ErrorAction SilentlyContinue
}
$dataOut = Join-Path $OutputDir "Interfaz/data"
New-Item -ItemType Directory -Force -Path $dataOut | Out-Null
foreach ($seed in @("Interfaz/data/Flag.json", "Interfaz/Funcion.json")) {
    if (Test-Path $seed) { Copy-Item $seed $dataOut -Force }
}

# Carpeta de datos en tiempo de ejecucion
New-Item -ItemType Directory -Force -Path (Join-Path $OutputDir "datos") | Out-Null

# ---------------------------------------------------------------- DLLs portables
$outEmbed = Join-Path $OutputDir (Split-Path $PythonEmbedDir -Leaf)
Bundle-VCRuntime $OutputDir

# ---------------------------------------------------------------- Recortar Python
if (-not $SkipTrim) {
    Trim-Python $outEmbed
}
else {
    Warn "Recorte de Python omitido (-SkipTrim)."
}

# ---------------------------------------------------------------- Verificacion
Info "Verificando archivos clave del release..."
$checks = @(
    "Riemann.exe",
    "Graficadora.py",
    "$(Split-Path $PythonEmbedDir -Leaf)/python.exe",
    "$(Split-Path $JavaDir -Leaf)/bin/java.exe",
    "$(Split-Path $JavaFxDir -Leaf)/lib",
    "$(Split-Path $PythonEmbedDir -Leaf)/Lib/site-packages/OpenGL",
    "$(Split-Path $PythonEmbedDir -Leaf)/Lib/site-packages/numpy",
    "$(Split-Path $PythonEmbedDir -Leaf)/Lib/site-packages/OpenGL/DLLS/freeglut64.vc14.dll"
)
$missing = @()
foreach ($c in $checks) {
    $full = Join-Path $OutputDir $c
    if (Test-Path $full) { Ok "OK  $c" } else { $missing += $c; Warn "FALTA  $c" }
}
if ($missing.Count -gt 0) {
    Warn "Faltan $($missing.Count) archivos del release. Revisa los avisos de arriba."
}

# ---------------------------------------------------------------- Comprimir
if (-not $NoZip) {
    if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }
    Info "Comprimiendo '$OutputDir' -> '$ZipPath' ..."
    Compress-Archive -Path (Join-Path $OutputDir "*") -DestinationPath $ZipPath -CompressionLevel Optimal -Force
    $size = "{0:N1} MB" -f ((Get-Item $ZipPath).Length / 1MB)
    Ok "Release portable generado: $ZipPath ($size)"
}
else {
    Ok "Release generado en '$OutputDir' (sin zip)."
}

Ok "Listo."
