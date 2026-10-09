//
// Created by Pop90 on 17/11/2025.
//

#include "JsonIO.h"
#include <fstream>
#include <iostream>
#include <thread>
#include <cmath>
#include <nlohmann/json.hpp>
#include <filesystem>
namespace fs = std::filesystem;

using json = nlohmann::json;

bool JsonIO::leerFuncion(const std::string& ruta,
                         std::string& f,
                         std::string& li,
                         std::string& ls,
                         std::string& n)
{
    try {
        std::ifstream file(ruta);
        if (!file.is_open()) {
            std::cerr << "No se pudo abrir JSON en: " << ruta << "\n";
            return false;
        }

        json j;
        file >> j;

        f  = j["funcion"].get<std::string>();
        li = j["li"].get<std::string>();
        ls = j["ls"].get<std::string>();
        n  = j["numRectangulos"].get<std::string>();

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error leyendo JSON: " << e.what() << "\n";
        return false;
    }
}


void JsonIO::escribirResultado(double resultado, double deltaX, bool ok, const std::string& ruta)
{
    // Nunca escribir NaN/Inf: nlohmann los serializa como null y la interfaz no
    // puede leerlos (provoca un NullPointerException). Si el resultado no es
    // finito se marca como inválido.
    bool okFinal = ok && std::isfinite(resultado) && std::isfinite(deltaX);

    json j;
    j["ok"] = okFinal;
    if (okFinal) {
        j["resultado_integral"] = resultado;
        j["delta_x"] = deltaX;
    } else if (ok) {
        // La expresión es válida pero el resultado no es finito: la función no
        // está definida en (algún punto de) el intervalo y la integral diverge.
        j["mensaje"] = "La función no es finita en el intervalo (la integral diverge o la función no está definida ahí).";
    }

    std::ofstream file(ruta);
    if (!file) {
        std::cerr << "Error escribiendo resultado en JSON en: " << ruta << "\n";
        return;
    }

    file << j.dump(4);
}

std::string JsonIO::hashArchivo(const std::string& ruta) {
    for (int i = 0; i < 10; i++) {
        std::ifstream file(ruta);

        if (!file.is_open()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            continue;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        std::string contenido = buffer.str();

        //  Si Java aún está escribiendo, el archivo puede estar vacío
        if (contenido.size() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            continue;
        }

        std::hash<std::string> hasher;
        return std::to_string(hasher(contenido));
    }

    return ""; // no se pudo leer un archivo válido
}












