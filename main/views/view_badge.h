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
    void updateNeonIntensity();
    void updateScanlineOffset();
    void renderBackground(LGFX_Sprite &spr);
    void renderHeader(LGFX_Sprite &spr);
    void renderName(LGFX_Sprite &spr);
    void renderSeparator(LGFX_Sprite &spr);
    void renderTeam(LGFX_Sprite &spr);
    void renderLocationAndRole(LGFX_Sprite &spr);
    void renderScanlines(LGFX_Sprite &spr);
    void renderNeonText(LGFX_Sprite &spr, const char *txt1, const char *txt2, int x, int y1, int y2);
    void renderNeonFullName(LGFX_Sprite &spr, const char *name1, const char *name2);

    AppState &m_state;
    LGFX &m_lcd;
};

#endif // VIEW_BADGE_H
