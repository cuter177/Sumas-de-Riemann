#include "JavaManager.h"
#include <windows.h>
#include <iostream>
#include <filesystem>
#include <vector>
namespace fs = std::filesystem;

JavaManager::JavaManager() {}

std::wstring JavaManager::obtenerRutaProyecto() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    fs::path exePath(buffer);

    // bin/Debug -> bin -> Riemann_2.0
    fs::path root = exePath.parent_path().parent_path().parent_path();

    return root.wstring();
}

void JavaManager::ejecutarJarEnThread() {
    // YA NO detach() — queremos esperar el cierre del proceso Java
    std::thread t(&JavaManager::ejecutarJar, this);
    t.detach();  // <--- si quieres que NO bloquee el hilo principal
}

void JavaManager::ejecutarJar() {
    std::wstring root = obtenerRutaProyecto();

    // == Rutas ==
    std::wstring javaExe = root + L"\\java\\bin\\java.exe";
    std::wstring fxLib   = root + L"\\javaFx\\lib";
    std::wstring jarFile = root + L"\\Interfaz\\target\\Interfaz-Riemann.jar";
    std::wstring deps    = root + L"\\Interfaz\\target\\dependency\\*";

    // Verificaciones
    if (!fs::exists(javaExe))
        std::wcerr << L"[ERROR] java.exe NO encontrado en: " << javaExe << std::endl;

    if (!fs::exists(fxLib))
        std::wcerr << L"[ERROR] Carpeta JavaFX NO encontrada en: " << fxLib << std::endl;

    if (!fs::exists(jarFile))
        std::wcerr << L"[ERROR] Interfaz.jar NO encontrado en: " << jarFile << std::endl;

    // == Comando ==
    std::wstring command =
        L"\"" + javaExe + L"\" "
        L"--module-path \"" + fxLib + L"\" "
        L"--add-modules javafx.controls,javafx.fxml,javafx.web "
        L"-cp \"" + jarFile + L";" + deps + L"\" "
        L"aplication.App";

    std::vector<wchar_t> cmd(command.begin(), command.end());
    cmd.push_back(0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    // Working directory → /Interfaz
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
        std::wcerr << L"[ERROR] CreateProcessW falló: " << GetLastError() << std::endl;
        return;
    }

    std::wcout << L"[C++] JavaFX ejecutándose... Esperando su cierre." << std::endl;

    // ⏳ Espera hasta que JavaFX termine
    WaitForSingleObject(pi.hProcess, INFINITE);

    std::wcout << L"[C++] JavaFX terminada. Cerrando aplicación C++..." << std::endl;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    //  Cerrar programa C++
    exit(0);
}
