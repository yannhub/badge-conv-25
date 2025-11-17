#ifndef VIEW_QRCODE_H
#define VIEW_QRCODE_H

#include "view.h"
#include "../user_info.h"

class ViewQRCode : public View
{
public:
    ViewQRCode() : View(false) {}
    void render(LGFX &display, LGFX_Sprite &spr) override;
};

#endif // VIEW_QRCODE_H
