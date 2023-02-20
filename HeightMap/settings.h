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
  };


settings read_settings(const char* filename);

void write_settings(const settings& s, const char* filename);
