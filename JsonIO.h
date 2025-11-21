//
// Created by Pop90 on 17/11/2025.
//
#ifndef JSONIO_H
#define JSONIO_H
#include <string>
#include <filesystem>



class JsonIO {
public:
    static bool leerFuncion(
        const std::string& ruta,
        std::string& f,
        std::string& li,
        std::string& ls,
        std::string& n
    );

    static void escribirResultado(
        double resultado,
        double deltaX,
        const std::string& ruta
    );

    static bool leerFlag(const std::string &ruta);

    static void escribirFlag(bool valor, const std::string &ruta);

    static std::string hashArchivo(const std::string &ruta);

};


#endif //JSONIO_H
