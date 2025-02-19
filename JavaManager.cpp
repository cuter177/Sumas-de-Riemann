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
    std::wstring jarFile = root + L"\\Interfaz\\target\\Interfaz-Riemann.jar";
    std::wstring deps    = root + L"\\Interfaz\\target\\dependency\\*";

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
        std::wcerr << L"[ERROR] Carpeta JavaFX NO encontrada en: "
                   << fxLib << std::endl;
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
    // =========================
    std::wstring command =
        L"\"" + javaExe + L"\" "
        L"--module-path \"" + fxLib + L"\" "
        L"--add-modules javafx.controls,javafx.fxml,javafx.web "
        L"-cp \"" + jarFile + L";" + deps + L"\" "
        L"aplication.App";

    std::vector<wchar_t> cmd(command.begin(), command.end());
    cmd.push_back(L'\0');

    STARTUPINFOW si{};
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi{};

    // Directorio de trabajo: Interfaz
    std::wstring workingDir = root + L"\\Interfaz";

    BOOL ok = CreateProcessW(
        nullptr,
        cmd.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        workingDir.c_str(),
        &si,
        &pi
    );

    if (!ok) {
        std::wcerr << L"[ERROR] CreateProcessW falló: "
                   << GetLastError() << std::endl;
        return;
    }

    std::wcout << L"[C++] JavaFX ejecutándose..." << std::endl;

    // Esperar a que cierre JavaFX
    WaitForSingleObject(pi.hProcess, INFINITE);

    std::wcout << L"[C++] JavaFX terminada. Cerrando aplicación C++..."
               << std::endl;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    exit(0);
}