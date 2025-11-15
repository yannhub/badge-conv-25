
#include "view_badge.h"
#include "user_info.h"
#include <string>
#include <vector>
#include "state.h"
#include <cmath>
#include "esp_timer.h"
#include "esp_random.h"
#include "esp_log.h"
#include "../Orbitron_Bold24pt7b.h"

// Couleurs badge (similaire à l'ancien display_manager)
static uint16_t colBackground = 0;
static uint16_t colCyan = 0;
static uint16_t colYellow = 0;
static uint16_t colPink = 0;
static uint16_t colMagenta = 0;

// Utilitaire pour initialiser les couleurs une seule fois
static void initColors(LGFX &display)
{
    if (colBackground == 0)
    {
        colBackground = display.color565(10, 0, 30);
        colCyan = display.color565(0, 255, 255);
        colYellow = display.color565(255, 255, 0);
        colPink = display.color565(255, 20, 220);
        colMagenta = display.color565(255, 64, 255);
    }
}

ViewBadge::ViewBadge(AppState &state, LGFX &lcd)
    : m_state(state), m_lcd(lcd)
{
    // Initialiser les couleurs une seule fois
    initColors(lcd);
    // Initialiser le prochain glitch
    m_state.glitch_next = (esp_timer_get_time() / 1000ULL) + ((esp_random() % 8000) + 7000); // 7-15 secondes
}

void ViewBadge::initParticles()
{
    if (!m_state.particles_initialized)
    {
        unsigned long now = esp_timer_get_time() / 1000ULL;
        for (int i = 0; i < 12; i++)
        {
            m_state.particles[i].x = (esp_random() % m_state.screenW);
            m_state.particles[i].y = (esp_random() % m_state.screenH);
            m_state.particles[i].speed = 2.0f + ((esp_random() % 100) / 100.0f) * 2.0f; // Durée de vie plus courte: 2 à 4
            m_state.particles[i].size = 1.5f + ((esp_random() % 100) / 100.0f) * 1.5f;  // Taille réduite: 1.5 à 3
            m_state.particles[i].alpha = 0.0f;                                          // Commence invisible
            m_state.particles[i].phase = ((esp_random() % 100) / 100.0f) * 6.28f;
            // Apparition initiale étalée entre 0 et 10 secondes
            m_state.particles[i].active = false;
            m_state.particles[i].next_spawn_time = now + (esp_random() % 10000);
        }
        m_state.particles_initialized = true;
    }
}

void ViewBadge::updateAnimations(float dt)
{

    // Pulsation des coins
    m_state.corner_pulse += dt * 2.0f;

    // Pulsation des bordures
    m_state.border_pulse += dt * 1.5f;

    // Animation du microprocesseur
    unsigned long now_ms = esp_timer_get_time() / 1000ULL;

    // Phase 1: Dessin (progress 0.0 -> 1.0)
    if (m_state.chip_animation_progress < 1.0f && m_state.chip_wait_start == 0)
    {
        m_state.chip_fade_alpha = 1.0f;                // S'assurer que l'alpha est à 1.0 pendant le dessin
        m_state.chip_animation_progress += dt * 0.25f; // Vitesse de dessin
        if (m_state.chip_animation_progress >= 1.0f)
        {
            m_state.chip_animation_progress = 1.0f;
            m_state.chip_pause_start = now_ms; // Démarrer la pause
        }
    }
    // Phase 2: Pause de 5 secondes après dessin complet
    else if (m_state.chip_pause_start > 0 && (now_ms - m_state.chip_pause_start) < 5000)
    {
        // En pause, ne rien faire
        m_state.chip_fade_alpha = 1.0f;
    }
    // Phase 3: Fondu pendant 3 secondes
    else if (m_state.chip_pause_start > 0 && m_state.chip_fade_alpha > 0.0f)
    {
        m_state.chip_fade_alpha -= dt / 3.0f; // Diminuer alpha sur 3 secondes
        if (m_state.chip_fade_alpha <= 0.0f)
        {
            m_state.chip_fade_alpha = 0.0f;
            m_state.chip_wait_start = now_ms; // Démarrer la pause avant redessinage
            m_state.chip_pause_start = 0;
        }
    }
    // Phase 4: Pause de 5 secondes avant de redessiner
    else if (m_state.chip_wait_start > 0 && (now_ms - m_state.chip_wait_start) < 5000)
    {
        // Attendre avant de redessiner (chip invisible)
        m_state.chip_fade_alpha = 0.0f;
    }
    // Phase 5: Recommencer le cycle
    else if (m_state.chip_wait_start > 0)
    {
        m_state.chip_animation_progress = 0.0f;
        m_state.chip_wait_start = 0;
        m_state.chip_fade_alpha = 1.0f;
    }

    // Animation des particules - apparition/disparition à différents endroits + mouvement vers le haut
    unsigned long now = esp_timer_get_time() / 1000ULL;

    for (int i = 0; i < 12; i++)
    {
        // Vérifier si la particule doit être activée
        if (!m_state.particles[i].active)
        {
            if (now >= m_state.particles[i].next_spawn_time)
            {
                // Activer la particule avec une nouvelle position
                m_state.particles[i].active = true;
                m_state.particles[i].phase = 0.0f;
                m_state.particles[i].x = 20 + (esp_random() % (m_state.screenW - 40));      // Marge de 20px
                m_state.particles[i].y = 20 + (esp_random() % (m_state.screenH - 40));      // Apparition aléatoire
                m_state.particles[i].speed = 2.0f + ((esp_random() % 100) / 100.0f) * 2.0f; // Durée de vie plus courte
                m_state.particles[i].size = 1.5f + ((esp_random() % 100) / 100.0f) * 1.5f;
            }
            continue; // Passer à la particule suivante
        }

        // Animer uniquement les particules actives
        m_state.particles[i].y -= 0.4f + ((float)(i % 3)) * 0.1f; // Vitesses variées: 0.2, 0.3, 0.4 (plus lent)

        // Incrémenter la phase pour l'effet de fade in/out
        m_state.particles[i].phase += dt * m_state.particles[i].speed;

        // Cycle de vie : 0 à 2π pour fade in, 2π à 4π pour fade out
        if (m_state.particles[i].phase > 6.28f || m_state.particles[i].y < -10)
        {
            // Désactiver la particule et définir le prochain temps d'apparition
            m_state.particles[i].active = false;
            // Délai aléatoire entre 1 et 10 secondes
            m_state.particles[i].next_spawn_time = now + 1000 + (esp_random() % 9000);
        }

        // Calculer l'alpha pour le fade in/out (seulement si active)
        if (m_state.particles[i].active)
        {
            float cycle = m_state.particles[i].phase / 3.14f; // 0 à 2
            if (cycle < 1.0f)
            {
                // Fade in
                m_state.particles[i].alpha = cycle;
            }
            else
            {
                // Fade out
                m_state.particles[i].alpha = 2.0f - cycle;
            }
        }
    }

    // Gestion du glitch
    if (!m_state.glitch_active && now >= m_state.glitch_next)
    {
        m_state.glitch_active = true;
        m_state.glitch_start = now;
    }

    if (m_state.glitch_active)
    {
        static unsigned long glitch_target_duration = 0;
        if (glitch_target_duration == 0)
        {
            glitch_target_duration = 150 + (esp_random() % 451); // 150 à 600ms
        }
        unsigned long glitch_duration = now - m_state.glitch_start;
        if (glitch_duration < glitch_target_duration)
        {
            m_state.glitch_offset_x = ((esp_random() % 7) - 3);
            m_state.glitch_offset_y = ((esp_random() % 5) - 2);
        }
        else
        {
            m_state.glitch_active = false;
            m_state.glitch_offset_x = 0;
            m_state.glitch_offset_y = 0;
            m_state.glitch_next = now + ((esp_random() % 6000) + 2000); // 2-8 secondes
            glitch_target_duration = 0;
        }
    }
}

void ViewBadge::updateScanlineOffset()
{
    // Animation fluide basée sur dt (vitesse: 15 pixels par seconde)
    m_state.scanline_offset = fmodf(m_state.scanline_offset + m_state.dt * 15.0f, 10.0f);
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
    drawNeonText(spr, "100% G2S", m_state.screenW / 2, 20, colCyan);
}

void ViewBadge::renderName(LGFX_Sprite &spr)
{
    renderNeonFullName(spr, user_info.prenom.c_str(), user_info.nom.c_str());
}

void ViewBadge::renderSeparator(LGFX_Sprite &spr)
{
    drawNeonLine(spr, 36, 160, m_state.screenW - 44, 160, colCyan);
}

void ViewBadge::renderTeam(LGFX_Sprite &spr)
{
    spr.setTextDatum(TC_DATUM);
    spr.setTextFont(1);
    spr.setTextSize(2);
    drawNeonText(spr, user_info.equipe.c_str(), m_state.screenW / 2, 180, colCyan);
}

void ViewBadge::renderLocationAndRole(LGFX_Sprite &spr)
{
    spr.setTextDatum(TC_DATUM);
    spr.setTextFont(1);
    spr.setTextSize(2);
    drawNeonText(spr, user_info.ville.c_str(), m_state.screenW / 2, 210, colYellow);
    drawNeonText(spr, user_info.poste.c_str(), m_state.screenW / 2, 240, colYellow);
}

void ViewBadge::renderScanlines(LGFX_Sprite &spr)
{
    // Dessiner les scanlines avec défilement fluide
    uint16_t col = m_lcd.color565(12, 8, 30);
    for (int y = 0; y < m_state.screenH; y += 10)
    {
        int animY = (y + (int)m_state.scanline_offset) % m_state.screenH;
        spr.drawFastHLine(0, animY, m_state.screenW, col);
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
    // Dessiner toutes les particules actives (jusqu'à 12)
    for (int i = 0; i < 12; i++)
    {
        // Ne dessiner que les particules actives avec alpha visible
        if (!m_state.particles[i].active || m_state.particles[i].alpha < 0.05f)
            continue;

        // Variantes de cyan uniquement
        uint8_t r, g, b;

        // Alterner entre différentes nuances de cyan
        int color_index = i % 3;
        if (color_index == 0)
        {
            // Cyan pur brillant
            r = 0;
            g = 255;
            b = 255;
        }
        else if (color_index == 1)
        {
            // Cyan électrique (plus de bleu)
            r = 0;
            g = 200;
            b = 255;
        }
        else
        {
            // Cyan doux (plus de vert)
            r = 100;
            g = 255;
            b = 200;
        }

        // Appliquer l'alpha pour le fade in/out
        float brightness = m_state.particles[i].alpha;
        uint16_t particleColor = m_lcd.color565(
            r * brightness,
            g * brightness,
            b * brightness);

        int x = (int)m_state.particles[i].x;
        int y = (int)m_state.particles[i].y;
        int size = (int)m_state.particles[i].size;

        // Dessiner uniquement des losanges / diamants
        spr.drawLine(x, y - size, x + size, y, particleColor);
        spr.drawLine(x + size, y, x, y + size, particleColor);
        spr.drawLine(x, y + size, x - size, y, particleColor);
        spr.drawLine(x - size, y, x, y - size, particleColor);
    }
}

void ViewBadge::renderBorders(LGFX_Sprite &spr)
{
    float pulse = (sinf(m_state.border_pulse) * 0.3f + 0.7f);
    uint8_t intensity = (uint8_t)(pulse * 150);
    uint16_t borderColor = m_lcd.color565(intensity * 0.9, intensity * 0.6, intensity);

    // Bordures fines avec effet de lueur
    spr.drawRect(2, 2, m_state.screenW - 4, m_state.screenH - 4, borderColor);

    // Effet de double bordure pour plus de profondeur
    uint16_t borderColor2 = m_lcd.color565(intensity * 0.5, intensity * 0.3, intensity * 0.6);
    spr.drawRect(1, 1, m_state.screenW - 2, m_state.screenH - 2, borderColor2);
}

void ViewBadge::drawTriangle(LGFX_Sprite &spr, int cornerX, int cornerY, bool pointRight, bool pointDown, int triSize, uint8_t intensity, uint16_t geomColor)
{
    // Triangle rectangle pointant vers le centre de l'écran
    int x1 = cornerX;
    int y1 = cornerY;
    int x2 = pointRight ? cornerX + triSize : cornerX - triSize;
    int y2 = cornerY;
    int x3 = cornerX;
    int y3 = pointDown ? cornerY + triSize : cornerY - triSize;

    // Dessiner le triangle avec double ligne pour effet de profondeur
    spr.drawLine(x1, y1, x2, y2, geomColor);
    spr.drawLine(x2, y2, x3, y3, geomColor);
    spr.drawLine(x3, y3, x1, y1, geomColor);

    // Ligne intérieure pour effet de remplissage partiel
    if (triSize > 4)
    {
        uint16_t innerColor = m_lcd.color565(intensity * 0.3, intensity * 0.6, intensity * 0.5);
        int innerSize = triSize - 3;
        int ix2 = pointRight ? cornerX + innerSize : cornerX - innerSize;
        int iy3 = pointDown ? cornerY + innerSize : cornerY - innerSize;
        spr.drawLine(x1, y1, ix2, y2, innerColor);
        spr.drawLine(ix2, y2, x1, iy3, innerColor);
    }
}

void ViewBadge::renderGeometricElements(LGFX_Sprite &spr)
{
    float pulse = (sinf(m_state.corner_pulse * 1.3f) * 0.5f + 0.5f);
    uint8_t intensity = (uint8_t)(80 + pulse * 80);
    uint16_t geomColor = m_lcd.color565(intensity * 0.5, intensity, intensity * 0.8);

    int triSize = 8;
    int cornerOffset = 10;

    // Triangle haut-gauche (pointe vers bas-droit)
    drawTriangle(spr, cornerOffset, cornerOffset, true, true, triSize, intensity, geomColor);

    // Triangle haut-droit (pointe vers bas-gauche)
    drawTriangle(spr, m_state.screenW - cornerOffset, cornerOffset, false, true, triSize, intensity, geomColor);

    // Triangle bas-gauche (pointe vers haut-droit)
    drawTriangle(spr, cornerOffset, m_state.screenH - cornerOffset, true, false, triSize, intensity, geomColor);

    // Triangle bas-droit (pointe vers haut-gauche)
    drawTriangle(spr, m_state.screenW - cornerOffset, m_state.screenH - cornerOffset, false, false, triSize, intensity, geomColor);

    // Lignes horizontales avec effet de vague et déploiement
    int lineY1 = 40;

    // Animation de longueur avec effet de vague (décalage de phase entre lignes)
    float wave1 = sinf(m_state.border_pulse * 0.5f);
    float wave2 = sinf(m_state.border_pulse * 0.5f + 1.57f); // Décalage de 90°

    int baseLen = 25;
    int maxExtension = 20;

    // Lignes du haut (gauche et droite avec animations opposées)
    int lineLen1_left = baseLen + (int)(wave1 * maxExtension);
    int lineLen1_right = baseLen + (int)(-wave1 * maxExtension);

    // Lignes du bas (effet inversé)
    int lineLen2_left = baseLen + (int)(wave2 * maxExtension);
    int lineLen2_right = baseLen + (int)(-wave2 * maxExtension);

    // Effet de double ligne pour plus de profondeur
    uint16_t geomColorDim = m_lcd.color565(intensity * 0.3, intensity * 0.6, intensity * 0.5);

    // Lignes haut
    spr.drawFastHLine(10, lineY1, lineLen1_left, geomColor);
    spr.drawFastHLine(10, lineY1 + 2, lineLen1_left * 0.7f, geomColorDim);
    spr.drawFastHLine(m_state.screenW - 10 - lineLen1_right, lineY1, lineLen1_right, geomColor);
    spr.drawFastHLine(m_state.screenW - 10 - (int)(lineLen1_right * 0.7f), lineY1 + 2, lineLen1_right * 0.7f, geomColorDim);

    // Lignes bas
    spr.drawFastHLine(10, m_state.screenH - lineY1, lineLen2_left, geomColor);
    spr.drawFastHLine(10, m_state.screenH - lineY1 - 2, lineLen2_left * 0.7f, geomColorDim);
    spr.drawFastHLine(m_state.screenW - 10 - lineLen2_right, m_state.screenH - lineY1, lineLen2_right, geomColor);
    spr.drawFastHLine(m_state.screenW - 10 - (int)(lineLen2_right * 0.7f), m_state.screenH - lineY1 - 2, lineLen2_right * 0.7f, geomColorDim);
}

void ViewBadge::renderMicroprocessor(LGFX_Sprite &spr)
{
    // Ne rien dessiner si complètement invisible ou si le fondu est presque terminé
    if (m_state.chip_fade_alpha <= 0.05f)
    {
        return;
    }

    // Interpolation entre la couleur cyan (0, 224, 255) et la couleur du background (6, 4, 16)
    uint8_t bgR = 6;
    uint8_t bgG = 4;
    uint8_t bgB = 16;
    uint8_t cyanR = 0;
    uint8_t cyanG = 224;
    uint8_t cyanB = 255;

    // Interpoler selon chip_fade_alpha : alpha=1.0 = cyan pur, alpha=0.0 = background
    float alpha = m_state.chip_fade_alpha;
    uint8_t redComponent = (uint8_t)(cyanR * alpha + bgR * (1.0f - alpha));
    uint8_t greenComponent = (uint8_t)(cyanG * alpha + bgG * (1.0f - alpha));
    uint8_t blueComponent = (uint8_t)(cyanB * alpha + bgB * (1.0f - alpha));
    uint16_t chipColor = m_lcd.color565(redComponent, greenComponent, blueComponent);

    // Position centrale en bas de l'écran
    int centerX = m_state.screenW / 2;
    int centerY = m_state.screenH - 35;
    int chipWidth = 40;
    int chipHeight = 30;
    int pinLength = 12;
    int pinCount = 6; // Nombre de pins de chaque côté

    // Définir les points du microprocesseur dans l'ordre du tracé
    struct Point
    {
        int x, y;
        bool draw; // true = dessiner, false = déplacement stylo relevé
    };

    // Liste des segments à dessiner (forme de microprocesseur avec pins)
    std::vector<Point> chipPath;

    // Commencer en haut à gauche du chip
    int left = centerX - chipWidth / 2;
    int right = centerX + chipWidth / 2;
    int top = centerY - chipHeight / 2;
    int bottom = centerY + chipHeight / 2;

    // Pins gauches (de haut en bas) - dessiner uniquement les pins, pas les déplacements entre
    for (int i = 0; i < pinCount; i++)
    {
        int pinY = top + (chipHeight * i / (pinCount - 1));

        if (i > 0)
        {
            // Déplacement vers la prochaine pin (stylo relevé)
            chipPath.push_back({left - pinLength, pinY, false});
        }
        else
        {
            // Premier point (démarrage)
            chipPath.push_back({left - pinLength, pinY, true});
        }

        // Dessiner la pin
        chipPath.push_back({left, pinY, true});
    }

    // Déplacement vers le coin haut-gauche du chip (stylo relevé)
    chipPath.push_back({left, top, false});

    // Contour du chip (sens horaire depuis coin haut-gauche)
    chipPath.push_back({left, top, true});
    chipPath.push_back({right, top, true});
    chipPath.push_back({right, bottom, true});
    chipPath.push_back({left, bottom, true});
    chipPath.push_back({left, top, true});

    // Déplacement vers la première pin droite (stylo relevé)
    chipPath.push_back({right, top, false});

    // Pins droites (de haut en bas)
    for (int i = 0; i < pinCount; i++)
    {
        int pinY = top + (chipHeight * i / (pinCount - 1));

        if (i > 0)
        {
            // Déplacement vers la prochaine pin (stylo relevé)
            chipPath.push_back({right, pinY, false});
        }

        // Dessiner la pin
        chipPath.push_back({right + pinLength, pinY, true});
    }

    // Déplacement vers le marqueur central (stylo relevé)
    int markSize = 6;
    chipPath.push_back({centerX - markSize, centerY - markSize, false});

    // Petit carré au centre (marqueur du chip)
    chipPath.push_back({centerX - markSize, centerY - markSize, true});
    chipPath.push_back({centerX + markSize, centerY - markSize, true});
    chipPath.push_back({centerX + markSize, centerY + markSize, true});
    chipPath.push_back({centerX - markSize, centerY + markSize, true});
    chipPath.push_back({centerX - markSize, centerY - markSize, true});

    // Calculer la longueur totale du chemin
    int totalSegments = chipPath.size() - 1;

    // Déterminer combien de segments dessiner selon la progression
    float progress = m_state.chip_animation_progress;
    int segmentsToDraw = 0;
    float segmentProgress = 0.0f;

    // Phase de dessin (0.0 à 1.0)
    segmentsToDraw = (int)(progress * totalSegments);
    segmentProgress = (progress * totalSegments) - segmentsToDraw;

    // Dessiner les segments complets
    for (int i = 0; i < segmentsToDraw && i < totalSegments; i++)
    {
        // Dessiner uniquement si le flag draw est true
        if (chipPath[i + 1].draw)
        {
            spr.drawLine(chipPath[i].x, chipPath[i].y,
                         chipPath[i + 1].x, chipPath[i + 1].y, chipColor);
        }
    }

    // Dessiner le segment partiel (en cours de tracé) et le pointeur
    if (segmentsToDraw < totalSegments && progress < 1.0f)
    {
        int x1 = chipPath[segmentsToDraw].x;
        int y1 = chipPath[segmentsToDraw].y;
        int x2 = chipPath[segmentsToDraw + 1].x;
        int y2 = chipPath[segmentsToDraw + 1].y;

        int partialX = x1 + (int)((x2 - x1) * segmentProgress);
        int partialY = y1 + (int)((y2 - y1) * segmentProgress);

        // Dessiner le segment partiel seulement si draw est true
        if (chipPath[segmentsToDraw + 1].draw && segmentProgress > 0.0f)
        {
            spr.drawLine(x1, y1, partialX, partialY, chipColor);
        }
    }
}

void ViewBadge::renderNeonFullName(LGFX_Sprite &spr, const char *name1, const char *name2)
{
    int x = m_state.screenW / 2;
    int y1 = 70;
    int y2 = 105;

    std::string name2_upper(name2);
    for (char &c : name2_upper)
        c = toupper(c);

    spr.setTextDatum(TC_DATUM);
    spr.setFont(&Orbitron_Bold24pt7b);
    spr.setTextSize(0.8f);

    drawNeonText(spr, name1, x, y1, colPink);
    drawNeonText(spr, name2_upper.c_str(), x, y2, colPink);

    spr.setFont(nullptr); // Revenir à la police par défaut après usage
}

// Helper pour effet néon simple sur un texte
void ViewBadge::drawNeonText(LGFX_Sprite &spr, const char *text, int x, int y, uint16_t baseColor)
{
    // Appliquer le glitch si actif
    int glitch_x = m_state.glitch_active ? m_state.glitch_offset_x : 0;
    int glitch_y = m_state.glitch_active ? m_state.glitch_offset_y : 0;

    // Effet glitch intensif : corruption de buffer avec séparation RGB extrême
    if (m_state.glitch_active)
    {
        // Générer des offsets aléatoires pour chaque canal (réduits)
        int red_offset_x = ((esp_random() % 5) - 2);
        int red_offset_y = ((esp_random() % 3) - 1);
        int green_offset_x = ((esp_random() % 3) - 1);
        int green_offset_y = ((esp_random() % 3) - 1);
        int blue_offset_x = ((esp_random() % 5) - 2);
        int blue_offset_y = ((esp_random() % 3) - 1);

        // Canal ROUGE ultra-décalé (effet mémoire corrompue)
        uint16_t redChannel = m_lcd.color565(255, 0, 0);
        spr.setTextColor(redChannel);
        spr.drawString(text, x + red_offset_x - 2, y + red_offset_y);

        // Canal VERT (offset moyen, simule le bug du milieu)
        uint16_t greenChannel = m_lcd.color565(0, 255, 0);
        spr.setTextColor(greenChannel);
        spr.drawString(text, x + green_offset_x, y + green_offset_y);

        // Canal BLEU ultra-décalé (à l'opposé du rouge)
        uint16_t blueChannel = m_lcd.color565(0, 0, 255);
        spr.setTextColor(blueChannel);
        spr.drawString(text, x + blue_offset_x + 2, y + blue_offset_y);

        // Lignes horizontales "corrompues" (effet scan line bug)
        int scan_y = y + ((esp_random() % 3) - 1);
        uint16_t scanColor = m_lcd.color565(
            (esp_random() % 2) * 255,
            (esp_random() % 2) * 255,
            (esp_random() % 2) * 255);
        spr.setTextColor(scanColor);
        spr.drawString(text, x + ((esp_random() % 3) - 1), scan_y);

        // "Fantômes" multiples (effet de buffer overflow)
        for (int ghost = 0; ghost < 2; ghost++)
        {
            int ghost_x = ((esp_random() % 7) - 3);
            int ghost_y = ((esp_random() % 5) - 2);
            uint8_t ghost_alpha = (40 + (esp_random() % 60));
            uint16_t ghostColor = m_lcd.color565(ghost_alpha, ghost_alpha * 0.3, ghost_alpha * 0.8);
            spr.setTextColor(ghostColor);
            spr.drawString(text, x + ghost_x, y + ghost_y);
        }
    }

    // Effet néon simple : ombre/glow décalée
    // Déterminer la couleur de l'ombre
    uint16_t shadowColor;
    if (baseColor == colCyan)
    {
        // Magenta flashy pour ombre du cyan
        shadowColor = colPink;
    }
    else if (baseColor == colYellow)
    {
        // Rose flashy pour ombre du jaune
        shadowColor = colMagenta;
    }
    else if (baseColor == colPink)
    {
        // Cyan flashy pour ombre du pink
        shadowColor = colCyan;
    }
    else
    {
        // Magenta profond par défaut
        shadowColor = m_lcd.color565(180, 0, 255);
    }

    // Ombre/glow décalée
    spr.setTextColor(shadowColor);
    spr.drawString(text, x + glitch_x + 1, y + glitch_y + 1);

    // Texte principal avec intensité variable
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
    uint8_t baseB = (baseColor & 0x1F) * 8;

    // Effet glitch sur la ligne (séparation RGB)
    if (m_state.glitch_active)
    {
        uint16_t redChannel = m_lcd.color565(baseR * 0.5, 0, 0);
        spr.drawFastHLine(x1 + glitch_x - 2, y1 + glitch_y, x2 - x1, redChannel);

        uint16_t blueChannel = m_lcd.color565(0, 0, baseB * 0.5);
        spr.drawFastHLine(x1 + glitch_x + 2, y1 + glitch_y, x2 - x1, blueChannel);
    }

    // Effet néon simple : ombre/glow décalée
    uint16_t shadowColor = m_lcd.color565(255, 0, 150); // Magenta néon

    // Ombre décalée
    spr.drawFastHLine(x1 + glitch_x, y1 + glitch_y + 1, x2 - x1, shadowColor);

    // Ligne principale avec intensité variable
    spr.drawFastHLine(x1 + glitch_x, y1 + glitch_y, x2 - x1, baseColor);
}

// Affichage principal du badge
void ViewBadge::render(LGFX &display, LGFX_Sprite &spr)
{

    // Initialiser les particules si nécessaire
    initParticles();

    // Utiliser le delta time calculé globalement par DisplayManager
    float dt = m_state.dt;

    // Mise à jour des animations
    updateScanlineOffset();
    updateAnimations(dt);

    // Rendu en couches (de l'arrière vers l'avant)
    renderBackground(spr);        // Fond sombre
    renderScanlines(spr);         // Lignes de scan CRT
    renderParticles(spr);         // Particules flottantes
    renderGeometricElements(spr); // Éléments géométriques décoratifs
    renderMicroprocessor(spr);    // Animation du microprocesseur
    renderCorners(spr);           // Coins décoratifs
    renderHeader(spr);            // Titre "100% G2S"
    renderName(spr);              // Nom avec effet néon
    renderSeparator(spr);         // Ligne de séparation
    renderTeam(spr);              // Équipe
    renderLocationAndRole(spr);   // Ville et poste
    renderBorders(spr);           // Bordures pulsantes
}
