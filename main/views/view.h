#ifndef VIEW_H
#define VIEW_H

#include "../lgfx_custom.h"

class View
{
public:
    View(bool needsRedraw = true) : m_needsRedraw(needsRedraw), m_hasInitialRender(false) {}
    virtual ~View() = default;
    virtual void render(LGFX &display, LGFX_Sprite &spr) = 0;

    // Indique si la vue doit être rendue à chaque frame (dynamique) ou une seule fois (statique)
    virtual bool needsRedraw() const { return m_needsRedraw; }
    // Permet de forcer le redraw d'une vue statique si besoin
    virtual void forceRedraw()
    {
        m_needsRedraw = true;
        m_hasInitialRender = false;
    }

    // Pour la gestion du rendu initial
    bool hasInitialRender() const { return m_hasInitialRender; }
    void setInitialRender(bool v)
    {
        m_hasInitialRender = v;
    }

    // Méthode virtuelle pour gérer les touches (optionnelle)
    // Retourne true si la vue a consommé le touch (ne pas changer de vue)
    virtual bool handleTouch(int x, int y) { return false; }
    bool m_needsRedraw;

protected:
    bool m_hasInitialRender;
};

#endif // VIEW_H
