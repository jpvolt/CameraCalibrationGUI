
# CameraCalibrationGUI

Camera calibration GUI util for opencv library [CameraCalibration](https://github.com/jpvolt/CameraCalibration) 

## **How to build:**
 *requirements:*
 - Ubuntu 18.04
 - Cmake
 - Sdl2
 - Opencv3.4+
 - g++
 - `sudo apt install cmake libsdl2-dev g++`
 
 *Building:*
 
 - `git clone https://github.com/jpvolt/CameraCalibrationGUI`
 - `cd CameraCalibrationGUI `
 - `mkdir build`
 - `cd build`
 - `cmake ..`
 - `make -j4`
 - `./DistortionCorrection`


*TODO*

-MACOS support need diferent linkage of SDL2.
-MACOS support need building through xcode
-Improve building on Linux/MACOS - maybe install opencv/sdl2 automatically?
