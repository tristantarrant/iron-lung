/*
 * Iron Lung - LV2 test harness
 *
 * Loads the plugin with the raw LV2 C API, exercises the DSP and checks
 * the behaviour the model encodes (1986 schematic; see README):
 *   - LEVEL at noon and SUSTAIN at minimum is unity gain;
 *   - SUSTAIN raises the static gain by about 22 dB (18..40 dB VCA);
 *   - above the fixed threshold the compressor is a high-ratio limiter
 *     (about 40:1 at 440 Hz) and the VCA output stays near 1 V peak;
 *   - TONE is a treble shelf, flat at the centre;
 *   - ATTACK trades attack against release (CW = slower attack, faster
 *     release);
 *   - the 339 Hz sidechain highpass compresses bass less than treble;
 *   - the later VCA adds about 2 dB of static gain;
 *   - bypass is exact and click-free.
 *
 * Usage: ./il_test /path/to/ironlung.lv2/ironlung_dsp.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>

#include "lv2host.h"

typedef struct _LV2_Descriptor LV2_Descriptor;
typedef void *LV2_Handle;

struct _LV2_Descriptor {
  const char *plugin_uri;
  LV2_Handle (*instantiate)(const LV2_Descriptor *, double, const char *,
                            const void *const *);
  void (*connect_port)(LV2_Handle, unsigned int, void *);
  void (*activate)(LV2_Handle);
  void (*run)(LV2_Handle, unsigned int);
  void (*deactivate)(LV2_Handle);
  void (*cleanup)(LV2_Handle);
  void *reserved;
};

/* DPF port layout: audio in, audio out, parameters, lv2:enabled */
enum { LEVEL, TONE, ATTACK, SUSTAIN, VCA, NCTRL };
#define P_IN      0
#define P_OUT     1
#define P_CTRL0   2
#define P_ENABLED 7

#define BLOCK 2048

static int failures = 0;
static const LV2_Descriptor *desc;
static float ctrl[NCTRL];
static float enabled = 1.0f;
static float in[BLOCK], out[BLOCK];

static void
check(int ok, const char *msg)
{
  printf("%s: %s\n", ok ? "PASS" : "FAIL", msg);
  if (!ok)
    ++failures;
}

static void
defaults(void)
{
  ctrl[LEVEL] = 5.0f; ctrl[TONE] = 5.0f;
  ctrl[ATTACK] = 5.0f; ctrl[SUSTAIN] = 5.0f; ctrl[VCA] = 0.0f;
}

static LV2_Handle
open_plugin(double sr)
{
  LV2_Handle h = desc->instantiate(desc, sr, NULL,
                                   (const void *const *)host_get_features(BLOCK));
  if (!h)
    return NULL;
  for (int i = 0; i < NCTRL; ++i)
    desc->connect_port(h, P_CTRL0 + i, &ctrl[i]);
  enabled = 1.0f;
  desc->connect_port(h, P_ENABLED, &enabled);
  desc->connect_port(h, P_OUT, out);
  desc->connect_port(h, P_IN, in);
  desc->activate(h);
  return h;
}

static void
close_plugin(LV2_Handle h)
{
  desc->deactivate(h);
  desc->cleanup(h);
}

static double
db(double x)
{
  return 20.0 * log10(x + 1e-30);
}

static double g_phase;

/* Run a continuous-phase sine for `secs`; return the output rms over the
   last `tail` blocks (tail=1: the last block). *finite reports NaN/Inf. */
static double
sine_rms(LV2_Handle h, double sr, double freq, double amp, double secs,
         int tail, int *finite)
{
  int nblocks = (int)(secs * sr / BLOCK) + 1;
  double acc = 0;
  int n = 0;
  if (tail < 1)
    tail = 1;
  *finite = 1;
  for (int b = 0; b < nblocks; ++b) {
    for (int i = 0; i < BLOCK; ++i, ++g_phase)
      in[i] = (float)(amp * sin(2.0 * M_PI * freq * g_phase / sr));
    desc->run(h, BLOCK);
    if (b >= nblocks - tail) {
      for (int i = 0; i < BLOCK; ++i) {
        if (!isfinite(out[i]))
          *finite = 0;
        acc += (double)out[i] * out[i];
      }
      n += BLOCK;
    }
  }
  return sqrt(acc / n);
}

/* rms of the last block and its peak */
static double
sine_peak(LV2_Handle h, double sr, double freq, double amp, double secs, int *finite)
{
  int nblocks = (int)(secs * sr / BLOCK) + 1;
  double peak = 0;
  *finite = 1;
  for (int b = 0; b < nblocks; ++b) {
    for (int i = 0; i < BLOCK; ++i, ++g_phase)
      in[i] = (float)(amp * sin(2.0 * M_PI * freq * g_phase / sr));
    desc->run(h, BLOCK);
    if (b >= nblocks - 2) {
      for (int i = 0; i < BLOCK; ++i) {
        if (!isfinite(out[i]))
          *finite = 0;
        if (fabs(out[i]) > peak)
          peak = fabs(out[i]);
      }
    }
  }
  return peak;
}

static void
advance(LV2_Handle h, long samples)
{
  while (samples > 0) {
    int n = samples > BLOCK ? BLOCK : (int)samples;
    memset(in, 0, sizeof(float) * n);
    desc->run(h, n);
    samples -= n;
  }
}

/* run `secs` of a sine and return the peak of the last block */
static double
last_peak(LV2_Handle h, double sr, double freq, double amp, double secs)
{
  int nblocks = (int)(secs * sr / BLOCK) + 1;
  double peak = 0;
  for (int b = 0; b < nblocks; ++b) {
    for (int i = 0; i < BLOCK; ++i, ++g_phase)
      in[i] = (float)(amp * sin(2.0 * M_PI * freq * g_phase / sr));
    desc->run(h, BLOCK);
    if (b == nblocks - 1)
      for (int i = 0; i < BLOCK; ++i)
        if (fabs(out[i]) > peak)
          peak = fabs(out[i]);
  }
  return peak;
}

/* a 1 kHz burst followed by a quiet probe: return the probe peak after
   `probe_delay` seconds (gain recovery) */
static double
release_probe(LV2_Handle h, double sr, double probe_delay)
{
  /* 0.5 s burst at 0.3 V, then 0.001 V probe */
  int burst = (int)(0.5 * sr);
  for (int i = 0; i < burst; ++i, ++g_phase)
    in[i % BLOCK] = (float)(0.3 * sin(2.0 * M_PI * 1000.0 * g_phase / sr));
  int done = 0;
  while (done < burst) {
    int n = burst - done > BLOCK ? BLOCK : burst - done;
    for (int i = 0; i < n; ++i, ++g_phase)
      in[i] = (float)(0.3 * sin(2.0 * M_PI * 1000.0 * (g_phase - n) / sr));
    desc->run(h, n);
    done += n;
  }
  /* probe for probe_delay + 10 ms, return the last block peak */
  int probe = (int)((probe_delay + 0.01) * sr);
  double peak = 0;
  done = 0;
  while (done < probe) {
    int n = probe - done > BLOCK ? BLOCK : probe - done;
    for (int i = 0; i < n; ++i, ++g_phase)
      in[i] = (float)(0.001 * sin(2.0 * M_PI * 1000.0 * g_phase / sr));
    desc->run(h, n);
    if (done + n >= probe)
      for (int i = 0; i < n; ++i)
        if (fabs(out[i]) > peak)
          peak = fabs(out[i]);
    done += n;
  }
  return peak;
}

/* set all knobs, warm up the smoothed parameters and the envelope */
static void
settle(LV2_Handle h, double sr)
{
  /* 3.5 s of silence: si.smooth(0.999) settles in ~1 s, C8 in ~0.3 s */
  advance(h, (long)(3.5 * sr));
}

static double
gain_at(LV2_Handle h, double sr, double freq, double amp)
{
  int fin;
  sine_rms(h, sr, freq, amp, 0.4, 2, &fin);   /* let the envelope settle */
  double r = sine_rms(h, sr, freq, amp, 0.3, 2, &fin);
  return r / (amp / sqrt(2.0));
}

int
main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "usage: %s <ironlung_dsp.so>\n", argv[0]);
    return 2;
  }
  void *lib = dlopen(argv[1], RTLD_NOW);
  if (!lib) {
    fprintf(stderr, "cannot load %s: %s\n", argv[1], dlerror());
    return 2;
  }
  const LV2_Descriptor *(*dfn)(uint32_t) = NULL;
  *(void **)(&dfn) = dlsym(lib, "lv2_descriptor");
  if (!dfn || !(desc = dfn(0))) {
    fprintf(stderr, "no descriptor\n");
    return 2;
  }
  double sr = 48000.0;
  int fin;

  /* ---- silence / defaults ---- */
  defaults();
  LV2_Handle h = open_plugin(sr);
  if (!h) {
    fprintf(stderr, "instantiate failed (host features?)\n");
    return 2;
  }
  settle(h, sr);
  check(sine_rms(h, sr, 440.0, 0.0, 0.2, 2, &fin) < 1e-6 && fin,
        "silence in -> silence out");

  /* ---- unity at noon / sustain minimum ---- */
  ctrl[SUSTAIN] = 0.0f;
  double g0 = gain_at(h, sr, 440.0, 0.02);
  printf("   default static gain: %+.1f dB\n", db(g0));
  check(fabs(db(g0)) < 2.0, "LEVEL 5, SUSTAIN 0 is unity (within 2 dB)");

  /* ---- sustain raises the static gain ---- */
  ctrl[SUSTAIN] = 10.0f;
  double g10 = gain_at(h, sr, 440.0, 0.002);
  printf("   sustain 10 static gain: %+.1f dB\n", db(g10));
  check(db(g10 / g0) > 15.0 && db(g10 / g0) < 30.0,
        "SUSTAIN 10 raises the static gain by 15..30 dB");

  /* ---- compression ratio and output ceiling ---- */
  ctrl[LEVEL] = 10.0f; ctrl[SUSTAIN] = 8.0f;
  double pk_hi = last_peak(h, sr, 440.0, 0.5, 1.0);
  printf("   440 Hz 0.5 V -> output peak %.3f V\n", pk_hi);
  check(pk_hi < 1.4, "compressed output stays below 1.4 V peak");
  int fin2;
  double r1 = sine_rms(h, sr, 440.0, 0.05, 1.0, 2, &fin2);
  double r2 = sine_rms(h, sr, 440.0, 0.5, 1.0, 2, &fin2);
  double ratio = db(r2 / r1) / db(0.5 / 0.05);
  printf("   compression: 20 dB input -> %+.1f dB output (ratio %.1f:1)\n",
         db(r2 / r1), 1.0 / ratio);
  check(ratio < 0.1, "high-ratio limiting (> 10:1) over 20 dB of input");

  /* ---- TONE is a treble shelf, flat at the centre ---- */
  ctrl[LEVEL] = 5.0f; ctrl[SUSTAIN] = 0.0f; ctrl[TONE] = 5.0f;
  double hf_flat = gain_at(h, sr, 4000.0, 0.005);
  double lf_flat = gain_at(h, sr, 200.0, 0.005);
  check(fabs(db(hf_flat / lf_flat)) < 3.0, "TONE centre is flat (within 3 dB)");
  ctrl[TONE] = 0.0f;
  double hf_cut = gain_at(h, sr, 4000.0, 0.005);
  ctrl[TONE] = 10.0f;
  double hf_boost = gain_at(h, sr, 4000.0, 0.005);
  printf("   TONE 4 kHz: min %+.1f dB, max %+.1f dB (range %.1f dB)\n",
         db(hf_cut / hf_flat), db(hf_boost / hf_flat), db(hf_boost / hf_cut));
  check(db(hf_boost / hf_cut) > 8.0, "TONE gives > 8 dB of treble range");
  ctrl[TONE] = 0.0f;
  double lf_cut = gain_at(h, sr, 200.0, 0.005);
  ctrl[TONE] = 10.0f;
  double lf_boost = gain_at(h, sr, 200.0, 0.005);
  check(fabs(db(lf_boost / lf_cut)) < 5.0, "TONE barely moves 200 Hz (< 5 dB)");

  /* ---- ATTACK trades attack against release ---- */
  ctrl[TONE] = 5.0f; ctrl[LEVEL] = 3.0f; ctrl[SUSTAIN] = 6.0f;
  ctrl[ATTACK] = 0.0f;
  double rec_fast_rel = release_probe(h, sr, 0.5);
  ctrl[ATTACK] = 10.0f;
  double rec_slow_rel = release_probe(h, sr, 0.5);
  printf("   probe 0.5 s after a burst: attack 0 -> %.4f, attack 10 -> %.4f\n",
         rec_fast_rel, rec_slow_rel);
  check(db(rec_slow_rel / rec_fast_rel) > 4.0,
        "ATTACK 10 releases faster than ATTACK 0");

  /* ---- sidechain is de-emphasised below 339 Hz ---- */
  ctrl[LEVEL] = 10.0f; ctrl[SUSTAIN] = 8.0f; ctrl[ATTACK] = 5.0f;
  int f1, f2;
  double lo = sine_rms(h, sr, 80.0, 0.3, 1.0, 2, &f1);
  double hi = sine_rms(h, sr, 1280.0, 0.3, 1.0, 2, &f2);
  printf("   0.3 V: 80 Hz out %.3f, 1280 Hz out %.3f (%.1f dB)\n",
         lo, hi, db(lo / hi));
  check(db(lo / hi) > 4.0, "bass is compressed less than treble (> 4 dB)");

  /* ---- VCA variant ---- */
  ctrl[LEVEL] = 5.0f; ctrl[SUSTAIN] = 10.0f; ctrl[VCA] = 0.0f;
  double v0 = gain_at(h, sr, 440.0, 0.002);
  ctrl[VCA] = 1.0f;
  double v1 = gain_at(h, sr, 440.0, 0.002);
  printf("   VCA static gain: 1986 %+.1f dB, later %+.1f dB\n", db(v0), db(v1));
  check(db(v1 / v0) > 0.5 && db(v1 / v0) < 4.0, "later VCA adds 0.5..4 dB");

  /* ---- bypass: exact dry, no click ---- */
  ctrl[SUSTAIN] = 5.0f; ctrl[LEVEL] = 5.0f; ctrl[VCA] = 0.0f;
  enabled = 0.0f;
  advance(h, (long)(0.05 * sr));
  {
    double maxdiff = 0, rms_in = 0, rms_out = 0;
    int nblocks = 20;
    for (int b = 0; b < nblocks; ++b) {
      for (int i = 0; i < BLOCK; ++i, ++g_phase)
        in[i] = (float)(0.05 * sin(2.0 * M_PI * 440.0 * g_phase / sr));
      desc->run(h, BLOCK);
      for (int i = 0; i < BLOCK; ++i) {
        double d = fabs(out[i] - in[i]);
        if (d > maxdiff)
          maxdiff = d;
        rms_in += (double)in[i] * in[i];
        rms_out += (double)out[i] * out[i];
      }
    }
    printf("   bypass: max |out-in| %.2e, rms ratio %.6f\n",
           maxdiff, sqrt(rms_out / rms_in));
    check(maxdiff < 1e-6, "bypass is exact dry");
  }
  /* click: toggle bypass on a running sine. Use SUSTAIN 0 / LEVEL 5 so
     the wet signal is unity and in phase with the dry: the crossfade then
     has nothing to step over. */
  ctrl[SUSTAIN] = 0.0f;
  settle(h, sr);
  {
    double maxstep = 0, prev = 0;
    int blocks = 40;
    /* prime `prev` with a running sample so the first sample is not a step */
    for (int i = 0; i < BLOCK; ++i, ++g_phase)
      in[i] = (float)(0.05 * sin(2.0 * M_PI * 440.0 * g_phase / sr));
    desc->run(h, BLOCK);
    prev = out[BLOCK - 1];
    for (int b = 0; b < blocks; ++b) {
      if (b == 10)
        enabled = 1.0f;
      for (int i = 0; i < BLOCK; ++i, ++g_phase)
        in[i] = (float)(0.05 * sin(2.0 * M_PI * 440.0 * g_phase / sr));
      desc->run(h, BLOCK);
      for (int i = 0; i < BLOCK; ++i) {
        double d = fabs(out[i] - prev);
        if (d > maxstep)
          maxstep = d;
        prev = out[i];
      }
    }
    /* the signal itself steps by 0.0028 per sample at 440 Hz / 48 kHz */
    printf("   bypass switch: max sample step %.4f\n", maxstep);
    check(maxstep < 0.02, "bypass switch is click-free");
    enabled = 1.0f;
  }

  /* ---- random controls, no NaN/Inf ---- */
  {
    int fin3 = 1;
    srand(7);
    for (int k = 0; k < 200; ++k) {
      ctrl[LEVEL] = (float)(rand() % 1001) / 100.0f;
      ctrl[TONE] = (float)(rand() % 1001) / 100.0f;
      ctrl[ATTACK] = (float)(rand() % 1001) / 100.0f;
      ctrl[SUSTAIN] = (float)(rand() % 1001) / 100.0f;
      ctrl[VCA] = (rand() & 1) ? 1.0f : 0.0f;
      for (int b = 0; b < 3; ++b) {
        for (int i = 0; i < BLOCK; ++i, ++g_phase)
          in[i] = (float)(0.3 * sin(2.0 * M_PI * 700.0 * g_phase / sr));
        desc->run(h, BLOCK);
        for (int i = 0; i < BLOCK; ++i)
          if (!isfinite(out[i]))
            fin3 = 0;
      }
    }
    check(fin3, "random-control stress stays finite");
  }

  /* ---- sample rates ---- */
  close_plugin(h);
  {
    double rates[] = { 44100.0, 48000.0, 96000.0, 192000.0 };
    for (int k = 0; k < 4; ++k) {
      defaults();
      ctrl[SUSTAIN] = 0.0f;
      LV2_Handle hh = open_plugin(rates[k]);
      if (!hh) {
        check(0, "instantiate at a high sample rate");
        continue;
      }
      int fin4;
      sine_rms(hh, rates[k], 440.0, 0.02, 2.5, 1, &fin4);
      double g = sine_rms(hh, rates[k], 440.0, 0.02, 0.5, 2, &fin4) / (0.02 / sqrt(2.0));
      printf("   %.0f Hz: unity %+.1f dB\n", rates[k], db(g));
      check(fin4 && fabs(db(g)) < 2.5, "unity gain at this sample rate");
      close_plugin(hh);
    }
  }

  printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASS",
         failures, failures == 1 ? "" : "s");
  dlclose(lib);
  return failures ? 1 : 0;
}
