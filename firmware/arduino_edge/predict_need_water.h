#pragma once
#include <Arduino.h>

inline int predict_need_water(const float x[5])
{
    if (x[0] <= 0.503345)
    {
        if (x[1] <= 0.551780)
        {
            if (x[0] <= 0.001672)
            {
                if (x[3] <= 0.298246)
                {
                    if (x[4] <= 0.229614)
                    {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}