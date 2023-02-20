#pragma once

#include <stdint.h>
#include <cmath>

/*
rgba color class designed for taking averages of colors.
*/

class rgba
  {
  public:

    rgba() : rsqr(0), gsqr(0), bsqr(0), asqr(0)
      {
      }

    rgba(uint32_t color)
      {
      init(color);
      }

    rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
      {
      init(r, g, b, a);
      }

    rgba(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
      {
      init(r, g, b, a);
      }

    rgba(int32_t r, int32_t g, int32_t b, int32_t a)
      {
      init(r, g, b, a);
      }

    rgba(char r, char g, char b, char a)
      {
      init(r, g, b, a);
      }

    rgba(float r, float g, float b, float a)
      {
      init(r, g, b, a);
      }

    rgba(double r, double g, double b, double a)
      {
      init(r, g, b, a);
      }

    void init(uint32_t color)
      {
      uint32_t r = color & 0xff;
      uint32_t g = (color >> 8) & 0xff;
      uint32_t b = (color >> 16) & 0xff;
      uint32_t a = (color >> 24) & 0xff;
      init(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(a));
      }

    void init(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
      {
      _clamp<uint32_t>(r, g, b, a, 0, 255);
      init(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(a));
      }

    void init(int32_t r, int32_t g, int32_t b, int32_t a)
      {
      _clamp<int32_t>(r, g, b, a, 0, 255);
      init(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(a));
      }

    void init(char r, char g, char b, char a)
      {
      init(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(a));
      }

    void init(float r, float g, float b, float a)
      {
      init(static_cast<double>(r), static_cast<double>(g), static_cast<double>(b), static_cast<double>(a));
      }

    void init(double r, double g, double b, double a)
      {
      _clamp<double>(r, g, b, a, 0.0, 1.0);
      rsqr = r * r;
      gsqr = g * g;
      bsqr = b * b;
      asqr = a * a;
      }

    void init_squared(double rr, double gg, double bb, double aa)
      {
      rsqr = rr;
      gsqr = gg;
      bsqr = bb;
      asqr = aa;
      }

    void init(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
      {
      const double rd = (double)r / 255.0;
      const double gd = (double)g / 255.0;
      const double bd = (double)b / 255.0;
      const double ad = (double)a / 255.0;
      rsqr = rd * rd;
      gsqr = gd * gd;
      bsqr = bd * bd;
      asqr = ad * ad;
      }

    double red() const
      {
      return sqrt(_clamp<double>(rsqr, 0.0, 1.0));
      }

    double green() const
      {
      return sqrt(_clamp<double>(gsqr, 0.0, 1.0));
      }

    double blue() const
      {
      return sqrt(_clamp<double>(bsqr, 0.0, 1.0));
      }

    double alpha() const
      {
      return sqrt(_clamp<double>(asqr, 0.0, 1.0));
      }

    double red_sqr() const
      {
      return _clamp<double>(rsqr, 0.0, 1.0);
      }

    double green_sqr() const
      {
      return _clamp<double>(gsqr, 0.0, 1.0);
      }

    double blue_sqr() const
      {
      return _clamp<double>(bsqr, 0.0, 1.0);
      }

    double alpha_sqr() const
      {
      return _clamp<double>(asqr, 0.0, 1.0);
      }

    double red_sqr_raw() const
      {
      return rsqr;
      }

    double green_sqr_raw() const
      {
      return gsqr;
      }

    double blue_sqr_raw() const
      {
      return bsqr;
      }

    double alpha_sqr_raw() const
      {
      return asqr;
      }

    uint32_t color() const
      {
      const uint32_t r = static_cast<uint32_t>(red() * 255.0);
      const uint32_t g = static_cast<uint32_t>(green() * 255.0);
      const uint32_t b = static_cast<uint32_t>(blue() * 255.0);
      const uint32_t a = static_cast<uint32_t>(alpha() * 255.0);
      return a << 24 | b << 16 | g << 8 | r;
      }

    template <class T>
    T r() const
      {
      return static_cast<T>(red() * 255.0);
      }

    template <class T>
    T g() const
      {
      return static_cast<T>(green() * 255.0);
      }

    template <class T>
    T b() const
      {
      return static_cast<T>(blue() * 255.0);
      }

    template <class T>
    T a() const
      {
      return static_cast<T>(alpha() * 255.0);
      }

    template <>
    double r<double>() const
      {
      return red();
      }

    template <>
    double g<double>() const
      {
      return green();
      }

    template <>
    double b<double>() const
      {
      return blue();
      }

    template <>
    double a<double>() const
      {
      return alpha();
      }

    template <>
    float r<float>() const
      {
      return static_cast<float>(red());
      }

    template <>
    float g<float>() const
      {
      return static_cast<float>(green());
      }

    template <>
    float b<float>() const
      {
      return static_cast<float>(blue());
      }

    template <>
    float a<float>() const
      {
      return static_cast<float>(alpha());
      }

    void operator += (const rgba& other)
      {
      rsqr += other.rsqr;
      gsqr += other.gsqr;
      bsqr += other.bsqr;
      asqr += other.asqr;
      }

    void operator -= (const rgba& other)
      {
      rsqr -= other.rsqr;
      gsqr -= other.gsqr;
      bsqr -= other.bsqr;
      asqr -= other.asqr;
      }

    template <class T>
    void operator /= (T denom)
      {
      rsqr /= static_cast<double>(denom);
      gsqr /= static_cast<double>(denom);
      bsqr /= static_cast<double>(denom);
      asqr /= static_cast<double>(denom);
      }

    template <class T>
    void operator *= (T s)
      {
      rsqr *= static_cast<double>(s);
      gsqr *= static_cast<double>(s);
      bsqr *= static_cast<double>(s);
      asqr *= static_cast<double>(s);
      }

    bool operator == (const rgba& other) const
      {
      return rsqr == other.rsqr && gsqr == other.gsqr && bsqr == other.bsqr && asqr == other.asqr;
      }

    bool operator != (const rgba& other) const
      {
      return !(*this == other);
      }

  private:

    template <class T>
    T _clamp(T v, T minimum, T maximum) const
      {
      return v < minimum ? minimum : v > maximum ? maximum : v;
      }

    template <class T>
    void _clamp(T& r, T& g, T& b, T& a, T minimum, T maximum) const
      {
      r = _clamp(r, minimum, maximum);
      g = _clamp(g, minimum, maximum);
      b = _clamp(b, minimum, maximum);
      a = _clamp(a, minimum, maximum);
      }

  private:
    double rsqr, gsqr, bsqr, asqr;
  };


inline rgba operator + (const rgba& left, const rgba& right)
  {
  rgba result(left);
  result += right;
  return result;
  }

inline rgba operator - (const rgba& left, const rgba& right)
  {
  rgba result(left);
  result -= right;
  return result;
  }

template <class T>
inline rgba operator / (const rgba& left, T denom)
  {
  rgba result(left);
  result /= denom;
  return result;
  }

template <class T>
inline rgba operator * (const rgba& left, T s)
  {
  rgba result(left);
  result *= s;
  return result;
  }

template <class T>
inline rgba operator * (T s, const rgba& right)
  {
  rgba result(right);
  result *= s;
  return result;
  }

double dot(const rgba& left, const rgba& right)
  {
  return left.alpha_sqr_raw() * right.alpha_sqr_raw() + left.blue_sqr_raw() * right.blue_sqr_raw() + left.green_sqr_raw() * right.green_sqr_raw() + left.red_sqr_raw() * right.red_sqr_raw();
  }

double distance_sqr(const rgba& left, const rgba& right)
  {
  const rgba diff = left - right;
  return dot(diff, diff);
  }

double distance(const rgba& left, const rgba& right)
  {
  return sqrt(distance_sqr(left, right));
  }