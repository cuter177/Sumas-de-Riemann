#include "Dominio.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cmath>
#include <memory>
#include "node.h"
#include "ExpressionParser.h"
#include "toPostFix.h"
#include "Utils.h"
#include <nlohmann/json.hpp>
#ifdef _WIN32
  #include <io.h>
  #include <fcntl.h>
#else
  #include <unistd.h>
  #include <fcntl.h>
#endif

using namespace std;
using json = nlohmann::json;
namespace fs = std::filesystem;

// Constructor
Dominio::Dominio(std::string exp, double dx) : expresion(exp), deltaX(dx) {}

// Evalúa la función en un punto x
double Dominio::f(double x) {
    try {
        string limite = to_string(x);
        toPostFix postfix(getMathExpression(expresion, limite));
        Expression_Parser parser(postfix.getPostFixExpression());
        shared_ptr<node> tree = parser.toTree();
        return parser.evaluateExpressionTree(tree);
    } catch (...) {
        return NAN;
    }
}

// Aproximación numérica de la derivada
double Dominio::derivada(double x, double h) {
    return (f(x + h) - f(x)) / h;
}

// Detecta los intervalos continuos de la función en un rango dado
std::vector<std::pair<double, double>> Dominio::detectarIntervalosContinuos(double start, double end) {
    std::vector<std::pair<double, double>> intervalos;
    double inicio = start;
    bool enIntervalo = false;

    // Precompute function values and derivatives
    std::vector<double> fx_values;
    std::vector<double> df_values;
    for (double x = start; x <= end; x += deltaX) {
        fx_values.push_back(f(x));
        df_values.push_back(derivada(x));
    }

    for (size_t i = 0; i < fx_values.size(); ++i) {
        double x = start + i * deltaX;
        double fx = fx_values[i];
        double df = df_values[i];

        if (isnan(fx) || isinf(fx) || fabs(df) > 1e5) {
            if (enIntervalo) {
                intervalos.push_back({inicio, x - deltaX});
                enIntervalo = false;
            }
        } else {
            if (!enIntervalo) {
                inicio = x;
                enIntervalo = true;
            }
        }
    }

    if (enIntervalo) {
        intervalos.push_back({inicio, end});
    }

    return intervalos;
}

// Define the calcularDominio method
void Dominio::calcularDominio() {
    // Implementation of calcularDominio
    // This method should contain the logic to calculate the domain of the function
}

// Guarda los puntos en un archivo JSON en tiempo real
void Dominio::guardarEnJsonTiempoReal(const std::string& filename, double start, double end, double zoom, double panx, double pany) {
    // Ruta fija al proyecto (dos niveles arriba de este archivo fuente)
    fs::path rutaProyecto = fs::path(__FILE__).parent_path(); // si Dominio.cpp está en la raíz del proyecto
    fs::path rutaCompleta = rutaProyecto / "datos" / filename;



    if (!fs::exists(rutaCompleta.parent_path()))
        fs::create_directories(rutaCompleta.parent_path());

    std::ofstream archivo(rutaCompleta);
    if (!archivo) {
        //std::cerr << "Error opening file: " << rutaCompleta.string() << std::endl;
        return;
    }

    std::vector<std::pair<double, double>> intervalos = detectarIntervalosContinuos(start, end);
    archivo << "{\n  \"puntos\": [\n";
    bool primerPunto = true;
    for (const auto& intervalo : intervalos) {
        for (double x = intervalo.first; x <= intervalo.second; x += deltaX) {
            double fx = f(x);
            if (!std::isnan(fx) && !std::isinf(fx)) {
                if (!primerPunto) archivo << ",\n";
                archivo << "    {\"x\": " << x << ", \"y\": " << fx << "}";
                primerPunto = false;
            }
        }
    }
    archivo << "\n  ]\n}\n";
    archivo.flush();
    // Commit call removed.
    archivo.close();
    //std::cout << "Archivo guardado en: " << fs::absolute(rutaCompleta).string() << std::endl;

}

void Dominio::guardarRectangulosJson(const std::string& filename, double limInferior, double limSuperior, double deltaX) {
    json jsonData;
    jsonData["rectangulos"] = json::array();

    int totalRectangulos = std::round((limSuperior - limInferior) / deltaX);
    for (int i = 1; i <= totalRectangulos; ++i) {
        double xi = limInferior + i * deltaX;
        double altura = f(xi);

        if (std::isnan(altura) || std::isinf(altura))
            continue;

        // Calcular los vértices para el rectángulo:
        // Suponiendo que la base está en y=0 y la altura es 'altura'
        double xIzq = xi;            // coordenada x izquierda
        double xDer = xi + deltaX;     // coordenada x derecha
        double yInf = 0.0;            // coordenada y inferior
        double ySup = altura;         // coordenada y superior

        json rectangulo;
        // Se guardan los vértices en el siguiente orden:
        // inferior izquierda, inferior derecha, superior derecha y superior izquierda.
        rectangulo["vertices"] = json::array({
            { xIzq, yInf, 0.0 },
            { xDer, yInf, 0.0 },
            { xDer, ySup, 0.0 },
            { xIzq, ySup, 0.0 }
        });

        jsonData["rectangulos"].push_back(rectangulo);
    }

    // Ruta fija al proyecto (dos niveles arriba de este archivo fuente)
    fs::path rutaProyecto = fs::path(__FILE__).parent_path(); // si Dominio.cpp está en la raíz del proyecto
    fs::path rutaCompleta = rutaProyecto / "datos" / filename;


    if (!fs::exists(rutaCompleta.parent_path()))
        fs::create_directories(rutaCompleta.parent_path());

    std::ofstream archivo(rutaCompleta);
    if (archivo.is_open()) {
        archivo << jsonData.dump(4);
        archivo.close();
       // std::cout << "JSON generado en: " << rutaCompleta.string() << std::endl;
    } else {
        //std::cerr << "Error al abrir el archivo: " << rutaCompleta << std::endl;
    }
}