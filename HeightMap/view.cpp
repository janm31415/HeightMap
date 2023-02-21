#include "view.h"
#include <algorithm>
#include <stdexcept>
#include <vector>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include "imguifilesystem.h"

#include "rgba.h"

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

  struct map_color
    {
    map_color(int32_t r, int32_t g, int32_t b, int32_t a, double h) : height(h)
      {
      clr = ((uint32_t)a) << 24 | ((uint32_t)b) << 16 | ((uint32_t)g) << 8 | ((uint32_t)r);
      }
    uint32_t clr;
    double height;
    };

  std::vector<map_color> build_map_colors()
    {
    std::vector<map_color> clrs;
    clrs.emplace_back(0, 0, 128, 0, 0.0); // deeps
    clrs.emplace_back(0, 0, 255, 255, 0.05); // shallow
    clrs.emplace_back(0, 128, 255, 255, 0.1); // shore
    clrs.emplace_back(240, 240, 64, 255, 0.1625); // sand
    clrs.emplace_back(32, 160, 0, 255, 0.250); // grass
    clrs.emplace_back(224, 224, 0, 255, 0.4750); // dirt
    clrs.emplace_back(128, 128, 128, 255, 0.75); // rock
    clrs.emplace_back(255, 255, 255, 255, 1.0); // snow
    return clrs;
    }

  std::unique_ptr<image> image_height_to_color(const std::unique_ptr<image>& im_height, std::vector<map_color> colors)
    {
    std::sort(colors.begin(), colors.end(), [](const auto& left, const auto& right)
      {
      return left.height < right.height;
      });

    std::unique_ptr<image> im_out = std::make_unique<image>();
    im_out->init(im_height->width(), im_height->height());

    double scale_range = colors.back().height - colors.front().height;

    uint64_t* dest = im_out->data();
    const uint64_t* height = im_height->data();
    uint32_t count = im_out->size();
    for (uint32_t i = 0; i < count; ++i)
      {
      double scale = (double)(*height & 0x7fff) / 0x7fff;
      scale *= scale_range;
      scale += colors.front().height;
      if (scale <= colors.front().height)
        {
        uint32_t target_color = colors.front().clr;
        *dest = get_color_64(target_color);
        }
      else if (scale >= colors.back().height)
        {
        uint32_t target_color = colors.back().clr;
        *dest = get_color_64(target_color);
        }
      else
        {
        int k = 1;
        while (colors[k].height < scale)
          ++k;
        uint32_t blend_color_1 = colors[k - 1].clr;
        uint32_t blend_color_2 = colors[k].clr;
        rgba c1(blend_color_1);
        rgba c2(blend_color_2);
        double alpha = (scale - colors[k - 1].height) / (colors[k].height - colors[k - 1].height);
        rgba c3 = c1 * (1 - alpha) + c2 * alpha;
        *dest = get_color_64(c3.color());
        }
      ++height;
      ++dest;
      }

    return im_out;
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
    if (ImGui::InputInt("Height mode", &_settings.mode))
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
    if (ImGui::InputFloat("Normal strength", &_settings.normalmap_strength))
      {
      _dirty = true;
      }
    if (ImGui::InputInt("Normal mode", &_settings.normalmap_mode))
      {
      _dirty = true;
      }

    if (ImGui::Checkbox("Make island", &_settings.make_island))
      {
      _dirty = true;
      }
    float island_center[2] = { _settings.island_center_x, _settings.island_center_y };
    if (ImGui::InputFloat2("Island center", island_center))
      {
      _settings.island_center_x = island_center[0];
      _settings.island_center_y = island_center[1];
      _dirty = true;
      }
    float island_radius[2] = { _settings.island_radius_x, _settings.island_radius_y };
    if (ImGui::InputFloat2("Island radius", island_radius))
      {
      _settings.island_radius_x = island_radius[0];
      _settings.island_radius_y = island_radius[1];
      _dirty = true;
      }
    float island_size[2] = { _settings.island_size_x, _settings.island_size_y };
    if (ImGui::InputFloat2("Island size", island_size))
      {
      _settings.island_size_x = island_size[0];
      _settings.island_size_y = island_size[1];
      _dirty = true;
      }
    if (ImGui::InputFloat("Island blend", &_settings.island_blend))
      {
      _dirty = true;
      }
    if (ImGui::InputFloat("Island power", &_settings.island_power))
      {
      _dirty = true;
      }
    if (ImGui::InputInt("Island wrap", &_settings.island_wrap))
      {
      _dirty = true;
      }
    if (ImGui::InputInt("Island flags", &_settings.island_flags))
      {
      _dirty = true;
      }
    if (ImGui::InputInt("Island merge mode", &_settings.island_merge_mode))
      {
      _dirty = true;
      }
    if (ImGui::Checkbox("Island invert", &_settings.island_invert))
      {
      _dirty = true;
      }

    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    if (ImGui::Button("Heightmap"))
      {
      _settings.render_target = 0;
      _dirty = true;
      }
    ImGui::SameLine();
    if (ImGui::Button("Normalmap"))
      {
      _settings.render_target = 1;
      _dirty = true;
      }
    ImGui::SameLine();
    if (ImGui::Button("Colormap"))
      {
      _settings.render_target = 2;
      _dirty = true;
      }
    ImGui::SameLine();
    if (ImGui::Button("Island gradient"))
      {
      _settings.render_target = 3;
      _dirty = true;
      }

    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    static bool open_export_settings_file = false;
    static bool open_import_settings_file = false;
    if (ImGui::Button("Export settings"))
      {
      open_export_settings_file = true;
      }
    ImGui::SameLine();
    if (ImGui::Button("Import settings"))
      {
      open_import_settings_file = true;
      }

    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    static bool open_export_folder = false;
    if (ImGui::Button("...##1"))
      {
      open_export_folder = true;
      }
    ImGui::SameLine();
    static char export_folder[1024];
    for (size_t i = 0; i <= _settings.export_folder.length(); ++i)
      export_folder[i] = _settings.export_folder[i];
    ImGui::InputText("Export folder", export_folder, IM_ARRAYSIZE(export_folder));
    _settings.export_folder = std::string(export_folder);
    if (ImGui::Button("Export images"))
      {
      _export_images();
      }

    static ImGuiFs::Dialog open_export_folder_dlg(false, true, true);
    const char* openExportFolderChosenPath = open_export_folder_dlg.chooseFolderDialog(open_export_folder, _settings.export_folder.c_str(), "Open export folder", ImVec2(-1, -1), ImVec2(50, 50));
    open_export_folder = false;
    if (strlen(openExportFolderChosenPath) > 0)
      {
      _settings.export_folder = open_export_folder_dlg.getLastDirectory();
      }

    static ImGuiFs::Dialog open_export_settings_dlg(false, true, true);
    const char* openExportSettingsChosenPath = open_export_settings_dlg.saveFileDialog(open_export_settings_file, _settings.export_folder.c_str(), 0, ".json", "Export settings", ImVec2(-1, -1), ImVec2(50, 50));
    open_export_settings_file = false;
    if (strlen(openExportSettingsChosenPath) > 0)
      {
      write_settings(_settings, open_export_settings_dlg.getChosenPath());
      }

    static ImGuiFs::Dialog open_import_settings_dlg(false, true, true);
    const char* openImportSettingsChosenPath = open_import_settings_dlg.chooseFileDialog(open_import_settings_file, _settings.export_folder.c_str(), ".json", "Import settings", ImVec2(-1, -1), ImVec2(50, 50));
    open_import_settings_file = false;
    if (strlen(openImportSettingsChosenPath) > 0)
      {
      _settings = read_settings(open_import_settings_dlg.getChosenPath());
      _dirty = true;
      }

    }

  ImGui::End();

  //ImGui::ShowDemoWindow();
  ImGui::Render();
  }

void view::_export_images()
  {
  std::string heightmap_filename = _settings.export_folder + "/heightmap.png";
  std::string normalmap_filename = _settings.export_folder + "/normalmap.png";
  std::string colormap_filename = _settings.export_folder + "/colormap.png";
  image_export(_heightmap, heightmap_filename.c_str(), image_export_filetype::png, 100);
  image_export(_normalmap, normalmap_filename.c_str(), image_export_filetype::png, 100);
  image_export(_colormap, colormap_filename.c_str(), image_export_filetype::png, 100);
  }

void view::_check_image()
  {
  if (!_dirty)
    return;
  bool _reallocate_sdl_surface = (_settings.width != _heightmap->width() || _settings.height != _heightmap->height());
  _heightmap = image_perlin(_settings.width, _settings.height, _settings.frequency, _settings.octaves, _settings.fadeoff, _settings.seed, static_cast<image_perlin_mode>(_settings.mode), _settings.amplify, _settings.gamma, 0xff000000, 0xffffffff);
  _islandgradient = image_flat(_settings.width, _settings.height, 0xff000000);
  image_glow_rect(_islandgradient,
    _settings.island_center_x,
    _settings.island_center_y,
    _settings.island_radius_x,
    _settings.island_radius_y,
    _settings.island_size_x,
    _settings.island_size_y,
    0xffffffff,
    _settings.island_blend,
    _settings.island_power,
    static_cast<image_glow_rect_wrap>(_settings.island_wrap),
    static_cast<image_glow_rect_flags>(_settings.island_flags));

  if (_settings.island_invert)
    {
    image_color(_islandgradient, image_color_mode::mul, 0x00ffffff);
    image_color(_islandgradient, image_color_mode::invert, 0);
    }
  if (_settings.make_island)
    {
    image_merge_mode mode = static_cast<image_merge_mode>(_settings.island_merge_mode);
    switch (mode)
      {
      case image_merge_mode::sub:
      {
      std::unique_ptr<image> grad = _islandgradient->copy();
      image_color(grad, image_color_mode::mul, 0x00ffffff);
      grad = image_merge(image_merge_mode::min, 2, &_heightmap, &grad);
      _heightmap = image_merge(mode, 2, &_heightmap, &grad);
      break;
      }
      case image_merge_mode::mul:
      {
      _heightmap = image_merge(mode, 2, &_heightmap, &_islandgradient);
      break;
      }
      default:
      {
      std::unique_ptr<image> grad = _islandgradient->copy();
      image_color(grad, image_color_mode::mul, 0x00ffffff);
      _heightmap = image_merge(mode, 2, &_heightmap, &grad);
      break;
      }
      }
    }
  _normalmap = image_normals(_heightmap, _settings.normalmap_strength, static_cast<image_normals_mode>(_settings.normalmap_mode));

  std::vector<map_color> colors = build_map_colors();
  _colormap = image_height_to_color(_heightmap, colors);
  if (_reallocate_sdl_surface)
    {
    SDL_FreeSurface(_heightmap_surface);
    _heightmap_surface = create_sdl_surface(_heightmap);
    }
  switch (_settings.render_target)
    {
    case 0:
      fill_sdl_surface(_heightmap_surface, _heightmap);
      break;
    case 1:
      fill_sdl_surface(_heightmap_surface, _normalmap);
      break;
    case 2:
      fill_sdl_surface(_heightmap_surface, _colormap);
      break;
    case 3:
      fill_sdl_surface(_heightmap_surface, _islandgradient);
      break;
    default:
      fill_sdl_surface(_heightmap_surface, _heightmap);
      break;
    }
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