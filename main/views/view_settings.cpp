#include "view_badge.h" // Pour initColors et couleurs badge
#include "view_settings.h"
#include "retro_colors.h"
#include "button.h"
#include <esp_log.h>

ViewSettings::ViewSettings(LGFX &lcd, DisplayManager &displayManager)
    : View(true), m_lcd(lcd), m_displayManager(displayManager)
{
    // Init badge colors if needed
    initColors(lcd);

    // Slider brightness
    m_sliderX = 20;
    m_sliderY = 50;
    m_sliderW = 160;
    m_sliderH = 18;

    // Slider sleep brightness
    m_sliderSleepX = 20;
    m_sliderSleepY = 90;
    m_sliderSleepW = 160;
    m_sliderSleepH = 18;

    // Stepper awakeTime
    this->m_stepperAwakeX = 20;
    this->m_stepperAwakeY = 130;
    this->m_stepperAwakeW = 160;
    this->m_stepperAwakeH = 28;
    this->m_stepperBtnW = 28;
    this->m_stepperBtnH = 28;
}

// --- Slider générique ---
void ViewSettings::drawSlider(LGFX_Sprite &spr, int x, int y, int w, int h, int colorFill, int colorGlow, int colorLabel, int colorValue, float value, float min, float max, const char *valueFormat, const char *label, int valueInt)
{
    // Background
    spr.fillRoundRect(x, y, w, h, 6, colBackground);
    // Glow
    spr.drawRoundRect(x - 1, y - 1, w + 2, h + 2, 8, colorGlow);
    // Fill
    int fillW = (value - min) * (w - 8) / (max - min);
    spr.fillRoundRect(x + 4, y + 4, fillW, h - 8, 4, colorFill);
    // Knob
    int knobX = x + 4 + fillW;
    spr.fillCircle(knobX, y + h / 2, h / 2 - 2, colYellow);
    // Value (small font)
    spr.setTextColor(colorValue);
    spr.setFont(&fonts::Font2);
    spr.setTextSize(1.0);
    spr.setTextDatum(textdatum_t::middle_left);
    char buf[16];
    if (valueInt >= 0)
        snprintf(buf, sizeof(buf), valueFormat, valueInt);
    else
        snprintf(buf, sizeof(buf), valueFormat, value);
    spr.drawString(buf, x + w + 8, y + h / 2);
    // Label (small font)
    spr.setTextColor(colorLabel);
    spr.setFont(&fonts::Font2);
    spr.setTextSize(1.0);
    spr.setTextDatum(textdatum_t::bottom_left);
    spr.drawString(label, x, y - 4);
}

void ViewSettings::render(LGFX &display, LGFX_Sprite &spr)
{
    spr.fillScreen(colBackground);
    // Title (no accent)
    spr.setTextColor(colYellow);
    spr.setFont(&fonts::Font2);
    spr.setTextSize(1.3);
    spr.setTextDatum(textdatum_t::top_center);
    spr.drawString("Reglages", display.width() / 2, 8);

    // Slider luminosité active
    drawSlider(spr, m_sliderX, m_sliderY, m_sliderW, m_sliderH, colCyan, colCyan, colPink, colCyan, (float)Config::activeBrightness, 0.0f, 100.0f, "%d%%", "Luminosite", (int)Config::activeBrightness);

    // Slider luminosité veille
    drawSlider(spr, m_sliderSleepX, m_sliderSleepY, m_sliderSleepW, m_sliderSleepH, colMagenta, colMagenta, colPink, colMagenta, (float)Config::sleepBrightness, 0.0f, 100.0f, "%d%%", "Lum. veille", (int)Config::sleepBrightness);

    // Stepper veille auto
    int stepperX = this->m_stepperAwakeX;
    int stepperY = this->m_stepperAwakeY + 20;
    int btnW = this->m_stepperBtnW;
    int btnH = this->m_stepperBtnH;
    int valueW = 55;
    int valueH = btnH;
    // Label
    spr.setTextColor(colPink);
    spr.setFont(&fonts::Font2);
    spr.setTextSize(1.0);
    spr.setTextDatum(textdatum_t::bottom_left);
    spr.drawString("Veille auto.", stepperX, stepperY - 4);
    // Bouton -
    spr.fillRoundRect(stepperX, stepperY, btnW, btnH, 5, colMagenta);
    spr.setTextColor(colYellow);
    spr.setTextSize(1.0);
    spr.setTextDatum(textdatum_t::middle_center);
    spr.drawString("-", stepperX + btnW / 2, stepperY + btnH / 2);
    // Valeur
    spr.fillRoundRect(stepperX + btnW + 6, stepperY, valueW, valueH, 5, colBackground);
    spr.setTextColor(colMagenta);
    spr.setFont(&fonts::Font2);
    spr.setTextSize(1.0);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d min", (int)Config::awakeTime);
    spr.drawString(buf, stepperX + btnW + 6 + valueW / 2, stepperY + valueH / 2);
    // Bouton +
    spr.fillRoundRect(stepperX + btnW + 6 + valueW + 6, stepperY, btnW, btnH, 5, colMagenta);
    spr.setTextColor(colYellow);
    spr.setTextSize(1.0);
    spr.drawString("+", stepperX + btnW + 6 + valueW + 6 + btnW / 2, stepperY + btnH / 2);

    // Checkbox rotation écran
    int cbx = m_checkboxRotX;
    int cby = m_checkboxRotY;
    int cbsize = 20;
    spr.drawRect(cbx, cby, cbsize, cbsize, colCyan);
    if (Config::display_rotated)
    {
        // Draw checkmark
        spr.drawLine(cbx + 3, cby + cbsize / 2, cbx + cbsize / 2 - 1, cby + cbsize - 3, colCyan);
        spr.drawLine(cbx + cbsize / 2 - 1, cby + cbsize - 3, cbx + cbsize - 3, cby + 3, colCyan);
    }
    spr.setTextColor(colCyan);
    spr.setFont(&fonts::Font2);
    spr.setTextSize(1.0);
    spr.setTextDatum(textdatum_t::middle_left);
    spr.drawString("Tourner l'ecran 180", cbx + cbsize + 8, cby + cbsize / 2);

    // Réinitialiser la police à la fin du rendu
    spr.setFont(nullptr);
    spr.setTextSize(1.0);
}

void ViewSettings::updateBrightnessFromTouch(int x)
{
    int relX = x - (m_sliderX + 4);
    int range = m_sliderW - 8;
    int value = relX * 100 / range;
    if (value < 1)
        value = 1;
    if (value > 100)
        value = 100;
    m_displayManager.updateBrightness(value);
}

void ViewSettings::updateSleepBrightnessFromTouch(int x)
{
    int relX = x - (m_sliderSleepX + 4);
    int range = m_sliderSleepW - 8;
    int value = relX * 100 / range;
    if (value < 0)
        value = 0;
    if (value > 100)
        value = 100;
    Config::setSleepBrightness(value);
}

void ViewSettings::updateAwakeTimeStepper(bool increment)
{
    int value = (int)Config::awakeTime;

    // En dessous de 5 minutes : paliers de 1 minute
    // À partir de 5 minutes : paliers de 5 minutes
    if (value < 5)
    {
        if (increment)
            value += 1;
        else
            value -= 1;
    }
    else
    {
        if (increment)
            value += 5;
        else
        {
            // Si on décrémente depuis 5, on passe à 4
            if (value == 5)
                value = 4;
            else
                value -= 5;
        }
    }

    if (value < 1)
        value = 1;
    if (value > 60)
        value = 60;
    this->m_displayManager.updateAwakeTime(value);
}

bool ViewSettings::handleTouch(int x, int y)
{
    // Checkbox rotation
    int cbx = m_checkboxRotX;
    int cby = m_checkboxRotY + 20;
    int cbsize = 20;
    if (isRectanglePressed(cbx, cby, cbsize, cbsize, x, y))
    {
        toggleRotation();
        return true;
    }

    // Slider luminosité active
    if (isRectanglePressed(m_sliderX, m_sliderY, m_sliderW, m_sliderH, x, y))
    {
        updateBrightnessFromTouch(x);
        return true;
    }
    // Slider luminosité veille
    if (isRectanglePressed(m_sliderSleepX, m_sliderSleepY, m_sliderSleepW, m_sliderSleepH, x, y))
    {
        updateSleepBrightnessFromTouch(x);
        return true;
    }
    // Stepper veille auto
    int stepperX = this->m_stepperAwakeX;
    int stepperY = this->m_stepperAwakeY + 20;
    int btnW = this->m_stepperBtnW;
    int btnH = this->m_stepperBtnH;
    int valueW = 55;
    // Bouton -
    if (isRectanglePressed(stepperX, stepperY, btnW, btnH, x, y))
    {
        this->updateAwakeTimeStepper(false);
        return true;
    }
    // Bouton +
    int plusX = stepperX + btnW + 6 + valueW + 6;
    if (isRectanglePressed(plusX, stepperY, btnW, btnH, x, y))
    {
        this->updateAwakeTimeStepper(true);
        return true;
    }
    return false;
}

void ViewSettings::toggleRotation()
{
    // Inverser la rotation dans Config
    Config::setDisplayRotated(!Config::display_rotated);
    ESP_LOGI("ViewSettings", "Display rotation set to %d", Config::display_rotated);
    // Appliquer la rotation via DisplayManager
    m_displayManager.applyRotationFromConfig();
}
