#pragma once

#include "lgfx_custom.h"
#include "state.h"

#include <vector>
#include <memory>

#include "views/view.h"
#include "views/view_settings.h"
#include <cstdint>

class DisplayManager
{

public:
    DisplayManager(LGFX &lcd, AppState &state);
    void init();
    void displayLoop();
    void addView(std::unique_ptr<View> view);
    void setSettingsView(std::unique_ptr<View> view);
    // Met à jour la luminosité et applique immédiatement
    void updateBrightness(uint8_t value);
    // Met à jour le temps d'éveil (minutes)
    void updateAwakeTime(float minutes);
    // Applique la rotation selon Config::display_rotated
    void applyRotationFromConfig();

private:
    LGFX &m_lcd;
    AppState &m_state;
    std::vector<std::unique_ptr<View>> m_views;
    size_t m_currentViewIdx = 0;
    View* m_currentView = nullptr;
    std::unique_ptr<View> m_settings_view;
    bool m_wasTouched = false;
    unsigned long m_touchStartTime = 0;
    bool m_longPressTriggered = false;
    int m_touchX = -1;
    int m_touchY = -1;
    LGFX_Sprite m_sprite;
    int m_targetFps = 30;
    unsigned long m_lastActivity = 0;
    bool m_sleepMode = false;
    void nextView(int direction = 1);
    bool shouldRenderFrame();
    void handleButton();
    void setBacklight(uint8_t percent);
};
