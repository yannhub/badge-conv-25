#include "view_qrcode.h"
#include <string>
#include "user_info.h"
#include "qrcodegen.h"

// Affiche le QR code généré dans le buffer global g_qrcode
void ViewQRCode::render(LGFX &display, LGFX_Sprite &spr)
{
    // Palette rétro-futuriste (Synthwave/Cyberpunk)
    uint16_t bg_dark_alt = spr.color565(5, 0, 20);     // Bleu-violet très foncé
    uint16_t bg_dark = spr.color565(15, 0, 35);        // Violet profond plus riche
    uint16_t neon_cyan = spr.color565(0, 255, 255);    // Cyan néon
    uint16_t neon_magenta = spr.color565(255, 0, 150); // Magenta néon
    uint16_t neon_purple = spr.color565(180, 0, 255);  // Violet néon
    uint16_t neon_yellow = spr.color565(255, 255, 0);  // Jaune néon

    // Fond rétro-futuriste (violet foncé)
    spr.fillRect(0, 0, spr.width(), spr.height(), bg_dark);

    // === SECTION HAUT (18%) ===
    // En-tête "CONVENTION G2S" avec effet néon
    spr.setTextDatum(TC_DATUM);
    spr.setTextFont(2);
    spr.setTextSize(2);

    // Effet ombre/glow
    spr.setTextColor(neon_magenta);
    spr.drawString("CONVENTION G2S", spr.width() / 2 + 1, 9);
    spr.setTextColor(neon_cyan);
    spr.drawString("CONVENTION G2S", spr.width() / 2, 8);

    // Date en jaune néon
    spr.setTextSize(1);
    spr.setTextColor(neon_yellow);
    spr.drawString("Mardi 25 Novembre 2025", spr.width() / 2, 38);

    // Ligne séparatrice néon avec effet dégradé
    int line_y = 58;
    for (int x = 15; x < spr.width() - 15; x++)
    {
        uint16_t line_color = (x % 4 < 2) ? neon_cyan : neon_magenta;
        spr.drawPixel(x, line_y, line_color);
        spr.drawPixel(x, line_y + 1, line_color);
    }

    // === SECTION QR CODE CENTRALE (45%) ===
    int scale = 5; // Taille d'un module (pixel) du QR code
    int qr_size = g_qrcode_size;
    int qr_pix_size = qr_size * scale;
    int x0 = (spr.width() - qr_pix_size) / 2;
    int y0 = 76;

    // Cadre néon multiple autour du QR code (effet rétro)
    spr.drawRect(x0 - 10, y0 - 10, qr_pix_size + 20, qr_pix_size + 20, neon_purple);
    spr.drawRect(x0 - 9, y0 - 9, qr_pix_size + 18, qr_pix_size + 18, neon_cyan);
    spr.drawRect(x0 - 8, y0 - 8, qr_pix_size + 16, qr_pix_size + 16, neon_magenta);

    spr.fillRect(x0 - 7, y0 - 7, qr_pix_size + 14, qr_pix_size + 14, TFT_WHITE);

    // Affichage du QR code avec effet néon
    for (int y = 0; y < qr_size; y++)
    {
        for (int x = 0; x < qr_size; x++)
        {
            if (qrcodegen_getModule(g_qrcode, x, y))
            {
                spr.fillRect(x0 + x * scale, y0 + y * scale, scale, scale, bg_dark_alt);
            }
        }
    }

    // Couleur de fond pour la partie basse du badge
    spr.fillRect(0, y0 + qr_pix_size + 18, spr.width(), spr.height() - (y0 + qr_pix_size + 18), bg_dark_alt);

    // Label "ACCESS BADGE" sous le QR code
    spr.setTextSize(2);
    spr.setTextColor(neon_magenta);
    spr.drawString("ACCESS BADGE", spr.width() / 2 + 1, y0 + qr_pix_size + 22);
    spr.setTextColor(neon_yellow);
    spr.drawString("ACCESS BADGE", spr.width() / 2, y0 + qr_pix_size + 21);

    // === SECTION BAS - LIEUX ===
    int bottom_start = y0 + qr_pix_size + 54;

    // Ligne séparatrice avec effet pixel
    for (int x = 20; x < spr.width() - 20; x++)
    {
        if (x % 3 == 0)
        {
            spr.drawPixel(x, bottom_start, neon_purple);
            spr.drawPixel(x, bottom_start + 1, neon_cyan);
        }
    }

    // Nom en majuscules style rétro
    spr.setTextSize(1);
    spr.setTextColor(TFT_WHITE);
    std::string nom_complet = user_info.prenom + std::string(" ") + user_info.nom;
    for (auto &c : nom_complet) {
        if (c == '\x7f') c = 'C'; // cheat : \x7f added to display c cedilla
        else c = toupper(c);
    }
    spr.drawString(nom_complet.c_str(), spr.width() / 2, bottom_start + 10);

    // Ligne de séparation fine
    spr.drawFastHLine(30, bottom_start + 32, spr.width() - 60, neon_magenta);

    // Lieu 1 avec icône
    spr.setTextColor(neon_yellow);
    spr.drawString(">> Maison de la Mutualite", spr.width() / 2, bottom_start + 42);

    // Lieu 2
    spr.setTextColor(neon_purple);
    spr.drawString(">> Theatre Mogador", spr.width() / 2, bottom_start + 60);

    // Bordures néon multiples (effet glitch)
    spr.drawRect(0, 0, spr.width(), spr.height(), neon_cyan);
    spr.drawRect(1, 1, spr.width() - 2, spr.height() - 2, neon_magenta);
    spr.drawRect(2, 2, spr.width() - 4, spr.height() - 4, neon_cyan);

    // Coins décoratifs néon (style cyber)
    // Coin haut gauche
    spr.drawFastHLine(4, 4, 15, neon_cyan);
    spr.drawFastVLine(4, 4, 15, neon_cyan);
    // Coin haut droit
    spr.drawFastHLine(spr.width() - 19, 4, 15, neon_magenta);
    spr.drawFastVLine(spr.width() - 5, 4, 15, neon_magenta);
    // Coin bas gauche
    spr.drawFastHLine(4, spr.height() - 5, 15, neon_purple);
    spr.drawFastVLine(4, spr.height() - 19, 15, neon_purple);
    // Coin bas droit
    spr.drawFastHLine(spr.width() - 19, spr.height() - 5, 15, neon_yellow);
    spr.drawFastVLine(spr.width() - 5, spr.height() - 19, 15, neon_yellow);
}
