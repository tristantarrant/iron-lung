/*
 * Minimal glue header for faust-generated DSP classes in this plugin.
 * Adapted (simplified, self-contained) from the Guitarix LV2 glue
 * headers (gx_common.h / gx_pluginlv2.h, Copyright (C) 2011-2012
 * Hermann Meyer, James Warden, Andreas Degert, Pete Shorthose,
 * Steve Poskitt, GPL-2.0-or-later).
 */

#pragma once

#ifndef IL_GLUE_H
#define IL_GLUE_H

#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <algorithm>

/////////////////////// FAUST SUPPORT ////////////////////////////////

#define FAUSTFLOAT float

#ifndef N_
#define N_(String) (String)
#endif

#ifndef __rt_func
#define __rt_func
#endif

#ifndef always_inline
#define always_inline inline
#endif

template <typename T> inline T mydsp_faustpower2_f(T x) { return (x * x); }
template <typename T> inline T mydsp_faustpower3_f(T x) { return ((x * x) * x); }
template <typename T> inline T mydsp_faustpower4_f(T x) { return ((x * x) * (x * x)); }
template <typename T> inline T mydsp_faustpower5_f(T x) { return ((x * x) * (x * x) * x); }
template <typename T> inline T mydsp_faustpower6_f(T x) { return ((x * x) * (x * x) * (x * x)); }

/////////////////////// DENORMAL PROTECTION WITH SSE /////////////////

#ifdef __SSE__
#include <immintrin.h>
#ifndef _IMMINTRIN_H_INCLUDED
#include <fxsrintrin.h>
#endif
/* On Intel set FZ (Flush to Zero) and DAZ (Denormals Are Zero)
   flags to avoid costly denormals */
inline void AVOIDDENORMALS()
{
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
}
#else
inline void AVOIDDENORMALS() {}
#endif //__SSE__

/////////////////////// PLUGIN DEFINITION STRUCTURE //////////////////

// forward declarations (need not be resolved for plugin definition)
struct PluginLV2;

typedef void (*inifunc)(uint32_t samplingFreq, PluginLV2 *plugin);
typedef int (*activatefunc)(bool start, PluginLV2 *plugin);
typedef void (*clearstatefunc)(PluginLV2 *plugin);
typedef void (*process_mono_audio) (int count, float *input, float *output, PluginLV2 *plugin);
typedef void (*process_stereo_audio) (int count, float *input1, float *input2,
				      float *output1, float *output2, PluginLV2 *plugin);
typedef void (*registerfunc)(uint32_t port, void* data, PluginLV2 *plugin);
typedef void (*deletefunc)(PluginLV2 *plugin);

typedef PluginLV2 *(*plug) ();

#define PLUGINLV2_VERMAJOR_MASK 0xff00
#define PLUGINLV2_VERSION       0x0500

struct PluginLV2 {
    int version;	// = PLUGINLV2_VERSION

    const char* id;	// must be unique
    const char* name;	// displayed name (not translated) (may be 0)
    // maximal one of mono_audio, stereo_audio must be set
    // all function pointers in PluginLV2 can be independently set to 0
    process_mono_audio mono_audio; // function for mono audio processing
    process_stereo_audio stereo_audio; // function for stereo audio processing

    inifunc set_samplerate; // called before audio processing and when rate changes
    activatefunc activate_plugin; // called when taking in / out of pressing chain
    registerfunc connect_ports; // called once after module loading (register parameter ids)
    clearstatefunc clear_state;	// clear internal audio state; may be called
				// before calling the process function
    deletefunc delete_instance; // delete this plugin instance
};

#endif // IL_GLUE_H
