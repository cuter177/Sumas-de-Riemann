import json
import threading
import time

from OpenGL.GL import *
from OpenGL.GLUT import *
from OpenGL.GLU import *
import numpy as np

# Lista de puntos para graficar la función (si se tiene)
puntos = []

# Lista que almacenará los vértices de cada rectángulo
rectangulos = []

# Variables para zoom y desplazamiento (pan)
zoom = 19.19434249577509
pan_x, pan_y = 0.0, 0.0

# Variables para el manejo del mouse (para pan)
mouse_pressed = False
mouse_x, mouse_y = 0, 0

<<<<<<< HEAD
# Obtener la carpeta raíz del proyecto (donde está este script)
RAIZ_DIR = os.path.dirname(os.path.abspath(__file__))
DATOS_DIR = os.path.join(RAIZ_DIR, "datos")

def guardar_parametros():
    """Guarda los parámetros de zoom y pan en un archivo JSON."""
=======
def guardar_parametros():
    """ Guarda los parámetros de zoom y pan en un archivo JSON. """
>>>>>>> 1f47153fc388540a12b13737798de23ae6ed20ee
    parametros = {
        "zoom": zoom,
        "pan_x": pan_x,
        "pan_y": pan_y
    }
<<<<<<< HEAD
    ruta_parametros = os.path.join(DATOS_DIR, "Parametros.json")
    with open(ruta_parametros, "w") as file:
=======
    with open("C:\\Users\\Pop90\\Documents\\Riemann_4.1\\datos\\Parametros.json", "w") as file:
>>>>>>> 1f47153fc388540a12b13737798de23ae6ed20ee
        json.dump(parametros, file)

def cargar_rectangulos_json():
    """
    Carga el JSON de rectángulos y guarda, para cada uno, la lista de vértices.
    Se espera que cada rectángulo tenga una clave "vertices" con 4 subarreglos.
    """
    global rectangulos
    ruta_rectangulos = os.path.join(DATOS_DIR, "Rectangulo.json")
    try:
        with open(ruta_rectangulos, "r") as f:
            data = json.load(f)
            for rect in data["rectangulos"]:
                vertices = rect.get("vertices", None)
                if vertices is not None:
                    rectangulos.append(vertices)
        print("Cantidad de rectángulos cargados:", len(rectangulos))
    except Exception as e:
        print(f"Error al cargar 'Rectangulo.json': {e}")

def cargar_datos_funcion():
<<<<<<< HEAD
    """Carga la lista de puntos de la función desde Datos.json"""
    global puntos
    ruta_datos = os.path.join(DATOS_DIR, "Datos.json")
=======
    global puntos
    file_path = r'C:\Users\Pop90\Documents\Riemann_4.1\datos\Datos.json'
>>>>>>> 1f47153fc388540a12b13737798de23ae6ed20ee
    max_attempts = 5
    attempt = 0
    while attempt < max_attempts:
        try:
<<<<<<< HEAD
            with open(ruta_datos, 'r') as file:
=======
            with open(file_path, 'r') as file:
>>>>>>> 1f47153fc388540a12b13737798de23ae6ed20ee
                content = file.read().strip()
                if not content:
                    raise ValueError("Empty file")
                datos = json.loads(content)
                if "puntos" not in datos or not isinstance(datos["puntos"], list):
                    raise ValueError("Malformed JSON or missing 'puntos'")
                puntos = [(p["x"], p["y"]) for p in datos["puntos"]]
                return
        except Exception as e:
<<<<<<< HEAD
            time.sleep(0.1)
            attempt += 1
=======
            print(f"Error loading Datos.json: {e}")
            time.sleep(0.1)
            attempt += 1
    print("Failed to load Datos.json after multiple attempts")
>>>>>>> 1f47153fc388540a12b13737798de23ae6ed20ee

def actualizar_datos_funcion():
    while True:
        cargar_datos_funcion()
        time.sleep(0.1)
def init_gl():
    """ Configuración inicial de OpenGL. """
    glClearColor(0.15, 0.15, 0.15, 1)
    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()
    # Configuración del sistema de coordenadas
    gluOrtho2D(-100, 100, -100, 100)
    glMatrixMode(GL_MODELVIEW)
    glLoadIdentity()

def draw_axes():
    """ Dibuja los ejes X e Y con números adaptativos según el zoom """
    global zoom
    glPushAttrib(GL_ALL_ATTRIB_BITS)
    glColor3f(1, 1, 1)  # Color blanco para los ejes

    glBegin(GL_LINES)
    # Eje X
    glVertex2f(-100, 0)
    glVertex2f(100, 0)
    # Eje Y
    glVertex2f(0, -100)
    glVertex2f(0, 100)
    glEnd()

    # Determinar la separación de los números en función del zoom
    base_spacing = 10
    if zoom > 5:
        base_spacing = 1
    if zoom > 20:
        base_spacing = 0.5
    if zoom > 50:
        base_spacing = 0.1
    spacing = max(base_spacing, 10 / zoom)

    # Dibujar números en el eje X
    x_start = int(-100 / spacing) * spacing
    x_end = int(100 / spacing) * spacing
    x_range = np.arange(x_start, x_end + spacing, spacing)
    for i in x_range:
        if abs(i) < 1e-3:
            i = 0
        glRasterPos2f(i, -3 / zoom)
        text = f"{i:.1f}" if zoom > 20 else f"{int(i)}"
        for char in text:
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, ord(char))

    # Dibujar números en el eje Y
    y_start = int(-100 / spacing) * spacing
    y_end = int(100 / spacing) * spacing
    y_range = np.arange(y_start, y_end + spacing, spacing)
    for i in y_range:
        if abs(i) < 1e-3:
            i = 0
        glRasterPos2f(-5 / zoom, i)
        text = f"{i:.1f}" if zoom > 20 else f"{int(i)}"
        for char in text:
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, ord(char))

    glPopAttrib()

def draw_function():
    """ Dibuja la función en verde, conectando los puntos sin saltos grandes. """
    if not puntos:
        return
    glColor3f(0, 1, 0)
    glBegin(GL_LINE_STRIP)
    threshold = 10  # Diferencia máxima permitida para evitar saltos grandes
    last_x, last_y = puntos[0]
    for x, y in puntos[1:]:
        if abs(x) > 0.05:
            if abs(y - last_y) > threshold:
                glEnd()
                glBegin(GL_LINE_STRIP)
            glVertex2f(x, y)
            last_x, last_y = x, y
    glEnd()

def draw_rectangles():
    """ Dibuja los rectángulos en amarillo utilizando los vértices cargados desde el JSON """
    glColor3f(1, 1, 0)
    for vertices in rectangulos:
        glBegin(GL_QUADS)
        for vertex in vertices:
            glVertex3f(vertex[0], vertex[1], vertex[2])
        glEnd()

def draw_grid():
    """ Dibuja una cuadricula en el fondo """
    glPushAttrib(GL_ALL_ATTRIB_BITS)
    glDisable(GL_LIGHTING)
    glColor3f(0.3, 0.3, 0.3)
    glBegin(GL_LINES)
    grid_spacing = 1
    # Líneas verticales
    for x in np.arange(-100, 100 + grid_spacing, grid_spacing):
        glVertex2f(x, -100)
        glVertex2f(x, 100)
    # Líneas horizontales
    for y in np.arange(-100, 100 + grid_spacing, grid_spacing):
        glVertex2f(-100, y)
        glVertex2f(100, y)
    glEnd()
    glPopAttrib()

def display():
    """ Función de renderizado. """
    glClear(GL_COLOR_BUFFER_BIT)
    glLoadIdentity()

    # Aplicar zoom y desplazamiento (pan)
    glTranslatef(pan_x, pan_y, 0)
    glScalef(zoom, zoom, 1)

    draw_grid()
    draw_axes()
    draw_function()
    draw_rectangles()

    glutSwapBuffers()

def keyboard(key, x, y):
    """ Maneja el zoom con las teclas '+' y '-' """
    global zoom
    if key == b'+':
        zoom *= 1.1
    elif key == b'-':
        zoom /= 1.1
    guardar_parametros()
    glutPostRedisplay()

usuario_interactuo = False

def mouse_motion(x, y):
<<<<<<< HEAD
    global pan_x, pan_y, mouse_x, mouse_y, usuario_interactuo, zoom
    if mouse_pressed:
        # Ajustar el factor divisor para que tenga en cuenta el zoom
        dx = (x - mouse_x) / (0.5 * zoom)
        dy = (mouse_y - y) / (0.5 * zoom)
=======
    global pan_x, pan_y, mouse_x, mouse_y, usuario_interactuo
    if mouse_pressed:
        dx = (x - mouse_x) / 5.0
        dy = (mouse_y - y) / 5.0
        # Solo actualiza si el movimiento es mayor a un umbral
>>>>>>> 1f47153fc388540a12b13737798de23ae6ed20ee
        if abs(dx) > 0.1 or abs(dy) > 0.1:
            pan_x += dx
            pan_y += dy
            usuario_interactuo = True
            glutPostRedisplay()
        mouse_x, mouse_y = x, y

def mouse_click(button, state, x, y):
<<<<<<< HEAD
    global mouse_pressed, mouse_x, mouse_y
=======
    global mouse_pressed
>>>>>>> 1f47153fc388540a12b13737798de23ae6ed20ee
    if button == GLUT_LEFT_BUTTON:
        if state == GLUT_DOWN:
            mouse_pressed = True
            mouse_x, mouse_y = x, y
        elif state == GLUT_UP:
            mouse_pressed = False
<<<<<<< HEAD
=======
            # Solo guardar si el usuario ha interactuado de verdad
>>>>>>> 1f47153fc388540a12b13737798de23ae6ed20ee
            if usuario_interactuo:
                guardar_parametros()

def mouse_wheel(button, state, x, y):
    """ Maneja el zoom con la rueda del mouse (si GLUT lo soporta). """
    global zoom
    if state == 1:  # Scroll up
        zoom *= 1.1
    elif state == -1:  # Scroll down
        zoom /= 1.1
    guardar_parametros()
    glutPostRedisplay()

def update_motion(value):
    """ Función para actualizar la pantalla a ~120 Hz. """
    glutPostRedisplay()

    glutTimerFunc(int(1/10000000), update_motion, 0)

def main():
    global zoom, pan_x, pan_y
    zoom = 19.19434249577509
    pan_x = 0.0
    pan_y = 0.0
    guardar_parametros()
    # Cargar datos: rectángulos y (opcionalmente) la función
    cargar_rectangulos_json()
    cargar_datos_funcion()  # Opcional, si cuentas con 'Datos.json'

    # Inicializar GLUT y crear la ventana
    glutInit()
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB)
    glutInitWindowSize(800, 600)
    glutCreateWindow("Rectángulos Riemann desde JSON".encode("utf-8"))
    init_gl()
    # Registrar callbacks de GLUT
    glutDisplayFunc(display)
    glutKeyboardFunc(keyboard)
    glutMouseFunc(mouse_click)
    glutMotionFunc(mouse_motion)
    glutMouseWheelFunc(mouse_wheel)

    # Iniciar el timer para actualizar a ~120 fps
    glutTimerFunc(int(1000/120), update_motion, 0)

    glutMainLoop()

if __name__ == "__main__":
    actualizacion_thread = threading.Thread(target=actualizar_datos_funcion, daemon=True)
    actualizacion_thread.start()
    main()