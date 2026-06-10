import json
import os
import threading
import time

from OpenGL.GL import *
from OpenGL.GLUT import *
from OpenGL.GLU import *
import numpy as np

puntos = []
rectangulos = []

zoom = 19.19434249577509
pan_x, pan_y = 0.0, 0.0
mouse_pressed = False
mouse_x, mouse_y = 0, 0
usuario_interactuo = False

RAIZ_DIR  = os.path.dirname(os.path.abspath(__file__))
DATOS_DIR = os.path.join(RAIZ_DIR, "datos")

def guardar_parametros():
    os.makedirs(DATOS_DIR, exist_ok=True)
    ruta = os.path.join(DATOS_DIR, "Parametros.json")
    with open(ruta, "w") as f:
        json.dump({"zoom": zoom, "pan_x": pan_x, "pan_y": pan_y}, f)

def cargar_rectangulos_json():
    global rectangulos
    ruta = os.path.join(DATOS_DIR, "Rectangulo.json")
    try:
        with open(ruta, "r") as f:
            data = json.load(f)
            rectangulos = [rect["vertices"] for rect in data.get("rectangulos", [])]
    except Exception as e:
        pass  # archivo aún no existe o está siendo escrito

def cargar_datos_funcion():
    global puntos
    ruta = os.path.join(DATOS_DIR, "Datos.json")
    for _ in range(5):
        try:
            with open(ruta, 'r') as f:
                content = f.read().strip()
                if not content:
                    raise ValueError("Empty file")
                datos = json.loads(content)
                if "puntos" not in datos:
                    raise ValueError("Missing 'puntos'")
                puntos = [(p["x"], p["y"]) for p in datos["puntos"]]
                return
        except Exception:
            time.sleep(0.1)

def actualizar_datos():
    while True:
        cargar_datos_funcion()
        cargar_rectangulos_json()
        time.sleep(0.05)

def init_gl():
    glClearColor(0.15, 0.15, 0.15, 1)
    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()
    gluOrtho2D(-100, 100, -100, 100)
    glMatrixMode(GL_MODELVIEW)
    glLoadIdentity()

def draw_axes():
    glPushAttrib(GL_ALL_ATTRIB_BITS)
    glColor3f(1, 1, 1)
    glBegin(GL_LINES)
    glVertex2f(-100, 0);  glVertex2f(100, 0)
    glVertex2f(0, -100);  glVertex2f(0, 100)
    glEnd()

    base_spacing = 10
    if zoom > 5:  base_spacing = 1
    if zoom > 20: base_spacing = 0.5
    if zoom > 50: base_spacing = 0.1
    spacing = max(base_spacing, 10 / zoom)

    for i in np.arange(int(-100/spacing)*spacing, int(100/spacing)*spacing + spacing, spacing):
        if abs(i) < 1e-3: i = 0
        glRasterPos2f(i, -3/zoom)
        text = f"{i:.1f}" if zoom > 20 else f"{int(i)}"
        for ch in text: glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, ord(ch))

    for i in np.arange(int(-100/spacing)*spacing, int(100/spacing)*spacing + spacing, spacing):
        if abs(i) < 1e-3: i = 0
        glRasterPos2f(-5/zoom, i)
        text = f"{i:.1f}" if zoom > 20 else f"{int(i)}"
        for ch in text: glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, ord(ch))

    glPopAttrib()

def draw_function():
    #print(f"puntos: {len(puntos)}")
    if not puntos:
        return
    glColor3f(0, 1, 0)
    glBegin(GL_LINE_STRIP)
    threshold = 10
    prev_y = puntos[0][1]
    for x, y in puntos:
        if abs(y - prev_y) > threshold:
            glEnd()
            glBegin(GL_LINE_STRIP)
        glVertex2f(x, y)
        prev_y = y
    glEnd()

def draw_rectangles():
    glColor3f(1, 1, 0)
    for vertices in rectangulos:
        glBegin(GL_QUADS)
        for v in vertices:
            glVertex3f(v[0], v[1], v[2])
        glEnd()

def draw_grid():
    glPushAttrib(GL_ALL_ATTRIB_BITS)
    glDisable(GL_LIGHTING)
    glColor3f(0.3, 0.3, 0.3)
    glBegin(GL_LINES)
    for x in np.arange(-100, 101, 1):
        glVertex2f(x, -100); glVertex2f(x, 100)
    for y in np.arange(-100, 101, 1):
        glVertex2f(-100, y); glVertex2f(100, y)
    glEnd()
    glPopAttrib()

def display():
    glClear(GL_COLOR_BUFFER_BIT)
    glLoadIdentity()
    glTranslatef(pan_x, pan_y, 0)
    glScalef(zoom, zoom, 1)
    draw_grid()
    draw_axes()
    draw_function()
    draw_rectangles()
    glutSwapBuffers()

def keyboard(key, x, y):
    global zoom
    if key == b'+': zoom *= 1.1
    elif key == b'-': zoom /= 1.1
    guardar_parametros()
    glutPostRedisplay()

def mouse_motion(mx, my):
    global pan_x, pan_y, mouse_x, mouse_y, usuario_interactuo
    if mouse_pressed:
        dx = (mx - mouse_x) / (0.5 * zoom)
        dy = (mouse_y - my) / (0.5 * zoom)
        if abs(dx) > 0.1 or abs(dy) > 0.1:
            pan_x += dx
            pan_y += dy
            usuario_interactuo = True
            glutPostRedisplay()
        mouse_x, mouse_y = mx, my

def mouse_click(button, state, x, y):
    global mouse_pressed, mouse_x, mouse_y, usuario_interactuo
    if button == GLUT_LEFT_BUTTON:
        if state == GLUT_DOWN:
            mouse_pressed = True
            mouse_x, mouse_y = x, y
        elif state == GLUT_UP:
            mouse_pressed = False
            if usuario_interactuo:
                guardar_parametros()
                usuario_interactuo = False

def mouse_wheel(button, state, x, y):
    global zoom
    if state == 1:    zoom *= 1.1
    elif state == -1: zoom /= 1.1
    guardar_parametros()
    glutPostRedisplay()

def update_motion(value):
    glutPostRedisplay()
    glutTimerFunc(int(1000/120), update_motion, 0)

def main():
    global zoom, pan_x, pan_y
    zoom  = 19.19434249577509
    pan_x = 0.0
    pan_y = 0.0
    guardar_parametros()
    cargar_rectangulos_json()
    cargar_datos_funcion()

    glutInit()
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB)
    glutInitWindowSize(800, 600)
    glutCreateWindow("Rectángulos Riemann".encode("utf-8"))
    init_gl()

    glutDisplayFunc(display)
    glutKeyboardFunc(keyboard)
    glutMouseFunc(mouse_click)
    glutMotionFunc(mouse_motion)
    glutMouseWheelFunc(mouse_wheel)
    glutTimerFunc(int(1000/120), update_motion, 0)
    glutIdleFunc(lambda: glutPostRedisplay())

    glutMainLoop()

if __name__ == "__main__":
    t = threading.Thread(target=actualizar_datos, daemon=True)
    t.start()
    main()