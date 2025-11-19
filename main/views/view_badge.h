#ifndef VIEW_BADGE_H
#define VIEW_BADGE_H

#include "view.h"
#include "../user_info.h"
#include "../state.h"
#include "../lgfx_custom.h"

class ViewBadge : public View
{
public:
    ViewBadge(AppState &state, LGFX &lcd);
    void render(LGFX &display, LGFX_Sprite &spr) override;
    bool handleTouch(int x, int y) override;

    void initParticles();
    void updateAnimations(float dt);
    void updateIntensityPulse(float dt);
    void updateChipAnimation(float dt);
    void updateParticlesAnimation(float dt);
    void updateGlitchEffect(float dt);
    void updateScanlineOffset(float dt);
    void renderBackground(LGFX_Sprite &spr);
    void renderHeader(LGFX_Sprite &spr);
    void renderName(LGFX_Sprite &spr);
    void renderSeparator(LGFX_Sprite &spr);
    void renderTeam(LGFX_Sprite &spr);
    void renderLocationAndRole(LGFX_Sprite &spr);
    void renderScanlines(LGFX_Sprite &spr);
    void renderNeonFullName(LGFX_Sprite &spr, const char *name1, const char *name2);
    void renderCorners(LGFX_Sprite &spr, uint8_t intensity);
    void renderParticles(LGFX_Sprite &spr);
    void renderBorders(LGFX_Sprite &spr, uint8_t intensity);
    void renderGeometricElements(LGFX_Sprite &spr, uint8_t intensity);
    void renderCornerTriangles(LGFX_Sprite &spr, uint8_t intensity, uint16_t geomColor);
    void renderAnimatedLines(LGFX_Sprite &spr, uint8_t intensity, uint16_t geomColor);
    void renderMicroprocessor(LGFX_Sprite &spr);

    // Helper pour effets néon
    void drawNeonText(LGFX_Sprite &spr, const char *text, int x, int y, uint16_t baseColor);
    void drawNeonLine(LGFX_Sprite &spr, int x1, int y1, int x2, int y2, uint16_t baseColor);
    void drawTriangle(LGFX_Sprite &spr, int cornerX, int cornerY, bool pointRight, bool pointDown, int triSize, uint8_t intensity, uint16_t geomColor);

    AppState &m_state;
    LGFX &m_lcd;
};

#endif // VIEW_BADGE_H
