cmake_minimum_required(VERSION 3.16)
project(BackendControl VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(FATAL_ERROR "This project must be built as x64. Reconfigure with: cmake -A x64 -S . -B build")
    endif()
    add_compile_options(/utf-8)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_compile_options(/wd4101 /wd4244 /wd4267)
    add_compile_options(/Zc:inline)
    add_link_options(/FORCE:MULTIPLE)
endif()

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# =========================
# Packages (vcpkg)
# =========================
find_package(SDL3 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(cppzmq CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(Threads REQUIRED)

# =========================
# IMGUI (SDL3 + OpenGL3)
# =========================
add_library(tp_imgui STATIC
    ${THIRD_PARTY_DIR}/imgui/imgui.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_draw.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_tables.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_sdl3.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(tp_imgui PUBLIC
    ${THIRD_PARTY_DIR}/imgui
    ${THIRD_PARTY_DIR}/imgui/backends
)
target_link_libraries(tp_imgui PUBLIC
    SDL3::SDL3
    GLEW::GLEW
    OpenGL::GL
)

# =========================
# IMPLOT
# =========================
add_library(tp_implot STATIC
    ${THIRD_PARTY_DIR}/implot/implot.cpp
    ${THIRD_PARTY_DIR}/implot/implot_items.cpp
)
target_include_directories(tp_implot PUBLIC
    ${THIRD_PARTY_DIR}/implot
    ${THIRD_PARTY_DIR}/imgui
)
target_link_libraries(tp_implot PUBLIC tp_imgui)

# =========================
# SOURCES (из нового CMake)
# =========================
add_executable(backend_control
    src/main.cpp
    src/gui_app.cpp
    src/gui_tabs.cpp
    src/net/zmq_receiver.cpp
    src/core/json_parser.cpp
    src/db/pgsql_minimal.cpp
    src/network/osm_tile_fetcher.cpp
    src/network/osm_tile_texture.cpp
    src/ui/menu_bar.cpp
    src/ui/data_window.cpp
    src/ui/graph_window.cpp
    src/ui/map_window.cpp
    src/ui/ui_widget.cpp
    src/logging.cpp
    src/core/heatmap_generator.cpp
)

target_include_directories(backend_control PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${THIRD_PARTY_DIR}/imgui
    ${THIRD_PARTY_DIR}/implot
    ${THIRD_PARTY_DIR}/stb
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(backend_control PRIVATE
    tp_imgui
    tp_implot
    SDL3::SDL3
    GLEW::GLEW
    OpenGL::GL
    cppzmq
    nlohmann_json::nlohmann_json
    CURL::libcurl
    PostgreSQL::PostgreSQL
    Threads::Threads
)

if(WIN32)
    target_link_libraries(backend_control PRIVATE
        ws2_32
        wsock32
        wininet
        iphlpapi
    )

    set(DEPLOY_DIR $<TARGET_FILE_DIR:backend_control>)
        add_custom_command(TARGET backend_control POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:SDL3::SDL3>" "${DEPLOY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:GLEW::GLEW>" "${DEPLOY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:CURL::libcurl>" "${DEPLOY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEPLOY_DIR}/fonts"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEPLOY_DIR}/cache"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/fonts/jetbrains.ttf" "${DEPLOY_DIR}/fonts/jetbrains.ttf"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/drive_test_log.json" "${DEPLOY_DIR}/drive_test_log.json"
        VERBATIM
    )
endif()

set_target_properties(backend_control PROPERTIES WIN32_EXECUTABLE OFF)
set(VCPKG_APPLOCAL_DEPS ON CACHE BOOL "Auto-copy vcpkg dependencies")