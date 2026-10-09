#!/usr/bin/env bash
#
# empaquetar.sh - Compila y empaqueta un release portable de Riemann para Linux.
#
# Equivalente en Linux a empaquetar.ps1 (Windows).
#
#   - Descarga (si faltan) los runtimes: Python standalone, JRE Temurin y JavaFX SDK.
#   - Compila el ejecutable C++ (CMake).
#   - Compila la interfaz Java (Maven).
#   - Ensambla la carpeta de release copiando solo lo necesario.
#   - Genera un .tar.gz portable.
#
# OpenGL/freeglut NO se empaquetan: se toman del sistema. Instalarlos segun distro:
#   Debian/Ubuntu: sudo apt install freeglut3 libglu1-mesa
#   Fedora:        sudo dnf install freeglut mesa-libGLU
#   Arch:          sudo pacman -S freeglut glu
#
# Uso:
#   ./empaquetar.sh
#   ./empaquetar.sh --skip-cpp --skip-java --skip-download
#
set -euo pipefail

# --------------------------------------------------------------------- Config
PYTHON_VERSION="3.13"
JAVA_FEATURE="21"
JAVAFX_VERSION="21.0.9"
OUTPUT_DIR="release"
TARBALL="Riemann-linux-portable.tar.gz"
REQUIREMENTS="requirements.txt"
PYDIR="python"
JAVADIR="java"
JAVAFXDIR="javaFx"

SKIP_CPP=0
SKIP_JAVA=0
SKIP_DOWNLOAD=0
NO_TAR=0

for arg in "$@"; do
    case "$arg" in
        --skip-cpp)       SKIP_CPP=1 ;;
        --skip-java)      SKIP_JAVA=1 ;;
        --skip-download)  SKIP_DOWNLOAD=1 ;;
        --no-tar)         NO_TAR=1 ;;
        -h|--help)
            sed -n '3,30p' "$0" | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *) echo "Argumento desconocido: $arg" >&2; exit 2 ;;
    esac
done

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

# --------------------------------------------------------------------- Logs
if [ -t 1 ]; then
    C_INFO=$'\033[36m'; C_OK=$'\033[32m'; C_WARN=$'\033[33m'; C_ERR=$'\033[31m'; C_OFF=$'\033[0m'
else
    C_INFO=""; C_OK=""; C_WARN=""; C_ERR=""; C_OFF=""
fi
info() { echo "${C_INFO}[*]${C_OFF} $*"; }
ok()   { echo "${C_OK}[+]${C_OFF} $*"; }
warn() { echo "${C_WARN}[!]${C_OFF} $*"; }
fail() { echo "${C_ERR}[x]${C_OFF} $*" >&2; exit 1; }

require_tool() {
    command -v "$1" >/dev/null 2>&1 || fail "No se encontró '$1' en el PATH."
}

# --------------------------------------------------------------------- Descargas
download_file() {
    local url="$1" dest="$2"
    if [ -f "$dest" ]; then return 0; fi
    info "Descargando $url"
    mkdir -p "$(dirname "$dest")"
    curl -fL --retry 3 --retry-delay 2 -o "$dest" "$url" || fail "Falló la descarga: $url"
    ok "Descargado: $dest"
}

extract_zip() {
    local zip="$1" dest="$2"
    rm -rf "$dest"; mkdir -p "$dest"
    if command -v unzip >/dev/null 2>&1; then
        unzip -q "$zip" -d "$dest"
    elif command -v bsdtar >/dev/null 2>&1; then
        bsdtar -xf "$zip" -C "$dest"
    else
        python3 -m zipfile -e "$zip" "$dest"
    fi
}

extract_tar() {
    local tar="$1" dest="$2"
    rm -rf "$dest"; mkdir -p "$dest"
    tar -xf "$tar" -C "$dest"
}

# --------------------------------------------------------------------- Python
ensure_python() {
    if [ -x "$PYDIR/bin/python3" ]; then
        ok "Python standalone ya presente: $PYDIR"
        return 0
    fi
    if [ "$SKIP_DOWNLOAD" -eq 1 ]; then fail "No existe $PYDIR y --skip-download está activo."; fi

    info "Buscando Python standalone (python-build-standalone)..."
    local api="https://api.github.com/repos/astral-sh/python-build-standalone/releases/latest"
    local json url
    json="$(curl -fsSL "$api")" || fail "No se pudo consultar la API de GitHub."
    url="$(printf '%s' "$json" \
        | grep -oE '"browser_download_url": *"[^"]*cpython-'"$PYTHON_VERSION"'\.[^"]*x86_64-unknown-linux-gnu-install_only\.tar\.gz"' \
        | head -n1 | sed -E 's/.*"(https[^"]+)".*/\1/')"
    if [ -z "$url" ]; then
        url="$(printf '%s' "$json" \
            | grep -oE '"browser_download_url": *"[^"]*x86_64-unknown-linux-gnu-install_only\.tar\.gz"' \
            | head -n1 | sed -E 's/.*"(https[^"]+)".*/\1/')"
    fi
    [ -n "$url" ] || fail "No se pudo obtener la URL del Python standalone."

    local tmp; tmp="$(mktemp -d)"
    download_file "$url" "$tmp/python.tar.gz"
    info "Extrayendo Python standalone..."
    extract_tar "$tmp/python.tar.gz" "$tmp/x"
    rm -rf "$PYDIR"
    mv "$tmp/x/python" "$PYDIR"
    rm -rf "$tmp"
    ok "Python standalone instalado en $PYDIR"
}

install_python_deps() {
    [ -f "$REQUIREMENTS" ] || { warn "No se encontró '$REQUIREMENTS'; se omiten dependencias Python."; return 0; }
    info "Instalando dependencias Python (numpy, PyOpenGL) en $PYDIR ..."
    "$PYDIR/bin/python3" -m pip install --disable-pip-version-check --no-warn-script-location \
        --only-binary=:all: -r "$REQUIREMENTS" \
        || "$PYDIR/bin/python3" -m pip install --disable-pip-version-check --no-warn-script-location \
             -r "$REQUIREMENTS"
    ok "Dependencias Python instaladas."
}

# --------------------------------------------------------------------- Java
ensure_java() {
    if [ -x "$JAVADIR/bin/java" ]; then
        ok "JRE ya presente en $JAVADIR"
        return 0
    fi
    if [ "$SKIP_DOWNLOAD" -eq 1 ]; then fail "No existe $JAVADIR y --skip-download está activo."; fi
    local url="https://api.adoptium.net/v3/binary/latest/$JAVA_FEATURE/ga/linux/x64/jre/hotspot/normal/eclipse"
    local tmp; tmp="$(mktemp -d)"
    download_file "$url" "$tmp/jre.tar.gz"
    extract_tar "$tmp/jre.tar.gz" "$tmp/x"
    local inner; inner="$(find "$tmp/x" -maxdepth 1 -mindepth 1 -type d | head -n1)"
    [ -n "$inner" ] || fail "No se pudo extraer el JRE."
    rm -rf "$JAVADIR"
    mv "$inner" "$JAVADIR"
    rm -rf "$tmp"
    ok "JRE Temurin $JAVA_FEATURE instalado en $JAVADIR"
}

ensure_javafx() {
    if [ -d "$JAVAFXDIR/lib" ]; then
        ok "JavaFX ya presente en $JAVAFXDIR/lib"
        return 0
    fi
    if [ "$SKIP_DOWNLOAD" -eq 1 ]; then fail "No existe $JAVAFXDIR/lib y --skip-download está activo."; fi
    local url="https://download2.gluonhq.com/openjfx/$JAVAFX_VERSION/openjfx-${JAVAFX_VERSION}_linux-x64_bin-sdk.zip"
    local tmp; tmp="$(mktemp -d)"
    download_file "$url" "$tmp/javafx.zip"
    extract_zip "$tmp/javafx.zip" "$tmp/x"
    local inner; inner="$(find "$tmp/x" -maxdepth 1 -mindepth 1 -type d | head -n1)"
    [ -d "$inner/lib" ] || fail "Estructura inesperada del SDK JavaFX."
    rm -rf "$JAVAFXDIR"; mkdir -p "$JAVAFXDIR"
    cp -a "$inner/lib" "$JAVAFXDIR/lib"
    rm -rf "$tmp"
    ok "JavaFX $JAVAFX_VERSION SDK instalado en $JAVAFXDIR/lib"
}

# --------------------------------------------------------------------- C++
build_cpp() {
    if [ "$SKIP_CPP" -eq 1 ]; then warn "Compilación C++ omitida (--skip-cpp)."; return 0; fi
    require_tool cmake
    info "Compilando C++ (CMake, Release)..."
    local gen=()
    if command -v ninja >/dev/null 2>&1; then gen=(-G Ninja); fi
    rm -rf build
    cmake -S . -B build "${gen[@]}" -DCMAKE_BUILD_TYPE=Release >/dev/null
    cmake --build build --config Release -j"$(nproc)"
    ok "C++ compilado."
}

# --------------------------------------------------------------------- Java build
build_java() {
    if [ "$SKIP_JAVA" -eq 1 ]; then warn "Compilación Java omitida (--skip-java)."; return 0; fi
    require_tool mvn
    info "Compilando Interfaz Java (Maven)..."
    mvn -q -f Interfaz/pom.xml clean package
    ok "Interfaz Java compilada."
}

# --------------------------------------------------------------------- Runtimes
ensure_python
install_python_deps
ensure_java
ensure_javafx

# --------------------------------------------------------------------- Compilar
build_cpp
build_java

EXE=""
for cand in bin/Debug/Riemann bin/Riemann build/Riemann; do
    if [ -f "$cand" ]; then EXE="$cand"; break; fi
done
[ -n "$EXE" ] || EXE="$(find bin build -type f -name Riemann 2>/dev/null | head -n1)"
[ -n "$EXE" ] && [ -f "$EXE" ] || fail "No se encontró el ejecutable Riemann. Compila primero."
ok "Ejecutable: $EXE"

JAR="$(find Interfaz/target -maxdepth 1 -name 'Interfaz*.jar' -type f 2>/dev/null | head -n1)"
[ -n "$JAR" ] || fail "No se encontró el jar de la interfaz en Interfaz/target."
DEPDIR="Interfaz/target/dependency"
[ -d "$DEPDIR" ] || warn "No existe $DEPDIR; el jar se copiará sin dependencias."

# --------------------------------------------------------------------- Ensamblar
info "Ensamblando release en '$OUTPUT_DIR' ..."
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

cp "$EXE" "$OUTPUT_DIR/Riemann"
cp Graficadora.py "$OUTPUT_DIR/Graficadora.py"

[ -d "$PYDIR" ]     || fail "Falta $PYDIR"
[ -d "$JAVADIR" ]   || fail "Falta $JAVADIR"
[ -d "$JAVAFXDIR" ] || fail "Falta $JAVAFXDIR"
cp -a "$PYDIR" "$OUTPUT_DIR/"
cp -a "$JAVADIR" "$OUTPUT_DIR/"
cp -a "$JAVAFXDIR" "$OUTPUT_DIR/"

# Interfaz: jar + dependencias + datos de entrada
TARGET_OUT="$OUTPUT_DIR/Interfaz/target"
DEP_OUT="$TARGET_OUT/dependency"
mkdir -p "$DEP_OUT"
cp "$JAR" "$TARGET_OUT/"
if [ -d "$DEPDIR" ]; then cp -a "$DEPDIR/." "$DEP_OUT/"; fi
DATA_OUT="$OUTPUT_DIR/Interfaz/data"
mkdir -p "$DATA_OUT"
if [ -f "Interfaz/data/Flag.json" ]; then cp "Interfaz/data/Flag.json" "$DATA_OUT/"; fi
if [ -f "Interfaz/Funcion.json" ];   then cp "Interfaz/Funcion.json"   "$DATA_OUT/"; fi

mkdir -p "$OUTPUT_DIR/datos"

# Lanzador de conveniencia
cat > "$OUTPUT_DIR/run.sh" <<'EOF'
#!/usr/bin/env bash
cd "$(dirname "$(readlink -f "$0")")" || exit 1
exec ./Riemann
EOF
chmod +x "$OUTPUT_DIR/run.sh"

# --------------------------------------------------------------------- Empaquetar
if [ "$NO_TAR" -eq 0 ]; then
    rm -f "$TARBALL"
    info "Comprimiendo '$OUTPUT_DIR' -> '$TARBALL' ..."
    tar -czf "$TARBALL" -C "$OUTPUT_DIR" .
    SIZE="$(du -h "$TARBALL" | cut -f1)"
    ok "Release portable generado: $TARBALL ($SIZE)"
else
    ok "Release generado en '$OUTPUT_DIR' (sin tar)."
fi

ok "Listo."
