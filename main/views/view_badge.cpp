
#include "view_badge.h"
#include "user_info.h"
#include <string>
#include "state.h"
#include <cmath>
#include "esp_timer.h"
#include "esp_random.h"

// Couleurs badge (similaire à l'ancien display_manager)
static uint16_t colBackground = 0;
static uint16_t colNeonViolet = 0;
static uint16_t colCyan = 0;
static uint16_t colYellow = 0;
static uint16_t colHeart = 0;

// Utilitaire pour initialiser les couleurs une seule fois
static void initColors(LGFX &display)
{
    if (colBackground == 0)
    {
        colBackground = display.color565(8, 6, 20);
        colNeonViolet = display.color565(199, 120, 255);
        colCyan = display.color565(0, 224, 255);
        colYellow = display.color565(255, 195, 0);
        colHeart = display.color565(255, 40, 180);
    }
}

ViewBadge::ViewBadge(AppState &state, LGFX &lcd)
    : m_state(state), m_lcd(lcd)
{
    // Initialiser le prochain clignotement
    uint32_t random_delay = (esp_random() % 15000) + 5000; // 5-20 secondes
    m_state.neon_next_flicker = (esp_timer_get_time() / 1000ULL) + random_delay;
}

void ViewBadge::updateNeonIntensity()
{
    unsigned long now = esp_timer_get_time() / 1000ULL; // Convertir microsecondes en millisecondes

    // Calcul du delta time en secondes
    float dt = 0.016f; // ~60fps par défaut
    if (m_state.neon_last_update > 0)
    {
        dt = (now - m_state.neon_last_update) / 1000.0f;
        dt = fminf(dt, 0.1f); // Limiter le delta time
    }
    m_state.neon_last_update = now;

    // Incrémenter le temps pour la sinusoïde
    m_state.neon_time += dt;

    // Vérifier si on doit déclencher un clignotement
    if (!m_state.neon_is_flickering && now >= m_state.neon_next_flicker)
    {
        m_state.neon_is_flickering = true;
        m_state.neon_flicker_start = now;
    }

    // Gérer le clignotement
    if (m_state.neon_is_flickering)
    {
        unsigned long flicker_duration = now - m_state.neon_flicker_start;
        uint32_t flicker_length = (esp_random() % 1000) + 1000; // 1-2 secondes

        if (flicker_duration < flicker_length)
        {
            // Clignotement rapide et fort avec variations aléatoires
            float flicker_speed = 15.0f + ((esp_random() % 100) / 100.0f) * 10.0f; // 15-25 Hz
            float flicker_phase = m_state.neon_time * flicker_speed;
            float flicker_base = sinf(flicker_phase);

            // Ajouter du bruit aléatoire
            float noise = ((esp_random() % 100) / 100.0f) * 0.3f - 0.15f;

            // Intensité très basse pendant le clignotement (presque éteint)
            m_state.neon_flicker_intensity = 0.15f + (flicker_base * 0.5f + 0.5f) * 0.25f + noise;
            m_state.neon_flicker_intensity = fmaxf(0.05f, fminf(1.0f, m_state.neon_flicker_intensity));
        }
        else
        {
            // Fin du clignotement
            m_state.neon_is_flickering = false;
            m_state.neon_flicker_intensity = 1.0f;
            // Planifier le prochain clignotement dans 5-20 secondes
            uint32_t random_delay = (esp_random() % 15000) + 5000;
            m_state.neon_next_flicker = now + random_delay;
        }
    }
    else
    {
        m_state.neon_flicker_intensity = 1.0f;
    }

    // Dim doux sinusoïdal avec faible amplitude (haute intensité la plupart du temps)
    float sine_wave = sinf(m_state.neon_time * 2.0f); // Fréquence douce ~0.3 Hz
    float base_intensity = 0.92f + sine_wave * 0.08f; // Oscille entre 0.84 et 1.0

    // Combiner avec le clignotement
    m_state.neon_intensity = base_intensity * m_state.neon_flicker_intensity;

    // Clamp final
    m_state.neon_intensity = fmaxf(0.05f, fminf(1.0f, m_state.neon_intensity));
}

void ViewBadge::updateScanlineOffset()
{
    m_state.scanline_offset = (m_state.scanline_offset + 1) % 10;
}

void ViewBadge::renderBackground(LGFX_Sprite &spr)
{
    spr.fillSprite(colBackground);
    renderScanlines(spr);
}

void ViewBadge::renderHeader(LGFX_Sprite &spr)
{
    spr.setTextDatum(TC_DATUM);
    spr.setTextFont(1);
    spr.setTextSize(2);
    spr.setTextColor(colCyan);
    spr.drawString("100% G2S", m_state.screenW / 2, 20);
}

void ViewBadge::renderName(LGFX_Sprite &spr)
{
    renderNeonFullName(spr, user_info.prenom.c_str(), user_info.nom.c_str());
}

void ViewBadge::renderSeparator(LGFX_Sprite &spr)
{
    spr.fillRect(36, 180, m_state.screenW - 80, 3, colCyan);
}

void ViewBadge::renderTeam(LGFX_Sprite &spr)
{
    spr.setTextDatum(TC_DATUM);
    spr.setTextFont(1);
    spr.setTextSize(2);
    spr.setTextColor(colCyan);
    spr.drawString(user_info.equipe.c_str(), m_state.screenW / 2, 200);
}

void ViewBadge::renderLocationAndRole(LGFX_Sprite &spr)
{
    spr.setTextDatum(TC_DATUM);
    spr.setTextFont(1);
    spr.setTextSize(2);
    spr.setTextColor(colYellow);
    spr.drawString(user_info.ville.c_str(), m_state.screenW / 2, 230);
    spr.drawString(user_info.poste.c_str(), m_state.screenW / 2, 260);
}

void ViewBadge::renderScanlines(LGFX_Sprite &spr)
{
    for (int y = 0; y < m_state.screenH; y += 10)
    {
        int animY = y + m_state.scanline_offset;
        if (animY < m_state.screenH)
        {
            uint16_t col = m_lcd.color565(12, 8, 30);
            spr.drawFastHLine(0, animY, m_state.screenW, col);
        }
    }
}

void ViewBadge::renderNeonText(LGFX_Sprite &spr, const char *txt1, const char *txt2, int x, int y1, int y2)
{
    uint8_t baseR = 255, baseG = 60, baseB = 200;
    float flicker = m_state.neon_intensity;
    float organic = fmaxf(flicker, 0.32f);
    spr.setTextDatum(MC_DATUM);
    spr.setTextFont(4);

    // Toujours 12 layers, mais leur intensité varie avec le flicker
    const int layers = 12;
    for (int i = layers; i > 0; i--)
    {
        float fade = powf(0.8f, i) * organic;
        float alpha = fade * 0.5f * flicker;
        uint8_t r = (uint8_t)(baseR * alpha + 8 * (1.0f - alpha));
        uint8_t g = (uint8_t)(baseG * alpha + 6 * (1.0f - alpha));
        uint8_t b = (uint8_t)(baseB * alpha + 20 * (1.0f - alpha));
        uint16_t color = m_lcd.color565(r, g, b);
        spr.setTextColor(color);
        spr.setTextSize(1.5f + 0.02f * i);
        spr.drawString(txt1, x + i, y1);
        spr.drawString(txt1, x - i, y1);
        spr.drawString(txt1, x, y1 + i);
        spr.drawString(txt1, x, y1 - i);
        spr.drawString(txt2, x + i, y2);
        spr.drawString(txt2, x - i, y2);
        spr.drawString(txt2, x, y2 + i);
        spr.drawString(txt2, x, y2 - i);
    }
    spr.setTextSize(1.5f);
    // Texte de base : plus foncé que le background quand le néon s'éteint
    // Background = (8, 6, 20), donc on utilise des valeurs encore plus basses quand flicker est bas
    float brightness = 0.3f + 0.6f * organic;
    uint8_t darkR = (uint8_t)(baseR * brightness * flicker + 4 * (1.0f - flicker));
    uint8_t darkG = (uint8_t)(baseG * brightness * flicker + 3 * (1.0f - flicker));
    uint8_t darkB = (uint8_t)(baseB * brightness * flicker + 10 * (1.0f - flicker));
    uint16_t brightColor = m_lcd.color565(darkR, darkG, darkB);
    spr.setTextColor(brightColor);
    spr.drawString(txt1, x, y1);
    spr.drawString(txt2, x, y2);
}

void ViewBadge::renderNeonFullName(LGFX_Sprite &spr, const char *name1, const char *name2)
{
    int x = m_state.screenW / 2;
    int y1 = 110;
    int y2 = 145;
    renderNeonText(spr, name1, name2, x, y1, y2);
}

// Affichage principal du badge
void ViewBadge::render(LGFX &display, LGFX_Sprite &spr)
{
    initColors(display);
    // Update AppState screen size
    m_state.screenW = spr.width();
    m_state.screenH = spr.height();

    // Mise à jour des variables d'animation
    updateNeonIntensity();
    updateScanlineOffset();

    // Rendu
    renderBackground(spr);
    renderHeader(spr);
    renderName(spr);
    renderSeparator(spr);
    renderTeam(spr);
    renderLocationAndRole(spr);
}
