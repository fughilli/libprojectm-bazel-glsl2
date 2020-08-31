#include "PerlinNoiseWithAlpha.hpp"

namespace {
float Noise(int x) {
  x = (x << 13) ^ x;
  return (((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) /
          2147483648.0);
}

float Noise(int x, int y) {
  int n = x + y * 57;
  return Noise(n);
}

float Noise(int x, int y, int z) {
  int n = x + y * 57 + z * 141;
  return Noise(n);
}

float CubicInterpolate(float v0, float v1, float v2, float v3, float x) {
  float P = (v3 - v2) - (v0 - v1);
  float Q = (v0 - v1) - P;
  float R = v2 - v0;

  return P * pow(x, 3) + Q * pow(x, 2) + R * x + v1;
}

float InterpolatedNoise(float x, float y) {
  int integer_x = int(x);
  float fractional_x = x - integer_x;

  int integer_y = int(y);
  float fractional_y = y - integer_y;

  float a0 = Noise(integer_x - 1, integer_y - 1);
  float a1 = Noise(integer_x, integer_y - 1);
  float a2 = Noise(integer_x + 1, integer_y - 1);
  float a3 = Noise(integer_x + 2, integer_y - 1);

  float x0 = Noise(integer_x - 1, integer_y);
  float x1 = Noise(integer_x, integer_y);
  float x2 = Noise(integer_x + 1, integer_y);
  float x3 = Noise(integer_x + 2, integer_y);

  float y0 = Noise(integer_x + 0, integer_y + 1);
  float y1 = Noise(integer_x, integer_y + 1);
  float y2 = Noise(integer_x + 1, integer_y + 1);
  float y3 = Noise(integer_x + 2, integer_y + 1);

  float b0 = Noise(integer_x - 1, integer_y + 2);
  float b1 = Noise(integer_x, integer_y + 2);
  float b2 = Noise(integer_x + 1, integer_y + 2);
  float b3 = Noise(integer_x + 2, integer_y + 2);

  float i0 = CubicInterpolate(a0, a1, a2, a3, fractional_x);
  float i1 = CubicInterpolate(x0, x1, x2, x3, fractional_x);
  float i2 = CubicInterpolate(y0, y1, y2, y3, fractional_x);
  float i3 = CubicInterpolate(b0, b1, b2, b3, fractional_x);

  return CubicInterpolate(i0, i1, i2, i3, fractional_y);
}

float PerlinOctave3d(float x, float y, float z, int width, int seed,
                     float period) {
  float freq = 1 / (float)(period);

  int num = (int)(width * freq);
  int step_x = (int)(x * freq);
  int step_y = (int)(y * freq);
  int step_z = (int)(z * freq);
  float zone_x = x * freq - step_x;
  float zone_y = y * freq - step_y;
  float zone_z = z * freq - step_z;

  int boxB = step_x + step_y + step_z * num;
  int boxC = step_x + step_y + step_z * (num + 1);
  int boxD = step_x + step_y + step_z * (num + 2);
  int boxA = step_x + step_y + step_z * (num - 1);

  float u, a, b, v, noisedata, box;

  box = boxA;
  noisedata = (box + seed);
  u = CubicInterpolate(Noise(noisedata - num - 1), Noise(noisedata - num),
                       Noise(noisedata - num + 1), Noise(noisedata - num + 2),
                       zone_x);
  a = CubicInterpolate(Noise(noisedata - 1), Noise(noisedata),
                       Noise(noisedata + 1), Noise(noisedata + 2), zone_x);
  b = CubicInterpolate(Noise(noisedata + num - 1), Noise(noisedata + num),
                       Noise(noisedata + 1 + num), Noise(noisedata + 2 + num),
                       zone_x);
  v = CubicInterpolate(
      Noise(noisedata + 2 * num - 1), Noise(noisedata + 2 * num),
      Noise(noisedata + 1 + 2 * num), Noise(noisedata + 2 + 2 * num), zone_x);
  float A = CubicInterpolate(u, a, b, v, zone_y);

  box = boxB;
  noisedata = (box + seed);
  u = CubicInterpolate(Noise(noisedata - num - 1), Noise(noisedata - num),
                       Noise(noisedata - num + 1), Noise(noisedata - num + 2),
                       zone_x);
  a = CubicInterpolate(Noise(noisedata - 1), Noise(noisedata),
                       Noise(noisedata + 1), Noise(noisedata + 2), zone_x);
  b = CubicInterpolate(Noise(noisedata + num - 1), Noise(noisedata + num),
                       Noise(noisedata + 1 + num), Noise(noisedata + 2 + num),
                       zone_x);
  v = CubicInterpolate(
      Noise(noisedata + 2 * num - 1), Noise(noisedata + 2 * num),
      Noise(noisedata + 1 + 2 * num), Noise(noisedata + 2 + 2 * num), zone_x);
  float B = CubicInterpolate(u, a, b, v, zone_y);

  box = boxC;
  noisedata = (box + seed);
  u = CubicInterpolate(Noise(noisedata - num - 1), Noise(noisedata - num),
                       Noise(noisedata - num + 1), Noise(noisedata - num + 2),
                       zone_x);
  a = CubicInterpolate(Noise(noisedata - 1), Noise(noisedata),
                       Noise(noisedata + 1), Noise(noisedata + 2), zone_x);
  b = CubicInterpolate(Noise(noisedata + num - 1), Noise(noisedata + num),
                       Noise(noisedata + 1 + num), Noise(noisedata + 2 + num),
                       zone_x);
  v = CubicInterpolate(
      Noise(noisedata + 2 * num - 1), Noise(noisedata + 2 * num),
      Noise(noisedata + 1 + 2 * num), Noise(noisedata + 2 + 2 * num), zone_x);
  float C = CubicInterpolate(u, a, b, v, zone_y);

  box = boxD;
  noisedata = (box + seed);
  u = CubicInterpolate(Noise(noisedata - num - 1), Noise(noisedata - num),
                       Noise(noisedata - num + 1), Noise(noisedata - num + 2),
                       zone_x);
  a = CubicInterpolate(Noise(noisedata - 1), Noise(noisedata),
                       Noise(noisedata + 1), Noise(noisedata + 2), zone_x);
  b = CubicInterpolate(Noise(noisedata + num - 1), Noise(noisedata + num),
                       Noise(noisedata + 1 + num), Noise(noisedata + 2 + num),
                       zone_x);
  v = CubicInterpolate(
      Noise(noisedata + 2 * num - 1), Noise(noisedata + 2 * num),
      Noise(noisedata + 1 + 2 * num), Noise(noisedata + 2 + 2 * num), zone_x);
  float D = CubicInterpolate(u, a, b, v, zone_y);

  float value = CubicInterpolate(A, B, C, D, zone_z);

  return value;
}

float Noise3d(int x, int y, int z, int width, int octaves, int seed,
              float persistence, float base_period) {
  float p = persistence;
  float val = 0.0;

  for (int i = 0; i < octaves; i++) {
    val += PerlinOctave3d(x, y, z, width, seed, base_period) * p;

    base_period *= 0.5;
    p *= persistence;
  }
  return val;
}

template <int W, int H, int D, int C>
void Fill(float output[W][H][D][C]) {
  for (int x = 0; x < W; ++x) {
    for (int y = 0; y < H; ++y) {
      for (int z = 0; z < D; ++z) {
        float value = Noise(x, y, z);
        for (int c = 0; c < C - 1; ++c) {
          output[x][y][z][c] = value;
        }
        output[x][y][z][C - 1] = 1.0f;
      }
    }
  }
}

template <int W, int H, int C>
void Fill(float x_scale, float y_scale, float output[W][H][C]) {
  for (int x = 0; x < W; ++x) {
    for (int y = 0; y < H; ++y) {
      float value = InterpolatedNoise(x * x_scale, y * y_scale);
      for (int c = 0; c < C - 1; ++c) {
        output[x][y][c] = value;
      }
      output[x][y][C - 1] = 1.0f;
    }
  }
}

template <int W, int H, int C>
void Fill(float output[W][H][C]) {
  for (int x = 0; x < W; ++x) {
    for (int y = 0; y < H; ++y) {
      float value = Noise(x, y);
      for (int c = 0; c < C - 1; ++c) {
        output[x][y][c] = value;
      }
      output[x][y][C - 1] = 1.0f;
    }
  }
}

}  // namespace

void FillPerlin(Image<float>* image) { FillPerlinScaled(1, 1, image); }

void FillPerlinScaled(float scale_x, float scale_y, Image<float>* image) {
  for (int x = 0; x < image->width(); ++x) {
    for (int y = 0; y < image->height(); ++y) {
      if (image->dimensionality() == Dimensionality::kDimensionality3d) {
        for (int z = 0; z < image->depth(); ++z) {
          for (int c = 0; c < image->num_channels() - 1; ++c) {
            image->at(x, y, z, c) =
                Noise3d(x, y, z, image->width(), 3, rand(), 0.2, scale_x);
          }
          image->at(x, y, z, image->num_channels() - 1) = 1.0f;
        }
      } else {
        for (int c = 0; c < image->num_channels() - 1; ++c) {
          image->at(x, y, c) = InterpolatedNoise(x * scale_x, y * scale_y);
        }
        image->at(x, y, image->num_channels() - 1) = 1.0f;
      }
    }
  }
}
