#include "view.h"
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <numeric>

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
  template <typename T, typename Compare>
  std::vector<std::size_t> sort_permutation(const std::vector<T>& vec, Compare& compare)
    {
    std::vector<std::size_t> p(vec.size());
    std::iota(p.begin(), p.end(), 0);
    std::sort(p.begin(), p.end(),
      [&](std::size_t i, std::size_t j) { return compare(vec[i], vec[j]); });
    return p;
    }

  template <typename T>
  std::vector<T> apply_permutation(const std::vector<T>& vec, const std::vector<std::size_t>& p)
    {
    std::vector<T> sorted_vec(vec.size());
    std::transform(p.begin(), p.end(), sorted_vec.begin(),
      [&](std::size_t i) { return vec[i]; });
    return sorted_vec;
    }

  template <class TType>
  void delete_items(std::vector<TType>& vec, const std::vector<uint32_t>& _indices_to_delete)
    {
    if (_indices_to_delete.empty() || vec.empty())
      return;
    std::vector<uint32_t> indices_to_delete(_indices_to_delete);
    assert(vec.size() > *std::max_element(indices_to_delete.begin(), indices_to_delete.end()));
    std::sort(indices_to_delete.begin(), indices_to_delete.end());
    indices_to_delete.erase(std::unique(indices_to_delete.begin(), indices_to_delete.end()), indices_to_delete.end());

    if (indices_to_delete.size() == vec.size())
      {
      vec.clear();
      return;
      }

    auto last = --vec.end();

    for (auto rit = indices_to_delete.rbegin(); rit != indices_to_delete.rend(); ++rit)
      {
      size_t index = *rit;
      auto it = vec.begin() + index;
      if (it != last)
        {
        std::swap(*it, *last);
        }
      --last;
      }
    vec.erase(++last, vec.end());
    }

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
    map_color(uint32_t c, double h) : clr(c), height(h) {}

    uint32_t clr;
    double height;
    };

  std::vector<map_color> build_map_colors(const std::vector<uint32_t>& colors, const std::vector<double>& heights)
    {
    std::vector<map_color> clrs;
    uint32_t sz = (uint32_t)colors.size();
    if (heights.size() < sz)
      sz = (uint32_t)heights.size();
    for (uint32_t i = 0; i < sz; ++i)
      clrs.emplace_back(colors[i], heights[i]);

    return clrs;
    }

  std::unique_ptr<image> image_height_to_color(const std::unique_ptr<image>& im_height, std::vector<map_color> colors)
    {
    if (colors.empty())
      {
      return image_flat(im_height->width(), im_height->height(), 0xff000000);
      }
    if (colors.size() == 1)
      {
      return image_flat(im_height->width(), im_height->height(), colors.front().clr);
      }

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

  void make_color_set1(settings& s)
    {
    s.heights.clear();
    s.colors.clear();
    s.heights.push_back(0.0);
    s.heights.push_back(0.05);
    s.heights.push_back(0.1);
    s.heights.push_back(0.1625);
    s.heights.push_back(0.250);
    s.heights.push_back(0.4750);
    s.heights.push_back(0.75);
    s.heights.push_back(1.0);
    s.colors.push_back(0x00800000);
    s.colors.push_back(0xffff0000);
    s.colors.push_back(0xffff8000);
    s.colors.push_back(0xff408c8c);
    s.colors.push_back(0xff00a020);
    s.colors.push_back(0xff00e0e0);
    s.colors.push_back(0xff808080);
    s.colors.push_back(0xffffffff);
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

  if (_settings.colors.empty() || _settings.heights.empty())
    {
    make_color_set1(_settings);
    }

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

  ImGui::SetNextWindowSize(ImVec2((float)(_w - 950), (float)(_h)), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImVec2((float)(950), (float)(0)), ImGuiCond_Always);

  if (ImGui::Begin("Parameters", 0, ImGuiWindowFlags_NoDecoration))
    {
    ImGui::BeginChild("Heightmap", ImVec2(0.0, 200.0f), true);
    ImGui::BeginGroup();
    int size[2] = { (int)_settings.width, (int)_settings.height };
    if (ImGui::InputInt2("Heightmap size", size))
      {
      if (size[0] > 0 && size[1] > 0)
        {
        _settings.width = size[0];
        _settings.height = size[1];
        _dirty = true;
        }
      }
    if (ImGui::InputInt("Heightmap frequency", &_settings.frequency))
      {
      _dirty = true;
      }
    if (ImGui::InputInt("Heightmap octaves", &_settings.octaves))
      {
      _dirty = true;
      }
    if (ImGui::SliderFloat("Heightmap fadeoff", &_settings.fadeoff, -1.5f, 1.5f))
      {
      _dirty = true;
      }
    if (ImGui::InputInt("Heightmap seed", &_settings.seed))
      {
      _dirty = true;
      }
    const char* height_mode[] = { "norm", "abs", "sin", "abs+sin" };
    if (ImGui::Combo("Heightmap mode", &_settings.mode, height_mode, IM_ARRAYSIZE(height_mode)))
      {
      _dirty = true;
      }
    if (ImGui::SliderFloat("Heightmap amplify", &_settings.amplify, 0.f, 5.f))
      {
      _dirty = true;
      }
    if (ImGui::SliderFloat("Heightmap gamma", &_settings.gamma, 0.f, 3.f))
      {
      _dirty = true;
      }
    ImGui::EndGroup();
    ImGui::EndChild();

    ImGui::BeginChild("Normalmap", ImVec2(0.0, 70.0f), true);
    ImGui::BeginGroup();
    if (ImGui::SliderFloat("Normalmap strength", &_settings.normalmap_strength, 0.f, 2.f))
      {
      _dirty = true;
      }
    const char* normal_mode[] = { "normal 2d", "normal 3d", "normal tangent 2d", "normal tangent 3d",  "extra sharp 2d", "extra sharp 3d", "extra sharp tangent 2d", "extra sharp tangent 3d" };
    if (ImGui::Combo("Normalmap mode", &_settings.normalmap_mode, normal_mode, IM_ARRAYSIZE(normal_mode)))
      {
      _dirty = true;
      }
    ImGui::EndGroup();
    ImGui::EndChild();

    ImGui::BeginChild("Island", ImVec2(0.0, 230.0f), true);
    ImGui::BeginGroup();
    if (ImGui::Checkbox("Make island", &_settings.make_island))
      {
      _dirty = true;
      }
    ImGui::SameLine();
    if (ImGui::Checkbox("Island invert", &_settings.island_invert))
      {
      _dirty = true;
      }
    float island_center[2] = { _settings.island_center_x, _settings.island_center_y };
    if (ImGui::SliderFloat2("Island center", island_center, 0.f, 1.f))
      {
      _settings.island_center_x = island_center[0];
      _settings.island_center_y = island_center[1];
      _dirty = true;
      }
    float island_radius[2] = { _settings.island_radius_x, _settings.island_radius_y };
    if (ImGui::SliderFloat2("Island radius", island_radius, 0.f, 1.f))
      {
      _settings.island_radius_x = island_radius[0];
      _settings.island_radius_y = island_radius[1];
      _dirty = true;
      }
    float island_size[2] = { _settings.island_size_x, _settings.island_size_y };
    if (ImGui::SliderFloat2("Island size", island_size, 0.f, 2.f))
      {
      _settings.island_size_x = island_size[0];
      _settings.island_size_y = island_size[1];
      _dirty = true;
      }
    if (ImGui::SliderFloat("Island blend", &_settings.island_blend, 0.f, 2.f))
      {
      _dirty = true;
      }
    if (ImGui::SliderFloat("Island power", &_settings.island_power, 0.f, 2.f))
      {
      _dirty = true;
      }
    const char* island_wrap[] = { "repeat", "on" };
    if (ImGui::Combo("Island wrap", &_settings.island_wrap, island_wrap, IM_ARRAYSIZE(island_wrap)))
      {
      _dirty = true;
      }
    const char* island_flags[] = { "normal ellipse", "alternative ellipse", "normal rectangle", "alternative rectangle" };
    if (ImGui::Combo("Island flags", &_settings.island_flags, island_flags, IM_ARRAYSIZE(island_flags)))
      {
      _dirty = true;
      }
    const char* island_merge_mode[] = { "add", "sub", "mul", "min", "max" };
    if (ImGui::Combo("Island merge mode", &_settings.island_merge_mode, island_merge_mode, IM_ARRAYSIZE(island_merge_mode)))
      {
      _dirty = true;
      }
    ImGui::EndGroup();
    ImGui::EndChild();
    ImGui::BeginChild("Colors", ImVec2(0.0, 270.0f), true);
    ImGui::BeginGroup();

    if (ImGui::Button("Add", ImVec2(ImGui::GetContentRegionAvail().x * 0.5, 0)))
      {
      _settings.heights.push_back(0.0);
      _settings.colors.push_back(0xff00ff00);
      _dirty = true;
      }
    ImGui::SameLine();
    if (ImGui::Button("Sort", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
      {
      auto p = sort_permutation(_settings.heights, [](auto left, auto right) { return left < right; });
      _settings.heights = apply_permutation(_settings.heights, p);
      _settings.colors = apply_permutation(_settings.colors, p);
      }

    uint32_t colors_size = (uint32_t)_settings.colors.size();
    if (_settings.heights.size() < colors_size)
      colors_size = (uint32_t)_settings.heights.size();
    std::vector<uint32_t> points_to_delete;
    for (uint32_t i = 0; i < colors_size; ++i)
      {
      ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
      std::stringstream str;
      str << "##color_" << i;
      float clr[4] = { (_settings.colors[i] & 255) / 255.f, ((_settings.colors[i] >> 8) & 255) / 255.f, ((_settings.colors[i] >> 16) & 255) / 255.f, ((_settings.colors[i] >> 24) & 255) / 255.f };
      if (ImGui::ColorEdit4(str.str().c_str(), clr))
        {
        uint32_t red = (uint32_t)(clr[0] * 255.f);
        uint32_t green = (uint32_t)(clr[1] * 255.f);
        uint32_t blue = (uint32_t)(clr[2] * 255.f);
        uint32_t alpha = (uint32_t)(clr[3] * 255.f);
        _settings.colors[i] = alpha << 24 | blue << 16 | green <<  8 | red;
        _dirty = true;
        }
      ImGui::PopItemWidth();

      ImGui::SameLine();
      ImGui::PushItemWidth(-30);
      str.str("");
      str.clear();
      str << "##point_" << i;
      float value = (float)_settings.heights[i];
      if (ImGui::SliderFloat(str.str().c_str(), &value, 0.f, 1.f))
        {
        _settings.heights[i] = (double)value;
        _dirty = true;
        }
      ImGui::PopItemWidth();
      ImGui::SameLine();

      str.str("");
      str.clear();
      str << "X##_" << i;
      if (ImGui::Button(str.str().c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
        points_to_delete.push_back(i);
        }
      }
    if (!points_to_delete.empty())
      {
      delete_items(_settings.colors, points_to_delete);
      delete_items(_settings.heights, points_to_delete);
      _dirty = true;
      }    
    ImGui::EndGroup();
    ImGui::EndChild();

    ImGui::BeginChild("ImportExport", ImVec2(0.0, 90.0f), true);
    ImGui::BeginGroup();
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
    ImGui::EndGroup();
    ImGui::EndChild();

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

  ImGui::SetNextWindowSize(ImVec2(800, 80), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImVec2(50, 850), ImGuiCond_Always);

  if (ImGui::Begin("Buttons", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground))
    {
    if (ImGui::Button("Height map"))
      {
      _settings.render_target = 0;
      _dirty = true;
      }
    ImGui::SameLine();
    if (ImGui::Button("Normal map"))
      {
      _settings.render_target = 1;
      _dirty = true;
      }
    ImGui::SameLine();
    if (ImGui::Button("Color map"))
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

  std::vector<map_color> colors = build_map_colors(_settings.colors, _settings.heights);
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