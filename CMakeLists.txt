cmake_minimum_required(VERSION 3.0)

project( DistortionCorrection )

find_package( OpenCV REQUIRED )

include_directories(src/vendor/imgui)
include_directories(src/vendor/glew/include)
add_subdirectory(src/vendor/glew/build/cmake)
set(CMAKE_CXX_STANDARD 11)
file(GLOB src_imgui
    "src/vendor/imgui/*.h"
    "src/vendor/imgui/*.cpp"
    
)
file(GLOB src_files
    "src/*.hpp"
    "src/*.cpp"
    
)

add_executable( DistortionCorrection 
${src_files}
${src_imgui})
target_link_libraries( DistortionCorrection  ${OpenCV_LIBS} SDL2  glew)
