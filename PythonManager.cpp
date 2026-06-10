// PythonManager.cpp
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "PythonManager.h"
#include "JsonIO.h"

std::atomic<bool> pythonScriptRunning{false};

PythonManager::PythonManager() {}

std::string PythonManager::obtenerDirectorioBase() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    return fs::weakly_canonical(fs::path(path)).parent_path().string();
}

std::string PythonManager::obtenerDirectorioRaiz() {
    fs::path dir = obtenerDirectorioBase();
    while (!dir.empty() && dir.filename() != "Riemann_2.0")
        dir = dir.parent_path();
    return dir.string();
}

bool PythonManager::fileExists(const std::string& p) {
    return fs::exists(p);
}

void PythonManager::leerParametros(double& zoom, double& pan_x, double& pan_y) {
    std::string raiz = obtenerDirectorioRaiz();
    std::string ruta = (fs::path(raiz) / "datos" / "Parametros.json").string();
    for (int i = 0; i < 5 && !fileExists(ruta); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::ifstream arch(ruta);
    if (!arch.is_open()) return;
    json j; arch >> j;
    zoom  = j["zoom"];
    pan_x = j["pan_x"];
    pan_y = j["pan_y"];
}

void PythonManager::ejecutarScriptPython() {
    std::string raiz = obtenerDirectorioRaiz();
    if (!SetCurrentDirectoryA(raiz.c_str())) {
        std::cerr << "Error SetCurrentDirectory: " << GetLastError() << "\n";
        pythonScriptRunning = false;
        return;
    }

    STARTUPINFOW si{};
    si.cb        = sizeof(si);
    si.dwFlags   = STARTF_USESTDHANDLES;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

    PROCESS_INFORMATION pi{};

    std::wstring cmd =
        L"\".\\python-3.13.9-embed-amd64\\python.exe\" \".\\Graficadora.py\"";
    std::vector<wchar_t> buf(cmd.begin(), cmd.end());
    buf.push_back(0);

    pythonScriptRunning = true;

    if (!CreateProcessW(nullptr, buf.data(),
                        nullptr, nullptr, TRUE, 0,
                        nullptr, nullptr, &si, &pi))
    {
        std::cerr << "Error CreateProcessW: " << GetLastError() << "\n";
        pythonScriptRunning = false;
        return;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    pythonScriptRunning = false;
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void PythonManager::ejecutarScriptPythonEnThread(
    Dominio& dominio,
    double zoom, double pan_x, double pan_y)
{
    std::thread pythonThread(&PythonManager::ejecutarScriptPython, this);

    // Esperar a que Python inicialice y escriba Parametros.json
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    do {
        leerParametros(zoom, pan_x, pan_y);
        double xMin = (-100.0 / zoom) - (pan_x / zoom);
        double xMax = ( 100.0 / zoom) - (pan_x / zoom);
        dominio.guardarEnJsonTiempoReal("Datos.json", xMin, xMax, zoom, pan_x, pan_y);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    } while (pythonScriptRunning);

    if (pythonThread.joinable()) pythonThread.join();
}