#include "view_program.h"
#include <cstring>

// Couleurs
static uint16_t colBackground = 0;
static uint16_t colCyan = 0;
static uint16_t colYellow = 0;
static uint16_t colPink = 0;
static uint16_t colMint = 0;
static uint16_t colOrange = 0;
static uint16_t colTimeline = 0;

static void initColors(LGFX &display)
{
    if (colBackground == 0)
    {
        colBackground = display.color565(5, 0, 15);
        colCyan = display.color565(0, 255, 255);       // Cyan néon
        colYellow = display.color565(255, 255, 0);     // Jaune vif
        colPink = display.color565(255, 20, 220);      // Rose néon
        colMint = display.color565(150, 255, 200);     // Vert menthe
        colOrange = display.color565(255, 165, 0);     // Orange vif
        colTimeline = display.color565(100, 150, 200); // Bleu pour timeline
    }
}

ViewProgram::ViewProgram(AppState &state, LGFX &lcd)
    : View(false), m_state(state), m_lcd(lcd)
{
    initColors(lcd);
}

void ViewProgram::renderProgramItem(LGFX_Sprite &spr, const char *time, const char *title, int y, uint16_t color, bool isLast)
{
    // Timeline verticale
    if (!isLast)
    {
        spr.drawLine(28, y + 10, 28, y + 38, colTimeline);
    }

    // Marqueur coloré selon le type d'événement
    spr.fillRect(26, y + 8, 5, 5, color);
    spr.drawRect(25, y + 7, 7, 7, color);

    // Heure à gauche en cyan néon
    spr.setTextDatum(textdatum_t::middle_left);
    spr.setTextColor(colCyan);
    spr.setFont(&fonts::Font2);
    spr.drawString(time, 38, y + 10);

    // Titre à droite de l'horaire avec la couleur de catégorie
    spr.setTextDatum(textdatum_t::middle_left);
    spr.setTextColor(color);
    spr.setFont(&fonts::Font2);
    spr.drawString(title, 90, y + 10);
}

void ViewProgram::render(LGFX &display, LGFX_Sprite &spr)
{
    spr.fillScreen(colBackground);

    // Bordures décoratives retrofuturiste
    spr.drawRect(2, 2, spr.width() - 4, spr.height() - 4, colCyan);
    spr.drawRect(3, 3, spr.width() - 6, spr.height() - 6, colPink);

    // Titre centré avec effet néon
    spr.setTextSize(1);
    spr.setTextDatum(textdatum_t::top_center);
    spr.setFont(&fonts::Font4);
    spr.setTextColor(colPink);
    spr.drawString("PROGRAMME", spr.width() / 2, 12);
    spr.setTextColor(spr.color565(255, 150, 230));
    spr.drawString("PROGRAMME", spr.width() / 2 + 1, 13);

    // Ligne séparatrice néon
    for (int i = 0; i < 3; i++)
    {
        spr.drawFastHLine(15, 40 + i, spr.width() - 30, colCyan);
    }

    int y = 53;
    int spacing = 28;

    // Programme de la journée avec code couleur cohérent:
    // Cyan = Ouvertures/Entrées | Jaune = Repas | Orange = Conférences | Rose = Spectacles

    renderProgramItem(spr, "12h00", "Ouverture Mutualite", y, colCyan);
    y += spacing;

    renderProgramItem(spr, "12h00", "Buffet dejeunatoire", y, colYellow);
    y += spacing;

    renderProgramItem(spr, "14h00", "Entree en salle", y, colCyan);
    y += spacing;

    renderProgramItem(spr, "14h30", "Pleniere", y, colOrange);
    y += spacing;

    renderProgramItem(spr, "16h30", "Cafe de cloture", y, colYellow);
    y += spacing;

    renderProgramItem(spr, "17h30", "Quartier libre", y, colCyan);
    y += spacing;

    renderProgramItem(spr, "19h00", "Ouverture Mogador", y, colCyan);
    y += spacing;

    renderProgramItem(spr, "20h00", "Spectacle musical", y, colPink);
    y += spacing;

    renderProgramItem(spr, "21h30", "Cocktail dinatoire", y, colYellow, true);
}
