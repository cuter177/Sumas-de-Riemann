#include "PythonManager.h"
#include <windows.h>

#include "JsonIO.h"

std::atomic<bool> pythonScriptRunning{false};
PythonManager::PythonManager() {}

std::string PythonManager::obtenerDirectorioBase() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    return fs::weakly_canonical(fs::path(path)).parent_path().string();
}

std::string PythonManager::obtenerDirectorioRaiz() {
    fs::path baseDir = obtenerDirectorioBase();
    while (!baseDir.empty() && baseDir.filename() != "Riemann_2.0") {
        baseDir = baseDir.parent_path();
    }
    return baseDir.string();
}

bool PythonManager::fileExists(const std::string &path) {
    return fs::exists(path);
}

void PythonManager::leerParametros(double &zoom, double &pan_x, double &pan_y) {
    std::string raizDir = obtenerDirectorioRaiz();
    std::string parametrosPath = (fs::path(raizDir) / "datos" / "Parametros.json").string();
    int retryCount = 0;
    while (!fileExists(parametrosPath) && retryCount < 5) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        retryCount++;
    }
    std::ifstream archivo(parametrosPath);
    if (archivo.is_open()) {
        json parametros;
        archivo >> parametros;
        zoom = parametros["zoom"];
        pan_x = parametros["pan_x"];
        pan_y = parametros["pan_y"];
    }
}

void PythonManager::ejecutarScriptPython() {
    // Obtener el directorio raíz del proyecto
    std::string raizDir = obtenerDirectorioRaiz();

    // Cambiar el directorio de trabajo a la raíz del proyecto
    if (!SetCurrentDirectoryA(raizDir.c_str())) {
        DWORD err = GetLastError();
        std::cerr << "Error al cambiar el directorio de trabajo: " << err << std::endl;
        return;
    }

    // Rutas relativas desde la raíz del proyecto
    std::wstring pythonPath = L".\\python-3.13.9-embed-amd64\\python.exe";
    std::wstring scriptPath = L".\\Graficadora.py";

    // Construir la línea de comando con comillas
    std::wstring commandLine = L"\"" + pythonPath + L"\" \"" + scriptPath + L"\"";
    std::wcout << L"Ejecutando comando: " << commandLine << std::endl;

    // Convertir a buffer para CreateProcessW
    std::vector<wchar_t> cmdBuffer(commandLine.begin(), commandLine.end());
    cmdBuffer.push_back(0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    // Crear el proceso
    if (!CreateProcessW(
        nullptr,
        cmdBuffer.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi))
    {
        DWORD err = GetLastError();
        std::wcerr << L"Error al ejecutar CreateProcessW: " << err << std::endl;
    } else {
        pythonScriptRunning = true;
        // Esperar a que termine el script
        WaitForSingleObject(pi.hProcess, INFINITE);
        pythonScriptRunning = false;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

void PythonManager::ejecutarScriptPythonEnThread(Dominio &dominio, double zoom, double pan_x, double pan_y) {
    std::thread pythonThread(&PythonManager::ejecutarScriptPython, this);

    do {
        leerParametros(zoom, pan_x, pan_y);
        double visibleX_min = (-100.0 / zoom) - (pan_x / zoom);
        double visibleX_max = (100.0 / zoom) - (pan_x / zoom);
        dominio.guardarEnJsonTiempoReal("Datos.json", visibleX_min, visibleX_max, zoom, pan_x, pan_y);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (pythonScriptRunning);

    if (pythonThread.joinable()) {
        pythonThread.join();
    }
}
