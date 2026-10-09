#include "JavaManager.h"
#include "Platform.h"

#include <iostream>
#include <filesystem>
#include <vector>
#include <thread>
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
  #include <fcntl.h>
  #include <cstring>
  #include <cerrno>
#endif

namespace fs = std::filesystem;

JavaManager::JavaManager() {}

fs::path JavaManager::obtenerRutaProyecto() {
    fs::path exeDir = platform::ejecutableDir();

    // ============================================
    // Modo desarrollo:
    //   bin/Debug/Riemann(.exe) -> raíz del proyecto
    // ============================================
    if (exeDir.filename() == "Debug" && exeDir.parent_path().filename() == "bin")
        return exeDir.parent_path().parent_path();

    // ============================================
    // Modo release: el ejecutable está en la raíz
    // ============================================
    return exeDir;
}

void JavaManager::ejecutarJarEnThread() {
    std::thread t(&JavaManager::ejecutarJar, this);
    t.detach();
}

void JavaManager::ejecutarJar() {
    fs::path root = obtenerRutaProyecto();

    std::cout << "[DEBUG] Ruta base detectada: " << root.string() << std::endl;

    // =========================
    // Rutas relativas al proyecto
    // =========================
#ifdef _WIN32
    fs::path javaExe     = root / "java" / "bin" / "java.exe";
    // Natives de JavaFX en Windows viven en javaFx\bin (glass.dll, prism_*.dll...)
    fs::path fxNativeDir = root / "javaFx" / "bin";
#else
    fs::path javaExe     = root / "java" / "bin" / "java";
    // En Linux el SDK de Gluon coloca jars y .so en javaFx/lib
    fs::path fxNativeDir = root / "javaFx" / "lib";
#endif
    fs::path fxLib       = root / "javaFx" / "lib";
    fs::path jarFile     = root / "Interfaz" / "target" / "Interfaz-Riemann.jar";
    fs::path deps        = root / "Interfaz" / "target" / "dependency";
    fs::path logFile     = root / "java.log";
    fs::path workingDir  = root / "Interfaz";

    // =========================
    // Verificaciones
    // =========================
    bool error = false;
    auto check = [&error](bool ok, const char* msg, const fs::path& p) {
        if (!ok) {
            std::cerr << msg << ": " << p.string() << std::endl;
            error = true;
        }
    };
    check(fs::exists(javaExe),     "[ERROR] java NO encontrado en", javaExe);
    check(fs::exists(fxLib),       "[ERROR] Carpeta JavaFX lib NO encontrada en", fxLib);
    check(fs::exists(fxNativeDir), "[ERROR] Carpeta nativa JavaFX NO encontrada en", fxNativeDir);
    check(fs::exists(jarFile),     "[ERROR] Interfaz.jar NO encontrado en", jarFile);

    if (error) {
        std::cerr << "[ERROR] No se puede iniciar JavaFX por archivos faltantes." << std::endl;
        return;
    }

#ifdef _WIN32
    // =========================
    // Windows: CreateProcessW
    // Los nativos de JavaFX (glass.dll, prism_*.dll, ...) viven en javaFx\bin;
    // hay que indicarlo con -Djava.library.path o JavaFX no arranca.
    // =========================
    std::wstring command =
        L"\"" + javaExe.wstring() + L"\" "
        L"-Djava.library.path=\"" + fxNativeDir.wstring() + L"\" "
        L"--module-path \"" + fxLib.wstring() + L"\" "
        L"--add-modules javafx.controls,javafx.fxml,javafx.web "
        L"-cp \"" + jarFile.wstring() + L";" + deps.wstring() + L"\\*\" "
        L"aplication.App";

    std::vector<wchar_t> cmd(command.begin(), command.end());
    cmd.push_back(L'\0');

    // Redirigir la salida de Java a java.log para poder diagnosticar fallos
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hLog = CreateFileW(
        logFile.wstring().c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        &sa,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    HANDLE hOut = (hLog != INVALID_HANDLE_VALUE) ? hLog : GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn  = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn == nullptr || hIn == INVALID_HANDLE_VALUE) hIn = hOut;

    si.dwFlags    = STARTF_USESTDHANDLES;
    si.hStdInput  = hIn;
    si.hStdOutput = hOut;
    si.hStdError  = hOut;

    PROCESS_INFORMATION pi{};

    BOOL ok = CreateProcessW(
        nullptr,
        cmd.data(),
        nullptr,
        nullptr,
        TRUE,
        0,
        nullptr,
        workingDir.wstring().c_str(),
        &si,
        &pi
    );

    if (!ok) {
        std::cerr << "[ERROR] CreateProcessW falló: " << GetLastError() << std::endl;
        if (hLog != INVALID_HANDLE_VALUE) CloseHandle(hLog);
        return;
    }

    std::cout << "[C++] JavaFX ejecutándose... (salida en java.log)" << std::endl;

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (hLog != INVALID_HANDLE_VALUE) CloseHandle(hLog);

    if (exitCode == 0) {
        std::cout << "[C++] JavaFX terminada. Cerrando aplicación C++..." << std::endl;
        exit(0);
    }

    std::cerr << "[ERROR] JavaFX terminó con código " << exitCode
              << ". Revisa '" << logFile.string() << "' para ver la causa." << std::endl;
    std::cerr << "[ERROR] La consola se mantiene abierta para mostrar el problema." << std::endl;
#else
    // =========================
    // Linux / macOS: fork + execvp
    // =========================
    // La biblioteca nativa puede estar en javaFx/lib (SDK de Gluon) y, en algunas
    // variantes, también en javaFx/bin. Se añaden las que existan.
    std::string libPath = fxNativeDir.string();
    if (fs::exists(root / "javaFx" / "bin"))
        libPath += ":" + (root / "javaFx" / "bin").string();

    std::string classpath = jarFile.string() + ":" + (deps / "*").string();

    std::vector<std::string> args = {
        javaExe.string(),
        "-Djava.library.path=" + libPath,
        "--module-path", fxLib.string(),
        "--add-modules", "javafx.controls,javafx.fxml,javafx.web",
        "-cp", classpath,
        "aplication.App"
    };

    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    // Redirigir stdout/stderr de Java a java.log
    int fd = ::open(logFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "[ERROR] fork falló: " << std::strerror(errno) << std::endl;
        if (fd >= 0) ::close(fd);
        return;
    }

    if (pid == 0) {
        // Hijo
        if (fd >= 0) {
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            ::close(fd);
        }
        if (!workingDir.empty() && chdir(workingDir.c_str()) != 0) {
            _exit(126);
        }
        execvp(argv[0], argv.data());
        _exit(127);
    }

    if (fd >= 0) ::close(fd);

    std::cout << "[C++] JavaFX ejecutándose... (salida en java.log)" << std::endl;

    int status = 0;
    waitpid(pid, &status, 0);
    int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : 1;

    if (exitCode == 0) {
        std::cout << "[C++] JavaFX terminada. Cerrando aplicación C++..." << std::endl;
        exit(0);
    }

    std::cerr << "[ERROR] JavaFX terminó con código " << exitCode
              << ". Revisa '" << logFile.string() << "' para ver la causa." << std::endl;
    std::cerr << "[ERROR] La consola se mantiene abierta para mostrar el problema." << std::endl;
#endif
}
