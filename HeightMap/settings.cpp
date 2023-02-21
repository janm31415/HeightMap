#include "settings.h"
#include "pref_file.h"

settings::settings()
  {
  width = 256;
  height = 256;
  frequency = 2;
  octaves = 6;
  fadeoff = 0.5f;
  seed = 0;
  mode = 0;
  amplify = 1.f;
  gamma = 1.f;
  normalmap_mode = 1;
  normalmap_strength = 1.f;
  render_target = 0;

  make_island = false;

  island_center_x = 0.5f;
  island_center_y = 0.5f;
  island_radius_x = 0.5f;
  island_radius_y = 0.5f;
  island_size_x = 0.f;
  island_size_y = 0.f;
  island_blend = 1.f;
  island_power = 0.1f;
  island_wrap = 0;
  island_flags = 1;
  island_merge_mode = 2;
  island_invert = false;
  }


settings read_settings(const char* filename)
  {
  settings s;
  pref_file f(filename, pref_file::READ);
  f["width"] >> s.width;
  f["height"] >> s.height;
  f["frequency"] >> s.frequency;
  f["octaves"] >> s.octaves;
  f["fadeoff"] >> s.fadeoff;
  f["seed"] >> s.seed;
  f["mode"] >> s.mode;
  f["amplify"] >> s.amplify;
  f["gamma"] >> s.gamma;
  f["normalmap_mode"] >> s.normalmap_mode;
  f["normalmap_strength"] >> s.normalmap_strength;
  f["render_target"] >> s.render_target;

  f["island_center_x"] >> s.island_center_x;
  f["island_center_y"] >> s.island_center_y;
  f["island_radius_x"] >> s.island_radius_x;
  f["island_radius_y"] >> s.island_radius_y;
  f["island_size_x"] >> s.island_size_x;
  f["island_size_y"] >> s.island_size_y;
  f["island_blend"] >> s.island_blend;
  f["island_power"] >> s.island_power;
  f["island_wrap"] >> s.island_wrap;
  f["island_flags"] >> s.island_flags;
  f["make_island"] >> s.make_island;
  f["island_merge_mode"] >> s.island_merge_mode;
  f["island_invert"] >> s.island_invert;

  f["export_folder"] >> s.export_folder;
  return s;
  }

void write_settings(const settings& s, const char* filename)
  {
  pref_file f(filename, pref_file::WRITE);
  f << "width" << s.width;
  f << "height" << s.height;
  f << "frequency" << s.frequency;
  f << "octaves" << s.octaves;
  f << "fadeoff" << s.fadeoff;
  f << "seed" << s.seed;
  f << "mode" << s.mode;
  f << "amplify" << s.amplify;
  f << "gamma" << s.gamma;
  f << "normalmap_mode" << s.normalmap_mode;
  f << "normalmap_strength" << s.normalmap_strength;
  f << "render_target" << s.render_target;

  f << "island_center_x" << s.island_center_x;
  f << "island_center_y" << s.island_center_y;
  f << "island_radius_x" << s.island_radius_x;
  f << "island_radius_y" << s.island_radius_y;
  f << "island_size_x" << s.island_size_x;
  f << "island_size_y" << s.island_size_y;
  f << "island_blend" << s.island_blend;
  f << "island_power" << s.island_power;
  f << "island_wrap" << s.island_wrap;
  f << "island_flags" << s.island_flags;
  f << "make_island" << s.make_island;
  f << "island_merge_mode" << s.island_merge_mode;
  f << "island_invert" << s.island_invert;

  f << "export_folder" << s.export_folder;

  f.release();
  }
