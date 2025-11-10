#include "display_manager.h"
#include <cmath>
#include <cstdio>

DisplayManager::DisplayManager(LGFX &lcd, AppState &state)
    : m_lcd(lcd), m_state(state), spr(&lcd) {}

void DisplayManager::init()
{
    m_state.screenW = m_lcd.width();
    m_state.screenH = m_lcd.height();
    colBackground = m_lcd.color565(8, 6, 20);
    colNeonViolet = m_lcd.color565(199, 120, 255);
    colCyan = m_lcd.color565(0, 224, 255);
    colYellow = m_lcd.color565(255, 195, 0);
    colHeart = m_lcd.color565(255, 40, 180);
    spr.setColorDepth(16);
    spr.createSprite(m_state.screenW, m_state.screenH);
}

void DisplayManager::displayLoop()
{
    if (!shouldRenderFrame())
        return;

    updateNeonFlicker();
    updateScanlineOffset();
    updateBrightness();
    renderBackground();
    renderHeader();
    renderHeart();
    renderName();
    renderSeparator();
    renderTeam();
    renderLocationAndRole();
    drawLikeCounter();

    spr.pushSprite(0, 0);

    vTaskDelay(pdMS_TO_TICKS(16));
}

bool DisplayManager::shouldRenderFrame()
{
    static unsigned long lastFrame = 0;
    unsigned long now = lgfx::v1::millis();
    if (now - lastFrame < 16)
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
        return false;
    }
    lastFrame = now;
    m_state.t = lgfx::v1::millis() * 0.001f;
    return true;
}

void DisplayManager::updateNeonFlicker()
{
    float flicker_fast = 0.75f + 0.35f * ((float)rand() / RAND_MAX) * ((float)rand() / RAND_MAX);
    if (m_state.neon_dim_frames <= 0 && ((float)rand() / RAND_MAX) < 0.016f)
    {
        m_state.neon_dim_frames = 6 + rand() % 10;
        float base_dim = 0.65f + 0.25f * ((float)rand() / RAND_MAX) * ((float)rand() / RAND_MAX);
        m_state.neon_dim_target = base_dim;
    }
    if (m_state.neon_dim_frames > 0)
    {
        float frame_dim = m_state.neon_dim_target + 0.08f * (((float)rand() / RAND_MAX) - 0.5f);
        flicker_fast *= frame_dim;
        m_state.neon_dim_frames--;
    }
    float smoothing = 0.72f;
    m_state.neon_flicker_smooth = smoothing * m_state.neon_flicker_smooth + (1.0f - smoothing) * flicker_fast;
    m_state.neon_flicker = m_state.neon_flicker_smooth;
}

void DisplayManager::updateScanlineOffset()
{
    m_state.scanline_offset = (m_state.scanline_offset + 1) % 10;
}

void DisplayManager::updateBrightness()
{
    int brightness = (int)(m_state.neon_flicker * 255.0f);
    brightness = brightness < 32 ? 32 : (brightness > 255 ? 255 : brightness);
    m_lcd.setBrightness(brightness);
}

void DisplayManager::renderBackground()
{
    spr.fillSprite(colBackground);
    drawScanlines();
}

void DisplayManager::renderHeader()
{
    spr.setTextDatum(TC_DATUM);
    spr.setTextFont(1);
    spr.setTextSize(2);
    spr.setTextColor(colCyan);
    spr.drawString("TAP TO LIKE", m_state.screenW / 2, 20);
}

void DisplayManager::renderHeart()
{
    spr.fillCircle(m_state.screenW / 2, 54, 16, colHeart);
}

void DisplayManager::renderName()
{
    drawNameWithGlow("YANN", "SERGENT");
}

void DisplayManager::renderSeparator()
{
    spr.fillRect(36, 180, m_state.screenW - 80, 3, colCyan);
}

void DisplayManager::renderTeam()
{
    spr.setTextDatum(TC_DATUM);
    spr.setTextFont(1);
    spr.setTextSize(2);
    spr.setTextColor(colCyan);
    spr.drawString("open|core", m_state.screenW / 2, 200);
}

void DisplayManager::renderLocationAndRole()
{
    spr.setTextDatum(TC_DATUM);
    spr.setTextFont(1);
    spr.setTextSize(2);
    spr.setTextColor(colYellow);
    spr.drawString("LYON", m_state.screenW / 2, 230);
    spr.drawString("DEV FRONTEND", m_state.screenW / 2, 260);
}

void DisplayManager::drawScanlines()
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

void DisplayManager::drawNeonText(const char *txt1, const char *txt2, int x, int y1, int y2)
{
    uint8_t baseR = 255, baseG = 60, baseB = 200;
    float flicker = m_state.neon_flicker;
    float organic = fmaxf(flicker, 0.32f);
    spr.setTextDatum(MC_DATUM);
    spr.setTextFont(4);
    int layers = (int)(m_state.neon_flicker * 12.0f);
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
    uint16_t brightColor = m_lcd.color565(
        baseR * (0.3f + 0.6f * organic),
        baseG * (0.3f + 0.6f * organic),
        baseB * (0.3f + 0.6f * organic));
    spr.setTextColor(brightColor);
    spr.drawString(txt1, x, y1);
    spr.drawString(txt2, x, y2);
}

void DisplayManager::drawNameWithGlow(const char *name1, const char *name2)
{
    int x = m_state.screenW / 2;
    int y1 = 110;
    int y2 = 145;
    drawNeonText(name1, name2, x, y1, y2);
}

void DisplayManager::drawLikeCounter()
{
    spr.setTextDatum(TR_DATUM);
    spr.setTextFont(1);
    spr.setTextSize(2);
    spr.setTextColor(colHeart);
    char buf[16];
    snprintf(buf, sizeof(buf), "\xe2\x99\xa5 %d", m_state.likes);
    spr.drawString(buf, m_state.screenW - 8, 8);
}
