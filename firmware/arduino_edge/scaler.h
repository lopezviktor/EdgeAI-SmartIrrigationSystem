#pragma once
#include <Arduino.h>

static const float DATA_MIN[5] = {
    450.0f, // soil1_min
    430.0f, // soil2_min
    18.0f,  // temperature_min
    35.0f,  // humidity_min
    200.0f  // light_min
};

static const float DATA_MAX[5] = {
    749.0f, // soil1_max
    739.0f, // soil2_max
    29.9f,  // temperature_max
    74.9f,  // humidity_max
    899.0f  // light_max
};

inline float minmax_scale(float value, float vmin, float vmax)
{
  if (vmax <= vmin)
    return 0.0f;
  float z = (value - vmin) / (vmax - vmin);
  if (z < 0.0f)
    z = 0.0f;
  if (z > 1.0f)
    z = 1.0f;
  return z;
}

inline void scale_features(const float in_raw[5], float out_scaled[5])
{
  for (int i = 0; i < 5; ++i)
  {
    out_scaled[i] = minmax_scale(in_raw[i], DATA_MIN[i], DATA_MAX[i]);
  }
}