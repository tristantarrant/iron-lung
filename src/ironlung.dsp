declare id "ironlung";
declare name "Iron Lung";
declare category "Dynamics";
declare author "Dataforte";
declare license "GPL-2.0-or-later";
declare description "Emulation of the 1986 BOSS CS-3 Compression Sustainer: JFET-buffered 9 V VCA compressor (uPC1252H2 / dbx 1252 VCA), op-amp input buffer, I/V converter with 8.8 kHz pole, active treble cut/boost tone stage, discrete class-A output buffer, and a full-wave transistor rectifier charging C8 4.7 uF with the ATTACK pot setting both charge (release) and attack times. SUSTAIN sets the D6 clamp on C8, i.e. the static gain. VCA selects the 1986 DBX-type part or the later THAT-type replacement. Controls are pot positions 0..10 as on the pedal.";

import("stdfaust.lib");

// ====================================================================
//  Signal level mapping
// ====================================================================
// 1.0 full scale <-> 1 V at the pedal input and output (the audio path
// is AC-coupled and biased at 4.5 V; only the signal is modelled).
VIN = 1.0;

// ====================================================================
//  Circuit constants (1986 service-notes schematic, ET board
//  7523252000 / VR board 7523254000)
// ====================================================================

// --- input stage ----------------------------------------------------
// R1 10k, C2 0.022u, R3 1M: first-order highpass at 1/(2pi*C*(R1+R3)).
F_IN_HP = 1/(2*ma.PI*22e-9*(10e3+1e6));      // 7.2 Hz

// IC1a (M5218L) follower on 9 V: output swing about +-3.6 V.
VR_BUF = 3.6;

// --- VCA ------------------------------------------------------------
// C4 1u into R9 10k (the VCA input is a virtual ground).
F_VCA_HP = 1/(2*ma.PI*1e-6*10e3);            // 15.9 Hz

// VCA output: IOUT into IC2a with R6 10k || C5 1.8n.
F_IV_LP = 1/(2*ma.PI*10e3*1.8e-9);           // 8.84 kHz

// VCA output current limit: the 1252 idles at ~0.6 mA, so 10k allows
// about 6 V peak; the M5218 output stage limits first, at ~3.6 V.
VR_VCA_DBX  = 3.6;
VR_VCA_THAT = 4.0;                            // THAT 2150/2159 headroom

// --- gain control ---------------------------------------------------
// The VCA gain is set by the current the JFET source follower (Q7,
// 2SK117) pushes into the 1252 idling-current pin; the datasheet only
// documents the GC1 law (5.9 mV/dB), not GC2. The static gain range
// below is calibrated to the pedal's documented behaviour (SUSTAIN
// minimum still compresses most pickups; maximum gives long sustain)
// and is an estimate: +18 dB at the C8 floor of the sustain range,
// +40 dB at the ceiling. See README.
G0      = 18.0;                               // dB at C8 = C8_MIN
G_SLOPE = 22.0/(8.71-4.86);                   // dB/V of C8 (22 dB span)

// --- envelope (C8 node) ---------------------------------------------
C8   = 4.7e-6;                                // C8 4.7/35
VBE  = 0.70;                                  // 2SC945 base-emitter
RB   = 47e3;                                  // C7/R12, C6/R17 base resistors
HFE  = 200.0;                                 // 2SC945 current gain (typ.)
IC_MAX = 5e-3;                                // collector saturation limit
KD   = 0.955;                                 // 1M/(1M+47k) base divider

// Sustain divider: 9 V -> R4 4.7k -> VR4 20kB -> R3 22k -> GND, wiper
// into IC4b. D6 (1S133) clamps C8 at Vsus + 0.62 V.
R4_SUS = 4.7e3;
R3_SUS = 22e3;
R_SUS  = 20e3;
VD6    = 0.62;

// Attack rheostat: 9 V -> R2 22k -> VR3 250kC -> C8. The reverse-log
// taper is approximated by R = (1-x)^3 (documented deviation).
R2_ATT = 22e3;
R_ATT  = 250e3;

// Sidechain input: C7 0.01u/R12 47k into Q8, and C6 1u/IC4a/C9 0.01u/
// R17 47k into Q6 (inverted): a first-order highpass at 339 Hz on both
// halves, so the detector is de-emphasised below the guitar's midrange.
F_SC_HP = 1/(2*ma.PI*47e3*10e-9);            // 339 Hz

// --- tone stage -----------------------------------------------------
// IC2b (M5218L) differential amplifier: R5 10k into +, R4 10k feedback,
// TONE 20kB between + and -, wiper through R1 4.7k + C1 0.027u to GND.
// H(s) = ((beta + R4/Ry)*sC + A) / ((beta + R10/Rx)*sC + A)
// with A = 1/Rx - 1/Ry and beta = 1 + A*R1.
R5_TONE = 10e3;
R4_TONE = 10e3;
R1_TONE = 4.7e3;
C1_TONE = 27e-9;
R_TONE  = 20e3;

// --- level stage ----------------------------------------------------
// IC2b out -> C13 47n -> R36 10k -> VR1 50kA -> Q4/Q1 buffers.
F_LVL_HP = 1/(2*ma.PI*47e-9*(10e3+50e3));    // 56 Hz
R36_LVL  = 10e3;
R_LVL    = 50e3;

// --- output stage ---------------------------------------------------
// C17 1u into R29 10k; Q1 emitter follower drives the 1k output.
F_OUT_HP = 1/(2*ma.PI*1e-6*10e3);            // 15.9 Hz
VR_OUT   = 3.9;

// ====================================================================
//  Controls (pot positions 0..10 = fraction of clockwise travel)
// ====================================================================
// Defaults: LEVEL 5 (noon), TONE 5 (noon; the original's flattest point
// is about 1 o'clock), ATTACK 5, SUSTAIN 5, DBX-type VCA.

level   = hslider("level[name:LEVEL]",      5.0, 0.0, 10.0, 0.01) : si.smooth(0.999);
tone    = hslider("tone[name:TONE]",        5.0, 0.0, 10.0, 0.01) : si.smooth(0.999);
attack  = hslider("attack[name:ATTACK]",    5.0, 0.0, 10.0, 0.01) : si.smooth(0.999);
sustain = hslider("sustain[name:SUSTAIN]",  5.0, 0.0, 10.0, 0.01) : si.smooth(0.999);
vca     = nentry("vca[name:VCA]", 0, 0, 1, 1);

// ====================================================================
//  Helpers
// ====================================================================

// Smooth limiter towards +-VR (BluesFuse pattern): transparent at half
// swing (-0.06 dB), hard-ish knee at the rail. Written with constant
// limits: faust's simplifier mis-rewrites the parameterised form
// (l*tanh(x/l) becomes x*tanh(l/x), which rectifies the signal).
rail36(x) = x / pow(1 + pow(abs(x)/VR_BUF, 4), 0.25);
rail39(x) = x / pow(1 + pow(abs(x)/VR_OUT, 4), 0.25);
rail40(x) = x / pow(1 + pow(abs(x)/VR_VCA_THAT, 4), 0.25);
rail_vca(x) = select2(vca > 0.5, rail36(x), rail40(x));

// Pot tapers. B = linear, A = audio (about 15% at half rotation),
// C = reverse audio (approximated by (1-x)^3, see README).
atap(x) = pow(x, 2.7);
ctap(x) = pow(1.0-x, 3.0);

// Sustain wiper voltage (20kB linear, CCW = bottom of the divider).
px_sus = aa.clip(0.0, 1.0, sustain/10.0);
vsus   = 9.0*(R3_SUS + R_SUS*px_sus)/(R4_SUS + R_SUS + R3_SUS);
c8ceil = vsus + VD6;
c8min  = 9.0*R3_SUS/(R4_SUS + R_SUS + R3_SUS) + VD6;   // 4.86 V

// ATTACK rheostat charge resistance from 9 V into C8.
px_att = aa.clip(0.0, 1.0, attack/10.0);
ratt   = R2_ATT + R_ATT*ctap(px_att);
icharge(c) = (9.0 - c)/ratt;

// Full-wave rectifier: Q6/Q8 conduct on the two signal halves above
// VBE, their collector current (limited by the 47k base resistors and
// hFE) pulls C8 down.
ic(v) = min(IC_MAX, HFE*max(0.0, v - VBE)/RB);
idis(v) = ic(v) + ic(-v);

// C8 update: charge through the ATTACK pot, discharge through Q6/Q8,
// clamped from above by D6 at the SUSTAIN voltage.
c8step(side, prev) = min(c8ceil, max(0.0, prev + (1.0/ma.SR)*(icharge(prev) - idis(side))/C8));

// Tone-stage coefficients from the pot position x (0 = cut end,
// 1 = boost end). The wiper splits the 20k element into Rx/Ry; the two
// halves are in parallel between the (equal) op-amp inputs and the
// wiper, so A = 1/Rx + 1/Ry.
px_tone = aa.clip(0.0005, 0.9995, tone/10.0);
rx_tone = R_TONE*px_tone;
ry_tone = R_TONE*(1.0-px_tone);
a_tone  = 1.0/rx_tone + 1.0/ry_tone;
beta_tn = 1.0 + a_tone*R1_TONE;

// Analytic H(s) = (A + n1 s)/(A + d1 s): unity at DC, HF gain n1/d1,
// shelf corner wp = A/d1. Implemented as 1 + (G-1)*HP1(wp) with a
// topology-preserving one-pole, which stays exact at the flat centre
// (the direct form degenerates to a pole on the unit circle there).
n1_tn = C1_TONE*(beta_tn + R4_TONE/ry_tone);
d1_tn = C1_TONE*(beta_tn + R5_TONE/rx_tone);
fc_tn = a_tone/(2*ma.PI*d1_tn);          // shelf pole corner (Hz)
g_tn  = n1_tn/d1_tn;                      // high-frequency gain
hp_tn(x) = x - fi.lowpass(1, fc_tn, x);
tonef(x) = x + (g_tn - 1.0)*hp_tn(x);

// LEVEL: log pot with R36 in series (the wiper is unloaded by Q4).
lvl_gain = (atap(aa.clip(0.0, 1.0, level/10.0))*R_LVL)/(R36_LVL + R_LVL);

// VCA variant: 0 = 1986 uPC1252H2, 1 = later THAT 2150/2159.
vr_vca  = select2(vca > 0.5, VR_VCA_DBX, VR_VCA_THAT);
g_trim  = select2(vca > 0.5, 0.0, 2.0);        // +2 dB (wider 2181 range)

// ====================================================================
//  Audio path
// ====================================================================
// One recursive state: C8. Its value from the previous sample sets the
// VCA gain (the control path is not instantaneous, so the one-sample
// delay is physical), and the new value is integrated from the
// rectified VCA output. Keeping the whole sample loop in the recursion
// is how faust schedules the feedback (a `mem` across the loop is
// rejected as an evaluation cycle).
process = loop ~ _ : !,_ with {
    loop(c8prev, x) = (c8, y) with {
        // --- VCA gain -----------------------------------------------
        gdb = G0 + g_trim + G_SLOPE*(c8prev - c8min);
        g   = pow(10.0, gdb/20.0);

        // --- audio --------------------------------------------------
        x1 = x : fi.highpass(1, F_IN_HP) : rail36;
        x2 = x1 : fi.highpass(1, F_VCA_HP) : *(g) : rail_vca;
        x3 = x2 : fi.lowpass(1, F_IV_LP);
        x4 = tonef(x3) : fi.highpass(1, F_LVL_HP) : *(lvl_gain);
        y  = x4 : fi.highpass(1, F_OUT_HP) : rail39;

        // --- sidechain (VCA output, before the tone stage) ----------
        side = x3 : fi.highpass(1, F_SC_HP) : *(KD);
        c8   = c8step(side, c8prev);
    };
};
