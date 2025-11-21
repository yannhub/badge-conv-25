#include <cstdint>

#include "display_manager.h"
#include "esp_log.h"
#include <cmath>
#include <cstdio>
#include "driver/gpio.h"
#include <map>
#include "config.h"

#define BUTTON_GPIO GPIO_NUM_0
#define LONG_PRESS_DURATION 2000

// Met à jour la luminosité et applique immédiatement
void DisplayManager::updateBrightness(uint8_t value)
{
    Config::setActiveBrightness((uint8_t)value);
    setBacklight(value);
}

// Met à jour le temps d'éveil (minutes)
void DisplayManager::updateAwakeTime(float minutes)
{
    if (minutes < 0.1f)
        minutes = 0.1f;
    Config::setAwakeTime(minutes);
}

DisplayManager::DisplayManager(LGFX &lcd, AppState &state)
    : m_lcd(lcd), m_state(state), m_sprite(&lcd)
{
    m_lastActivity = lgfx::v1::millis();
}

void DisplayManager::init()
{
    // Configuration du bouton GPIO 0 (BOOT)
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << BUTTON_GPIO);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    setBacklight(Config::activeBrightness);
    applyRotationFromConfig();

    m_state.screenW = m_lcd.width();
    m_state.screenH = m_lcd.height();
    m_sprite.setColorDepth(16);
    m_sprite.createSprite(m_state.screenW, m_state.screenH);
}

void DisplayManager::displayLoop()
{
    if (!shouldRenderFrame())
        return;

    unsigned long now = lgfx::v1::millis();
    bool activity = false;

    // Gérer le bouton
    handleButton();

    int pixel_x = -1, pixel_y = -1;
    int raw_x = -1, raw_y = -1;
    m_lcd.getTouchRaw(&raw_x, &raw_y);
    bool touchDetected = m_lcd.getTouch(&pixel_x, &pixel_y);
    if (touchDetected)
    {
        activity = true;
        if (!m_wasTouched)
        {
            ESP_LOGI("DisplayManager", "Touch detected at (%d, %d)", pixel_x, pixel_y);
            // Début du touch - sauvegarder les coordonnées
            m_touchX = pixel_x;
            m_touchY = pixel_y;
            m_touchStartTime = now;
            m_longPressTriggered = false;
            m_wasTouched = true;
            
            if (m_sleepMode)
            {
                // Quitter le mode veille sans changer de vue
                m_sleepMode = false;
                setBacklight(Config::activeBrightness);
            }
        }
        else
        {
            // Touch maintenu - vérifier pour appui long
            unsigned long touchDuration = now - m_touchStartTime;
            if (!m_longPressTriggered && touchDuration > 1000 && m_settings_view)
            {
                // Appui long détecté (> 1 seconde)
                m_longPressTriggered = true;
                if (m_currentView != m_settings_view.get())
                {
                    ESP_LOGI("DisplayManager", "Long press detected - opening settings");
                    // Aller aux réglages
                    m_currentView = m_settings_view.get();
                    m_currentView->setInitialRender(false);
                }
            }
        }
    }
    else
    {
        // Touch relâché
        if (m_wasTouched && !m_longPressTriggered && !m_sleepMode)
        {
            // C'était un clic court - traiter normalement
            unsigned long touchDuration = now - m_touchStartTime;
            if (touchDuration < 1000)
            {
                // Essayer de passer le touch à la vue courante
                bool touchHandled = false;
                if (m_currentView != nullptr)
                {
                    int touch_x = m_touchX;
                    int touch_y = m_touchY;
                    if (Config::display_rotated)
                    {
                        // Ajuster les coordonnées touchées si l'écran est en rotation 180°
                        touch_y = touch_y + 20;
                    }
                    ESP_LOGI("DisplayManager", "Passing touch at (%d, %d) to current view", touch_x, touch_y);
                    touchHandled = m_currentView->handleTouch(touch_x, touch_y);
                }
                // Si la vue n'a pas géré le touch, changer de vue
                if (!touchHandled)
                {
                    // si clic à gauche de l'écran, vue précédente, sinon suivante
                    if (m_currentView == m_settings_view.get())
                        nextView(0); // Retour de la vue réglages à la vue précédente
                    else if (m_touchX < (m_state.screenW / 2))
                        nextView(-1);
                    else
                        nextView();
                }
            }
        }
        m_wasTouched = false;
        m_longPressTriggered = false;
    }

    // Sortie de veille si bouton pressé
    if (!m_sleepMode && activity)
        m_lastActivity = now;

    // Entrée en veille après config.awakeTime minutes d'inactivité
    unsigned long awakeTimeMs = (unsigned long)(Config::awakeTime * 60.0f * 1000.0f);
    if (!m_sleepMode && (now - m_lastActivity > awakeTimeMs))
    {
        m_sleepMode = true;
        setBacklight(Config::sleepBrightness);
    }

    if (m_currentView != nullptr)
    {
        if (!m_currentView->needsRedraw() && m_currentView->hasInitialRender())
        {
            // Vue statique déjà rendue, ne rien faire
            vTaskDelay(1);
            return;
        }

        // Sinon, rendre la vue normalement
        m_currentView->render(m_lcd, m_sprite);
        m_currentView->setInitialRender(true);
        // Attendre que les opérations SPI précédentes soient terminées
        m_lcd.waitDisplay();
        m_sprite.pushSprite(0, 0);
        vTaskDelay(1);
    }
}
void DisplayManager::setBacklight(uint8_t percent)
{
    // Clamp percent entre 0 et 100
    if (percent > 100)
        percent = 100;
    // La méthode setBrightness attend une valeur entre 0 et 255
    uint8_t value = (percent * 255) / 100;
    m_lcd.setBrightness(value);
}

void DisplayManager::addView(std::unique_ptr<View> view)
{
    m_views.push_back(std::move(view));
    if (m_currentView == nullptr)
        m_currentView = m_views[0].get();
}

void DisplayManager::setSettingsView(std::unique_ptr<View> view)
{
    m_settings_view = std::move(view);
}

void DisplayManager::nextView(int direction)
{
    if (m_views.empty())
        return;
    m_currentViewIdx = (m_currentViewIdx + direction + m_views.size()) % m_views.size();
    m_currentView = m_views[m_currentViewIdx].get();
    m_currentView->setInitialRender(false);
}

bool DisplayManager::shouldRenderFrame()
{
    static unsigned long lastFrame = 0;
    static bool firstFrame = true;

    unsigned long now = lgfx::v1::millis();
    unsigned long frameDurationMs = 1000 / m_targetFps;

    // Initialiser lors de la première frame
    if (firstFrame)
    {
        lastFrame = now;
        m_state.dt = 1.0f / m_targetFps; // Valeur par défaut pour la première frame
        m_state.t = now * 0.001f;
        firstFrame = false;
        return true;
    }

    if ((now - lastFrame) < frameDurationMs)
    {
        vTaskDelay(pdMS_TO_TICKS(1));
        return false;
    }

    // Calculer le delta time
    m_state.dt = (now - lastFrame) * 0.001f; // Convertir en secondes
    if (m_state.dt > 0.1f)                   // Limiter pour éviter les sauts
        m_state.dt = 0.1f;

    lastFrame = now;
    m_state.t = now * 0.001f;
    return true;
}

void DisplayManager::applyRotationFromConfig()
{
    m_lcd.waitDisplay(); // S'assurer que le LCD est prêt

    ESP_LOGI("DisplayManager", "Applying rotation: %d", Config::display_rotated);

    if (Config::display_rotated)
    {
        m_lcd.setRotation(2);
    }
    else
    {
        m_lcd.setRotation(0);
    }
}

void DisplayManager::handleButton()
{
    // Lire l'état du bouton (GPIO 0 est LOW quand pressé)
    bool button_current = (gpio_get_level(BUTTON_GPIO) == 0);
    unsigned long now = lgfx::v1::millis();

    if (button_current && !m_state.button_pressed)
    {
        // Début de la pression
        m_state.button_pressed = true;
        m_state.button_press_start = now;
    }
    else if (!button_current && m_state.button_pressed)
    {
        // Fin de la pression
        unsigned long press_duration = now - m_state.button_press_start;

        if (press_duration >= LONG_PRESS_DURATION)
        {
            // Clic long détecté : inverser la rotation
            Config::setDisplayRotated(!Config::display_rotated);
            applyRotationFromConfig();
            ESP_LOGI("DisplayManager", "Rotation changée: %s", Config::display_rotated ? "180°" : "0°");
        }

        m_state.button_pressed = false;
    }
}
