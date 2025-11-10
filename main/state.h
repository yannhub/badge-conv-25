#pragma once

#include <cstdint>

class AppState
{
public:
    int likes = 0;
    float t = 0.0f;
    float neon_flicker = 1.0f;
    float neon_flicker_smooth = 1.0f;
    int neon_dim_frames = 0;
    float neon_dim_target = 1.0f;
    int scanline_offset = 0;
    int anim_glow_level = 0;
    int anim_glow_direction = 1;
    int screenW = 0;
    int screenH = 0;
};
