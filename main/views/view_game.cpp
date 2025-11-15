#include "view_game.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "esp_log.h"
#include <cmath>

static const char *TAG = "ViewGame";

ViewGame::ViewGame(AppState &state, LGFX &lcd)
    : m_state(state), m_lcd(lcd)
{
    // Initialiser les couleurs
    m_colBackground = lcd.color565(5, 0, 15);
    m_colCyan = lcd.color565(0, 255, 255);
    m_colGreen = lcd.color565(0, 255, 100);
    m_colYellow = lcd.color565(255, 220, 0);
    m_colRed = lcd.color565(255, 0, 0);
    m_colOrange = lcd.color565(255, 140, 0);
    m_colMagenta = lcd.color565(255, 0, 255);
    m_colWhite = lcd.color565(255, 255, 255);

    // Initialiser les boutons
    m_backButton = {m_state.screenW - 32, 5, 25, 25, "X"};
    m_playButton = {m_state.screenW / 2 - 40, m_state.screenH - 60, 80, 35, "JOUER"};
}

void ViewGame::init()
{
    if (m_initialized)
        return;

    ESP_LOGI(TAG, "Initializing Crop Defense game");

    // Mettre à jour la position du bouton retour selon les dimensions actuelles
    m_backButton.x = m_state.screenW - 32;
    m_backButton.y = 5;
    m_backButton.w = 25;
    m_backButton.h = 25;

    m_game_over = false;
    m_victory = false;
    m_score = 0;
    m_game_time = 0.0f;
    m_spawn_interval = 1400; // Ajusté pour difficulté équilibrée
    m_last_spawn = esp_timer_get_time() / 1000ULL;
    m_last_update = m_last_spawn;

    // Initialiser les cultures (disposition en grille 2x3, mieux centrée et regroupée)
    int crop_spacing_x = 55; // Espacement horizontal réduit
    int crop_spacing_y = 60; // Espacement vertical réduit

    // Calculer la largeur et hauteur totales de la grille
    int grid_width = 2 * crop_spacing_x;  // 2 colonnes
    int grid_height = 1 * crop_spacing_y; // 1 rangée d'espacement (2 rangées)

    // Centrer la grille dans l'écran
    int start_x = (m_state.screenW - grid_width) / 2;
    int start_y = 50 + (m_state.screenH - 50 - grid_height) / 2 - 20; // -20 pour remonter un peu

    for (int i = 0; i < MAX_CROPS; i++)
    {
        int row = i / 3;
        int col = i % 3;
        m_crops[i].x = start_x + col * crop_spacing_x;
        m_crops[i].y = start_y + row * crop_spacing_y;
        m_crops[i].health = 100;
        m_crops[i].infected = false;
        m_crops[i].pulse = (float)(esp_random() % 100) / 100.0f * 6.28f;
    }

    // Initialiser les menaces
    for (int i = 0; i < MAX_THREATS; i++)
    {
        m_threats[i].active = false;
    }

    // Initialiser les particules
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        m_particles[i].active = false;
    }

    m_initialized = true;
    ESP_LOGI(TAG, "Game initialized - Ready to play!");
}

void ViewGame::update(float dt)
{
    if (m_game_over)
    {
        // Ne rien faire pendant le game over
        return;
    }

    unsigned long now = esp_timer_get_time() / 1000ULL;
    m_last_update = now;

    m_game_time += dt;

    // Vérifier victoire (30 secondes)
    if (m_game_time >= VICTORY_TIME)
    {
        m_victory = true;
        m_game_over = true;
        m_game_over_time = now;
        ESP_LOGI(TAG, "Victory! Final score: %d", m_score);
        return;
    }

    // Augmenter la difficulté avec le temps
    if (m_spawn_interval > 500)
    {
        m_spawn_interval = 1400 - (unsigned long)(m_game_time * 15);
        if (m_spawn_interval < 500)
            m_spawn_interval = 500;
    }

    // Spawn des menaces
    if (now - m_last_spawn > m_spawn_interval)
    {
        spawnThreat();
        m_last_spawn = now;
    }

    // Mettre à jour les cultures
    int alive_crops = 0;
    int infected_crops = 0;
    for (int i = 0; i < MAX_CROPS; i++)
    {
        m_crops[i].pulse += dt * 3.0f;

        if (m_crops[i].infected && m_crops[i].health > 0)
        {
            m_crops[i].health -= (int)(dt * 15.0f); // Perte de santé
            if (m_crops[i].health <= 0)
            {
                m_crops[i].health = 0;
            }
        }

        if (m_crops[i].health > 0)
        {
            alive_crops++;
        }

        if (m_crops[i].infected)
        {
            infected_crops++;
        }
    }

    // Game over si toutes les cultures sont mortes
    if (alive_crops == 0)
    {
        m_game_over = true;
        m_victory = false;
        m_game_over_time = now;
        ESP_LOGI(TAG, "Game Over! All crops destroyed. Final score: %d", m_score);
        return;
    }

    // Game over si tous les tournesols sont infectés/blessés
    if (infected_crops == MAX_CROPS)
    {
        m_game_over = true;
        m_victory = false;
        m_game_over_time = now;
        ESP_LOGI(TAG, "Game Over! All crops infected. Final score: %d", m_score);
        return;
    }

    // Mettre à jour les menaces
    for (int i = 0; i < MAX_THREATS; i++)
    {
        if (!m_threats[i].active)
            continue;

        // Appliquer le pattern de mouvement
        if (m_threats[i].movement_pattern == 1) // Sinusoïdal
        {
            m_threats[i].phase += dt * 5.0f;
            float perpendicular_offset = sin(m_threats[i].phase) * 30.0f;
            // Calculer la direction perpendiculaire
            float dx = m_threats[i].target_x - m_threats[i].x;
            float dy = m_threats[i].target_y - m_threats[i].y;
            float len = sqrt(dx * dx + dy * dy);
            if (len > 0.1f)
            {
                float perp_x = -dy / len;
                float perp_y = dx / len;
                m_threats[i].x += (m_threats[i].vx + perp_x * perpendicular_offset) * dt;
                m_threats[i].y += (m_threats[i].vy + perp_y * perpendicular_offset) * dt;
            }
            else
            {
                m_threats[i].x += m_threats[i].vx * dt;
                m_threats[i].y += m_threats[i].vy * dt;
            }
        }
        else if (m_threats[i].movement_pattern == 2) // Circulaire/erratique
        {
            m_threats[i].phase += dt * 8.0f;
            float wobble_x = cos(m_threats[i].phase) * 15.0f;
            float wobble_y = sin(m_threats[i].phase * 1.3f) * 15.0f;
            m_threats[i].x += (m_threats[i].vx + wobble_x) * dt;
            m_threats[i].y += (m_threats[i].vy + wobble_y) * dt;
        }
        else // Direct (pattern 0)
        {
            m_threats[i].x += m_threats[i].vx * dt;
            m_threats[i].y += m_threats[i].vy * dt;
        }

        // Retirer si hors écran
        if (m_threats[i].x < -30 || m_threats[i].x > m_state.screenW + 30 ||
            m_threats[i].y < -30 || m_threats[i].y > m_state.screenH + 30)
        {
            m_threats[i].active = false;
        }
    }

    // Mettre à jour les particules
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (!m_particles[i].active)
            continue;

        m_particles[i].x += m_particles[i].vx * dt;
        m_particles[i].y += m_particles[i].vy * dt;
        m_particles[i].life -= dt;

        if (m_particles[i].life <= 0)
        {
            m_particles[i].active = false;
        }
    }

    // Vérifier les collisions
    checkCollisions();
}

void ViewGame::spawnThreat()
{
    // Parfois spawner 2 menaces d'un coup pour plus de difficulté (réduit à 15%)
    int spawn_count = (esp_random() % 100 < 15 && m_game_time > 10.0f) ? 2 : 1;

    for (int spawn = 0; spawn < spawn_count; spawn++)
    {
        // Trouver un slot libre
        int slot = -1;
        for (int i = 0; i < MAX_THREATS; i++)
        {
            if (!m_threats[i].active)
            {
                slot = i;
                break;
            }
        }

        if (slot == -1)
            continue; // Pas de slot disponible

        // Type de menace aléatoire
        m_threats[slot].type = esp_random() % 3;
        m_threats[slot].active = true;
        m_threats[slot].spawn_time = esp_timer_get_time() / 1000ULL;
        m_threats[slot].phase = (esp_random() % 100) / 100.0f * 6.28f;

        // Pattern de mouvement aléatoire (plus de variété après 10s)
        if (m_game_time > 10.0f)
        {
            m_threats[slot].movement_pattern = esp_random() % 3;
        }
        else
        {
            m_threats[slot].movement_pattern = (esp_random() % 100 < 70) ? 0 : 1;
        }

        // Position et vitesse selon le type
        int side = esp_random() % 4; // 0=haut, 1=droite, 2=bas, 3=gauche

        // Vitesse réduite pour meilleure jouabilité
        float speed_mult = 0.7f + (esp_random() % 20) / 100.0f;

        // Choisir une culture aléatoire comme cible
        int target_crop = esp_random() % MAX_CROPS;
        m_threats[slot].target_x = m_crops[target_crop].x;
        m_threats[slot].target_y = m_crops[target_crop].y;

        switch (side)
        {
        case 0: // Haut
            m_threats[slot].x = esp_random() % m_state.screenW;
            m_threats[slot].y = -10;
            // Viser vers une culture
            {
                float dx = m_threats[slot].target_x - m_threats[slot].x;
                float dy = m_threats[slot].target_y - m_threats[slot].y;
                float dist = sqrt(dx * dx + dy * dy);
                if (dist > 0)
                {
                    m_threats[slot].vx = (dx / dist) * 50 * speed_mult;
                    m_threats[slot].vy = (dy / dist) * 50 * speed_mult;
                }
            }
            break;
        case 1: // Droite
            m_threats[slot].x = m_state.screenW + 10;
            m_threats[slot].y = esp_random() % m_state.screenH;
            {
                float dx = m_threats[slot].target_x - m_threats[slot].x;
                float dy = m_threats[slot].target_y - m_threats[slot].y;
                float dist = sqrt(dx * dx + dy * dy);
                if (dist > 0)
                {
                    m_threats[slot].vx = (dx / dist) * 50 * speed_mult;
                    m_threats[slot].vy = (dy / dist) * 50 * speed_mult;
                }
            }
            break;
        case 2: // Bas
            m_threats[slot].x = esp_random() % m_state.screenW;
            m_threats[slot].y = m_state.screenH + 10;
            {
                float dx = m_threats[slot].target_x - m_threats[slot].x;
                float dy = m_threats[slot].target_y - m_threats[slot].y;
                float dist = sqrt(dx * dx + dy * dy);
                if (dist > 0)
                {
                    m_threats[slot].vx = (dx / dist) * 50 * speed_mult;
                    m_threats[slot].vy = (dy / dist) * 50 * speed_mult;
                }
            }
            break;
        case 3: // Gauche
            m_threats[slot].x = -10;
            m_threats[slot].y = esp_random() % m_state.screenH;
            {
                float dx = m_threats[slot].target_x - m_threats[slot].x;
                float dy = m_threats[slot].target_y - m_threats[slot].y;
                float dist = sqrt(dx * dx + dy * dy);
                if (dist > 0)
                {
                    m_threats[slot].vx = (dx / dist) * 50 * speed_mult;
                    m_threats[slot].vy = (dy / dist) * 50 * speed_mult;
                }
            }
            break;
        }

        m_threats[slot].size = 14 + (esp_random() % 6);
    }
}

void ViewGame::checkCollisions()
{
    // Collision menaces <-> cultures
    for (int t = 0; t < MAX_THREATS; t++)
    {
        if (!m_threats[t].active)
            continue;

        for (int c = 0; c < MAX_CROPS; c++)
        {
            if (m_crops[c].health <= 0)
                continue;

            float dx = m_threats[t].x - m_crops[c].x;
            float dy = m_threats[t].y - m_crops[c].y;
            float dist = sqrt(dx * dx + dy * dy);

            if (dist < (m_threats[t].size + 15))
            {
                m_crops[c].infected = true;
                m_threats[t].active = false;
                createExplosion(m_threats[t].x, m_threats[t].y, m_colRed);
            }
        }
    }
}

void ViewGame::handleTouchInternal(int touch_x, int touch_y)
{
    // Vérifier si on touche une menace
    for (int i = 0; i < MAX_THREATS; i++)
    {
        if (!m_threats[i].active)
            continue;

        float dx = touch_x - m_threats[i].x;
        float dy = touch_y - m_threats[i].y;
        float dist = sqrt(dx * dx + dy * dy);

        if (dist < (m_threats[i].size + 10))
        {
            // Menace détruite !
            m_threats[i].active = false;
            m_score += 10;
            createExplosion(m_threats[i].x, m_threats[i].y, m_colCyan);
            ESP_LOGI(TAG, "Threat destroyed! Score: %d", m_score);
        }
    }

    // Vérifier si on touche une culture infectée pour la soigner
    for (int i = 0; i < MAX_CROPS; i++)
    {
        if (!m_crops[i].infected)
            continue;

        float dx = touch_x - m_crops[i].x;
        float dy = touch_y - m_crops[i].y;
        float dist = sqrt(dx * dx + dy * dy);

        if (dist < 20)
        {
            m_crops[i].infected = false;
            m_crops[i].health = 100;
            m_score += 5;
            createExplosion(m_crops[i].x, m_crops[i].y, m_colGreen);
            ESP_LOGI(TAG, "Crop healed! Score: %d", m_score);
        }
    }
}

bool ViewGame::handleTouch(int x, int y)
{
    // Gestion de l'écran d'intro
    if (m_show_intro)
    {
        // Vérifier si on clique sur le bouton Jouer
        if (isButtonPressed(m_playButton, x, y))
        {
            ESP_LOGI(TAG, "Play button pressed - Starting game");
            m_show_intro = false;
            m_game_started = true;
            return true;
        }

        // Clic ailleurs = comportement par défaut (passer à la vue suivante)
        return false;
    }

    // Si le jeu est terminé, appui n'importe où pour recommencer (sauf bouton retour)
    if (m_game_over)
    {
        // Vérifier si on appuie sur le bouton retour
        if (isButtonPressed(m_backButton, x, y))
        {
            ESP_LOGI(TAG, "Back button pressed from game over");
            // Réinitialiser l'état du jeu
            m_initialized = false;
            m_show_intro = true;
            m_game_started = false;
            m_game_over = false;
            m_victory = false;
            m_score = 0;
            m_game_time = 0.0f;
            return false; // Ne pas consommer - permet de changer de vue
        }

        // Sinon, réinitialiser le jeu
        ESP_LOGI(TAG, "Restarting game...");
        m_initialized = false; // Forcer la réinitialisation
        m_show_intro = true;   // Réafficher l'intro
        m_game_started = false;
        init(); // Réinitialiser le jeu
        return true;
    }

    // Vérifier si on appuie sur le bouton retour pendant le jeu
    if (isButtonPressed(m_backButton, x, y))
    {
        ESP_LOGI(TAG, "Back button pressed");
        // Réinitialiser l'état du jeu
        m_initialized = false;
        m_show_intro = true;
        m_game_started = false;
        m_game_over = false;
        m_victory = false;
        m_score = 0;
        m_game_time = 0.0f;
        return false; // Ne pas consommer - permet de changer de vue
    }

    // Jeu en cours - traiter le touch pour tirer sur les menaces
    handleTouchInternal(x, y);
    return true;
}

bool ViewGame::isButtonPressed(const Button &btn, int touch_x, int touch_y)
{
    return touch_x >= btn.x && touch_x <= btn.x + btn.w &&
           touch_y >= btn.y && touch_y <= btn.y + btn.h;
}

void ViewGame::createExplosion(float x, float y, uint16_t color)
{
    int count = 0;
    for (int i = 0; i < MAX_PARTICLES && count < 8; i++)
    {
        if (m_particles[i].active)
            continue;

        float angle = (count / 8.0f) * 6.28f;
        float speed = 50 + (esp_random() % 50);

        m_particles[i].x = x;
        m_particles[i].y = y;
        m_particles[i].vx = cos(angle) * speed;
        m_particles[i].vy = sin(angle) * speed;
        m_particles[i].life = 0.5f + (esp_random() % 100) / 200.0f;
        m_particles[i].color = color;
        m_particles[i].active = true;

        count++;
    }
}

void ViewGame::render(LGFX &display, LGFX_Sprite &spr)
{
    if (!m_initialized)
    {
        init();
    }

    // Afficher l'écran d'intro si le jeu n'a pas démarré
    if (m_show_intro)
    {
        renderIntro(spr);
        return;
    }

    // Utiliser le delta time calculé globalement par DisplayManager
    update(m_state.dt);

    renderBackground(spr);
    renderCrops(spr);
    renderThreats(spr);
    renderParticles(spr);
    renderHUD(spr);

    if (m_game_over)
    {
        renderGameOver(spr);
    }
}

void ViewGame::renderBackground(LGFX_Sprite &spr)
{
    spr.fillScreen(m_colBackground);

    // Grille cyber en arrière-plan
    for (int y = 50; y < m_state.screenH; y += 20)
    {
        spr.drawLine(0, y, m_state.screenW, y, m_lcd.color565(0, 50, 50));
    }
    for (int x = 0; x < m_state.screenW; x += 20)
    {
        spr.drawLine(x, 50, x, m_state.screenH, m_lcd.color565(0, 50, 50));
    }
}

void ViewGame::renderCrops(LGFX_Sprite &spr)
{
    for (int i = 0; i < MAX_CROPS; i++)
    {
        if (m_crops[i].health <= 0)
            continue;

        int x = m_crops[i].x;
        int y = m_crops[i].y;

        // Pulsation
        float pulse = sin(m_crops[i].pulse) * 0.2f + 1.0f;
        int size = (int)(12 * pulse);

        uint16_t color;
        if (m_crops[i].infected)
        {
            // Rouge clignotant si infecté
            int blink = (int)(m_crops[i].pulse * 3) % 2;
            color = blink ? m_colRed : m_colOrange;
        }
        else
        {
            // Couleur selon santé
            if (m_crops[i].health > 60)
                color = m_colGreen;
            else if (m_crops[i].health > 30)
                color = m_colYellow;
            else
                color = m_colOrange;
        }

        // Dessiner le tournesol
        // Coeur central
        spr.fillCircle(x, y, size / 2, color);
        spr.drawCircle(x, y, size / 2, m_colCyan);

        // Pétales jaunes autour
        for (int p = 0; p < 8; p++)
        {
            float angle = (p / 8.0f) * 6.28f + m_crops[i].pulse * 0.1f; // Légère rotation
            int px = x + cos(angle) * (size * 0.6f);
            int py = y + sin(angle) * (size * 0.6f);
            spr.fillCircle(px, py, 3, m_colYellow);
            spr.drawCircle(px, py, 3, m_colOrange);
        }

        // Barre de santé
        int bar_w = 20;
        int bar_h = 3;
        int bar_x = x - bar_w / 2;
        int bar_y = y + size + 3;
        spr.drawRect(bar_x, bar_y, bar_w, bar_h, m_colWhite);
        int health_w = (m_crops[i].health * bar_w) / 100;
        spr.fillRect(bar_x, bar_y, health_w, bar_h, m_crops[i].health > 50 ? m_colGreen : m_colRed);
    }
}

void ViewGame::renderThreats(LGFX_Sprite &spr)
{
    unsigned long now = esp_timer_get_time() / 1000ULL;

    for (int i = 0; i < MAX_THREATS; i++)
    {
        if (!m_threats[i].active)
            continue;

        int x = (int)m_threats[i].x;
        int y = (int)m_threats[i].y;

        // Ne pas dessiner si au-dessus du header (y < 45)
        if (y < 45)
            continue;

        int size = 16; // Taille augmentée pour meilleure visibilité

        // Animation de pulsation légère
        float age = (now - m_threats[i].spawn_time) / 1000.0f;
        float pulse = sin(age * 8.0f) * 0.15f + 1.0f;
        size = (int)(size * pulse);

        // Style Space Invaders uniforme
        // Corps principal (carré/rectangle)
        spr.fillRect(x - size / 2, y - size / 2, size, size, m_colRed);
        spr.drawRect(x - size / 2, y - size / 2, size, size, m_colOrange);

        // "Yeux" ou détails
        spr.fillRect(x - size / 3, y - size / 4, 2, 2, m_colYellow);
        spr.fillRect(x + size / 4, y - size / 4, 2, 2, m_colYellow);

        // "Antennes" ou appendices
        spr.drawLine(x - size / 2, y - size / 2, x - size / 2 - 2, y - size / 2 - 3, m_colOrange);
        spr.drawLine(x + size / 2, y - size / 2, x + size / 2 + 2, y - size / 2 - 3, m_colOrange);

        // "Pattes" en bas (selon le type pour légère variation)
        if (m_threats[i].type == 0)
        {
            spr.drawLine(x - size / 2, y + size / 2, x - size / 2 - 2, y + size / 2 + 3, m_colRed);
            spr.drawLine(x, y + size / 2, x, y + size / 2 + 3, m_colRed);
            spr.drawLine(x + size / 2, y + size / 2, x + size / 2 + 2, y + size / 2 + 3, m_colRed);
        }
        else if (m_threats[i].type == 1)
        {
            spr.drawLine(x - size / 3, y + size / 2, x - size / 3 - 2, y + size / 2 + 2, m_colRed);
            spr.drawLine(x + size / 3, y + size / 2, x + size / 3 + 2, y + size / 2 + 2, m_colRed);
        }
        else
        {
            spr.fillRect(x - size / 4, y + size / 2, size / 2, 2, m_colRed);
        }
    }
}

void ViewGame::renderParticles(LGFX_Sprite &spr)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (!m_particles[i].active)
            continue;

        int x = (int)m_particles[i].x;
        int y = (int)m_particles[i].y;

        // Dessiner la particule (petit carré ou cercle)
        spr.fillCircle(x, y, 2, m_particles[i].color);
    }
}

void ViewGame::renderHUD(LGFX_Sprite &spr)
{
    // Titre du jeu (à gauche)
    spr.setTextColor(m_colCyan);
    spr.setTextSize(1);
    spr.setCursor(5, 5);
    spr.print("DEFENDER");

    // Logo Groupama centré (ligne du haut)
    int center_x = m_state.screenW / 2;
    spr.setTextColor(m_colGreen);
    spr.setCursor(center_x - 30, 5);
    spr.print("GROUPAMA");

    // Bouton retour (carré avec croix, en haut à droite)
    spr.drawRect(m_backButton.x, m_backButton.y, m_backButton.w, m_backButton.h, m_colWhite);
    // Dessiner une croix centrée dans le rectangle
    int cx = m_backButton.x + m_backButton.w / 2;
    int cy = m_backButton.y + m_backButton.h / 2;
    int cross_size = 8;
    spr.drawLine(cx - cross_size / 2, cy - cross_size / 2, cx + cross_size / 2, cy + cross_size / 2, m_colWhite);
    spr.drawLine(cx + cross_size / 2, cy - cross_size / 2, cx - cross_size / 2, cy + cross_size / 2, m_colWhite);

    // Score (en bas à gauche)
    spr.setTextColor(m_colYellow);
    spr.setCursor(5, 20);
    spr.printf("Score: %d", m_score);

    // Timer avec compte à rebours centré (ligne du bas)
    int remaining_time = (int)(VICTORY_TIME - m_game_time);
    if (remaining_time < 0)
        remaining_time = 0;

    uint16_t timer_color;
    if (remaining_time > 15)
        timer_color = m_colGreen;
    else if (remaining_time > 5)
        timer_color = m_colYellow;
    else
        timer_color = m_colRed;

    spr.setTextColor(timer_color);
    spr.setCursor(center_x - 18, 20);
    spr.printf("%02d:%02d", remaining_time / 60, remaining_time % 60);

    // Bordure top
    spr.drawLine(0, 38, m_state.screenW, 38, m_colCyan);
    spr.drawLine(0, 39, m_state.screenW, 39, m_lcd.color565(0, 150, 150));
}

void ViewGame::renderGameOver(LGFX_Sprite &spr)
{
    // Fond semi-transparent stable (sans pixels aléatoires qui clignotent)
    int box_x1 = 10;
    int box_y1 = 60;
    int box_x2 = m_state.screenW - 10;
    int box_y2 = m_state.screenH - 20;

    // Fond sombre uniforme - pas de boucle pour éviter les clignotements
    spr.fillRect(box_x1 + 2, box_y1 + 2, box_x2 - box_x1 - 4, box_y2 - box_y1 - 4, m_lcd.color565(10, 0, 25));

    // Double bordure pour effet cyber
    spr.drawRect(box_x1, box_y1, box_x2 - box_x1, box_y2 - box_y1, m_colCyan);
    spr.drawRect(box_x1 + 1, box_y1 + 1, box_x2 - box_x1 - 2, box_y2 - box_y1 - 2, m_colCyan);

    // Centrer le contenu
    int center_x = m_state.screenW / 2;

    spr.setTextDatum(TC_DATUM); // Top Center

    if (m_victory)
    {
        // === VICTOIRE! ===
        spr.setTextColor(m_colGreen);
        spr.setTextSize(2);
        spr.drawString("VICTOIRE!", center_x, 85);

        spr.setTextSize(1);
        spr.setTextColor(m_colCyan);
        spr.drawString("30 secondes!", center_x, 115);
    }
    else
    {
        // === GAME OVER ===
        spr.setTextColor(m_colRed);
        spr.setTextSize(2);
        spr.drawString("GAME OVER", center_x, 85);

        spr.setTextSize(1);
        spr.setTextColor(m_colOrange);
        spr.drawString("Cultures perdues", center_x, 115);
    }

    // Score final
    spr.setTextColor(m_colYellow);
    spr.setTextSize(1);
    char score_str[20];
    sprintf(score_str, "Score: %d", m_score);
    spr.drawString(score_str, center_x, 140);

    // Message Groupama
    spr.setTextColor(m_colGreen);
    spr.drawString("Groupama protege", center_x, 165);
    spr.drawString("vos cultures!", center_x, 180);

    // Instructions pour rejouer
    spr.setTextColor(m_colCyan);
    spr.drawString("Touchez pour rejouer", center_x, 205);

    spr.setTextDatum(TL_DATUM); // Revenir à Top Left par défaut
}

void ViewGame::renderIntro(LGFX_Sprite &spr)
{
    // Fond uni
    spr.fillScreen(m_colBackground);

    int center_x = m_state.screenW / 2;
    spr.setTextDatum(TC_DATUM); // Top Center

    // Titre
    spr.setTextColor(m_colCyan);
    spr.setTextSize(2);
    spr.drawString("CROP DEFENSE", center_x, 20);

    // Sous-titre
    spr.setTextSize(1);
    spr.setTextColor(m_colGreen);
    spr.drawString("Protegez vos cultures!", center_x, 50);

    // Instructions
    spr.setTextColor(m_colWhite);
    spr.drawString("COMMENT JOUER:", center_x, 80);

    spr.setTextColor(m_colYellow);
    spr.setTextSize(1);

    // Icône tournesol
    spr.fillCircle(20, 110, 6, m_colGreen);
    for (int p = 0; p < 6; p++)
    {
        float angle = (p / 6.0f) * 6.28f;
        int px = 20 + cos(angle) * 8;
        int py = 110 + sin(angle) * 8;
        spr.fillCircle(px, py, 3, m_colYellow);
    }
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(m_colWhite);
    spr.drawString("Cultures a proteger", 35, 105);

    // Icône ennemi
    spr.setTextDatum(TL_DATUM);
    spr.fillRect(15, 130, 12, 12, m_colRed);
    spr.drawRect(15, 130, 12, 12, m_colOrange);
    spr.drawString("Ennemis a eliminer", 35, 130);

    // Instructions tactiles
    spr.setTextDatum(TC_DATUM);
    spr.setTextColor(m_colCyan);
    spr.drawString("Touchez les ennemis", center_x, 160);
    spr.drawString("pour les eliminer", center_x, 175);

    spr.setTextColor(m_colOrange);
    spr.drawString("Soignez les cultures", center_x, 195);
    spr.drawString("infectees (rouges)", center_x, 210);

    // Objectif
    spr.setTextColor(m_colGreen);
    spr.drawString("Survivez 30 secondes!", center_x, 235);

    // Bouton Jouer
    spr.fillRect(m_playButton.x, m_playButton.y, m_playButton.w, m_playButton.h, m_colGreen);
    spr.drawRect(m_playButton.x, m_playButton.y, m_playButton.w, m_playButton.h, m_colCyan);
    spr.drawRect(m_playButton.x + 1, m_playButton.y + 1, m_playButton.w - 2, m_playButton.h - 2, m_colCyan);

    spr.setTextDatum(MC_DATUM); // Middle Center
    spr.setTextColor(m_colBackground);
    spr.setTextSize(2);
    spr.drawString("JOUER", center_x, m_playButton.y + m_playButton.h / 2);

    spr.setTextDatum(TL_DATUM); // Revenir à Top Left par défaut
}
