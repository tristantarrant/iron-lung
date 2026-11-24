/*
 * Iron Lung - pedal artwork, drawn with cairo.
 *
 * Shared by the scalable plugin UI (UIIronLung.cpp) and by the modgui
 * renderer (tools/render_modgui.cpp), so the desktop GUI and the
 * modgui look the same. Everything is drawn in a fixed design
 * space of W x H units; callers scale the cairo context.
 *
 * The layout recalls the 1986 compact compressor: blue die-cast
 * enclosure, black control panel with the CHECK lamp and four cream
 * knobs, white model lettering on the blue body, and a black tread
 * plate that doubles as the footswitch. Names and branding are our own;
 * the VCA selector comes from the later production part.
 *
 * Copyright (C) 2026 Dataforte
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cairo.h>
#include <cmath>
#include <cstring>

#include "BrandPath.hpp"
#include "MakerPath.hpp"

namespace il_art {

constexpr double W = 340.0;
constexpr double H = 540.0;

// ---- palette ---------------------------------------------------------
struct RGB { double r, g, b; };
constexpr RGB kBodyTop     { 0.208, 0.478, 0.706 };   // enclosure, lit
constexpr RGB kBodyBottom  { 0.118, 0.318, 0.514 };
constexpr RGB kPanelTop    { 0.075, 0.078, 0.090 };
constexpr RGB kPanelBottom { 0.035, 0.037, 0.045 };
constexpr RGB kInk         { 0.925, 0.933, 0.941 };   // white print
constexpr RGB kSoft        { 0.66, 0.69, 0.73 };
constexpr RGB kCream       { 0.902, 0.875, 0.812 };

// ---- control layout --------------------------------------------------
enum Kind { kKnob, kToggle, kFootswitch };

struct Control {
    int    param;      // DPF parameter index
    Kind   kind;
    double x, y;       // centre
    double r;          // hit radius (knobs)
    double w, h;       // hit box (toggles, footswitch), centred on x, y
    double min, max;
};

// parameter indices (mirrors DistrhoPluginInfo.h)
enum { P_LEVEL, P_TONE, P_ATTACK, P_SUSTAIN, P_VCA, P_BYPASS, P_COUNT };

// Positions are snapped so that every modgui element (centre minus half
// its film box) lands on a whole CSS pixel (1.25 design units): browsers
// place backgrounds at fractional offsets with sub-pixel blur otherwise.
// tools/render_modgui.cpp refuses to generate fractional positions.
constexpr double KNOB_Y = 140.0, KNOB_R = 26.0;
constexpr double KX[4] = { 53.75, 128.75, 203.75, 278.75 };
constexpr double SW_X = 170.0, SW_Y = 225.0;
constexpr double LED_X = 170.0, LED_Y = 45.0;
constexpr double FOOT_X = 170.0, FOOT_Y = 462.5, FOOT_W = 150.0, FOOT_H = 100.0;

constexpr Control kControls[] = {
    { P_LEVEL,   kKnob,       KX[0], KNOB_Y, KNOB_R, 0, 0, 0, 10 },
    { P_TONE,    kKnob,       KX[1], KNOB_Y, KNOB_R, 0, 0, 0, 10 },
    { P_ATTACK,  kKnob,       KX[2], KNOB_Y, KNOB_R, 0, 0, 0, 10 },
    { P_SUSTAIN, kKnob,       KX[3], KNOB_Y, KNOB_R, 0, 0, 0, 10 },
    { P_VCA,     kToggle,     SW_X,  SW_Y,   0,      44, 16, 0, 1 },
    { P_BYPASS,  kFootswitch, FOOT_X, FOOT_Y, 0, FOOT_W, FOOT_H, 0, 1 },
};
constexpr int kNumControls = sizeof(kControls) / sizeof(kControls[0]);

struct State {
    float value[P_COUNT] = { 5.0f, 5.0f, 5.0f, 5.0f, 0.0f, 0.0f };
};

// ---- helpers ---------------------------------------------------------
inline void set(cairo_t* cr, const RGB& c, double a = 1.0)
{
    cairo_set_source_rgba(cr, c.r, c.g, c.b, a);
}

inline void rounded_rect(cairo_t* cr, double x, double y, double w, double h,
                         double rtl, double rtr, double rbr, double rbl)
{
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - rtr, y + rtr, rtr, -M_PI / 2, 0);
    cairo_arc(cr, x + w - rbr, y + h - rbr, rbr, 0, M_PI / 2);
    cairo_arc(cr, x + rbl, y + h - rbl, rbl, M_PI / 2, M_PI);
    cairo_arc(cr, x + rtl, y + rtl, rtl, M_PI, 3 * M_PI / 2);
    cairo_close_path(cr);
}

enum Align { kLeft, kCenter, kRight };

// Draw text with its baseline at y. maxw squeezes it to fit; fitw stretches
// or squeezes it to exactly fitw.
inline void text(cairo_t* cr, const char* s, double x, double y, double size,
                 const RGB& c, Align align = kCenter, bool bold = true,
                 const char* family = "DejaVu Sans Condensed",
                 double maxw = 0.0, double fitw = 0.0,
                 cairo_font_slant_t slant = CAIRO_FONT_SLANT_NORMAL)
{
    cairo_save(cr);
    cairo_select_font_face(cr, family, slant,
                           bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, size);
    cairo_text_extents_t e;
    cairo_text_extents(cr, s, &e);
    double sx = 1.0;
    if (fitw > 0.0 && e.x_advance > 0.0)
        sx = fitw / e.x_advance;
    else if (maxw > 0.0 && e.x_advance > maxw)
        sx = maxw / e.x_advance;
    const double w = e.x_advance * sx;
    double x0 = x;
    if (align == kCenter) x0 = x - w / 2;
    else if (align == kRight) x0 = x - w;
    cairo_translate(cr, x0, y);
    cairo_scale(cr, sx, 1.0);
    cairo_move_to(cr, 0, 0);
    set(cr, c);
    cairo_show_text(cr, s);
    cairo_restore(cr);
    cairo_new_path(cr);
}

// Letter-spaced text: draws each character with extra advance.
inline void spaced(cairo_t* cr, const char* s, double x, double y, double size,
                   double tracking, const RGB& c, bool bold = true,
                   const char* family = "DejaVu Sans Condensed")
{
    cairo_save(cr);
    cairo_select_font_face(cr, family, CAIRO_FONT_SLANT_NORMAL,
                           bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, size);
    cairo_text_extents_t e;
    cairo_text_extents(cr, s, &e);
    const double total = e.x_advance + tracking * (std::strlen(s) - 1);
    double cx = x - total / 2;
    set(cr, c);
    for (const char* p = s; *p; ++p) {
        char b[2] = { *p, 0 };
        cairo_text_extents(cr, b, &e);
        cairo_move_to(cr, cx, y);
        cairo_show_text(cr, b);
        cx += e.x_advance + tracking;
    }
    cairo_restore(cr);
    cairo_new_path(cr);
}

// 0..1 -> angle (radians, cairo convention), 7 o'clock .. 5 o'clock
inline double knob_angle(double norm)
{
    const double deg = -150.0 + 300.0 * norm;
    return (deg - 90.0) * M_PI / 180.0;
}

inline cairo_pattern_t* body_gradient()
{
    cairo_pattern_t* g = cairo_pattern_create_linear(0, 0, 0, H);
    cairo_pattern_add_color_stop_rgb(g, 0, kBodyTop.r, kBodyTop.g, kBodyTop.b);
    cairo_pattern_add_color_stop_rgb(g, 1, kBodyBottom.r, kBodyBottom.g, kBodyBottom.b);
    return g;
}

inline cairo_pattern_t* panel_gradient()
{
    cairo_pattern_t* g = cairo_pattern_create_linear(0, 12, 0, 262);
    cairo_pattern_add_color_stop_rgb(g, 0, kPanelTop.r, kPanelTop.g, kPanelTop.b);
    cairo_pattern_add_color_stop_rgb(g, 1, kPanelBottom.r, kPanelBottom.g, kPanelBottom.b);
    return g;
}

// ---- parts -----------------------------------------------------------
inline void screw(cairo_t* cr, double x, double y, double r, bool dark = false)
{
    cairo_new_path(cr);
    cairo_pattern_t* g = cairo_pattern_create_radial(x - r * 0.4, y - r * 0.4, r * 0.1, x, y, r);
    if (dark) {
        cairo_pattern_add_color_stop_rgb(g, 0, 0.55, 0.55, 0.55);
        cairo_pattern_add_color_stop_rgb(g, 1, 0.20, 0.20, 0.20);
    } else {
        cairo_pattern_add_color_stop_rgb(g, 0, 0.92, 0.92, 0.90);
        cairo_pattern_add_color_stop_rgb(g, 1, 0.45, 0.45, 0.43);
    }
    cairo_arc(cr, x, y, r, 0, 2 * M_PI);
    cairo_set_source(cr, g);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(g);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.6);
    cairo_set_line_width(cr, 0.6);
    cairo_stroke(cr);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.7);
    cairo_set_line_width(cr, r * 0.28);
    cairo_move_to(cr, x - r * 0.6, y - r * 0.15); cairo_line_to(cr, x + r * 0.6, y + r * 0.15);
    cairo_move_to(cr, x - r * 0.15, y + r * 0.6); cairo_line_to(cr, x + r * 0.15, y - r * 0.6);
    cairo_stroke(cr);
}

// Cream "vintage" knob: dark base ring, cream top face, black pointer
// line, lit from the top left (the lighting does not rotate).
inline void cream_knob(cairo_t* cr, double cx, double cy, double r, double norm)
{
    const double a = knob_angle(norm);
    cairo_new_path(cr);

    // cast shadow
    cairo_pattern_t* sh = cairo_pattern_create_radial(cx + r * 0.14, cy + r * 0.20, r * 0.75,
                                                      cx + r * 0.14, cy + r * 0.20, r * 1.3);
    cairo_pattern_add_color_stop_rgba(sh, 0, 0, 0, 0, 0.45);
    cairo_pattern_add_color_stop_rgba(sh, 1, 0, 0, 0, 0.0);
    cairo_arc(cr, cx + r * 0.14, cy + r * 0.20, r * 1.3, 0, 2 * M_PI);
    cairo_set_source(cr, sh);
    cairo_fill(cr);
    cairo_pattern_destroy(sh);

    // base ring
    cairo_new_path(cr);
    cairo_arc(cr, cx, cy, r, 0, 2 * M_PI);
    cairo_pattern_t* sk = cairo_pattern_create_linear(cx - r * 0.8, cy - r, cx + r, cy + r);
    cairo_pattern_add_color_stop_rgb(sk, 0, 0.30, 0.30, 0.30);
    cairo_pattern_add_color_stop_rgb(sk, 0.5, 0.12, 0.12, 0.12);
    cairo_pattern_add_color_stop_rgb(sk, 1, 0.04, 0.04, 0.04);
    cairo_set_source(cr, sk);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(sk);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.85);
    cairo_set_line_width(cr, r * 0.05);
    cairo_stroke(cr);

    // cream top face
    cairo_new_path(cr);
    cairo_arc(cr, cx, cy, r * 0.82, 0, 2 * M_PI);
    cairo_pattern_t* f = cairo_pattern_create_radial(cx - r * 0.32, cy - r * 0.40, r * 0.05,
                                                     cx, cy, r * 0.85);
    cairo_pattern_add_color_stop_rgb(f, 0, 0.97, 0.95, 0.90);
    cairo_pattern_add_color_stop_rgb(f, 0.6, 0.89, 0.86, 0.79);
    cairo_pattern_add_color_stop_rgb(f, 1, 0.72, 0.69, 0.62);
    cairo_set_source(cr, f);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(f);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.55);
    cairo_set_line_width(cr, r * 0.03);
    cairo_stroke(cr);

    // black pointer line, centre to near the edge
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_width(cr, r * 0.10);
    cairo_set_source_rgb(cr, 0.10, 0.10, 0.10);
    cairo_move_to(cr, cx + r * 0.14 * cos(a), cy + r * 0.14 * sin(a));
    cairo_line_to(cr, cx + r * 0.80 * cos(a), cy + r * 0.80 * sin(a));
    cairo_stroke(cr);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
}

// Two-position slide switch, thumb left (0) or right (1).
inline void slide_switch(cairo_t* cr, double cx, double cy, int pos)
{
    const double w = 38, h = 15;
    cairo_new_path(cr);
    rounded_rect(cr, cx - w / 2, cy - h / 2, w, h, 3, 3, 3, 3);
    cairo_set_source_rgb(cr, 0.10, 0.10, 0.11);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.35);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);
    const double tw = 17, th = 13;
    const double tx = pos ? cx + w / 2 - tw - 1 : cx - w / 2 + 1;
    cairo_pattern_t* g = cairo_pattern_create_linear(tx, cy - th / 2, tx, cy + th / 2);
    cairo_pattern_add_color_stop_rgb(g, 0, 0.96, 0.95, 0.92);
    cairo_pattern_add_color_stop_rgb(g, 1, 0.66, 0.64, 0.59);
    rounded_rect(cr, tx, cy - th / 2, tw, th, 2, 2, 2, 2);
    cairo_set_source(cr, g);
    cairo_fill(cr);
    cairo_pattern_destroy(g);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.45);
    cairo_set_line_width(cr, 0.8);
    for (int i = 0; i < 3; ++i) {
        const double lx = tx + 4.5 + i * 3.5;
        cairo_move_to(cr, lx, cy - th / 2 + 3);
        cairo_line_to(cr, lx, cy + th / 2 - 3);
    }
    cairo_stroke(cr);
}

// Black tread plate that is also the footswitch.
inline void tread(cairo_t* cr, bool down)
{
    cairo_new_path(cr);
    const double x = FOOT_X - FOOT_W / 2, y = FOOT_Y - FOOT_H / 2;
    rounded_rect(cr, x, y + (down ? 1.0 : 0.0), FOOT_W, FOOT_H, 8, 8, 8, 8);
    cairo_pattern_t* g = cairo_pattern_create_linear(x, y, x, y + FOOT_H);
    cairo_pattern_add_color_stop_rgb(g, 0, 0.11, 0.11, 0.12);
    cairo_pattern_add_color_stop_rgb(g, 1, 0.045, 0.045, 0.05);
    cairo_set_source(cr, g);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(g);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.7);
    cairo_set_line_width(cr, 1.4);
    cairo_stroke(cr);
    // inner bevel
    rounded_rect(cr, x + 3, y + 3, FOOT_W - 6, FOOT_H - 6, 6, 6, 6, 6);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.06);
    cairo_set_line_width(cr, 0.9);
    cairo_stroke(cr);
    // fine diamond tread pattern
    cairo_save(cr);
    rounded_rect(cr, x + 3, y + 3, FOOT_W - 6, FOOT_H - 6, 6, 6, 6, 6);
    cairo_clip(cr);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.045);
    cairo_set_line_width(cr, 0.7);
    for (double d = -FOOT_H; d < FOOT_W + FOOT_H; d += 7.0) {
        cairo_move_to(cr, x + d, y);
        cairo_line_to(cr, x + d + FOOT_H, y + FOOT_H);
        cairo_move_to(cr, x + d, y + FOOT_H);
        cairo_line_to(cr, x + d + FOOT_H, y);
    }
    cairo_stroke(cr);
    cairo_restore(cr);
    // maker wordmark debossed into the tread
    cairo_save(cr);
    cairo_translate(cr, 0, 2.0);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.14);
    il_maker::draw(cr, FOOT_X, FOOT_Y + 8, 24.0);
    cairo_restore(cr);
}

inline void led(cairo_t* cr, double cx, double cy, bool lit)
{
    cairo_new_path(cr);
    cairo_arc(cr, cx, cy, 5.5, 0, 2 * M_PI);
    cairo_set_source_rgb(cr, 0.20, 0.20, 0.20);
    cairo_fill(cr);
    if (lit) {
        cairo_pattern_t* glow = cairo_pattern_create_radial(cx, cy, 2, cx, cy, 14);
        cairo_pattern_add_color_stop_rgba(glow, 0, 1.0, 0.16, 0.10, 0.55);
        cairo_pattern_add_color_stop_rgba(glow, 1, 1.0, 0.16, 0.10, 0.0);
        cairo_arc(cr, cx, cy, 14, 0, 2 * M_PI);
        cairo_set_source(cr, glow);
        cairo_fill(cr);
    }
    cairo_pattern_t* g = cairo_pattern_create_radial(cx - 1.2, cy - 1.4, 0.5, cx, cy, 4.2);
    if (lit) {
        cairo_pattern_add_color_stop_rgb(g, 0, 1.0, 0.86, 0.80);
        cairo_pattern_add_color_stop_rgb(g, 0.4, 1.0, 0.18, 0.10);
        cairo_pattern_add_color_stop_rgb(g, 1, 0.62, 0.03, 0.02);
    } else {
        cairo_pattern_add_color_stop_rgb(g, 0, 0.52, 0.22, 0.20);
        cairo_pattern_add_color_stop_rgb(g, 1, 0.20, 0.03, 0.02);
    }
    cairo_arc(cr, cx, cy, 4.2, 0, 2 * M_PI);
    cairo_set_source(cr, g);
    cairo_fill(cr);
    cairo_pattern_destroy(g);
}

// The model name: baked "IRON LUNG" wordmark plus a small descriptor.
inline void logo(cairo_t* cr)
{
    set(cr, kInk);
    il_brand::draw(cr, W / 2, 352.0, 62.0);
    spaced(cr, "COMPRESSION SUSTAINER", W / 2, 380.0, 13.5, 1.6, kInk);
}

// ---- layers ----------------------------------------------------------
enum Layer {
    kLayerBody     = 1 << 0,   // enclosure, panel, print, tread, footswitch
    kLayerKnobs    = 1 << 1,   // LEVEL / TONE / ATTACK / SUSTAIN
    kLayerSwitch   = 1 << 2,   // VCA 1986 / LATER
    kLayerLed      = 1 << 3,
    kLayerAll      = 0xff
};

inline double norm(const State& s, int p)
{
    for (const Control& c : kControls)
        if (c.param == p)
            return (s.value[p] - c.min) / (c.max - c.min);
    return 0.0;
}

inline void draw(cairo_t* cr, const State& s, unsigned layers = kLayerAll)
{
    if (layers & kLayerBody) {
        // enclosure
        cairo_new_path(cr);
        cairo_pattern_t* g = body_gradient();
        rounded_rect(cr, 1, 1, W - 2, H - 2, 11, 11, 11, 11);
        cairo_set_source(cr, g);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(g);
        cairo_set_source_rgba(cr, 0, 0, 0, 0.55);
        cairo_set_line_width(cr, 1.5);
        cairo_stroke(cr);
        rounded_rect(cr, 3, 3, W - 6, H - 6, 9, 9, 9, 9);
        cairo_set_source_rgba(cr, 1, 1, 1, 0.30);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);

        // black control panel
        cairo_new_path(cr);
        rounded_rect(cr, 14, 12, W - 28, 250, 6, 6, 6, 6);
        cairo_pattern_t* pg = panel_gradient();
        cairo_set_source(cr, pg);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(pg);
        cairo_set_source_rgba(cr, 0, 0, 0, 0.55);
        cairo_set_line_width(cr, 1.1);
        cairo_stroke(cr);
        rounded_rect(cr, 16, 14, W - 32, 246, 5, 5, 5, 5);
        cairo_set_source_rgba(cr, 1, 1, 1, 0.10);
        cairo_set_line_width(cr, 0.8);
        cairo_stroke(cr);

        // CHECK lamp legend
        spaced(cr, "CHECK", W / 2, 30.0, 10.0, 1.4, kInk);

        // knob legends
        const char* names[4] = { "LEVEL", "TONE", "ATTACK", "SUSTAIN" };
        for (int i = 0; i < 4; ++i) {
            text(cr, names[i], KX[i], 93.0, 12.5, kInk, kCenter, true,
                 "DejaVu Sans", 0.0, 56.0);
            const char* lo = i == 1 ? "LO" : "MIN";
            const char* hi = i == 1 ? "HI" : "MAX";
            text(cr, lo, KX[i] - 23, 181.0, 7.0, kSoft, kRight, false, "DejaVu Sans");
            text(cr, hi, KX[i] + 23, 181.0, 7.0, kSoft, kLeft, false, "DejaVu Sans");
        }

        // VCA selector
        text(cr, "VCA", SW_X, SW_Y - 14.0, 11.0, kInk, kCenter, true,
             "DejaVu Sans", 0.0, 34.0);
        text(cr, "1986", SW_X - 30, SW_Y + 4.0, 8.5, kSoft, kRight, false, "DejaVu Sans");
        text(cr, "LATER", SW_X + 30, SW_Y + 4.0, 8.5, kSoft, kLeft, false, "DejaVu Sans");

        // body print: in/out and the model name
        text(cr, "OUTPUT", 46, 286, 13, kInk, kLeft, true, "DejaVu Sans", 0.0, 66.0);
        text(cr, "INPUT", W - 46, 286, 13, kInk, kRight, true, "DejaVu Sans", 0.0, 58.0);
        cairo_set_source_rgba(cr, 1, 1, 1, 0.85);
        cairo_set_line_width(cr, 1.2);
        cairo_move_to(cr, 18, 296); cairo_line_to(cr, 34, 296);
        cairo_move_to(cr, W - 34, 296); cairo_line_to(cr, W - 18, 296);
        cairo_stroke(cr);
        // arrows
        cairo_move_to(cr, 30, 292); cairo_line_to(cr, 24, 296); cairo_line_to(cr, 30, 300);
        cairo_move_to(cr, W - 30, 292); cairo_line_to(cr, W - 24, 296); cairo_line_to(cr, W - 30, 300);
        cairo_stroke(cr);

        logo(cr);

        tread(cr, s.value[P_BYPASS] > 0.5f);
        screw(cr, W / 2, H - 11, 4.6, true);
    }

    if (layers & kLayerKnobs)
        for (int i = 0; i < 4; ++i)
            cream_knob(cr, KX[i], KNOB_Y, KNOB_R, norm(s, P_LEVEL + i));

    if (layers & kLayerSwitch)
        slide_switch(cr, SW_X, SW_Y, s.value[P_VCA] > 0.5f ? 1 : 0);

    if (layers & kLayerLed)
        led(cr, LED_X, LED_Y, s.value[P_BYPASS] < 0.5f);
}

} // namespace il_art
