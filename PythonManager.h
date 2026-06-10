#pragma once
// PythonManager.h
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "Dominio.h"

namespace fs = std::filesystem;
using json   = nlohmann::json;

extern std::atomic<bool> pythonScriptRunning;

class PythonManager {
public:
    PythonManager();
    void ejecutarScriptPythonEnThread(
        Dominio& dominio,
        double zoom  = 1.0,
        double pan_x = 0.0,
        double pan_y = 0.0);

private:
    void        ejecutarScriptPython();
    void        leerParametros(double& zoom, double& pan_x, double& pan_y);
    std::string obtenerDirectorioBase();
    std::string obtenerDirectorioRaiz();
    bool        fileExists(const std::string& path);
};