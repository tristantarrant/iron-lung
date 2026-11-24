/*
 * Iron Lung - scalable cairo UI (DPF).
 *
 * The pedal is drawn by Artwork.hpp in a fixed design space and scaled
 * to the window, keeping the aspect ratio. Interaction is mapped through
 * the artwork's control table:
 *   knobs        drag up/down (Shift: fine), mouse wheel, double-click
 *                resets to the default
 *   switch       click toggles 1986 / LATER
 *   footswitch   click toggles bypass
 *
 * Copyright (C) 2026 Dataforte
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DistrhoUI.hpp"
#include "Artwork.hpp"

#include <algorithm>

START_NAMESPACE_DISTRHO

using DGL_NAMESPACE::CairoGraphicsContext;

static_assert(static_cast<int>(il_art::P_COUNT) == static_cast<int>(kParamCount), "artwork and plugin parameter lists differ");

class UIIronLung : public UI
{
public:
    UIIronLung()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        const double s = getScaleFactor();
        if (s != 1.0)
            setSize(static_cast<uint>(il_art::W * s), static_cast<uint>(il_art::H * s));
        // resizable from half to any size, aspect ratio kept
        setGeometryConstraints(static_cast<uint>(il_art::W * 0.5 * s),
                               static_cast<uint>(il_art::H * 0.5 * s), true, false);

        for (int i = 0; i < il_art::P_COUNT; ++i)
            fDefaults[i] = fState.value[i];
    }

protected:
    // ---- host -> UI ------------------------------------------------
    void parameterChanged(const uint32_t index, const float value) override
    {
        if (index < static_cast<uint32_t>(il_art::P_COUNT)) {
            fState.value[index] = value;
            repaint();
        }
    }

    // ---- drawing ---------------------------------------------------
    void onCairoDisplay(const CairoGraphicsContext& context) override
    {
        cairo_t* const cr = context.handle;
        updateTransform();

        cairo_set_source_rgb(cr, 0.10, 0.10, 0.11);
        cairo_paint(cr);

        cairo_save(cr);
        cairo_translate(cr, fOffX, fOffY);
        cairo_scale(cr, fScale, fScale);
        il_art::draw(cr, fState);
        cairo_restore(cr);
    }

    // ---- interaction -----------------------------------------------
    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button != 1)
            return false;

        if (!ev.press) {
            if (fDragging >= 0) {
                editParameter(fDragging, false);
                fDragging = -1;
                return true;
            }
            return false;
        }

        const il_art::Control* c = hit(ev.pos.getX(), ev.pos.getY());
        if (c == nullptr)
            return false;

        if (c->kind == il_art::kKnob) {
            const bool dbl = c->param == fLastClickParam && ev.time - fLastClickTime < 350;
            fLastClickParam = c->param;
            fLastClickTime = ev.time;
            if (dbl) {
                setValue(c->param, fDefaults[c->param], true);
                return true;
            }
            fDragging = c->param;
            fDragY = ev.pos.getY();
            fDragValue = fState.value[c->param];
            editParameter(c->param, true);
            return true;
        }

        // switch and footswitch
        setValue(c->param, fState.value[c->param] > 0.5f ? 0.0f : 1.0f, true);
        return true;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        if (fDragging < 0)
            return false;
        const il_art::Control* c = control(fDragging);
        const double span = c->max - c->min;
        // full travel over 200 design units (Shift: 10x finer)
        double pixelsPerTravel = 200.0 * fScale;
        if (ev.mod & kModifierShift)
            pixelsPerTravel *= 10.0;
        const double dv = (fDragY - ev.pos.getY()) / pixelsPerTravel * span;
        setValue(fDragging, static_cast<float>(fDragValue + dv), false);
        return true;
    }

    bool onScroll(const ScrollEvent& ev) override
    {
        const il_art::Control* c = hit(ev.pos.getX(), ev.pos.getY());
        if (c == nullptr || c->kind != il_art::kKnob)
            return false;
        const double step = (c->max - c->min) / ((ev.mod & kModifierShift) ? 500.0 : 50.0);
        setValue(c->param, static_cast<float>(fState.value[c->param] + ev.delta.getY() * step), true);
        return true;
    }

private:
    il_art::State fState;
    float fDefaults[il_art::P_COUNT];
    double fScale = 1.0, fOffX = 0.0, fOffY = 0.0;
    int fDragging = -1;
    double fDragY = 0.0;
    double fDragValue = 0.0;
    int fLastClickParam = -1;
    uint fLastClickTime = 0;

    void updateTransform()
    {
        const double w = getWidth(), h = getHeight();
        fScale = std::min(w / il_art::W, h / il_art::H);
        fOffX = (w - il_art::W * fScale) / 2.0;
        fOffY = (h - il_art::H * fScale) / 2.0;
    }

    static const il_art::Control* control(const int param)
    {
        for (const il_art::Control& c : il_art::kControls)
            if (c.param == param)
                return &c;
        return nullptr;
    }

    const il_art::Control* hit(const double px, const double py)
    {
        updateTransform();
        const double x = (px - fOffX) / fScale;
        const double y = (py - fOffY) / fScale;
        for (const il_art::Control& c : il_art::kControls) {
            if (c.kind == il_art::kKnob) {
                const double dx = x - c.x, dy = y - c.y;
                if (dx * dx + dy * dy <= c.r * c.r)
                    return &c;
            } else {
                if (std::abs(x - c.x) <= c.w / 2 && std::abs(y - c.y) <= c.h / 2)
                    return &c;
            }
        }
        return nullptr;
    }

    // set locally, notify the host (wrapping in a gesture when needed)
    void setValue(const int param, float value, const bool gesture)
    {
        const il_art::Control* c = control(param);
        value = std::max(static_cast<float>(c->min), std::min(static_cast<float>(c->max), value));
        if (c->kind != il_art::kKnob)
            value = value > 0.5f ? 1.0f : 0.0f;
        if (gesture)
            editParameter(param, true);
        fState.value[param] = value;
        setParameterValue(param, value);
        if (gesture)
            editParameter(param, false);
        repaint();
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIIronLung)
};

UI* createUI()
{
    return new UIIronLung();
}

END_NAMESPACE_DISTRHO
