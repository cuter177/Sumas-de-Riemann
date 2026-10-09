# Sumas de Riemann

Aplicación de escritorio multiplataforma (**Windows / Linux**) para calcular sumas
de Riemann y visualizar la función, los rectángulos y el resultado.

Arquitectura:

- **Backend C++**: vigila `Interfaz/data/Funcion.json`, calcula la sumatoria y
  genera `Resultado.json`, `datos/Rectangulo.json` y `datos/Datos.json`.
- **Interfaz JavaFX** (`Interfaz/`): entrada de datos y resultado. La lanza el C++.
- **Graficadora Python** (`Graficadora.py`, PyOpenGL/GLUT): dibujo de la función y
  los rectángulos. La lanza el C++.

---

## Requisitos por distro

El C++ y el Python del release solo necesitan las librerías **OpenGL/GLUT** del
sistema (no se empaquetan). Además, para compilar:

| Distro | OpenGL/GLUT | Java 21+ | Python 3 | Otros |
|--------|-------------|----------|----------|-------|
| Debian/Ubuntu | `freeglut3 libglu1-mesa` | `openjdk-21-jre` | `python3` | `build-essential cmake ninja-build maven curl` |
| Fedora | `freeglut mesa-libGLU` | `java-21-openjdk` | `python3` | `gcc-c++ cmake ninja-build maven curl` |
| Arch | `freeglut glu` | `jre-openjdk` | `python` | `base-devel cmake ninja maven curl` |
| openSUSE | `freeglut libGLU1` | `java-21-openjdk` | `python3` | `gcc-c++ cmake ninja maven curl` |

> Para **ejecutar** un release solo hace falta OpenGL/GLUT del sistema; el JRE,
> JavaFX y Python van incluidos dentro del paquete.

---

## Compilar en Linux

```bash
# 1. Dependencias del sistema (ejemplo Debian/Ubuntu)
sudo apt install build-essential cmake ninja-build maven curl \
                 freeglut3 libglu1-mesa

# 2. Compilar C++
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

El binario queda en `bin/Debug/Riemann` (esta ruta se conserva por compatibilidad;
el programa sube desde ahí hasta la raíz del proyecto).

### Ejecutar en desarrollo

El C++ necesita, en la raíz del proyecto, los runtimes `java/`, `javaFx/`,
`Interfaz/target/Interfaz-Riemann.jar` y, para la gráfica, un Python con
`numpy` y `PyOpenGL`. La forma rápida de dejarlo todo listo es:

```bash
./empaquetar.sh --no-tar     # descarga runtimes + compila C++ y Java
./bin/Debug/Riemann
```

Si prefieres usar el Python del sistema, instala `numpy` y `PyOpenGL`
(`pip install -r requirements.txt`) y asegúrate de tener `freeglut` instalado.

---

## Compilar / empaquetar en Windows

```powershell
powershell -ExecutionPolicy Bypass -File .\empaquetar.ps1
```

Genera `Riemann-portable.zip`.

---

## Generar el release Linux

```bash
./empaquetar.sh
```

Genera `Riemann-linux-portable.tar.gz` con:

```
Riemann
Graficadora.py
run.sh
datos/
python/            # Python standalone + numpy + PyOpenGL
java/              # JRE Temurin 21 (linux x64)
javaFx/lib/        # SDK JavaFX 21 (linux x64, jars + .so)
Interfaz/target/Interfaz-Riemann.jar
Interfaz/target/dependency/
Interfaz/data/
```

Opciones útiles: `--skip-cpp`, `--skip-java`, `--skip-download`, `--no-tar`.

Ejecutar el release:

```bash
tar -xzf Riemann-linux-portable.tar.gz -C Riemann
./Riemann/run.sh
```

---

## Integración continua

`.github/workflows/build.yml` compila y publica en cada release:

- `Riemann-windows-<ver>.zip` (job `build-windows`)
- `Riemann-linux-<ver>.tar.gz` (job `build-linux`, compilado en Ubuntu 22.04)

---

## Interfaz gráfica y gestores de ventanas

La ventana JavaFX se adapta al entorno:

- **Escritorios flotantes con compositor** (GNOME, KDE, XFCE, Cinnamon, MATE,
  Unity, Budgie…): ventana sin bordes con barra de título propia; se puede
  arrastrar y con doble clic maximizar, y tiene botones propios de
  minimizar/cerrar.
- **Gestores de mosaico** (i3, sway, Hyprland, bspwm, dwm…) o entornos no
  reconocidos: ventana con **decoración nativa** gestionada por el WM (tileo,
  mover y redimensionar); se ocultan los botones propios.

Se puede forzar el modo con la variable de entorno
`RIEMANN_WINDOW_STYLE=transparent` o `RIEMANN_WINDOW_STYLE=decorated`.

El layout es **responsivo** (`BorderPane`/`VBox`/`HBox`): los campos y el visor
de LaTeX se redimensionan con la ventana, con un tamaño mínimo de 460×420.

---

## Notas de portabilidad

- Todo el código dependiente de plataforma está aislado con `#ifdef _WIN32`
  (`Platform.h`, `PythonManager.cpp`, `JavaManager.cpp`).
- En Linux la biblioteca nativa de JavaFX vive en `javaFx/lib` (junto a los
  `.jar`); en Windows vive en `javaFx/bin` (DLLs). El separador del classpath es
  `:` en Linux y `;` en Windows.
- El empaquetado Linux compila en **Ubuntu 22.04** (glibc 2.35) y enlaza
  `libstdc++`/`libgcc` de forma estática para maximizar compatibilidad con otras
  distros. `libGL`/`freeglut` sí se toman del sistema.
