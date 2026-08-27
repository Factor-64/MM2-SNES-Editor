cmake_minimum_required(VERSION 3.16)

project(SNTE LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL_TEST OFF CACHE BOOL "" FORCE)

add_subdirectory(external/SDL)
add_subdirectory(external/json)

add_library(glad external/glad/src/glad.c "src/game/level.cpp" "src/game/level.h" "src/game/objects.cpp" "src/game/objects.h" "src/game/collision.h" "src/snes/address.h"  "src/snes/address.cpp")
target_include_directories(glad PUBLIC external/glad/include)

add_library(ImGuiFileDialog external/ImGuiFileDialog/ImGuiFileDialog.cpp "src/game/level.cpp" "src/game/level.h" "src/game/objects.cpp" "src/game/objects.h" "src/game/collision.h" "src/snes/address.h"  "src/snes/address.cpp")
target_include_directories(ImGuiFileDialog PUBLIC external/ImGuiFileDialog)

file(GLOB IMGUI_SOURCES
    external/imgui/*.cpp
    external/imgui/backends/imgui_impl_sdl3.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

add_library(imgui ${IMGUI_SOURCES} "src/game/level.cpp" "src/game/level.h" "src/game/objects.cpp" "src/game/objects.h" "src/game/collision.h" "src/snes/address.h"  "src/snes/address.cpp")
target_include_directories(imgui PUBLIC external/imgui external/imgui/backends)

target_link_libraries(ImGuiFileDialog PUBLIC imgui)

target_link_libraries(imgui PUBLIC SDL3::SDL3 glad)

add_library(lodepng external/lodepng/lodepng.cpp)
target_include_directories(lodepng PUBLIC external/lodepng)

target_link_libraries(glad PUBLIC lodepng)

add_executable(SNTE
    src/main.cpp
    src/app.h src/app.cpp
    src/ui/mainmenu.h src/ui/mainmenu.cpp
    src/mm2data.h src/mm2data.cpp
    src/snes/cgramdecoder.h src/snes/cgramdecoder.cpp
    src/snes/snesheaderreader.h src/snes/snesheaderreader.cpp
    src/snes/tiledecoder.h src/snes/tiledecoder.cpp
    src/snes/tilemap.h src/snes/tilemap.cpp    
    src/snes/palette.h
    src/game/level.h src/game/level.cpp
    src/game/objects.h src/game/objects.cpp
    src/game/collision.h 
    src/snes/address.h src/snes/address.cpp
    src/game/checkpoint.h src/game/checkpoint.cpp
    src/imageloader.h src/imageloader.cpp
)

target_link_libraries(SNTE
    PRIVATE
        SDL3::SDL3
        glad
        imgui
        nlohmann_json::nlohmann_json
        ImGuiFileDialog
)

include(GNUInstallDirs)
install(TARGETS SNTE
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
