cmake_minimum_required(VERSION 4.2)
project(PolySolver LANGUAGES CXX)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Option to enable/disable static compilation
option(STATIC_BUILD "Enable static compilation" ON)

if(STATIC_BUILD)
    message(STATUS "Configuring static compilation...")
    if(MSVC)
        # Statically link MSVC C/C++ runtime (MT / MTd)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        # Statically link standard libraries for MinGW, GCC, Clang
        # macOS does not support fully static binaries via -static
        if(NOT APPLE)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
        else()
            message(WARNING "Static compilation (-static) is not supported on macOS. Compiling normally.")
        endif()
    endif()
endif()

# Core polynomial solver library
add_library(PolyCore STATIC
    src/polynomial.cpp
        lib/polynomial.h
    src/step_solver.cpp
        lib/step_solver.h
)
target_include_directories(PolyCore PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# Подключаем библиотеку Dear ImGui через CMake FetchContent.
# Поскольку у официального ImGui в репозитории нет файла CMakeLists.txt,
# мы не используем FetchContent_MakeAvailable (который пытается сделать add_subdirectory).
# Вместо этого мы скачиваем файлы через FetchContent_Populate и настраиваем сборку библиотеки вручную.
# Для новых версий CMake (3.30+ / 4.x) отключаем предупреждение/ошибку политики CMP0169:
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.8
)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# Создаем статическую библиотеку imgui из скачанных исходных файлов
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Main GUI / Application Executable
add_executable(PolySolver src/main.cpp src/file_utils.cpp)
target_link_libraries(PolySolver PRIVATE 
    PolyCore 
    imgui 
    d3d11 
    d3dcompiler 
    dxgi 
    dwmapi
    comdlg32
)

# Set the subsystem to Windows for PolySolver so it launches as a GUI app without a console window (in Release builds)
# We can keep it console-enabled for Debug if desired, or set it universally. Let's make it WIN32.
set_target_properties(PolySolver PROPERTIES WIN32_EXECUTABLE TRUE)

# Unit Tests Executable (non-GUI)
add_executable(PolySolverTests src/test_polynomial.cpp)
target_link_libraries(PolySolverTests PRIVATE PolyCore)
