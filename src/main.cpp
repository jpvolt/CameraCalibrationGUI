#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>     
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <fstream>
#include "window.hpp"
using namespace cv;

#include "vendor/opencvtosdl.hpp" 
bool saved =  false;
bool inputEnabled = true;
bool end = false;

bool saveConfig(Mat& CamMat, Mat& distCoeffs, Mat& newCamMat, std::string filepath){
    static bool s = true;
    if(s){
        FileStorage fs(filepath, FileStorage::WRITE);
        fs << "cameraMatrix" << CamMat << "distCoeffs" << distCoeffs;
        fs.release();
        saved = true;
        s = false;
        end = true;
    }
}

void getCornerPositions(Size gridsize, float squareSize, std::vector<Point3f>& corners, int mode ){
    corners.clear();

    switch(mode){
        case 0:
        case 2:
            for( int i = 0; i < gridsize.height; ++i )
                for( int j = 0; j < gridsize.width; ++j )
                    corners.push_back(Point3f(float( j*squareSize ), float( i*squareSize ), 0));
            break;

        case 1:
            for( int i = 0; i < gridsize.height; i++ )
                for( int j = 0; j < gridsize.width; j++ )
                    corners.push_back(Point3f(float((2*j + i % 2)*squareSize), float(i*squareSize), 0));
            break;

        default:
            break;
    }
}

int main(int argc, char *argv[])
{
    Window window;
    SDL_Renderer* renderer = window.GetRenderer();
   
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool done = false;
    bool KEYS[322];
    for(int i = 0; i<322; ++i)
        KEYS[i] = false;
    
    while (!done)
    {   
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window.GetWindow()))
                done = true;
            if (event.type == SDL_KEYDOWN)
                if(event.key.keysym.sym <=322)
                    KEYS[event.key.keysym.sym] = true;
            if (event.type == SDL_KEYUP)
                if(event.key.keysym.sym <=322)
                    KEYS[event.key.keysym.sym] = false;
        }

        static int cameraIndex = 0;
        static int oldIndex = 0;
        static int captureNumber = 20;
        static char savepath[256] = "config.json";
        static int mode;
        enum{
            CHESS, ASYMCIRCLES, SYMCIRCLES
        };
        static int gridsize[2] = {9,6};
        static bool found = false;
        static bool Failed =  false;
        static bool changedMode = false;
        static float realsize = 1.0f;
        static std::vector<std::vector<Point2f>> FinalCorners;
        static bool showcorrected = false;
        static bool savefilebtn = false;
        static bool savefile = false;

        //imgui stuff 
        window.ImguiNewFrame();
        inputEnabled = true;
        {
           
            static ImVec4 textcolor = { 0.0f,0.0f,0.0f,1.0f };
            ImGui::Begin("Menu");                         

            ImGui::TextColored(ImVec4(0.8f, 1.0f, 1.0f, 1.0f),"Welcome to this Camera distortion calibration software!");
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 0.0f), "#");

            ImGui::TextColored(textcolor, "Camera Index:");
            ImGui::SliderInt("Default=0", &cameraIndex, 0, 5);

            ImGui::TextColored(textcolor, "Number of captures:");
            ImGui::SliderInt("Default=20", &captureNumber, 1, 100);

            ImGui::TextColored(textcolor, "Pattern type:");
            if(ImGui::RadioButton("ChessBoard", mode==CHESS)){
                mode = CHESS;
                changedMode = true;
            }
            if(ImGui::RadioButton("Asymmetric Cicles Grid", mode==ASYMCIRCLES)){
                mode = ASYMCIRCLES;
                changedMode = true;
            }
            if(ImGui::RadioButton("Symmetric Cicles Grid", mode==SYMCIRCLES)){
                mode = SYMCIRCLES;
                changedMode = true;
            }

            ImGui::TextColored(textcolor, "Grid size:");
            ImGui::SliderInt2("Default=9x6", gridsize, 1, 20);

            ImGui::TextColored(textcolor, "Show corrected preview(cpu intense):");
            ImGui::Checkbox("Default=False", &showcorrected);
                      
            ImGui::TextColored(textcolor, "Name of config file to save:");
            ImGui::InputText("Default=config.json",savepath, 256);

            if (!savefilebtn){
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }
            if(!end){
            if(ImGui::Button("Save", ImVec2(40, 20))){
                savefile = true;
            }
            }else{
                if(ImGui::Button("Exit", ImVec2(40, 20))){
                    return 0;
                }      
            }

            if (!savefilebtn){
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }
            
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 0.0f), "#");
            ImGui::TextColored(textcolor, "Press SPACE to capture a frame.");
            
            
            ImGui::Text("Captures remaining: (%d)",  captureNumber);        

            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 0.0f), "#");
            ImGui::Text("Application FPS: (%.1f FPS)",  ImGui::GetIO().Framerate);

            if (Failed){
                 ImGui::OpenPopup("Failed to find pattern!");
                 Failed = false;
             }
                
              
            bool open = true;
            if (ImGui::BeginPopupModal("Failed to find pattern!", &open))
            {
                inputEnabled = false;
                ImGui::Text("Failed to find a pattern! check if the pattern is visible and for possible errors in pattern settings on the menu.");
                if (ImGui::Button("Close"))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            if (saved){
                 ImGui::OpenPopup("Config file saved!");
                 saved = false;
            }

            bool open2 = true;
            if (ImGui::BeginPopupModal("Config file saved!", &open2))
            {
                inputEnabled = false;
                ImGui::Text("Config file saved! check out https://github.com/jpvolt/CameraCalibrationGUI for usage details.");
                if (ImGui::Button("Close"))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            
            ImGui::End();
        }

        //opencv stuff


        static VideoCapture cap(cameraIndex);

        if(oldIndex != cameraIndex){
            cap = VideoCapture(cameraIndex);
            oldIndex = cameraIndex;
        }
            
        SDL_Texture* texture, *texture2;
        SDL_Rect texture_rect, texture_rect2;
        texture_rect.x = 0;  //the x coordinate
        texture_rect.y = 0; // the y coordinate
        found = false;
        
       

        if(cap.isOpened()){
            Mat frame, gray, correctedframe;
            cap >> frame;

            if(KEYS[SDLK_SPACE] && captureNumber > 0 && inputEnabled){ //capture frame 
                cvtColor(frame, gray, CV_BGR2GRAY);
                std::vector<Point2f> corners;
                Size patternsize(gridsize[0],gridsize[1]);

                //lambdas!
                auto findChess = [gray](int v[2], std::vector<Point2f>& corners){
                    bool a = false;
                    Size patternsize(v[0],v[1]);
                    a = findChessboardCorners(gray, patternsize, corners);
                    if(!a){
                        patternsize = {v[1],v[0]};
                        a = findChessboardCorners(gray, patternsize, corners);
                    }
                    return a;
                }; 
                auto findCircles = [gray](int v[2], std::vector<Point2f>& corners, int mode){
                    bool a =  false;
                    Size patternsize(v[0],v[1]);
                    a = findCirclesGrid(gray, patternsize, corners, mode);
                    if(!a){
                        patternsize = {v[1], v[0]};
                        a = findCirclesGrid(gray, patternsize, corners, mode);
                    }
                    return a;
                };
                switch (mode)
                {
                    case CHESS:
                        found = findChess(gridsize, corners);
                        if(found){
                            cornerSubPix(gray, corners, Size(11, 11), Size(-1, -1),TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
                            drawChessboardCorners(frame, patternsize, corners, found);
                            FinalCorners.push_back(corners);
                            captureNumber --;
                        }
                
                        break;
                    case ASYMCIRCLES:
                        found = findCircles(gridsize, corners, CALIB_CB_ASYMMETRIC_GRID);
                        if(found){
                            drawChessboardCorners(frame, patternsize, corners, found);
                            FinalCorners.push_back(corners);
                            captureNumber --;
                        }
                            
                        break;
                    case SYMCIRCLES:
                        found = findCircles(gridsize, corners, CALIB_CB_SYMMETRIC_GRID);
                        if(found){
                            drawChessboardCorners(frame, patternsize, corners, found);
                            FinalCorners.push_back(corners);
                            captureNumber --;
                        }
                        break;

                    default:
                        break;
                }
                
                Failed = !found;
            }else if(captureNumber == 0){
                static bool calculated = false;
                static Size patternsize(gridsize[0],gridsize[1]);
                Mat cameraMatrix, distCoeffs, newCamMat;
                if(savefile || !calculated){
                    cameraMatrix = Mat::eye(3, 3, CV_64F);
                    distCoeffs = Mat::zeros(8, 1, CV_64F);
                    std::vector<Mat> rvecs, tvecs;
                    std::vector<std::vector<Point3f>> objectPoints(1);
                    getCornerPositions(patternsize, realsize, objectPoints[0], mode);
                    objectPoints.resize(FinalCorners.size(),objectPoints[0]);
                    Size imsize;
                    imsize.height = frame.rows;
                    imsize.width = frame.cols;
                    double rms = calibrateCamera(objectPoints, FinalCorners, imsize, cameraMatrix,distCoeffs, rvecs, tvecs, CV_CALIB_FIX_K4|CV_CALIB_FIX_K5);
                    newCamMat = getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, imsize, 1.0f, imsize);
                    calculated = true;
                    savefilebtn = true;
                }
                if(savefile){
                    saveConfig(cameraMatrix, distCoeffs, newCamMat, savepath);
                    savefile = false;
                }
                if(showcorrected)
                    undistort(frame, correctedframe, cameraMatrix, distCoeffs, newCamMat);
            }

            if(changedMode){ //check for calibration mode change
                captureNumber = 20;
                FinalCorners.clear();
                savefilebtn = false;
                saved = false;
                changedMode = false;
            }    

            texture_rect.w = frame.size[1]; //the width of the texture
            texture_rect.h = frame.size[0]; //the height of the texture
            SDL_Surface* surface =  convertCV_MatToSDL_Surface(frame);
            texture = SDL_CreateTextureFromSurface(window.GetRenderer(), surface);
            SDL_FreeSurface(surface);

            texture_rect2.x = texture_rect.w;
            texture_rect2.y = 0;
            texture_rect2.w = correctedframe.size[1]; //the width of the texture
            texture_rect2.h = correctedframe.size[0]; //the height of the texture
            if(showcorrected){
                SDL_Surface* surface2 = convertCV_MatToSDL_Surface(correctedframe);
                texture2 = SDL_CreateTextureFromSurface(window.GetRenderer(), surface2);
                SDL_FreeSurface(surface2);
            }



    
        }else{
            cameraIndex = oldIndex;
            cap = VideoCapture(cameraIndex);
        } 
        // Rendering
        ImGui::Render();
        SDL_GL_MakeCurrent(window.GetWindow(), window.GetGLContext());
        glViewport(0, 0, (int)window.GetImGuiIO().DisplaySize.x, (int)window.GetImGuiIO().DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        SDL_RenderCopy(renderer, texture, NULL, &texture_rect);
        SDL_RenderCopy(renderer, texture2, NULL, &texture_rect2);
        SDL_DestroyTexture(texture);
        SDL_DestroyTexture(texture2);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window.GetWindow());

        if(found){
            std::this_thread::sleep_for(std::chrono::milliseconds(600));
        }
    }

  return 0;
}