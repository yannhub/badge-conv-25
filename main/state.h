#pragma once

#include <cstdint>

// Structure pour les particules flottantes
struct Particle
{
    float x;
    float y;
    float speed;
    float size;
    float alpha;
    float phase;
};

class AppState
{
public:
    int likes = 0;
    float t = 0.0f;
    float neon_intensity = 1.0f;
    int scanline_offset = 0;
    int anim_glow_level = 0;
    int anim_glow_direction = 1;
    int screenW = 0;
    int screenH = 0;
    int touch_x = -1;
    int touch_y = -1;

    // Variables pour l'animation du néon
    unsigned long neon_last_update = 0;
    float neon_time = 0.0f;
    unsigned long neon_flicker_start = 0;
    unsigned long neon_next_flicker = 0;
    bool neon_is_flickering = false;
    float neon_flicker_intensity = 1.0f;

    // Variables pour les effets rétro-futuristes
    float grid_offset = 0.0f;
    float corner_pulse = 0.0f;
    float border_pulse = 0.0f;
    Particle particles[8];
    bool particles_initialized = false;
    unsigned long glitch_next = 0;
    bool glitch_active = false;
    unsigned long glitch_start = 0;
    int glitch_offset_x = 0;
    int glitch_offset_y = 0;
};
