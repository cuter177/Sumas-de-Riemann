#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <filesystem>
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
namespace fs = std::filesystem;
using json = nlohmann::json;

int main() {
    cortarIntegral integral;
    PythonManager  pythonManager;
    JavaManager    jm;
    jm.ejecutarJarEnThread();

    fs::path exePath       = fs::current_path();
    fs::path raiz          = exePath.parent_path().parent_path();
    fs::path rutaJson      = raiz / "Interfaz" / "data" / "Funcion.json";
    fs::path rutaResultado = raiz / "Interfaz" / "data" / "Resultado.json";

    std::string ultimo_hash = JsonIO::hashArchivo(rutaJson.string());
    std::cout << "=== Esperando cambios en Funcion.json ===\n";

    while (true) {
        bool ok = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        if (!fs::exists(rutaJson)) continue;

        std::string hash_actual = JsonIO::hashArchivo(rutaJson.string());
        if (hash_actual == ultimo_hash) continue;

        std::cout << "\n=== CAMBIO DETECTADO EN Funcion.json ===\n";
        ultimo_hash = hash_actual;

        // ── 1. Leer JSON ───────────────────────────────────────────────────
        std::string f, li_raw, ls_raw, n_raw;
        if (!JsonIO::leerFuncion(rutaJson.string(), f, li_raw, ls_raw, n_raw)) {
            std::cout << "Error leyendo JSON\n";
            continue;
        }
        std::cout << "Funcion: " << f << "  LI: " << li_raw
                  << "  LS: " << ls_raw << "  N: " << n_raw << "\n";

        std::string entrada = "(" + f + "," + li_raw + "," + ls_raw + "," + n_raw + ")";
        integral.cortar(entrada);
        integral.mostrarDatos();

        // ── 2. Un solo loop: sumatoria + vértices de rectángulos ──────────
        // expresion es no-const para pasarla a getMathExpression(string&, string&)
        std::string expresion = integral.getArg();
        const double limInferior = stod(integral.getLimI());
        const double limSuperior = stod(integral.getLimS());
        const double deltaDeX    = stod(integral.getDeltaX());
        const int    n           = integral.getN();

        double sumatoria = 0.0;

        struct RectVerts { double xIzq, xDer, altura; };
        std::vector<RectVerts> rects;
        rects.reserve(n);

        for (int i = 0; i < n; ++i) {
            double xi = limInferior + i * deltaDeX;       // índice entero → sin error acumulado
            double xd = limInferior + (i + 1) * deltaDeX;
            if (xd > limSuperior) xd = limSuperior;

            std::string limite = std::to_string(xi);      // no-const: compatible con getMathExpression
            toPostFix pfx(getMathExpression(expresion, limite));
            Expression_Parser parser(pfx.getPostFixExpression());
            auto tree = parser.toTree();
            double fxi = parser.evaluateExpressionTree(tree);

            if (parser.getError()) { ok = false; break; }

            sumatoria += fxi * deltaDeX;
            rects.push_back({xi, xd, fxi});
        }

        std::cout << "Sumatoria: " << std::setprecision(15) << sumatoria << "\n";

        // ── 3. Guardar resultado ───────────────────────────────────────────
        JsonIO::escribirResultado(sumatoria, deltaDeX, ok, rutaResultado.string());
        std::cout << "Resultado guardado en Resultado.json\n";

        // ── 4. Escribir Rectangulo.json reutilizando valores ya calculados ─
        if (ok) {
            nlohmann::json jsonData;
            jsonData["rectangulos"] = nlohmann::json::array();

            for (const auto& r : rects) {
                if (std::isnan(r.altura) || std::isinf(r.altura)) continue;
                nlohmann::json rect;
                rect["vertices"] = nlohmann::json::array({
                    nlohmann::json::array({ r.xIzq, 0.0,      0.0 }),
                    nlohmann::json::array({ r.xDer, 0.0,      0.0 }),
                    nlohmann::json::array({ r.xDer, r.altura, 0.0 }),
                    nlohmann::json::array({ r.xIzq, r.altura, 0.0 })
                });
                jsonData["rectangulos"].push_back(rect);
            }

            fs::path rutaRects = fs::path(__FILE__).parent_path() / "datos" / "Rectangulo.json";
            fs::create_directories(rutaRects.parent_path());
            std::ofstream archivoRects(rutaRects);
            if (archivoRects) archivoRects << jsonData.dump(2);
        }

        // ── 5. Lanzar graficadora ──────────────────────────────────────────
        std::cout << "expresion para graficar: [" << expresion << "]" << std::endl;
        Dominio dominio(expresion, 0.001);
        double zoom = 19.19434249577509, pan_x = 0.0, pan_y = 0.0;
        if (ok) {
            pythonManager.ejecutarScriptPythonEnThread(dominio, zoom, pan_x, pan_y);
        }

        std::cout << "=== Esperando nuevos cambios... ===\n";
    }
    return 0;
}