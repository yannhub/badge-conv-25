#include "display_manager.h"
#include "esp_log.h"
#include <cmath>
#include <cstdio>
#include "driver/gpio.h"

#define BUTTON_GPIO GPIO_NUM_0
#define LONG_PRESS_DURATION 2000

DisplayManager::DisplayManager(LGFX &lcd, AppState &state)
    : m_lcd(lcd), m_state(state), m_sprite(&lcd) {}

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

    m_state.screenW = m_lcd.width();
    m_state.screenH = m_lcd.height();
    m_sprite.setColorDepth(16);
    m_sprite.createSprite(m_state.screenW, m_state.screenH);
}

void DisplayManager::displayLoop()
{
    if (!shouldRenderFrame())
        return;

    // Gérer le bouton
    handleButton();

    int pixel_x = -1, pixel_y = -1;
    bool touchDetected = m_lcd.getTouch(&pixel_x, &pixel_y);
    if (touchDetected)
    {
        if (!m_wasTouched)
        {
            // Essayer de passer le touch à la vue courante
            bool touchHandled = false;
            if (!m_views.empty())
            {
                touchHandled = m_views[m_currentView]->handleTouch(pixel_x, pixel_y);
            }

            // Si la vue n'a pas géré le touch, changer de vue
            if (!touchHandled)
            {
                nextView();
            }

            m_wasTouched = true;
        }
    }
    else
    {
        m_wasTouched = false;
    }

    if (!m_views.empty())
    {
        m_views[m_currentView]->render(m_lcd, m_sprite);
    }

    // Attendre que les opérations SPI précédentes soient terminées
    m_lcd.waitDisplay();

    m_sprite.pushSprite(0, 0);

    // Yield pour laisser tourner les autres tâches (notamment IDLE pour le watchdog)
    vTaskDelay(1);
}

void DisplayManager::addView(std::unique_ptr<View> view)
{
    m_views.push_back(std::move(view));
}

void DisplayManager::nextView()
{
    if (m_views.empty())
        return;
    m_currentView = (m_currentView + 1) % m_views.size();
}

bool DisplayManager::shouldRenderFrame()
{
    static unsigned long lastFrame = 0;
    static bool firstFrame = true;

    unsigned long now = lgfx::v1::millis();

    // Initialiser lors de la première frame
    if (firstFrame)
    {
        lastFrame = now;
        m_state.dt = 0.016f; // Valeur par défaut pour la première frame
        m_state.t = now * 0.001f;
        firstFrame = false;
        return true;
    }

    if (now - lastFrame < 16)
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
            m_state.display_rotated = !m_state.display_rotated;

            // Appliquer la rotation
            if (m_state.display_rotated)
            {
                m_lcd.setRotation(2); // alim en bas
            }
            else
            {
                m_lcd.setRotation(0); // retablissement standard (alim en haut)
            }

            // Recréer le sprite avec les nouvelles dimensions si nécessaire
            m_state.screenW = m_lcd.width();
            m_state.screenH = m_lcd.height();
            m_sprite.deleteSprite();
            m_sprite.createSprite(m_state.screenW, m_state.screenH);

            ESP_LOGI("DisplayManager", "Rotation changée: %s", m_state.display_rotated ? "180°" : "0°");
        }

        m_state.button_pressed = false;
    }
}
