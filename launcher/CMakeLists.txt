project(Launcher)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Setup ImGui
include(FetchContent)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui
  GIT_TAG        v1.90
)
FetchContent_MakeAvailable(imgui)

set(SOURCES
    src/main.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

# GUI application (uses WinMain)
add_executable(launcher WIN32 ${SOURCES})

target_include_directories(launcher PRIVATE 
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    src
)

# Static link runtime
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(launcher PRIVATE 
    d3d11
    d3dcompiler
    dwmapi
    wininet
    kernel32 
    user32 
    advapi32
    shell32
)
