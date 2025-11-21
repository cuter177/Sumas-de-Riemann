#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <iomanip>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <nlohmann/json.hpp>

#include "node.h"
#include "ExpressionParser.h"
#include "toPostFix.h"
#include "Utils.h"
#include "cutIntegral.h"
#include "Dominio.h"
#include "PythonManager.h"
#include "JsonIO.h"
#include "JavaManager.h"
using namespace std;
using json = nlohmann::json;

int main() {
    cortarIntegral integral;
    PythonManager pythonManager;
    JavaManager jm;
    jm.ejecutarJarEnThread();
    // RUTA AL JSON
    // Obtener directorio donde está el ejecutable
    fs::path exePath = fs::current_path(); // esto apunta a bin/Debug
    // Subir dos niveles para llegar a la raíz del proyecto
    fs::path raiz = exePath.parent_path().parent_path();

    fs::path rutaJson = raiz / "Interfaz" / "data" / "Funcion.json";
    fs::path rutaResultado = raiz / "Interfaz" / "data" / "Resultado.json";
    fs::path rutaJar = raiz / "Interfaz" / "target" / "Interfaz-RiemannInterfaz.jar";




    // Guardamos el timestamp inicial
    // Hash inicial (si no existe, queda vacío)
    std::string ultimo_hash = JsonIO::hashArchivo(rutaJson.string());

    std::cout << "=== Esperando cambios en Funcion.json ===\n";

    while (true) {

        // revisar cada 300 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        // Si el archivo no existe aún, esperar
        if (!fs::exists(rutaJson))
            continue;

        // Hash actual
        std::string hash_actual = JsonIO::hashArchivo(rutaJson.string());

        // Si hash igual → NO hay cambio
        if (hash_actual == ultimo_hash)
            continue;


        std::cout << "\n=== CAMBIO DETECTADO EN Funcion.json ===\n";
        ultimo_hash = hash_actual; // actualizar hash


        // ----------------------------------------------------------
        //   1. LEER EL JSON DE LA FUNCIÓN
        // ----------------------------------------------------------

        std::string f, li_raw, ls_raw, n_raw;

        if (!JsonIO::leerFuncion(rutaJson.string(), f, li_raw, ls_raw, n_raw)) {
            std::cout << " Error leyendo JSON\n";
            continue;   // sigue esperando cambios
        }

        std::cout << "Funcion: " << f << "\n";
        std::cout << "LI: " << li_raw << "\n";
        std::cout << "LS: " << ls_raw << "\n";
        std::cout << "N rect: " << n_raw << "\n";

        std::string entrada = "(" + f + "," + li_raw + "," + ls_raw + "," + n_raw + ")";
        std::cout << "\nReconstruida: " << entrada << "\n";

        // ----------------------------------------------------------
        //   2. PROCESAR LOS DATOS
        // ----------------------------------------------------------

        integral.cortar(entrada);
        integral.mostrarDatos();

        std::string expresion = integral.getArg();
        double limInferior = stod(integral.getLimI());
        double limSuperior = stod(integral.getLimS());
        double deltaDeX = stod(integral.getDeltaX());
        int n = integral.getN();
        double xi = 0, sumatoria = 0;

        for (double i = limInferior; i <= limSuperior; i += deltaDeX) {
            std::string limite = std::to_string(i);
            toPostFix x(getMathExpression(expresion, limite));
            Expression_Parser parser(x.getPostFixExpression());
            auto tree = parser.toTree();
            xi = parser.evaluateExpressionTree(tree) * deltaDeX;
            sumatoria += xi;
        }

        std::cout << "El valor de la sumatoria es: "
                  << std::setprecision(15) << sumatoria << "\n";

        // ----------------------------------------------------------
        //   3. GUARDAR RESULTADO EN JSON
        // ----------------------------------------------------------
        std::cout << "[DEBUG C++] Resultado.json -> "
          << rutaResultado.string() << std::endl;
        JsonIO::escribirResultado(sumatoria,deltaDeX, rutaResultado.string());
        std::cout << "Resultado guardado en Resultado.json\n";

        // ----------------------------------------------------------
        //   4. GUARDAR RECTÁNGULOS Y ACTUALIZAR GRAFICACIÓN
        // ----------------------------------------------------------

        Dominio dominio(expresion, 0.001);
        dominio.guardarRectangulosJson("Rectangulo.json", limInferior, limSuperior, deltaDeX, n);

        double zoom = 1.0, pan_x = 0.0, pan_y = 0.0;

        pythonManager.ejecutarScriptPythonEnThread(dominio, zoom, pan_x, pan_y);
        // limpiar flag después de graficar
        std::cout << "=== Esperando nuevos cambios... ===\n";
    }
    return 0;
}
