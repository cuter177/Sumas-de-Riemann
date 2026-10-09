#pragma once
// Platform.h
// Utilidades multiplataforma (Windows / Linux / macOS).

#include <filesystem>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#elif defined(__APPLE__)
  #include <mach-o/dyld.h>
  #include <cstdint>
  #include <vector>
#else
  #include <unistd.h>
#endif

namespace platform {

// Ruta absoluta del ejecutable en curso:
//   Windows -> GetModuleFileNameW
//   Linux   -> /proc/self/exe
//   macOS   -> _NSGetExecutablePath
inline std::filesystem::path ejecutablePath() {
#ifdef _WIN32
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0) return std::filesystem::current_path();
    return std::filesystem::path(buf);
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buf(size + 1, 0);
    if (_NSGetExecutablePath(buf.data(), &size) != 0)
        return std::filesystem::current_path();
    return std::filesystem::weakly_canonical(std::filesystem::path(buf.data()));
#else
    std::error_code ec;
    std::filesystem::path p = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (ec || p.empty()) return std::filesystem::current_path();
    return std::filesystem::weakly_canonical(p, ec);
#endif
}

// Directorio que contiene el ejecutable.
inline std::filesystem::path ejecutableDir() {
    return ejecutablePath().parent_path();
}

} // namespace platform
