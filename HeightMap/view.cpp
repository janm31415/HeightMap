#include "view.h"
#include <stdexcept>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer.h"

namespace
  {

  SDL_Surface* create_sdl_surface(const std::unique_ptr<image>& im)
    {
    SDL_Surface* surf = SDL_CreateRGBSurface(0, im->width(), im->height(), 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    return surf;
    }

  void fill_sdl_surface(SDL_Surface* surf, const std::unique_ptr<image>& im)
    {
    assert(surf->w == im->width());
    assert(surf->h == im->height());
    SDL_LockSurface(surf);
    fill_rgba_buffer_with_image(surf->pixels, surf->pitch, im);
    SDL_UnlockSurface(surf);
    }

  }

view::view() : _w(1600), _h(900), _quit(false)
  {
  image_init();
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

  _settings = read_settings("heightmapsettings.json");

  _heightmap = image_flat(_settings.width, _settings.height, 0xff00ff00);
  _heightmap_surface = create_sdl_surface(_heightmap);
  fill_sdl_surface(_heightmap_surface, _heightmap);
  _heightmap_texture = SDL_CreateTextureFromSurface(_renderer, _heightmap_surface);
  _dirty = true;
  }

view::~view()
  {
  write_settings(_settings, "heightmapsettings.json");
  ImGui_ImplSDLRenderer_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyTexture(_heightmap_texture);
  SDL_FreeSurface(_heightmap_surface);
  SDL_DestroyRenderer(_renderer);
  SDL_DestroyWindow(_window);
  }

void view::_imgui_ui()
  {
  ImGui_ImplSDLRenderer_NewFrame();
  ImGui_ImplSDL2_NewFrame(_window);
  ImGui::NewFrame();

  ImGui::SetNextWindowSize(ImVec2((float)(_w - 950), (float)(800)), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImVec2((float)(900), (float)(50)), ImGuiCond_Always);

  if (ImGui::Begin("Parameters", 0, 0))
    {

    int size[2] = { (int)_settings.width, (int)_settings.height };
    if (ImGui::InputInt2("Size", size))
      {
      if (size[0] > 0 && size[1] > 0)
        {
        _settings.width = size[0];
        _settings.height = size[1];
        _dirty = true;
        }
      }
    if (ImGui::InputInt("Frequency", &_settings.frequency))
      {
      _dirty = true;
      }
    if (ImGui::InputInt("Octaves", &_settings.octaves))
      {
      _dirty = true;
      }
    if (ImGui::InputFloat("Fadeoff", &_settings.fadeoff))
      {
      _dirty = true;
      }
    if (ImGui::InputInt("Seed", &_settings.seed))
      {
      _dirty = true;
      }
    if (ImGui::InputInt("Mode", &_settings.mode))
      {
      _dirty = true;
      }
    if (ImGui::InputFloat("Amplify", &_settings.amplify))
      {
      _dirty = true;
      }
    if (ImGui::InputFloat("Gamma", &_settings.gamma))
      {
      _dirty = true;
      }
    }
  ImGui::End();

  //ImGui::ShowDemoWindow();
  ImGui::Render();
  }

void view::_check_image()
  {
  if (!_dirty)
    return;
  bool _reallocate_sdl_surface = (_settings.width != _heightmap->width() || _settings.height != _heightmap->height());  
  _heightmap = image_perlin(_settings.width, _settings.height, _settings.frequency, _settings.octaves, _settings.fadeoff, _settings.seed, _settings.mode, _settings.amplify, _settings.gamma, 0xff000000, 0xffffffff);

  if (_reallocate_sdl_surface)
    {
    SDL_FreeSurface(_heightmap_surface);
    _heightmap_surface = create_sdl_surface(_heightmap);
    }
  fill_sdl_surface(_heightmap_surface, _heightmap);
  SDL_DestroyTexture(_heightmap_texture);
  _heightmap_texture = SDL_CreateTextureFromSurface(_renderer, _heightmap_surface);
  _dirty = false;
  }

void view::loop()
  {
  ImGuiIO& io = ImGui::GetIO();
  while (!_quit)
    {
    _poll_for_events();
    _imgui_ui();
    _check_image();

    SDL_RenderSetScale(_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(_renderer, 10, 40, 80, 255);
    SDL_RenderClear(_renderer);

    SDL_Rect destination;
    destination.x = 50;
    destination.y = 50;
    destination.w = 800;
    double scale = (double)_heightmap->height() / (double)_heightmap->width();
    destination.h = (int)(800 * scale);
    SDL_RenderCopy(_renderer, _heightmap_texture, NULL, &destination);

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