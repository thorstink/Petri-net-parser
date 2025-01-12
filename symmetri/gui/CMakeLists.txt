cmake_minimum_required(VERSION 3.26)
project(Farbart)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(glfw3 REQUIRED)

if(ASAN_BUILD AND NOT TSAN_BUILD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address,undefined -fno-omit-frame-pointer -O0")
elseif(TSAN_BUILD AND NOT ASAN_BUILD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=thread -fno-omit-frame-pointer -O0")
endif()

# just because metal it seems
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")

execute_process(COMMAND git describe --dirty --always --tags OUTPUT_VARIABLE FOO)
message(${FOO})

include_directories(
    rpp
    imgui
    imgui/backends
    ../externals
    ../
    gui
    extensions
)

set(APP_SOURCES
    reducers.cpp
    draw_context_menu.cpp
    draw_graph.cpp
    draw_menu.cpp
    draw_ui.cpp
    draw_about.cpp
    simulation_menu.cpp
    model.cpp
    load_file.cpp
    menu_bar.cpp
    draw_graph.cpp
    rxdispatch.cpp
    position_parsers.cpp
    write_graph_to_disk.cpp
    shared.cpp
)

set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/backends/imgui_impl_glfw.cpp
)

find_library(OPENGL_LIBRARY OpenGL)
set(EXTRA_LIBS
    ${OPENGL_LIBRARY}
    glfw
)
if(APPLE)
    message("Using Metal backend")
    include_directories(/opt/homebrew/include)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    set(ALL_SOURCES ${APP_SOURCES} ${IMGUI_SOURCES} imgui/backends/imgui_impl_metal.mm metal_main.mm)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY} "-framework Metal" "-framework MetalKit" "-framework AppKit" "-framework Foundation" "-framework QuartzCore")
else()
    message("Using OpenGL3 backend")
    set(ALL_SOURCES ${APP_SOURCES} ${IMGUI_SOURCES} imgui/backends/imgui_impl_opengl3.cpp opengl3_main.cpp)
endif()

add_executable(Farbart ${ALL_SOURCES})
target_link_libraries(Farbart static_symmetri ${EXTRA_LIBS})
target_compile_definitions(Farbart PRIVATE VERSION=${FOO})

if (APPLE)
    add_executable(generate_icon generate_icon.cpp  ../submodules/tinyxml2/tinyxml2.cpp)
    add_dependencies(generate_icon Farbart)
    add_custom_command(TARGET generate_icon POST_BUILD
    COMMAND bash -c "bash ../.github/workflows/macos/create_app_bundle.sh"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build
    COMMENT "Creating MacOS app-bundle"
)
endif()
