#ifndef PYTHON_MANAGER_H
#define PYTHON_MANAGER_H

#include <string>
#include <thread>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <nlohmann/json.hpp>
#include "Dominio.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

extern std::atomic<bool> pythonScriptRunning;

class PythonManager {
public:
    PythonManager();
    void ejecutarScriptPythonEnThread(Dominio &dominio, double zoom, double pan_x, double pan_y);

private:
    std::string obtenerDirectorioBase();
    std::string obtenerDirectorioRaiz();
    bool fileExists(const std::string &path);
    void leerParametros(double &zoom, double &pan_x, double &pan_y);
    void ejecutarScriptPython();
};

#endif
