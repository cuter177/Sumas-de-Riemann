cmake_minimum_required(VERSION 3.30)
project(Documents)

set(CMAKE_CXX_STANDARD 17)

# Incluir headers de la carpeta include/ del proyecto
include_directories(${CMAKE_SOURCE_DIR}/include)

# Configura carpeta de salida
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/Debug)

# Archivos fuente
add_executable(Riemann
        main.cpp
        cutintegral.cpp
        Dominio.cpp
        ExpressionParser.cpp
        toPostFix.cpp
        Utils.cpp
        PythonManager.cpp
        JsonIO.cpp
        JavaManager.cpp
)

# Si necesitas librerías adicionales en Windows
if(WIN32)
    target_link_libraries(Riemann shlwapi)
endif()
