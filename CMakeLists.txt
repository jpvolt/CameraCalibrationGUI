cmake_minimum_required(VERSION 3.0)

project( DistortionCorrection )

find_package( OpenCV REQUIRED )

include_directories(src/vendor/imgui)
include_directories(src/vendor/glew/include)
add_subdirectory(src/vendor/glew/build/cmake)

file(GLOB src_imgui
    "src/vendor/imgui/*.h"
    "src/vendor/imgui/*.cpp"
    
)

add_executable( DistortionCorrection 
src/main.cpp
${src_imgui})
target_link_libraries( DistortionCorrection  ${OpenCV_LIBS} SDL2main SDL2 glew)
