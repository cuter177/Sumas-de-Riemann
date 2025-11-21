#ifndef DOMINIO_H
#define DOMINIO_H

#include <string>
#include <vector>
#include <utility>

class Dominio {
private:
    std::string expresion;
    double deltaX;

    // Métodos privados
    double f(double x);
    double derivada(double x, double h = 1e-5);
    std::vector<std::pair<double, double>> detectarIntervalosContinuos(double start, double end);

public:
    // Constructor
    Dominio(std::string exp, double dx);

    // Métodos públicos
    void calcularDominio();
    void guardarEnJson(const std::string& filename);
    void guardarRectangulosJson(const std::string &filename, double limInferior, double limSuperior, double deltaX, int totalRectangulos);
    void guardarEnJsonTiempoReal(const std::string& filename, double start, double end, double zoom, double panx, double pany);
};

#endif // DOMINIO_H // DOMINIO_H
