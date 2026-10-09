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
        cutIntegral.cpp
        Dominio.cpp
        ExpressionParser.cpp
        toPostFix.cpp
        Utils.cpp
        PythonManager.cpp
        JsonIO.cpp
        JavaManager.cpp
)

# Hilos (std::thread / std::async). En Linux es obligatorio enlazar pthread.
find_package(Threads REQUIRED)
target_link_libraries(Riemann PRIVATE Threads::Threads)

# Si necesitas librerías adicionales en Windows
if(WIN32)
    target_link_libraries(Riemann shlwapi)

    # Enlazado estático del runtime de MinGW para release portable:
    # evita depender de libgcc_s_seh-1.dll, libstdc++-6.dll y libwinpthread-1.dll
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(Riemann PRIVATE
            -static
            -static-libgcc
            -static-libstdc++
        )
    endif()
endif()

# En Linux se enlazan estáticamente libstdc++/libgcc para reducir la dependencia
# de versiones concretas de la distro (la libc del sistema sí se usa).
if(UNIX AND NOT APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(Riemann PRIVATE
        -static-libstdc++
        -static-libgcc
    )
endif()
