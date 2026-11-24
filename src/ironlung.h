#pragma once

#ifndef IL_IRONLUNG_H
#define IL_IRONLUNG_H

#include <lv2.h>

#define ILPLUGIN_URI "http://dataforte.net/plugins/ironlung"

// Control ports in the order of the faust sliders in
// src/ironlung.dsp (dsp2cc binds them by this enum) and of the
// DPF Parameters enum in plugins/IronLung/DistrhoPluginInfo.h.
typedef enum
{
  LEVEL,
  TONE,
  ATTACK,
  SUSTAIN,
  VCA,
  EFFECTS_OUTPUT,
  EFFECTS_INPUT,
} PortIndex;

#endif // IL_IRONLUNG_H
