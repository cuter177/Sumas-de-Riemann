#include "JavaManager.h"
#include <windows.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <thread>

namespace fs = std::filesystem;

JavaManager::JavaManager() {}

std::wstring JavaManager::obtenerRutaProyecto() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);

    // Ruta completa del ejecutable
    fs::path exeDir = fs::path(buffer).parent_path();

    // ============================================
    // Modo desarrollo:
    //   Riemann_2.0/bin/Debug/Documents.exe
    // ============================================
    if (exeDir.filename() == L"Debug" &&
        exeDir.parent_path().filename() == L"bin") {
        return exeDir.parent_path().parent_path().wstring();
    }

    // ============================================
    // Modo release:
    //   Riemann_2.0/Documents.exe
    // ============================================
    return exeDir.wstring();
}

void JavaManager::ejecutarJarEnThread() {
    std::thread t(&JavaManager::ejecutarJar, this);
    t.detach();
}

void JavaManager::ejecutarJar() {
    std::wstring root = obtenerRutaProyecto();

    std::wcout << L"[DEBUG] Ruta base detectada: " << root << std::endl;

    // =========================
    // Rutas relativas al proyecto
    // =========================
    std::wstring javaExe = root + L"\\java\\bin\\java.exe";
    std::wstring fxLib   = root + L"\\javaFx\\lib";
    std::wstring fxBin   = root + L"\\javaFx\\bin";
    std::wstring jarFile = root + L"\\Interfaz\\target\\Interfaz-Riemann.jar";
    std::wstring deps    = root + L"\\Interfaz\\target\\dependency\\*";
    std::wstring logFile = root + L"\\java.log";

    // =========================
    // Verificaciones
    // =========================
    bool error = false;

    if (!fs::exists(javaExe)) {
        std::wcerr << L"[ERROR] java.exe NO encontrado en: "
                   << javaExe << std::endl;
        error = true;
    }

    if (!fs::exists(fxLib)) {
        std::wcerr << L"[ERROR] Carpeta JavaFX lib NO encontrada en: "
                   << fxLib << std::endl;
        error = true;
    }

    if (!fs::exists(fxBin)) {
        std::wcerr << L"[ERROR] Carpeta JavaFX bin (DLLs nativos) NO encontrada en: "
                   << fxBin << std::endl;
        error = true;
    }

    if (!fs::exists(jarFile)) {
        std::wcerr << L"[ERROR] Interfaz.jar NO encontrado en: "
                   << jarFile << std::endl;
        error = true;
    }

    if (error) {
        std::wcerr << L"[ERROR] No se puede iniciar JavaFX por archivos faltantes."
                   << std::endl;
        return;
    }

    // =========================
    // Construcción del comando
    // Los nativos de JavaFX (glass.dll, prism_*.dll, ...) viven en javaFx\bin;
    // hay que indicarlo con -Djava.library.path o JavaFX no arranca.
    // =========================
    std::wstring command =
        L"\"" + javaExe + L"\" "
        L"-Djava.library.path=\"" + fxBin + L"\" "
        L"--module-path \"" + fxLib + L"\" "
        L"--add-modules javafx.controls,javafx.fxml,javafx.web "
        L"-cp \"" + jarFile + L";" + deps + L"\" "
        L"aplication.App";

    std::vector<wchar_t> cmd(command.begin(), command.end());
    cmd.push_back(L'\0');

    // Redirigir la salida de Java a java.log para poder diagnosticar fallos
    // incluso si la consola se cierra.
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hLog = CreateFileW(
        logFile.c_str(),
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

    // Directorio de trabajo: Interfaz
    std::wstring workingDir = root + L"\\Interfaz";

    BOOL ok = CreateProcessW(
        nullptr,
        cmd.data(),
        nullptr,
        nullptr,
        TRUE,
        0,
        nullptr,
        workingDir.c_str(),
        &si,
        &pi
    );

    if (!ok) {
        std::wcerr << L"[ERROR] CreateProcessW falló: "
                   << GetLastError() << std::endl;
        if (hLog != INVALID_HANDLE_VALUE) CloseHandle(hLog);
        return;
    }

    std::wcout << L"[C++] JavaFX ejecutándose... (salida en java.log)" << std::endl;

    // Esperar a que cierre JavaFX
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (hLog != INVALID_HANDLE_VALUE) CloseHandle(hLog);

    // Si JavaFX arrancó y el usuario la cerró, terminamos limpiamente.
    if (exitCode == 0) {
        std::wcout << L"[C++] JavaFX terminada. Cerrando aplicación C++..."
                   << std::endl;
        exit(0);
    }

    // Si JavaFX falló, NO cerramos la app: dejamos la consola abierta con el
    // error visible y el detalle en java.log.
    std::wcerr << L"[ERROR] JavaFX terminó con código " << exitCode
               << L". Revisa '" << logFile << L"' para ver la causa." << std::endl;
    std::wcerr << L"[ERROR] La consola se mantiene abierta para mostrar el problema."
               << std::endl;
}