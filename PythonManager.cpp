// PythonManager.cpp
#include "PythonManager.h"
#include "JsonIO.h"
#include "Platform.h"

#include <vector>
#include <cstdlib>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#else
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/wait.h>
  #include <cstring>
  #include <cerrno>
#endif

std::atomic<bool> pythonScriptRunning{false};

PythonManager::PythonManager() {
    directorioRaiz = obtenerDirectorioRaiz();
}

std::string PythonManager::obtenerDirectorioBase() {
    return platform::ejecutableDir().string();
}

std::string PythonManager::obtenerDirectorioRaiz() {
    fs::path dir = obtenerDirectorioBase();
    while (!dir.empty() && !fs::exists(dir / "Graficadora.py"))
        dir = dir.parent_path();
    return dir.string();
}

bool PythonManager::fileExists(const std::string& p) {
    return fs::exists(p);
}

namespace {
// Intérprete a usar. Windows: Python embebido del release.
// Linux/macOS: Python del sistema, un venv del proyecto o un Python
// standalone bundleado en el release.
std::string resolverPython(const std::string& raiz) {
    if (const char* env = std::getenv("RIEMANN_PYTHON")) {
        if (*env) return env;
    }
#ifdef _WIN32
    fs::path embed = fs::path(raiz) / "python-3.13.9-embed-amd64" / "python.exe";
    if (fs::exists(embed)) return embed.string();
    return "python";
#else
    const fs::path candidatos[] = {
        fs::path(raiz) / "python" / "bin" / "python3",
        fs::path(raiz) / ".venv" / "bin" / "python",
        fs::path(raiz) / "venv" / "bin" / "python",
    };
    for (const auto& c : candidatos)
        if (fs::exists(c)) return c.string();
    return "python3";
#endif
}
} // namespace

void PythonManager::leerParametros(double& zoom, double& pan_x, double& pan_y) {
    std::string ruta = (fs::path(directorioRaiz) / "datos" / "Parametros.json").string();
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
    std::string raiz = directorioRaiz;
    std::cout << "Raiz Python: " << raiz << "\n";
    std::cout << "Graficadora exists: " << fileExists((fs::path(raiz) / "Graficadora.py").string()) << "\n";

#ifdef _WIN32
    std::cout << "Python exists: "
              << fileExists((fs::path(raiz) / "python-3.13.9-embed-amd64" / "python.exe").string())
              << "\n";

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
#else
    if (chdir(raiz.c_str()) != 0) {
        std::cerr << "Error chdir: " << std::strerror(errno) << "\n";
        pythonScriptRunning = false;
        return;
    }

    const std::string python = resolverPython(raiz);
    const std::string script = (fs::path(raiz) / "Graficadora.py").string();
    std::cout << "Python usado: " << python << "\n";

    std::vector<std::string> args = { python, script };
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    pythonScriptRunning = true;

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Error fork: " << std::strerror(errno) << "\n";
        pythonScriptRunning = false;
        return;
    }
    if (pid == 0) {
        // Hijo: reemplazar la imagen del proceso por Python.
        execvp(python.c_str(), argv.data());
        std::cerr << "Error execvp: " << std::strerror(errno) << "\n";
        _exit(127);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    pythonScriptRunning = false;
#endif
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
