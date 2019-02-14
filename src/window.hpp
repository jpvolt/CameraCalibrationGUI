#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <SDL2/SDL.h>
#include <GL/glew.h>    

class Window{
    private:
    SDL_DisplayMode current;
    SDL_WindowFlags window_flags;
    SDL_Window* window;
    SDL_GLContext gl_context;
    SDL_Renderer *renderer;
    

    public:
    Window();
    void ImguiNewFrame();
    SDL_Renderer* GetRenderer();
    SDL_Window* GetWindow();
    SDL_GLContext GetGLContext();
    ImGuiIO& GetImGuiIO();
};

