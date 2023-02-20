#include "view.h"
#include <stdexcept>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer.h"

view::view() : _w(1600), _h(900), _quit(false)
  {
  _window = SDL_CreateWindow("HeightMap",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    _w, _h,
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
  if (!_window)
    throw std::runtime_error("SDL can't create a window");
  _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
  if (!_window)
    throw std::runtime_error("SDL can't create a renderer");
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); 
  (void)io;

  ImGui_ImplSDL2_InitForSDLRenderer(_window, _renderer);
  ImGui_ImplSDLRenderer_Init(_renderer);

  // Setup Style
  ImGui::StyleColorsDark();
  ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive];


  }

view::~view()
  {
  ImGui_ImplSDLRenderer_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyRenderer(_renderer);
  SDL_DestroyWindow(_window);
  }

void view::loop()
  {
  ImGuiIO& io = ImGui::GetIO();
  while (!_quit)
    {
    _poll_for_events();

    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame(_window);
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    ImGui::Render();

    SDL_RenderSetScale(_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(_renderer, 10, 40, 80, 255);
    SDL_RenderClear(_renderer);
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(_renderer);

    }
  }

void view::_poll_for_events()
  {
  SDL_Event event;
  while (SDL_PollEvent(&event))
    {
    ImGui_ImplSDL2_ProcessEvent(&event);
    switch (event.type)
      {
      case SDL_QUIT:
      {
      _quit = true;
      break;
      }
      case SDL_KEYDOWN:
      {
      switch (event.key.keysym.scancode)
        {
        case SDL_SCANCODE_ESCAPE:
          _quit = true;
          break;
        }
      break;
      }
      }
    }
  }