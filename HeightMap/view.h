#pragma once

#include "SDL.h"

class view
  {
  public:
    view();
    ~view();

    void loop();

  private:

    void _poll_for_events();

  private:
    uint32_t _w, _h;
    SDL_Window* _window;
    SDL_Renderer* _renderer;
    bool _quit;
  };