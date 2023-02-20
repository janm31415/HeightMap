#include "image.h"
#include <string.h>
#include <string>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

namespace
  {

  int32_t range7fff(int32_t a)
    {
    if ((uint32_t)a < 0x7fff)
      return a;
    else if (a < 0)
      return 0;
    else
      return 0x7fff;
    }

  uint64_t get_color_64(uint32_t color)
    {
    uint64_t col = color;
    col = ((col & 0xff000000) << 24)
      | ((col & 0x00ff0000) << 16)
      | ((col & 0x0000ff00) << 8)
      | ((col & 0x000000ff));
    col = ((col | (col << 8)) >> 1) & 0x7fff7fff7fff7fff;
    return col;
    }

  float perlin_random[256][2];
  uint8_t perlin_permute[512];


  static uint32_t random_seed = 0x74382381;

  uint32_t get_random()
    {
    uint32_t eax = random_seed;
    eax = eax * 0x343fd + 0x269ec3;
    uint32_t ebx = eax;
    eax = eax * 0x343fd + 0x269ec3;
    random_seed = eax;
    eax = (eax >> 10) & 0x0000ffff;
    ebx = (ebx << 6) & 0xffff0000;
    return eax | ebx;
    }

  uint32_t get_random(uint32_t maximum)
    {
    return get_random() % maximum;
    }

  float get_random_float()
    {
    return ((get_random() & 0x3fffffff) * 1.0f) / 0x40000000;
    }

  void set_random_seed(uint32_t seed)
    {
    random_seed = seed + seed * 17 + seed * 121 + (seed * 121 / 17);
    get_random();
    random_seed ^= seed + seed * 17 + seed * 121 + (seed * 121 / 17);
    get_random();
    random_seed ^= seed + seed * 17 + seed * 121 + (seed * 121 / 17);
    get_random();
    random_seed ^= seed + seed * 17 + seed * 121 + (seed * 121 / 17);
    get_random();
    }

  void init_perlin()
    {
    set_random_seed(1);

    for (int i = 0; i < 256; ++i)
      {
      perlin_random[i][0] = get_random(0x10000);
      perlin_permute[i] = i;
      }

    for (int i = 0; i < 255; ++i)
      {
      for (int j = i + 1; j < 256; ++j)
        {
        if (perlin_random[i][0] > perlin_random[j][0])
          {
          std::swap(perlin_random[i][0], perlin_random[j][0]);
          std::swap(perlin_permute[i], perlin_permute[j]);
          }
        }
      }

    memcpy(perlin_permute + 256, perlin_permute, 256);

    for (int i = 0; i < 256;)
      {
      int32_t x = get_random(0x10000) - 0x8000;
      int32_t y = get_random(0x10000) - 0x8000;
      if (x * x + y * y < 0x8000 * 0x8000)
        {
        perlin_random[i][0] = x / 32768.0f;
        perlin_random[i][1] = y / 32768.0f;
        ++i;
        }
      }

    }

  int32_t mul_shift(int32_t a, int32_t b)
    {
    // overflow multiplication
    int64_t res = (int64_t)a * (int64_t)b;
    return (int32_t)(res >> 16);
    }

  int32_t mul_div(int32_t a, int32_t b, int32_t c)
    {
    int64_t m = (int64_t)a * (int64_t)b;
    return (int32_t)(m / c);
    }

  int32_t div_shift(int32_t a, int32_t b)
    {
    return int32_t((int64_t(a) << 16) / b);
    }

  int32_t get_gamma(int32_t value, int32_t* gamma_table)
    {
    int32_t vi = value >> 5;
    return gamma_table[vi] + (((gamma_table[vi + 1] - gamma_table[vi]) * (value & 31)) >> 5);
    }

  void fade_64(uint64_t& result, uint64_t& c0, uint64_t& c1, int32_t fade)
    {
#ifdef _WIN32
    __m128i col0 = _mm_loadl_epi64((const __m128i*) & c0);
    __m128i col1 = _mm_loadl_epi64((const __m128i*) & c1);

    __m128i fadei = _mm_cvtsi32_si128(-(fade >> 1));
    __m128i fadem = _mm_shufflelo_epi16(fadei, 0x00);

    __m128i diff = _mm_sub_epi16(col1, col0);
    __m128i diffm = _mm_mulhi_epi16(diff, fadem);
    __m128i diffms = _mm_slli_epi16(diffm, 1);
    __m128i res = _mm_subs_epi16(col0, diffms);

    _mm_storel_epi64((__m128i*) & result, res);
#else
    int64_t f1 = fade;
    int64_t f0 = 0x10000 - fade;
    uint64_t a = c0;
    uint64_t b = c1;
    result = ((((((a >> 0) & 0xffff) * f0) >> 16) + ((((b >> 0) & 0xffff) * f1) >> 16)) << 0)
      + ((((((a >> 16) & 0xffff) * f0) >> 16) + ((((b >> 16) & 0xffff) * f1) >> 16)) << 16)
      + ((((((a >> 32) & 0xffff) * f0) >> 16) + ((((b >> 32) & 0xffff) * f1) >> 16)) << 32)
      + ((((((a >> 48) & 0xffff) * f0) >> 16) + ((((b >> 48) & 0xffff) * f1) >> 16)) << 48);
#endif
    }

  void set_mem_8(uint64_t* destination, uint64_t value, int count)
    {
    while (count--)
      *destination++ = value;
    }

  template <class T>
  inline T clamp(T a, T minimum, T maximum)
    {
    return a < minimum ? minimum : a > maximum ? maximum : a;
    }

  uint32_t get_power_2(uint32_t val)
    {
    uint32_t p = 1;
    while ((1 << p) < val)
      ++p;
    return p;
    }

  uint32_t positive_modulo(int32_t value, uint32_t m)
    {
    int32_t mod = value % m;
    return mod < 0 ? mod + m : mod;
    }

  int32_t filterbump(uint16_t* s, int32_t pos, int32_t mod, int32_t step)
    {
    return s[positive_modulo(pos - step - step, mod)] * 1
      + s[positive_modulo(pos - step, mod)] * 3
      - s[positive_modulo(pos, mod)] * 3
      - s[positive_modulo(pos + step, mod)] * 1;
    }

  int32_t filterbumpsharp(uint16_t* s, int32_t pos, int32_t mod, int32_t step)
    {
    return 4 * (s[positive_modulo(pos - step, mod)] - s[positive_modulo(pos, mod)]);
    }

  enum e_merge_mode
    {
    MERGEMODE_ADD,
    MERGEMODE_SUB,
    MERGEMODE_MUL
    };

  void image_inner(uint64_t* d, uint64_t* s, int32_t count, int32_t mode)
    {
    for (uint32_t i = 0; i < count; ++i)
      {
      switch (mode)
        {
        case MERGEMODE_ADD:
        {
        uint16_t* d16 = (uint16_t*)d;
        uint16_t* s16 = (uint16_t*)s;
        *d16++ += *s16++;
        *d16++ += *s16++;
        *d16++ += *s16++;
        *d16++ += *s16++;
        ++d;
        ++s;
        break;
        }
        case MERGEMODE_SUB:
        {
        uint16_t* d16 = (uint16_t*)d;
        uint16_t* s16 = (uint16_t*)s;
        *d16++ -= *s16++;
        *d16++ -= *s16++;
        *d16++ -= *s16++;
        *d16++ -= *s16++;
        ++d;
        ++s;
        break;
        }
        case MERGEMODE_MUL:
        {
        uint16_t* d16 = (uint16_t*)d;
        uint16_t* s16 = (uint16_t*)s;
        uint32_t res;
        res = (uint32_t)(*d16) * (uint32_t)(*s16++);
        *d16++ = res >> 15;
        res = (uint32_t)(*d16) * (uint32_t)(*s16++);
        *d16++ = res >> 15;
        res = (uint32_t)(*d16) * (uint32_t)(*s16++);
        *d16++ = res >> 15;
        res = (uint32_t)(*d16) * (uint32_t)(*s16++);
        *d16++ = res >> 15;

        ++d;
        ++s;
        break;
        }
        default:
        {
        *d++ = *s++;
        break;
        }
        }
      }
    }

  } // namespace

image::image() : _data(nullptr), _width(0), _height(0), _size(0), _format(image_format::rgba16)
  {
  }

image::~image()
  {
  if (_data)
    delete[] _data;
  _data = nullptr;
  }

void image::copy(const image& other)
  {
  if (_data)
    {
    delete[] _data;
    _data = nullptr;
    }
  init(other._width, other._height);
  _format = other._format;
  memcpy(_data, other._data, _size * sizeof(uint64_t));
  }

std::unique_ptr<image> image::copy() const
  {
  std::unique_ptr<image> i = std::make_unique<image>();
  i->copy(*this);
  return i;
  }

void image::init(int32_t w, int32_t h)
  {
  _width = w;
  _height = h;
  _size = w * h;
  _data = new uint64_t[_size];
  }

void image::init(int32_t w, int32_t h, uint64_t* data)
  {
  _width = w;
  _height = h;
  _size = w * h;
  _data = data;
  }

void image::set_format(image_format f)
  {
  _format = f;
  }


std::unique_ptr<image> image_import(const char* filename)
  {
  int w, h, nr_of_channels;
  if (!filename)
    return nullptr;
  unsigned char* im = stbi_load(filename, &w, &h, &nr_of_channels, 0);
  if (!im)
    return nullptr;
  std::unique_ptr<image> out = std::make_unique<image>();
  out->init(w, h);

  for (uint32_t y = 0; y < (uint32_t)h; ++y)
    {
    uint64_t* p_out = out->data() + y * w;
    const unsigned char* p_im = im + y * w * nr_of_channels;
    for (uint32_t x = 0; x < (uint32_t)w; ++x, ++p_out, p_im += nr_of_channels)
      {
      uint32_t color = 0;
      switch (nr_of_channels)
        {
        case 1:
        {
        uint32_t g = *p_im;
        color = 0xff000000 | (g << 16) | (g << 8) | g;
        break;
        }
        case 3:
        {
        uint32_t r = p_im[0];
        uint32_t g = p_im[1];
        uint32_t b = p_im[2];
        color = 0xff000000 | (b << 16) | (g << 8) | r;
        break;
        }
        case 4:
        {
        color = *((const uint32_t*)p_im);
        break;
        }
        }
      *p_out = get_color_64(color);
      }
    }

  stbi_image_free(im);
  return out;
  }

bool image_export(const std::unique_ptr<image>& im, const char* filename, image_export_filetype filetype, int32_t jpeg_quality)
  {
  jpeg_quality = clamp(jpeg_quality, 1, 100);
  int32_t w = im->width();
  int32_t h = im->height();
  int32_t c = 0;
  uint8_t* bytes = nullptr;
  if (im->format() == image_format::rgba16)
    {
    c = 4;
    bytes = new uint8_t[w * h * c];
    uint32_t* d = (uint32_t*)bytes;
    const uint16_t* s = (const uint16_t*)im->data();
    for (int y = 0; y < h; ++y)
      {
      for (int x = 0; x < w; ++x, ++d, s += 4)
        {
        *d = (((s[0] >> 7) & 0xff)) |
          (((s[1] >> 7) & 0xff) << 8) |
          (((s[2] >> 7) & 0xff) << 16) |
          (((s[3] >> 7) & 0xff) << 24);
        }
      }
    }

  int res = 0;

  switch (filetype)
    {
    case image_export_filetype::png:
    {
    res = stbi_write_png(filename, w, h, c, (void*)bytes, w * c);
    break;
    }
    case image_export_filetype::jpg:
    {
    res = stbi_write_jpg(filename, w, h, c, (void*)bytes, jpeg_quality);
    break;
    }
    case image_export_filetype::bmp:
    {
    res = stbi_write_bmp(filename, w, h, c, (void*)bytes);
    break;
    }
    case image_export_filetype::tga:
    {
    res = stbi_write_tga(filename, w, h, c, (void*)bytes);
    break;
    }
    }
  delete[] bytes;
  return res != 0;
  }

std::unique_ptr<image> image_flat(int32_t width, int32_t height, uint32_t color)
  {
  if (width < 0)
    return nullptr;
  if (height < 0)
    return nullptr;
  std::unique_ptr<image> out = std::make_unique<image>();
  out->init(width, height);
  set_mem_8(out->data(), get_color_64(color), out->size());
  return out;
  }

bool fill_rgba_buffer_with_image(void* buffer, uint32_t buffer_bytes_per_row, const std::unique_ptr<image>& im)
  {
  const int32_t w = im->width();
  const int32_t h = im->height();
  const uint16_t* s = (const uint16_t*)im->data();
  switch (im->format())
    {
    case image_format::rgba16:
    {
    for (int y = 0; y < h; ++y)
      {
      uint32_t* p_buffer_row = (uint32_t*)((uint8_t*)buffer + y * buffer_bytes_per_row);

      for (int x = 0; x < w; ++x, s += 4)
        {
        uint32_t clr = (((s[0] >> 7) & 0xff)) |
          (((s[1] >> 7) & 0xff) << 8) |
          (((s[2] >> 7) & 0xff) << 16) |
          (((s[3] >> 7) & 0xff) << 24);
        *p_buffer_row++ = clr;
        }
      }
    return true;
    }
    default:
      break;
    }
  return false;
  }


std::unique_ptr<image> image_perlin(int32_t xs, int32_t ys, int32_t freq, int32_t oct, float fadeoff, int32_t seed, int32_t mode, float amp, float gamma, uint32_t col0, uint32_t col1)
  {
  if (xs < 1)
    return nullptr;
  if (ys < 1)
    return nullptr;
  std::unique_ptr<image> bm = std::make_unique<image>();
  bm->init(xs, ys);

  uint64_t c0 = get_color_64(col0);
  uint64_t c1 = get_color_64(col1);

  const int32_t w = 1 << get_power_2(bm->width());

  int32_t shiftx = 16 - get_power_2(w);
  int32_t shifty = 16 - get_power_2(bm->height());
  uint64_t* tile = bm->data();
  seed &= 255;
  mode &= 3;

  int32_t i, noffs, x, y;
  int32_t gamma_table[1025];
  for (i = 0; i < 1025; ++i)
    gamma_table[i] = range7fff(std::pow(i / 1024.0f, gamma) * 0x8000) * 2;

  if (mode & 1)
    {
    amp *= 0x8000;
    noffs = 0;
    }
  else
    {
    amp *= 0x4000;
    noffs = 0x4000;
    }

  int32_t ampi = (int32_t)(amp);

  int32_t int32_tab[257];
  if (mode & 2)
    {
    for (x = 0; x < 257; x++)
      int32_tab[x] = (int32_t)(std::sin(2.f * 3.1415926535897f * x / 256.0f) * 0.5f * 65536.0f);
    }

  int32_t* nrow = new int32_t[bm->width()];
  int32_t* poly = new int32_t[(w) >> freq];

  for (x = 0; x < ((w) >> freq); ++x)
    {
    float f = 1.0f * x / (w >> freq);
    poly[x] = (int32_t)(f * f * f * (10 + f * (6 * f - 15)) * 16384.0f);
    }

  for (y = 0; y < bm->height(); ++y)
    {
    memset(nrow, 0, sizeof(int32_t) * bm->width());
    float s = 1.0f;

    // make some noise
    for (i = freq; i < freq + oct; ++i)
      {
      int32_t xGrpSize = (shiftx + i < 16) ? std::min<int32_t>(w, 1 << (16 - shiftx - i)) : 1;
      int32_t groups = (shiftx + i < 16) ? w >> (16 - shiftx - i) : w;
      int32_t mask = ((1 << i) - 1) & 255;
      int32_t py = y << (shifty + i);

      int32_t vy = (py >> 16) & mask;
      int32_t dtx = 1 << (shiftx + i);
      float ty = (py & 0xffff) / 65536.0f;
      float tyf = ty * ty * ty * (10 + ty * (6 * ty - 15));
      float ty0f = ty * (1 - tyf);
      float ty1f = (ty - 1) * tyf;
      int32_t vy0 = perlin_permute[((vy + 0)) ^ seed];
      int32_t vy1 = perlin_permute[((vy + 1) & mask) ^ seed];
      int32_t shf = i - freq;
      int32_t si = (int32_t)(s * 16384.0f);

      if (shiftx + i < 16 || (py & 0xffff)) // otherwise, the contribution is always zero
        {
        int32_t* rowp = nrow;
        int32_t xcount = 0;
        for (int32_t vx = 0; vx < groups; vx++)
          {
          int32_t v00 = perlin_permute[((vx + 0) & mask) + vy0];
          int32_t v01 = perlin_permute[((vx + 1) & mask) + vy0];
          int32_t v10 = perlin_permute[((vx + 0) & mask) + vy1];
          int32_t v11 = perlin_permute[((vx + 1) & mask) + vy1];

          float f_0h = perlin_random[v00][0] + (perlin_random[v10][0] - perlin_random[v00][0]) * tyf;
          float f_1h = perlin_random[v01][0] + (perlin_random[v11][0] - perlin_random[v01][0]) * tyf;
          float f_0v = perlin_random[v00][1] * ty0f + perlin_random[v10][1] * ty1f;
          float f_1v = perlin_random[v01][1] * ty0f + perlin_random[v11][1] * ty1f;

          int32_t fa = (int32_t)(f_0v * 65536.0f);
          int32_t fb = (int32_t)((f_1v - f_1h) * 65536.0f);
          int32_t fad = (int32_t)(f_0h * dtx);
          int32_t fbd = (int32_t)(f_1h * dtx);

          for (int32_t xg = 0; xg < xGrpSize && xcount < bm->width(); ++xg)
            {
            int32_t nni = fa + (((fb - fa) * poly[xg << shf]) >> 14);
            switch (mode)
              {
              case 0:   break;
              case 1:   nni = std::abs(nni); break;
              case 3:   nni &= 0x7fff;
              case 2:
              {
              int32_t ind = (nni >> 8) & 0xff;
              nni = int32_tab[ind] + (((int32_tab[ind + 1] - int32_tab[ind]) * (nni & 0xff)) >> 8);
              }
              break;
              default: break;
              }
            *rowp++ += (nni * si) >> 14;
            fa += fad;
            fb += fbd;
            ++xcount;
            }
          }
        }

      s *= fadeoff;
      }

    // resolve
    for (x = 0; x < bm->width(); ++x)
      fade_64(*tile++, c0, c1, get_gamma(range7fff(mul_shift(nrow[x], ampi) + noffs), gamma_table));
    }

  delete[] nrow;
  delete[] poly;

  return bm;
  }

void image_init()
  {
  init_perlin();
  }

std::unique_ptr<image> image_normals(const std::unique_ptr<image>& im, float _dist, int32_t mode)
  {
  int32_t shiftx, shifty;
  int32_t xs, ys;
  int32_t x, y;
  uint16_t* sx, * sy, * s, * d;
  int32_t vx, vy, vz;
  float e;
  int32_t dist;

  std::unique_ptr<image> bm = std::make_unique<image>();
  bm->init(im->width(), im->height());
  dist = (int32_t)(_dist * 65536.0f);
  d = (uint16_t*)bm->data();
  sx = sy = s = (uint16_t*)im->data();
  xs = im->width();
  ys = im->height();
  shiftx = get_power_2(im->width());
  shifty = get_power_2(im->height());

  for (y = 0; y < ys; y++)
    {
    sy = s;
    for (x = 0; x < xs; x++)
      {
      if (mode & 4)
        {
        vx = filterbumpsharp(sx, x * 4, xs * 4, 4);
        vy = filterbumpsharp(sy, y * xs * 4, ys * xs * 4, xs * 4);
        }
      else
        {
        vx = filterbump(sx, x * 4, xs * 4, 4);
        vy = filterbump(sy, y * xs * 4, ys * xs * 4, xs * 4);
        }
      vx = range7fff((((vx) * (dist >> 4)) >> (20 - shiftx)) + 0x4000) - 0x4000;
      vy = range7fff((((vy) * (dist >> 4)) >> (20 - shifty)) + 0x4000) - 0x4000;
      vz = 0;

      if (mode & 1)
        {
        vz = (0x3fff * 0x3fff) - vx * vx - vy * vy;
        if (vz > 0)
          {
          vz = std::sqrt(vz);
          }
        else
          {
          e = 1.f / std::sqrt(vx * vx + vy * vy) * 0x3fff;
          vx *= e;
          vy *= e;
          vz = 0;
          }
        }
      if (mode & 2)
        {
        std::swap(vx, vy);
        vy = -vy;
        }

      d[0] = vx + 0x4000;
      d[1] = vy + 0x4000;
      d[2] = vz + 0x4000;
      d[3] = 0xffff;

      d += 4;
      sy += 4;
      }
    sx += xs * 4;
    }
  return bm;
  }

std::unique_ptr<image> image_gradient(int32_t xs, int32_t ys, uint32_t col0, uint32_t col1, float posf, float a, float length, int32_t mode)
  {
  if (xs < 1)
    return nullptr;
  if (ys < 1)
    return nullptr;
  std::unique_ptr<image> bm = std::make_unique<image>();
  bm->init(xs, ys);

  uint64_t c0, c1;
  int32_t c, cdx, cdy, x, y;
  int32_t dx, dy, pos;
  float l;
  int32_t val;
  uint64_t* tile;

  c0 = get_color_64(col0);
  c1 = get_color_64(col1);

  l = 32768.0f / length;
  pos = posf * 32768.0f;
  dx = (int32_t)(std::cos(a * 3.1415926535897f * 2.f) * l);
  dy = (int32_t)(std::sin(a * 3.1415926535897f * 2.f) * l);
  cdx = mul_shift(dx, 0x10000 / bm->width());
  cdy = mul_shift(dy, 0x10000 / bm->height()) - bm->width() * cdx;

  c = 0x4000 - (dx / 2 + dy / 2) * (posf + 1);

  tile = bm->data();
  for (y = 0; y < bm->height(); y++)
    {
    for (x = 0; x < bm->width(); x++)
      {
      switch (mode)
        {
        case 0:
          val = range7fff(c) * 2;
          break;
        case 1:
          val = (int32_t)(std::sin(range7fff(c) * 3.1415926535897f * 2.f / 0x8000 + 0x2000) * 0x7fff + 0x7fff);
          break;
        case 2:
          val = (int32_t)(std::sin(range7fff(c) * 3.1415926535897f * 2.f / 0x10000) * 0xffff);
          break;
        }
      fade_64(*tile, c0, c1, val);
      c += cdx;
      tile++;
      }

    c += cdy;
    }

  return bm;
  }

void image_glow_rect(std::unique_ptr<image>& im, float cx, float cy, float rx, float ry, float sx, float sy, uint32_t color, float alpha, float power, uint32_t wrap, uint32_t flags)
  {
  uint64_t* d;
  int32_t x, y;
  float a;
  int32_t f, fm;
  uint64_t col;
  float fx, fy, thresh;
  int32_t low_table[32];
  bool circular = (flags & 2) == 0;

  if (wrap == 1)
    {
    if (cx + rx + sx > 1.0f)
      image_glow_rect(im, cx - 1.0f, cy, rx, ry, sx, sy, color, alpha, power, 2, flags);
    if (cx - rx - sx < -0.0f)
      image_glow_rect(im, cx + 1.0f, cy, rx, ry, sx, sy, color, alpha, power, 2, flags);
    }
  if (wrap == 1 || wrap == 2)
    {
    if (cy + ry + sy > 1.0f)
      image_glow_rect(im, cx, cy - 1.0f, rx, ry, sx, sy, color, alpha, power, 0, flags);
    if (cy - ry - sy < -0.0f)
      image_glow_rect(im, cx, cy + 1.0f, rx, ry, sx, sy, color, alpha, power, 0, flags);
    }

  if (power == 0)
    power = (1.0f / 65536.0f);
  power = 0.25 / power;

  cx *= im->width();
  cy *= im->height();
  rx *= im->width();
  ry *= im->height();
  sx *= im->width();
  sy *= im->height();

  int32_t icx = cx, icy = cy, isx = sx, isy = sy;

  thresh = 1.0f / 65536.0f;
  if (rx < thresh)
    rx = thresh;
  rx = circular ? 1.0f / (rx * rx) : 1.0f / rx;

  if (ry < thresh)
    ry = thresh;
  ry = circular ? 1.0f / (ry * ry) : 1.0f / ry;

  alpha *= 32768.0f;
  col = get_color_64(color);
  fm = alpha;
  int32_t gamma_table[1025];

  for (x = 0; x < 1025; ++x)
    {
    if (flags & 1)
      gamma_table[x] = range7fff(std::pow(1.0f - x / 1024.0f, power) * alpha) * 2;
    else
      gamma_table[x] = range7fff((1.0f - std::pow(x / 1024.0f, power * 2.0f)) * alpha) * 2;
    }

  // there are very steep slopes around 0, so don't try approximating them
  for (x = 0; x < 32; ++x)
    {
    if (flags & 1)
      low_table[x] = range7fff(std::pow(1.0f - x / 32768.0f, power) * alpha) * 2;
    else
      low_table[x] = range7fff((1.0f - std::pow(x / 32768.0f, power * 2.0f)) * alpha) * 2;
    }

  d = im->data();

  for (y = 0; y < im->height(); ++y)
    {
    fy = std::abs(y - cy) - sy;
    if (fy < 0)
      fy = 0;
    fy *= circular ? fy * ry : ry;

    for (x = 0; x < im->width(); ++x)
      {
      fx = std::abs(x - cx) - sx;
      if (fx < 0)
        fx = 0;

      a = circular ? fx * fx * rx + fy : std::max(fx * rx, fy);
      if (a < 1.0f - 1.0f / 32768.0f) // to cull a few more pixels...
        {
        f = (int32_t)(a * 32768);
        if (f < 32)
          f = low_table[f];
        else
          f = get_gamma(f, gamma_table);

        fade_64(*d, *d, col, f);
        }

      ++d;
      }
    }
  }

std::unique_ptr<image> image_merge(int32_t mode, int32_t count, const std::unique_ptr<image>* i0, ...)
  {
  if (i0 == nullptr)
    return nullptr;
  va_list args;
  va_start(args, i0);
  const std::unique_ptr<image>* ii;
  int i;
  for (i = 1; i < count; ++i)
    {
    ii = va_arg(args, const std::unique_ptr<image>*);
    if (ii == nullptr)
      return nullptr;
    if ((*i0)->width() != (*ii)->width() || (*i0)->height() != (*ii)->height())
      return nullptr;
    }
  va_end(args);

  std::unique_ptr<image> im_out = (*i0)->copy();

  va_start(args, i0);
  i = 1;
  while (i < count)
    {
    ii = va_arg(args, const std::unique_ptr<image>*);
    image_inner(im_out->data(), (*ii)->data(), im_out->size(), mode);
    ++i;
    }
  va_end(args);
  return im_out;
  }