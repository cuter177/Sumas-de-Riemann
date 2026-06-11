#pragma once
// Dominio.h
#include <string>
#include <vector>
#include <utility>
#include <functional>

class Dominio {
public:
    Dominio(std::string exp, double dx);

    double f(double x);
    double derivada(double x, double h = 1e-5);

    std::vector<std::pair<double,double>> detectarIntervalosContinuos(double start, double end);
    void calcularDominio();

    // Muestreo adaptativo (MVT) + paralelismo → devuelve pares (x, y)
    std::vector<std::pair<double,double>> calcularPuntosAdaptativos(
        double start, double end,
        double tolerancia     = 0.01,
        int    profundidadMax = 12);

    // Escribe Datos.json con muestreo adaptativo
    void guardarEnJsonTiempoReal(
         const std::string& filename,
         double start, double end,
        double zoom = 1.0, double panx = 0.0, double pany = 0.0,
        const std::string& raiz = "");

    // Compatibilidad (no se usa en el flujo principal)
    void guardarRectangulosJson(
    const std::string& filename,
    double limInferior, double limSuperior,
    double dX, int totalRectangulos,
    const std::string& raiz);

private:
    std::string expresion;
    double      deltaX;
};