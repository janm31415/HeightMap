#pragma once

#include <string>
#include <vector>


struct settings
  {
  settings();

  int32_t width;
  int32_t height;
  int32_t frequency;
  int32_t octaves;
  float fadeoff;  
  int32_t seed;
  int32_t mode;
  float amplify;
  float gamma;

  int32_t normalmap_mode;
  float normalmap_strength;
  
  bool make_island;
  float island_center_x;
  float island_center_y;
  float island_radius_x;
  float island_radius_y;
  float island_size_x;
  float island_size_y;
  float island_blend;
  float island_power;
  int32_t island_wrap;
  int32_t island_flags;
  int32_t island_merge_mode;
  bool island_invert;
  bool auto_vary_colors;

  int32_t render_target;

  std::string export_folder;

  float variation_fadeoff;
  int32_t variation_strength;
  int32_t variation_mode;
  int32_t variation_frequency;


  std::vector<uint32_t> colors;
  std::vector<double> heights;
  };


settings read_settings(const char* filename);

void write_settings(const settings& s, const char* filename);
