cmake_minimum_required(VERSION 3.16)
project(BackendControl VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_compile_options(/wd4101 /wd4244 /wd4267)
    add_compile_options(/Zc:inline)  # Добавьте эту строку
endif()

# =========================
# Пакеты
# =========================
find_package(SDL2 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(cppzmq CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)

# =========================
# PostgreSQL / libpqxx
# =========================
find_package(libpqxx CONFIG REQUIRED)

# libpq не имеет CMake конфига, ищем вручную
find_path(PQ_INCLUDE_DIR NAMES libpq-fe.h
    HINTS "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/include"
)

find_library(PQ_LIB NAMES pq libpq
    HINTS "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/lib"
)

if(PQ_INCLUDE_DIR AND PQ_LIB)
    message(STATUS "Found libpq: ${PQ_LIB}")
    message(STATUS "Found libpq include: ${PQ_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "libpq not found in vcpkg!")
endif()

# Пути к DLL для копирования
set(PQ_LIB_DLL "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/libpq.dll")
set(PQXX_LIB_DLL "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/libpqxx.dll")

# =========================
# IMGUI
# =========================
add_library(imgui STATIC
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_sdl2.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    third_party/imgui
    third_party/imgui/backends
)

target_link_libraries(imgui PRIVATE
    SDL2::SDL2
    GLEW::GLEW
    OpenGL::GL
)

# =========================
# IMPLOT
# =========================
add_library(implot STATIC
    third_party/implot/implot.cpp
    third_party/implot/implot_items.cpp
)

target_include_directories(implot PUBLIC
    third_party/implot
    third_party/imgui
)

target_link_libraries(implot PRIVATE imgui)

# =========================
# SOURCES
# =========================
set(SOURCES
    src/main.cpp
    src/tile_manager.cpp
    src/curl_funcs.cpp
    src/map_renderer.cpp
    src/database.cpp
    src/app_gui.cpp
    src/app_server.cpp
    src/app_utils.cpp
)

set(HEADERS
    src/shared_structs.h
    src/tile_manager.h
    src/curl_funcs.h
    src/map_renderer.h
    src/database.h
)

add_executable(backend_control ${SOURCES} ${HEADERS})

set_target_properties(backend_control PROPERTIES WIN32_EXECUTABLE OFF)

target_include_directories(backend_control PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${PQ_INCLUDE_DIR}
    third_party/stb
    third_party/imgui
    third_party/imgui/backends
    third_party/implot
)

target_link_libraries(backend_control PRIVATE
    implot
    imgui
    SDL2::SDL2
    GLEW::GLEW
    OpenGL::GL
    cppzmq
    nlohmann_json::nlohmann_json
    CURL::libcurl
    libpqxx::pqxx
    ${PQ_LIB}
    ws2_32
    wsock32
    wininet
)

# =========================
# POST_BUILD
# =========================
if(WIN32)
    # Копируем все DLL из vcpkg в папку с exe
    add_custom_command(TARGET backend_control POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:backend_control>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/SDL2.dll"
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/glew32.dll"
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/libcurl.dll"
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/libpq.dll"
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/pqxx.dll"
            "$<TARGET_FILE_DIR:backend_control>"
    )
    
    # Копируем шрифт
    add_custom_command(TARGET backend_control POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:backend_control>/fonts"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/consola.ttf" "$<TARGET_FILE_DIR:backend_control>/fonts/jetbrains.ttf"
    )
    
    # Копируем остальные файлы
    add_custom_command(TARGET backend_control POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/drive_test_log.json" "$<TARGET_FILE_DIR:backend_control>"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:backend_control>/tiles"
    )
endif()