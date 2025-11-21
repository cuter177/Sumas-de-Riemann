//
// Created by Pop90 on 17/11/2025.
//
#ifndef JAVAMANAGER_H
#define JAVAMANAGER_H

#include <string>
#include <thread>

class JavaManager {
public:
    JavaManager();
    void ejecutarJarEnThread();

private:
    void ejecutarJar();
    std::wstring obtenerRutaProyecto();
};

#endif
