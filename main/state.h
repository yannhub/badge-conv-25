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
    bool active;                   // Particule active ou en attente
    unsigned long next_spawn_time; // Temps avant la prochaine apparition (ms)
};

class AppState
{
public:
    float t = 0.0f;
    float dt = 0.016f; // Delta time (calculé globalement par DisplayManager)
    int screenW = 0;
    int screenH = 0;
    int touch_x = -1;
    int touch_y = -1;

    float scanline_offset = 0.0f;

    float intensity_pulse = 0.0f;

    Particle particles[12];
    bool particles_initialized = false;

    unsigned long glitch_next = 0;
    bool glitch_active = false;
    unsigned long glitch_start = 0;
    int glitch_offset_x = 0;
    int glitch_offset_y = 0;

    // Variables pour le bouton
    bool button_pressed = false;
    unsigned long button_press_start = 0;

    // Animation du microprocesseur
    float chip_animation_progress = 0.0f; // 0.0 à 1.0 = dessin, pause 5s, puis fondu 3s
    unsigned long chip_pause_start = 0;   // Début de la pause après dessin complet
    unsigned long chip_wait_start = 0;    // Début de la pause avant de redessiner
    float chip_fade_alpha = 1.0f;         // Alpha pour le fondu (1.0 = opaque, 0.0 = invisible)

    // Animation du pourcentage G2S
    float g2s_percent_anim = 0.0f; // 0.0 à 100.0, animé à l'arrivée sur l'écran
    bool g2s_percent_anim_started = false;
    float g2s_percent_anim_time = 0.0f; // Temps écoulé depuis le début de l'anim
};
