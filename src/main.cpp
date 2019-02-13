#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>     
#include <iostream>
#include <vector>
using namespace cv;

#include "vendor/opencvtosdl.hpp" 

int main(int argc, char *argv[])
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
    {
     printf("Error: %s\n", SDL_GetError());
     return -1;
    }

    #if __APPLE__
        // GL 3.2 Core + GLSL 150
        const char* glsl_version = "#version 150";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    #else
        // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    #endif
        // Create window with graphics context
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_DisplayMode current;
        SDL_GetCurrentDisplayMode(0, &current);
        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        SDL_Window* window = SDL_CreateWindow("Camera Calibration", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
        SDL_GLContext gl_context = SDL_GL_CreateContext(window);
        SDL_Renderer *renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED);
        SDL_GL_SetSwapInterval(1); // Enable vsync

    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

  
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool done = false;
    bool KEYS[322]; 
    while (!done)
    {
       
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
            if (event.type == SDL_KEYDOWN)
                KEYS[event.key.keysym.sym] = true;
            if (event.type == SDL_KEYUP)
                KEYS[event.key.keysym.sym] = false;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

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
        static std::vector<std::vector<Point2f>> FinalCorners;

        //imgui stuff 
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        {
           
            static ImVec4 textcolor = { 1.0f,1.0f,1.0f,1.0f };
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
                      
            ImGui::TextColored(textcolor, "Name of config file to save:");
            ImGui::InputText("Default=config.json",savepath, 256);
            
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
                ImGui::Text("Failed to find a pattern! check if the pattern is visible and for possible errors in pattern settings on the menu.");
                if (ImGui::Button("Close"))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }




            ImGui::Checkbox("Demo Window", &show_demo_window);      

            ImGui::End();
        }

        //opencv stuff



        static VideoCapture cap(cameraIndex);

        if(oldIndex != cameraIndex){
            cap = VideoCapture(cameraIndex);
            oldIndex = cameraIndex;
        }
            
        SDL_Texture* texture;
        SDL_Rect texture_rect;
        texture_rect.x = 0;  //the x coordinate
        texture_rect.y = 0; // the y coordinate
        found = false;
        
       

        if(cap.isOpened()){
            Mat frame, gray;
            cap >> frame;

            

            if(KEYS[SDLK_SPACE]){ //capture frame 
                
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
            }

        if(changedMode){ //check for calibration mode change
                captureNumber = 20;
                FinalCorners.clear();
                changedMode = false;
        }
            


   

            
                

            texture_rect.w = frame.size[1]; //the width of the texture
            texture_rect.h = frame.size[0]; //the height of the texture
            SDL_Surface* surface =  convertCV_MatToSDL_Surface(frame);
            texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);

    
        }else{
            cameraIndex = oldIndex;
            cap = VideoCapture(cameraIndex);
        } 
        // Rendering
        ImGui::Render();
        SDL_GL_MakeCurrent(window, gl_context);
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        SDL_RenderCopy(renderer, texture, NULL, &texture_rect);
        SDL_DestroyTexture(texture);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);

        if(found){
            //waitKey(1000);
        }
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

  return 0;
}