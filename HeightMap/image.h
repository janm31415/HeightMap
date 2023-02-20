#pragma once

#include <stdint.h>
#include <memory>

enum class image_format
  {
  rgba16
  };

class image
  {
  public:
    image();
    ~image();

    void copy(const image& other);
    std::unique_ptr<image> copy() const;

    void init(int32_t w, int32_t h);
    void init(int32_t w, int32_t h, uint64_t* data);

    const uint64_t* data() const { return _data; }
    uint64_t* data() { return _data; }
    int32_t size() const { return _size; }
    int32_t width() const { return _width; }
    int32_t height() const { return _height; }
    image_format format() const { return _format; }

    void set_format(image_format f);

  private:
    uint64_t* _data;
    int32_t _width;
    int32_t _height;
    int32_t _size; // width * height
    image_format _format;
  };


std::unique_ptr<image> image_import(const char* filename);

enum class image_export_filetype
  {
  png,
  jpg,
  bmp,
  tga
  };

void image_init();

bool image_export(const std::unique_ptr<image>& im, const char* filename, image_export_filetype filetype, int32_t jpeg_quality);

std::unique_ptr<image> image_flat(int32_t width, int32_t height, uint32_t color);

bool fill_rgba_buffer_with_image(void* buffer, uint32_t buffer_bytes_per_row, const std::unique_ptr<image>& im);

std::unique_ptr<image> image_perlin(int32_t xs, int32_t ys, int32_t freq, int32_t oct, float fadeoff, int32_t seed, int32_t mode, float amp, float gamma, uint32_t col0, uint32_t col1);