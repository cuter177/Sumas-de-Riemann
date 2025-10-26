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
using namespace std;

int main() {
    cout << "\nBienvenido al programa para aproximar integrales por sumatoria de Riemann" << endl;
    cout << "\n***************************************************************************" << endl;
    cout << "\nPara insertar una integral se debe de hacer de la siguiente forma:" << endl;
    cout << "integral(ln(abs(x+1)),1,e,0.0001)" << endl;
    cout << "Funciones que acepta el programa:" << endl;
    cout << "sin(x), cos(x), tan(x), sinh(x), cosh(x), tanh(x), asin(x), acos(x), atan(x)," << endl;
    cout << "ln(x), e^x, abs(x), sqrt(x), cbrt(x)" << endl;
    cout << "\nPresiona enter :" << endl;

    cortarIntegral integral;
    PythonManager pythonManager;
    char answer{};

    do {
        cin.ignore();
        cout << "Introduce tu integral: " << endl;
        string entrada;
        getline(cin, entrada);

        integral.cortar(entrada);
        integral.mostrarDatos();

        string expresion = integral.getArg();
        double limInferior = stod(integral.getLimI());
        double limSuperior = stod(integral.getLimS());
        double deltaDeX = stod(integral.getDeltaX());
        double xi = 0, sumatoria = 0;

        for (double i = limInferior; i <= limSuperior; i += deltaDeX) {
            string limite = to_string(i);
            toPostFix x(getMathExpression(expresion, limite));
            Expression_Parser parser(x.getPostFixExpression());
            auto tree = parser.toTree();
            xi = parser.evaluateExpressionTree(tree) * deltaDeX;
            sumatoria += xi;
        }

        cout << "El valor de la sumatoria es: " << setprecision(15) << sumatoria << endl;

        Dominio dominio(expresion, 0.001);
        dominio.guardarRectangulosJson("Rectangulo.json", limInferior, limSuperior, deltaDeX);

        double zoom = 1.0, pan_x = 0.0, pan_y = 0.0;
        pythonManager.ejecutarScriptPythonEnThread(dominio, zoom, pan_x, pan_y);

        cout << "\n¿Introducir otra expresión matemática? (Y/N): ";
        cin >> answer;
        cout << "\n";

    } while (tolower(answer) == 'y');

    return 0;
}

