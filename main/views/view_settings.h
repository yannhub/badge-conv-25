class DisplayManager;

#ifndef VIEW_SETTINGS_H
#define VIEW_SETTINGS_H

#include <cstdint>

#include "view.h"
#include "button.h"
#include "../lgfx_custom.h"
#include "../config.h"
#include "../display_manager.h"
#include "../Orbitron_Bold24pt7b.h"

// Couleurs badge rétrofuturistes (similaires à ViewBadge)
extern uint16_t colBackground;
extern uint16_t colCyan;
extern uint16_t colYellow;
extern uint16_t colPink;
extern uint16_t colMagenta;

class ViewSettings : public View
{
public:
    ViewSettings(LGFX &lcd, DisplayManager &displayManager);
    void render(LGFX &display, LGFX_Sprite &spr) override;
    bool handleTouch(int x, int y) override;

private:
    LGFX &m_lcd;
    DisplayManager &m_displayManager;
    // Slider brightness
    int m_sliderX, m_sliderY, m_sliderW, m_sliderH;
    // Slider sleep brightness
    int m_sliderSleepX, m_sliderSleepY, m_sliderSleepW, m_sliderSleepH;
    // Stepper awakeTime
    int m_stepperAwakeX, m_stepperAwakeY, m_stepperAwakeW, m_stepperAwakeH;
    int m_stepperBtnW, m_stepperBtnH;
    // Checkbox rotation
    int m_checkboxRotX = 20, m_checkboxRotY = 210, m_checkboxRotSize = 24;
    void drawSlider(LGFX_Sprite &spr, int x, int y, int w, int h, int colorFill, int colorGlow, int colorLabel, int colorValue, float value, float min, float max, const char *valueFormat, const char *label, int valueInt = -1);
    void updateBrightnessFromTouch(int x);
    void updateSleepBrightnessFromTouch(int x);
    void updateAwakeTimeStepper(bool increment);
    void toggleRotation();
};

#endif // VIEW_SETTINGS_H
