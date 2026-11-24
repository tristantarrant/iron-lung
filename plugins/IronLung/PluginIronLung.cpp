/*
 * Iron Lung - 1986 VCA compressor emulation
 * DPF plugin wrapper around the faust-generated DSP class.
 *
 * Copyright (C) 2026 Dataforte
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DistrhoPlugin.hpp"

#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <vector>

// Faust/dsp2cc generated DSP (binds controls by PortIndex)
#include "../../src/il_glue.h"
#include "../../src/ironlung.h"
#include "../../src/ironlung.cc"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------
// Presets: documented settings (see README). Program 0 equals the
// parameter defaults.

struct IlProgram {
    const char* name;
    float level, tone, attack, sustain;
    float vca;        // 0 = 1986 VCA, 1 = later VCA
};

static const IlProgram kPrograms[] = {
    { "Noon - default",                                              5.0f, 5.0f, 5.0f, 5.0f, 0.0f },
    { "Manual - as a compressor (LEVEL 10:00, TONE 1:30, ATTACK 11:00)", 3.0f, 6.6f, 4.0f, 4.6f, 0.0f },
    { "Manual - as a limiter (SUSTAIN 10:00)",                       3.0f, 6.6f, 4.0f, 3.0f, 0.0f },
    { "Always-on, subtle (BOSS article: attack/sustain at 12)",      5.0f, 6.0f, 5.0f, 5.0f, 0.0f },
    { "Clean and open (reviewer: sustain 9:00, tone 1:00)",          5.0f, 6.0f, 5.0f, 2.0f, 0.0f },
    { "Country snap (pick attack forward)",                          6.0f, 7.0f, 8.0f, 5.0f, 0.0f },
    { "Squashed funk (fast attack, high sustain)",                   7.0f, 8.0f, 2.0f, 8.0f, 0.0f },
    { "Infinite sustain lead (later VCA)",                           6.0f, 5.0f, 6.0f, 9.0f, 1.0f },
};
static constexpr uint32_t kProgramCount = sizeof(kPrograms) / sizeof(kPrograms[0]);

// --------------------------------------------------------------------

class PluginIronLung : public Plugin
{
public:
    PluginIronLung()
        : Plugin(kParamCount, kProgramCount, 0),
          fDsp(ironlung::plugin())
    {
        for (uint32_t i = 0; i < kParamBypass; ++i)
            fParams[i] = 0.0f;
        fParams[kParamBypass] = 0.0f;
        setProgramValues(0);

        for (uint32_t i = 0; i < kParamBypass; ++i)
            fDsp->connect_ports(i, &fParams[i], fDsp);

        fDsp->set_samplerate(static_cast<uint32_t>(getSampleRate()), fDsp);
        fDry.resize(getBufferSize());
        fFadeStep = 1.0f / (0.010f * static_cast<float>(getSampleRate()));
    }

    ~PluginIronLung() override
    {
        if (fDsp->activate_plugin != nullptr)
            fDsp->activate_plugin(false, fDsp);
        fDsp->delete_instance(fDsp);
    }

protected:
    const char* getLabel() const override { return "ironlung"; }
    const char* getDescription() const override
    {
        return "Emulation of the 1986 VCA compression sustainer: op-amp input "
               "buffer with protection clamps, current-mode VCA with a 8.8 kHz "
               "I/V pole, active treble cut/boost tone stage, discrete class-A "
               "output buffer, and a full-wave transistor rectifier that "
               "charges C8 4.7 uF; SUSTAIN sets the C8 clamp (static gain), "
               "ATTACK sets the charge current (release and attack). VCA selects "
               "the 1986 part or the later production VCA. Hard knee, fixed "
               "threshold, high ratio. Controls are pot positions as on the pedal.";
    }
    const char* getMaker() const override { return "Dataforte"; }
    const char* getHomePage() const override { return "https://github.com/tristantarrant/iron-lung"; }
    const char* getLicense() const override { return "GPL-2.0-or-later"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('I', 'r', 'L', 'u'); }

    void initAudioPort(const bool input, const uint32_t index, AudioPort& port) override
    {
        port.groupId = kPortGroupMono;
        Plugin::initAudioPort(input, index, port);
        // keep the symbols of the pre-DPF plugin: pedalboards store them
        port.symbol = input ? "in" : "out";
        port.name   = input ? "In" : "Out";
    }

    void initParameter(const uint32_t index, Parameter& p) override
    {
        p.hints = kParameterIsAutomatable;
        p.ranges.min = 0.0f;
        p.ranges.max = 10.0f;

        switch (index)
        {
        case kParamLevel:
            p.name = "LEVEL"; p.symbol = "level";
            p.ranges.def = kPrograms[0].level;
            break;
        case kParamTone:
            p.name = "TONE"; p.symbol = "tone";
            p.ranges.def = kPrograms[0].tone;
            break;
        case kParamAttack:
            p.name = "ATTACK"; p.symbol = "attack";
            p.ranges.def = kPrograms[0].attack;
            break;
        case kParamSustain:
            p.name = "SUSTAIN"; p.symbol = "sustain";
            p.ranges.def = kPrograms[0].sustain;
            break;
        case kParamVca:
            p.hints |= kParameterIsInteger;
            p.name = "VCA"; p.symbol = "vca";
            p.ranges.max = 1.0f;
            p.ranges.def = 0.0f;
            setEnum(p, { "1986", "later" });
            break;
        case kParamBypass:
            p.initDesignation(kParameterDesignationBypass);
            break;
        }
    }

    void initProgramName(const uint32_t index, String& name) override
    {
        if (index < kProgramCount)
            name = kPrograms[index].name;
    }

    void loadProgram(const uint32_t index) override
    {
        if (index < kProgramCount)
            setProgramValues(index);
    }

    float getParameterValue(const uint32_t index) const override
    {
        return index < kParamCount ? fParams[index] : 0.0f;
    }

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index < kParamCount)
            fParams[index] = value;
    }

    void activate() override
    {
        if (fDsp->activate_plugin != nullptr)
            fDsp->activate_plugin(true, fDsp);
        fWet = fParams[kParamBypass] > 0.5f ? 0.0f : 1.0f;
    }

    void deactivate() override
    {
        if (fDsp->activate_plugin != nullptr)
            fDsp->activate_plugin(false, fDsp);
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        fDry.resize(newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        fDsp->set_samplerate(static_cast<uint32_t>(newSampleRate), fDsp);
        fFadeStep = 1.0f / (0.010f * static_cast<float>(newSampleRate));
    }

    void run(const float** inputs, float** outputs, const uint32_t frames) override
    {
        AVOIDDENORMALS();

        const float* const in = inputs[0];
        float* const out = outputs[0];
        if (frames == 0)
            return;

        const float target = fParams[kParamBypass] > 0.5f ? 0.0f : 1.0f;
        const bool fading = fWet != target;

        // keep the DSP running while bypassed so that re-engaging is seamless
        if (fading || target == 0.0f)
            std::memcpy(fDry.data(), in, sizeof(float) * frames);

        fDsp->mono_audio(static_cast<int>(frames), const_cast<float*>(in), out, fDsp);

        if (!fading && target == 1.0f)
            return;
        if (!fading)
        {
            std::memcpy(out, fDry.data(), sizeof(float) * frames);
            return;
        }
        for (uint32_t i = 0; i < frames; ++i)
        {
            if (fWet < target)
                fWet = std::min(target, fWet + fFadeStep);
            else if (fWet > target)
                fWet = std::max(target, fWet - fFadeStep);
            out[i] = fDry[i] + fWet * (out[i] - fDry[i]);
        }
    }

private:
    PluginLV2* const fDsp;
    float fParams[kParamCount];
    std::vector<float> fDry;
    float fWet = 1.0f;
    float fFadeStep = 0.002f;

    void setProgramValues(const uint32_t index)
    {
        fParams[kParamLevel]   = kPrograms[index].level;
        fParams[kParamTone]    = kPrograms[index].tone;
        fParams[kParamAttack]  = kPrograms[index].attack;
        fParams[kParamSustain] = kPrograms[index].sustain;
        fParams[kParamVca]     = kPrograms[index].vca;
    }

    static void setEnum(Parameter& p, std::initializer_list<const char*> labels)
    {
        const uint32_t n = static_cast<uint32_t>(labels.size());
        ParameterEnumerationValue* const values = new ParameterEnumerationValue[n];
        uint32_t i = 0;
        for (const char* label : labels)
        {
            values[i].label = label;
            values[i].value = static_cast<float>(i);
            ++i;
        }
        p.enumValues.count = n;
        p.enumValues.restrictedMode = true;
        p.enumValues.values = values;
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginIronLung)
};

Plugin* createPlugin()
{
    return new PluginIronLung();
}

END_NAMESPACE_DISTRHO
