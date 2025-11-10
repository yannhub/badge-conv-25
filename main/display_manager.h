#pragma once

#include "lgfx_custom.h"
#include "state.h"

class DisplayManager
{
public:
    DisplayManager(LGFX &lcd, AppState &state);
    void init();
    void displayLoop();

private:
    void drawScanlines();
    void drawNameWithGlow(const char *name1, const char *name2);
    void drawLikeCounter();
    LGFX &m_lcd;
    AppState &m_state;
    LGFX_Sprite spr;
    uint16_t colBackground, colNeonViolet, colCyan, colYellow, colHeart;
    void drawNeonText(const char *txt1, const char *txt2, int x, int y1, int y2);
    void updateNeonFlicker();
    void drawStaticElements();
    bool shouldRenderFrame();
    void updateScanlineOffset();
    void updateBrightness();
    void renderBackground();
    void renderHeader();
    void renderHeart();
    void renderName();
    void renderSeparator();
    void renderTeam();
    void renderLocationAndRole();
};
