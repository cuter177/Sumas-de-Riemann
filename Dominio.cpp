#include "Dominio.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cmath>
#include <memory>
#include <future>
#include <algorithm>
#include <nlohmann/json.hpp>

#include "node.h"
#include "ExpressionParser.h"
#include "toPostFix.h"
#include "Utils.h"
#include "cutIntegral.h"

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

Dominio::Dominio(std::string exp, double dx) : expresion(exp), deltaX(dx) {}

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

double Dominio::derivada(double x, double h) {
    return (f(x + h) - f(x)) / h;
}

std::vector<std::pair<double, double>>
Dominio::detectarIntervalosContinuos(double start, double end) {
    std::vector<std::pair<double, double>> intervalos;
    double inicio    = start;
    bool enIntervalo = false;

    for (double x = start; x <= end; x += deltaX) {
        double fx = f(x);
        double df = derivada(x);

        if (std::isnan(fx) || std::isinf(fx) || std::fabs(df) > 1e5) {
            if (enIntervalo) {
                intervalos.push_back({inicio, x - deltaX});
                enIntervalo = false;
            }
        } else {
            if (!enIntervalo) { inicio = x; enIntervalo = true; }
        }
    }
    if (enIntervalo) intervalos.push_back({inicio, end});
    return intervalos;
}

void Dominio::calcularDominio() {}

static void muestreoAdaptativo(
    std::function<double(double)> func,
    double a, double fa,
    double b, double fb,
    double tolerancia,
    int profundidadMax, int profundidad,
    std::vector<std::pair<double,double>>& salida)
{
    double m  = (a + b) * 0.5;
    double fm = func(m);
    double fmEsperado = (fa + fb) * 0.5;

    bool subdivide = (profundidad < profundidadMax) &&
                     (!std::isnan(fm) && !std::isinf(fm)) &&
                     (std::fabs(fm - fmEsperado) > tolerancia);

    if (subdivide) {
        muestreoAdaptativo(func, a, fa, m, fm, tolerancia,
                           profundidadMax, profundidad + 1, salida);
        salida.push_back({m, fm});
        muestreoAdaptativo(func, m, fm, b, fb, tolerancia,
                           profundidadMax, profundidad + 1, salida);
    }
}

std::vector<std::pair<double,double>>
Dominio::calcularPuntosAdaptativos(double start, double end,
                                   double tolerancia, int profundidadMax)
{
    auto intervalos = detectarIntervalosContinuos(start, end);

    std::vector<std::future<std::vector<std::pair<double,double>>>> futuros;
    futuros.reserve(intervalos.size());

    for (const auto& [iStart, iEnd] : intervalos) {
        futuros.push_back(std::async(std::launch::async,
            [this, iStart, iEnd, tolerancia, profundidadMax]()
        {
            std::vector<std::pair<double,double>> local;
            local.reserve(512);
            double fa = f(iStart);
            double fb = f(iEnd);
            if (std::isnan(fa) || std::isinf(fa)) return local;

            int segmentosBase = 20;
            double paso = (iEnd - iStart) / segmentosBase;

            for (int k = 0; k < segmentosBase; k++) {
                double a   = iStart + k * paso;
                double b   = iStart + (k + 1) * paso;
                double fa2 = f(a);
                double fb2 = f(b);
                if (std::isnan(fa2) || std::isinf(fa2)) continue;
                local.push_back({a, fa2});
                muestreoAdaptativo([this](double x){ return f(x); },
                                   a, fa2, b, fb2,
                                   tolerancia, profundidadMax, 0, local);
            }

            if (!std::isnan(fb) && !std::isinf(fb))
                local.push_back({iEnd, fb});
            return local;
        }));
    }

    std::vector<std::pair<double,double>> puntos;
    for (auto& fut : futuros) {
        auto parcial = fut.get();
        puntos.insert(puntos.end(), parcial.begin(), parcial.end());
    }
    std::sort(puntos.begin(), puntos.end(),
              [](const auto& a, const auto& b){ return a.first < b.first; });
    return puntos;
}

void Dominio::guardarEnJsonTiempoReal(
    const std::string& filename,
    double start, double end,
    double /*zoom*/, double /*panx*/, double /*pany*/,
    const std::string& raiz)
{
    fs::path rutaCompleta = fs::path(raiz) / "datos" / filename;
    fs::create_directories(rutaCompleta.parent_path());

    std::ofstream archivo(rutaCompleta);
    if (!archivo) return;

    auto puntos = calcularPuntosAdaptativos(start, end, 0.01, 12);

    archivo << "{\n  \"puntos\": [\n";
    bool primero = true;
    for (const auto& [x, y] : puntos) {
        if (!std::isnan(y) && !std::isinf(y)) {
            if (!primero) archivo << ",\n";
            archivo << "    {\"x\": " << x << ", \"y\": " << y << "}";
            primero = false;
        }
    }
    archivo << "\n  ]\n}\n";
    archivo.flush();
    archivo.close();
}

void Dominio::guardarRectangulosJson(
    const std::string& filename,
    double limInferior, double /*limSuperior*/,
    double dX, int totalRectangulos,
    const std::string& raiz)
{
    json jsonData;
    jsonData["rectangulos"] = json::array();

    for (int i = 0; i < totalRectangulos; ++i) {
        double xi     = limInferior + i * dX;
        double altura = f(xi);
        if (std::isnan(altura) || std::isinf(altura)) continue;

        json rect;
        rect["vertices"] = json::array({
            json::array({ xi,      0.0,    0.0 }),
            json::array({ xi + dX, 0.0,    0.0 }),
            json::array({ xi + dX, altura, 0.0 }),
            json::array({ xi,      altura, 0.0 })
        });
        jsonData["rectangulos"].push_back(rect);
    }

    fs::path rutaCompleta = fs::path(raiz) / "datos" / filename;
    fs::create_directories(rutaCompleta.parent_path());
    std::ofstream archivo(rutaCompleta);
    if (archivo) archivo << jsonData.dump(4);
}