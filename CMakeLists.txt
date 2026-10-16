cmake_minimum_required(VERSION 3.16)
project(cat_gif LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in vendored/SDL
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

# SDL_image, with only the GIF decoder enabled (it has no external dependencies).
set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_INSTALL OFF CACHE BOOL "" FORCE)
foreach(fmt ANI AVIF BMP JPG JXL LBM PCX PNG PNM QOI SVG TGA TIF WEBP XCF XPM XV)
    set(SDLIMAGE_${fmt} OFF CACHE BOOL "" FORCE)
endforeach()
set(SDLIMAGE_GIF ON CACHE BOOL "" FORCE)
add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)

add_executable(cat_gif
    main.cpp
    gif_loader.cpp
    gif_displayer.cpp)

target_link_libraries(cat_gif PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)

# fallback.gif is supplied by the build/install system; copy it next to the
# executable when present (the program looks for ./fallback.gif at runtime).
if(EXISTS "${CMAKE_SOURCE_DIR}/fallback.gif")
    add_custom_command(TARGET cat_gif POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/fallback.gif" "$<TARGET_FILE_DIR:cat_gif>/fallback.gif")
endif()
