#pragma once

#include "SDL.h"

#include "image.h"
#include "settings.h"

class view
  {
  public:
    view();
    ~view();

    void loop();

  private:

    void _poll_for_events();
    void _imgui_ui();    
    void _check_image();

  private:
    uint32_t _w, _h;
    SDL_Window* _window;
    SDL_Renderer* _renderer;
    SDL_Surface* _heightmap_surface;
    SDL_Texture* _heightmap_texture;
    bool _quit;
    std::unique_ptr<image> _heightmap;    
    std::unique_ptr<image> _normalmap;
    std::unique_ptr<image> _colormap;
    std::unique_ptr<image> _islandgradient;
    settings _settings;
    bool _dirty;    
  };