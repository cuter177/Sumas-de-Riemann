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
<<<<<<< HEAD
#include "PythonManager.h"
=======

using json = nlohmann::json;
>>>>>>> 1f47153fc388540a12b13737798de23ae6ed20ee
using namespace std;
std::atomic<bool> pythonScriptRunning{false};

bool fileExists(const string &path) {
    ifstream file(path);
    return file.good();
}

void leerParametros(double &zoom, double &pan_x, double &pan_y) {
    const string parametrosPath = "C:\\Users\\Pop90\\Documents\\Riemann_4.1\\datos\\Parametros.json";
    // Retry if file does not exist immediately.
    int retryCount = 0;
    while (!fileExists(parametrosPath) && retryCount < 5) {
        this_thread::sleep_for(chrono::milliseconds(100));
        retryCount++;
    }
    ifstream archivo(parametrosPath);
    if (archivo.is_open()) {
        json parametros;
        archivo >> parametros;
        zoom = parametros["zoom"];
        pan_x = parametros["pan_x"];
        pan_y = parametros["pan_y"];
        cout << "Parametros leidos: zoom=" << zoom << ", pan_x=" << pan_x << ", pan_y=" << pan_y << endl;
    } else {
        cerr << "Error al abrir el archivo de parámetros en " << parametrosPath << endl;
    }
}

void ejecutarScriptPython() {
    wstring commandLine =
        L"\"C:\\Users\\Pop90\\AppData\\Local\\Programs\\Python\\Python313\\python.exe\" "
        L"\"C:\\Users\\Pop90\\Documents\\Riemann_4.1\\Graficadora.py\"";
    wcout << L"Ejecutando comando: " << commandLine << endl;

    vector<wchar_t> cmdBuffer(commandLine.begin(), commandLine.end());
    cmdBuffer.push_back(0);

    wchar_t currentDir[MAX_PATH];
    if (!GetCurrentDirectoryW(MAX_PATH, currentDir)) {
        wcerr << L"Error al obtener el directorio actual." << endl;
    }

    if (GetFileAttributesW(L"C:\\Users\\Pop90\\AppData\\Local\\Programs\\Python\\Python313\\python.exe") == INVALID_FILE_ATTRIBUTES)
        wcerr << L"Error: No se encontró python.exe en la ruta especificada." << endl;
    if (GetFileAttributesW(L"C:\\Users\\Pop90\\Documents\\Riemann_4.1\\Graficadora.py") == INVALID_FILE_ATTRIBUTES)
        wcerr << L"Error: No se encontró Graficadora.py en la ruta especificada." << endl;

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(
         nullptr,
         cmdBuffer.data(),
         nullptr,
         nullptr,
         FALSE,
         0,
         nullptr,
         currentDir,
         &si,
         &pi))
    {
        DWORD err = GetLastError();
        wcerr << L"Error al ejecutar CreateProcessW: " << err << endl;
    }
    else
    {
        pythonScriptRunning = true;
        cout << "pythonScriptRunning establecido a true" << endl;
        WaitForSingleObject(pi.hProcess, INFINITE);
        pythonScriptRunning = false;
        cout << "pythonScriptRunning establecido a false" << endl;
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        wcout << L"Código de retorno: " << exitCode << endl;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

void ejecutarScriptPythonEnThread(Dominio &dominio, double zoom, double pan_x, double pan_y) {
    // Create thread to execute the Python script.
    thread pythonThread(ejecutarScriptPython);

    // While the Python script is running, update parameters and save the JSON.
    do {
        // In addition to reading from the JSON file, wait for it to be updated.
        leerParametros(zoom, pan_x, pan_y);
        cout << zoom << " " << pan_x << " " << pan_y << endl;
        // Compute visible x-axis range based on the current zoom and pan_x.
        double visibleX_min = (-100.0 / zoom) - pan_x;
        double visibleX_max = (100.0 / zoom) - pan_x;
        // Save JSON with current visible limits.
        dominio.guardarEnJsonTiempoReal("Datos.json", visibleX_min, visibleX_max, zoom, pan_x, pan_y);
        cout << "Datos.json actualizado" << endl;
        this_thread::sleep_for(chrono::milliseconds(100));
    } while (pythonScriptRunning);

    if (pythonThread.joinable()) {
        pythonThread.join();
    }
}

<<<<<<< HEAD
int main() {
    cout << "\nBienvenido al programa para aproximar integrales por sumatoria de Riemann" << endl;
=======
int main()
{
    cout << "\nBienvenido al programa para aproximar integrales por sumatoria de Reiemann" << endl;
>>>>>>> 1f47153fc388540a12b13737798de23ae6ed20ee
    cout << "\n***************************************************************************" << endl;
    cout << "\nPara insertar una integral se debe de hacer de la siguiente forma:" << endl;
    cout << "integral(ln(abs(x+1)),1,e,0.0001)" << endl;
    cout << "Funciones que acepta el programa:" << endl;
    cout << "sin(x), cos(x), tan(x), sinh(x), cosh(x), tanh(x), asin(x), acos(x), atan(x)," << endl;
    cout << "ln(x), e^x, abs(x), sqrt(x), cbrt(x)" << endl;
    cout << "\nPresiona enter :" << endl;

    cortarIntegral integral;
<<<<<<< HEAD
    PythonManager pythonManager;
    char answer{};

    do {
=======
    char answer{};

    do {
        string entrada;
>>>>>>> 1f47153fc388540a12b13737798de23ae6ed20ee
        cin.ignore();
        cout << "Introduce tu integral: " << endl;
        string entrada;
        getline(cin, entrada);

        integral.cortar(entrada);
        integral.mostrarDatos();

        string expresion = integral.getArg();
<<<<<<< HEAD
=======
        string &r_expresion = expresion;

        double xi = 0, sumatoria = 0;
>>>>>>> 1f47153fc388540a12b13737798de23ae6ed20ee
        double limInferior = stod(integral.getLimI());
        double limSuperior = stod(integral.getLimS());
        double deltaDeX = stod(integral.getDeltaX());
        double xi = 0, sumatoria = 0;

        for (double i = limInferior; i <= limSuperior; i += deltaDeX) {
            string limite = to_string(i);
<<<<<<< HEAD
            toPostFix x(getMathExpression(expresion, limite));
            Expression_Parser parser(x.getPostFixExpression());
            auto tree = parser.toTree();
            xi = parser.evaluateExpressionTree(tree) * deltaDeX;
=======
            string &r_limite = limite;
            toPostFix x(getMathExpression(r_expresion, r_limite));
            Expression_Parser myTree(x.getPostFixExpression());
            auto tree = myTree.toTree();
            xi = myTree.evaluateExpressionTree(tree) * deltaDeX;
>>>>>>> 1f47153fc388540a12b13737798de23ae6ed20ee
            sumatoria += xi;
        }
        cout << "El valor de la sumatoria es: " << setprecision(15) << sumatoria << endl;
<<<<<<< HEAD

        Dominio dominio(expresion, 0.001);
        dominio.guardarRectangulosJson("Rectangulo.json", limInferior, limSuperior, deltaDeX);

        double zoom = 1.0, pan_x = 0.0, pan_y = 0.0;
        pythonManager.ejecutarScriptPythonEnThread(dominio, zoom, pan_x, pan_y);

        cout << "\n¿Introducir otra expresión matemática? (Y/N): ";
=======
        Dominio dominio(r_expresion, 0.001);
        dominio.guardarRectangulosJson("Rectangulo.json", limInferior, limSuperior, deltaDeX);

        double zoom, pan_x, pan_y;
        ejecutarScriptPythonEnThread(dominio, zoom, pan_x, pan_y);

        xi = 0;
        sumatoria = 0;
        cout << "\n¿Introducir otra expresion matematica? (Y/N):";
>>>>>>> 1f47153fc388540a12b13737798de23ae6ed20ee
        cin >> answer;
        cout << "\n";

    } while (tolower(answer) == 'y');

    return 0;
}

