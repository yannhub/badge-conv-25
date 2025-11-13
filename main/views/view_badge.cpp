
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

    // Initialiser le prochain glitch
    m_state.glitch_next = (esp_timer_get_time() / 1000ULL) + ((esp_random() % 8000) + 7000); // 7-15 secondes
}

void ViewBadge::initParticles()
{
    if (!m_state.particles_initialized)
    {
        for (int i = 0; i < 8; i++)
        {
            m_state.particles[i].x = (esp_random() % m_state.screenW);
            m_state.particles[i].y = (esp_random() % m_state.screenH);
            m_state.particles[i].speed = 0.3f + ((esp_random() % 100) / 100.0f) * 0.5f;
            m_state.particles[i].size = 1.0f + ((esp_random() % 100) / 100.0f) * 1.5f;
            m_state.particles[i].alpha = 0.3f + ((esp_random() % 100) / 100.0f) * 0.4f;
            m_state.particles[i].phase = ((esp_random() % 100) / 100.0f) * 6.28f;
        }
        m_state.particles_initialized = true;
    }
}

void ViewBadge::updateAnimations(float dt)
{
    // Grille animée
    m_state.grid_offset += dt * 5.0f;
    if (m_state.grid_offset >= 20.0f)
        m_state.grid_offset -= 20.0f;

    // Pulsation des coins
    m_state.corner_pulse += dt * 2.0f;

    // Pulsation des bordures
    m_state.border_pulse += dt * 1.5f;

    // Animation des particules
    for (int i = 0; i < 8; i++)
    {
        m_state.particles[i].y -= m_state.particles[i].speed;
        if (m_state.particles[i].y < -10)
        {
            m_state.particles[i].y = m_state.screenH + 10;
            m_state.particles[i].x = (esp_random() % m_state.screenW);
        }
        m_state.particles[i].phase += dt * 2.0f;
    }

    // Gestion du glitch
    unsigned long now = esp_timer_get_time() / 1000ULL;
    if (!m_state.glitch_active && now >= m_state.glitch_next)
    {
        m_state.glitch_active = true;
        m_state.glitch_start = now;
    }

    if (m_state.glitch_active)
    {
        unsigned long glitch_duration = now - m_state.glitch_start;
        if (glitch_duration < 150) // 150ms de glitch
        {
            m_state.glitch_offset_x = ((esp_random() % 7) - 3);
            m_state.glitch_offset_y = ((esp_random() % 5) - 2);
        }
        else
        {
            m_state.glitch_active = false;
            m_state.glitch_offset_x = 0;
            m_state.glitch_offset_y = 0;
            m_state.glitch_next = now + ((esp_random() % 8000) + 7000); // 7-15 secondes
        }
    }
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
    drawSimpleNeonText(spr, "100% G2S", m_state.screenW / 2, 20, colCyan);
}

void ViewBadge::renderName(LGFX_Sprite &spr)
{
    renderNeonFullName(spr, user_info.prenom.c_str(), user_info.nom.c_str());
}

void ViewBadge::renderSeparator(LGFX_Sprite &spr)
{
    drawNeonLine(spr, 36, 180, m_state.screenW - 44, 180, colCyan);
}

void ViewBadge::renderTeam(LGFX_Sprite &spr)
{
    spr.setTextDatum(TC_DATUM);
    spr.setTextFont(1);
    spr.setTextSize(2);
    drawSimpleNeonText(spr, user_info.equipe.c_str(), m_state.screenW / 2, 200, colCyan);
}

void ViewBadge::renderLocationAndRole(LGFX_Sprite &spr)
{
    spr.setTextDatum(TC_DATUM);
    spr.setTextFont(1);
    spr.setTextSize(2);
    drawSimpleNeonText(spr, user_info.ville.c_str(), m_state.screenW / 2, 230, colYellow);
    drawSimpleNeonText(spr, user_info.poste.c_str(), m_state.screenW / 2, 260, colYellow);
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

void ViewBadge::renderCorners(LGFX_Sprite &spr)
{
    float pulse = (sinf(m_state.corner_pulse) * 0.5f + 0.5f);
    uint8_t intensity = (uint8_t)(100 + pulse * 155);
    uint16_t cornerColor = m_lcd.color565(intensity * 0.7, intensity * 0.8, intensity);

    int cornerSize = 15;
    int thickness = 2;

    // Coin haut-gauche
    spr.fillRect(5, 5, cornerSize, thickness, cornerColor);
    spr.fillRect(5, 5, thickness, cornerSize, cornerColor);

    // Coin haut-droit
    spr.fillRect(m_state.screenW - cornerSize - 5, 5, cornerSize, thickness, cornerColor);
    spr.fillRect(m_state.screenW - 5 - thickness, 5, thickness, cornerSize, cornerColor);

    // Coin bas-gauche
    spr.fillRect(5, m_state.screenH - 5 - thickness, cornerSize, thickness, cornerColor);
    spr.fillRect(5, m_state.screenH - cornerSize - 5, thickness, cornerSize, cornerColor);

    // Coin bas-droit
    spr.fillRect(m_state.screenW - cornerSize - 5, m_state.screenH - 5 - thickness, cornerSize, thickness, cornerColor);
    spr.fillRect(m_state.screenW - 5 - thickness, m_state.screenH - cornerSize - 5, thickness, cornerSize, cornerColor);
}

void ViewBadge::renderParticles(LGFX_Sprite &spr)
{
    for (int i = 0; i < 8; i++)
    {
        float pulse = sinf(m_state.particles[i].phase) * 0.5f + 0.5f;
        uint8_t intensity = (uint8_t)(m_state.particles[i].alpha * pulse * 200);
        uint16_t particleColor = m_lcd.color565(
            intensity * 0.6,
            intensity * 0.8,
            intensity);

        int x = (int)m_state.particles[i].x;
        int y = (int)m_state.particles[i].y;
        int size = (int)m_state.particles[i].size;

        if (size > 1)
        {
            spr.fillCircle(x, y, size, particleColor);
        }
        else
        {
            spr.drawPixel(x, y, particleColor);
        }
    }
}

void ViewBadge::renderBorders(LGFX_Sprite &spr)
{
    float pulse = (sinf(m_state.border_pulse) * 0.3f + 0.7f) * m_state.neon_intensity;
    uint8_t intensity = (uint8_t)(pulse * 150);
    uint16_t borderColor = m_lcd.color565(intensity * 0.9, intensity * 0.6, intensity);

    // Bordures fines avec effet de lueur
    spr.drawRect(2, 2, m_state.screenW - 4, m_state.screenH - 4, borderColor);

    // Effet de double bordure pour plus de profondeur
    uint16_t borderColor2 = m_lcd.color565(intensity * 0.5, intensity * 0.3, intensity * 0.6);
    spr.drawRect(1, 1, m_state.screenW - 2, m_state.screenH - 2, borderColor2);
}

void ViewBadge::renderGeometricElements(LGFX_Sprite &spr)
{
    float pulse = (sinf(m_state.corner_pulse * 1.3f) * 0.5f + 0.5f);
    uint8_t intensity = (uint8_t)(80 + pulse * 80);
    uint16_t geomColor = m_lcd.color565(intensity * 0.5, intensity, intensity * 0.8);

    // Petits triangles dans les coins
    int triSize = 8;

    // Triangle haut-gauche (pointant vers le bas-droit)
    spr.drawLine(25, 15, 25 + triSize, 15, geomColor);
    spr.drawLine(25, 15, 25, 15 + triSize, geomColor);
    spr.drawLine(25 + triSize, 15, 25, 15 + triSize, geomColor);

    // Triangle haut-droit (pointant vers le bas-gauche)
    spr.drawLine(m_state.screenW - 25 - triSize, 15, m_state.screenW - 25, 15, geomColor);
    spr.drawLine(m_state.screenW - 25, 15, m_state.screenW - 25, 15 + triSize, geomColor);
    spr.drawLine(m_state.screenW - 25 - triSize, 15, m_state.screenW - 25, 15 + triSize, geomColor);

    // Lignes horizontales décoratives pulsantes
    int lineY1 = 50;
    int lineY2 = m_state.screenH - 50;
    int lineLen = (int)(30 + pulse * 10);

    spr.drawFastHLine(10, lineY1, lineLen, geomColor);
    spr.drawFastHLine(m_state.screenW - 10 - lineLen, lineY1, lineLen, geomColor);
    spr.drawFastHLine(10, lineY2, lineLen, geomColor);
    spr.drawFastHLine(m_state.screenW - 10 - lineLen, lineY2, lineLen, geomColor);
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

// Helper pour effet néon simple sur un texte
void ViewBadge::drawSimpleNeonText(LGFX_Sprite &spr, const char *text, int x, int y, uint16_t baseColor)
{
    // Appliquer le glitch si actif
    int glitch_x = m_state.glitch_active ? m_state.glitch_offset_x : 0;
    int glitch_y = m_state.glitch_active ? m_state.glitch_offset_y : 0;

    // Extraire les composantes RGB de la couleur de base
    uint8_t baseR = ((baseColor >> 11) & 0x1F) * 8;
    uint8_t baseG = ((baseColor >> 5) & 0x3F) * 4;
    uint8_t baseB = (baseColor & 0x1F) * 8;

    // Effet glitch RGB (séparation des canaux)
    if (m_state.glitch_active)
    {
        uint16_t redChannel = m_lcd.color565(baseR * 0.6, 0, 0);
        spr.setTextColor(redChannel);
        spr.drawString(text, x + glitch_x - 2, y + glitch_y);

        uint16_t blueChannel = m_lcd.color565(0, 0, baseB * 0.6);
        spr.setTextColor(blueChannel);
        spr.drawString(text, x + glitch_x + 2, y + glitch_y);
    }

    // Effet néon simple : ombre/glow décalée (style QR code)
    // Déterminer la couleur de l'ombre (magenta pour cyan, rose pour jaune)
    uint16_t shadowColor;
    if (baseColor == colCyan)
    {
        shadowColor = m_lcd.color565(255, 0, 150); // Magenta néon
    }
    else if (baseColor == colYellow)
    {
        shadowColor = m_lcd.color565(255, 40, 180); // Rose/magenta
    }
    else
    {
        shadowColor = colNeonViolet; // Par défaut
    }

    // Ombre/glow décalée
    spr.setTextColor(shadowColor);
    spr.drawString(text, x + glitch_x + 1, y + glitch_y + 1);

    // Texte principal
    spr.setTextColor(baseColor);
    spr.drawString(text, x + glitch_x, y + glitch_y);
}

// Helper pour effet néon sur une ligne
void ViewBadge::drawNeonLine(LGFX_Sprite &spr, int x1, int y1, int x2, int y2, uint16_t baseColor)
{
    // Appliquer le glitch si actif
    int glitch_x = m_state.glitch_active ? m_state.glitch_offset_x : 0;
    int glitch_y = m_state.glitch_active ? m_state.glitch_offset_y : 0;

    // Extraire les composantes RGB
    uint8_t baseR = ((baseColor >> 11) & 0x1F) * 8;
    uint8_t baseG = ((baseColor >> 5) & 0x3F) * 4;
    uint8_t baseB = (baseColor & 0x1F) * 8;

    // Effet glitch sur la ligne (séparation RGB)
    if (m_state.glitch_active)
    {
        uint16_t redChannel = m_lcd.color565(baseR * 0.5, 0, 0);
        spr.drawFastHLine(x1 + glitch_x - 2, y1 + glitch_y, x2 - x1, redChannel);

        uint16_t blueChannel = m_lcd.color565(0, 0, baseB * 0.5);
        spr.drawFastHLine(x1 + glitch_x + 2, y1 + glitch_y, x2 - x1, blueChannel);
    }

    // Effet néon simple : ombre/glow décalée (style QR code)
    uint16_t shadowColor = m_lcd.color565(255, 0, 150); // Magenta néon

    // Ombre décalée
    spr.drawFastHLine(x1 + glitch_x, y1 + glitch_y + 1, x2 - x1, shadowColor);

    // Ligne principale
    spr.drawFastHLine(x1 + glitch_x, y1 + glitch_y, x2 - x1, baseColor);
}

// Affichage principal du badge
void ViewBadge::render(LGFX &display, LGFX_Sprite &spr)
{
    initColors(display);
    // Update AppState screen size
    m_state.screenW = spr.width();
    m_state.screenH = spr.height();

    // Initialiser les particules si nécessaire
    initParticles();

    // Calcul du delta time
    float dt = 0.016f; // ~60fps par défaut
    unsigned long now = esp_timer_get_time() / 1000ULL;
    if (m_state.neon_last_update > 0)
    {
        dt = (now - m_state.neon_last_update) / 1000.0f;
        dt = fminf(dt, 0.1f);
    }

    // Mise à jour des animations
    updateNeonIntensity();
    updateScanlineOffset();
    updateAnimations(dt);

    // Rendu en couches (de l'arrière vers l'avant)
    renderBackground(spr);        // Fond sombre
    renderParticles(spr);         // Particules flottantes
    renderScanlines(spr);         // Lignes de scan CRT
    renderGeometricElements(spr); // Éléments géométriques décoratifs
    renderBorders(spr);           // Bordures pulsantes
    renderCorners(spr);           // Coins décoratifs
    renderHeader(spr);            // Titre "100% G2S"
    renderName(spr);              // Nom avec effet néon
    renderSeparator(spr);         // Ligne de séparation
    renderTeam(spr);              // Équipe
    renderLocationAndRole(spr);   // Ville et poste
}
