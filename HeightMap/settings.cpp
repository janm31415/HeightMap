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
  f.release();
  }
