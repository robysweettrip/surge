/*
** Surge Synthesizer is Free and Open Source Software
**
** Surge is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html
**
** Copyright 2004-2020 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Surge was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Surge
** open source in September 2018.
*/

#pragma once

#include <string>
#include <locale>
#include <fmt/format.h>

#include "globals.h"
#include "UnitConversions.h"
#include <vembertech/basic_dsp.h>
#include <functional>
#include "StringOps.h"

#define setzero(x) memset(x, 0, sizeof(*x))

class quadr_osc
{
  public:
    quadr_osc()
    {
        r = 0;
        i = -1;
    }

    inline void set_rate(float w)
    {
        dr = cos(w);
        di = sin(w);

        // normalize vector
        double n = 1 / sqrt(r * r + i * i);
        r *= n;
        i *= n;
    }

    inline void set_phase(float w)
    {
        r = sin(w);
        i = -cos(w);
    }

    inline void process()
    {
        float lr = r, li = i;
        r = dr * lr - di * li;
        i = dr * li + di * lr;
    }

  public:
    float r, i;

  private:
    float dr, di;
};

template <class T, bool first_run_checks = true> class lipol
{
  public:
    lipol() { reset(); }

    void reset()
    {
        if (first_run_checks)
        {
            first_run = true;
        }

        new_v = 0;
        v = 0;
        dv = 0;

        setBlockSize(BLOCK_SIZE);
    }

    inline void newValue(T f)
    {
        v = new_v;
        new_v = f;

        if (first_run_checks && first_run)
        {
            v = f;
            first_run = false;
        }

        dv = (new_v - v) * bs_inv;
    }

    inline T getTargetValue() { return new_v; }

    inline void instantize()
    {
        v = new_v;
        dv = 0;
    }

    inline void process() { v += dv; }

    void setBlockSize(int n) { bs_inv = 1 / (T)n; }

    T v;
    T new_v;
    T dv;

  private:
    T bs_inv;
    bool first_run;
};

template <class T, bool first_run_checks = true> class lag
{
  public:
    lag(T lp)
    {
        this->lp = lp;
        lpinv = 1 - lp;
        v = 0;
        target_v = 0;

        if (first_run_checks)
        {
            first_run = true;
        }
    }

    lag()
    {
        lp = 0.004;
        lpinv = 1 - lp;
        v = 0;
        target_v = 0;

        if (first_run_checks)
        {
            first_run = true;
        }
    }

    void setRate(T lp)
    {
        this->lp = lp;
        lpinv = 1 - lp;
    }

    inline void newValue(T f)
    {
        target_v = f;

        if (first_run_checks && first_run)
        {
            v = target_v;
            first_run = false;
        }
    }

    inline void startValue(T f)
    {
        target_v = f;
        v = f;

        if (first_run_checks && first_run)
        {
            first_run = false;
        }
    }

    inline void instantize() { v = target_v; }

    inline T getTargetValue() { return target_v; }

    inline void process() { v = v * lpinv + target_v * lp; }

    T v;
    T target_v;

  private:
    bool first_run;
    T lp, lpinv;
};

inline void flush_denormal(double &d)
{
    if (fabs(d) < 1E-30)
    {
        d = 0;
    }
}

inline bool within_range(int lo, int value, int hi) { return ((value >= lo) && (value <= hi)); }

inline float lerp(float a, float b, float x) { return (1 - x) * a + x * b; }

inline float cos_ipol(float y1, float y2, float mu)
{
    float mu2;

    mu2 = (1.f - cos(mu * M_PI)) * 0.5;

    return (y1 * (1.f - mu2) + y2 * mu2);
}

inline float cubic_ipol(float y0, float y1, float y2, float y3, float mu)
{
    float a0, a1, a2, a3, mu2;

    mu2 = mu * mu;
    a0 = y3 - y2 - y0 + y1;
    a1 = y0 - y1 - a0;
    a2 = y2 - y0;
    a3 = y1;

    return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3);
}

inline float quad_spline_ipol(float y0, float y1, float y2, float y3, float mu, int odd)
{
    if (odd)
    {
        mu = 0.5f + mu * 0.5f;
        float f0 = mu * y1 + (1.f - mu) * y0;
        float f1 = mu * y2 + (1.f - mu) * y1;

        return (mu * f1 + (1.f - mu) * f0);
    }
    else
    {
        mu = mu * 0.5f;
        float f0 = mu * y2 + (1.f - mu) * y1;
        float f1 = mu * y3 + (1.f - mu) * y2;

        return (mu * f1 + (1.f - mu) * f0);
    }
}

inline float quad_bspline(float y0, float y1, float y2, float mu)
{
    return 0.5f * (y2 * (mu * mu) + y1 * (-2 * mu * mu + 2 * mu + 1) + y0 * (mu * mu - 2 * mu + 1));
}

// panning which always lets both channels through unattenuated (separate hard-panning)
inline void trixpan(float &L, float &R, float x)
{
    if (x < 0.f)
    {
        L = L - x * R;
        R = (1.f + x) * R;
    }
    else
    {
        R = x * L + R;
        L = (1.f - x) * L;
    }
}

inline double tanh_fast(double in)
{
    double x = fabs(in);
    const double a = 2 / 3;
    double xx = x * x;
    double denom = 1 + x + xx + a * x * xx;

    return ((in > 0) ? 1 : -1) * (1 - 1 / denom);
}

inline float tanh_fast(float in)
{
    const float a = 2 / 3;
    float x = fabs(in);
    float xx = x * x;
    float denom = 1 + x + xx + a * x * xx;

    _mm_store_ss(&denom, _mm_rcp_ss(_mm_load_ss(&denom)));

    return ((in > 0.f) ? 1.f : -1.f) * (1.f - denom);
}

inline double tanh_faster1(double x)
{
    const double a = -1 / 3, b = 2 / 15;
    double xs = x * x;
    double y = 1 + xs * a + xs * xs * b;

    return y * x;
}

inline float clamp01(float in)
{
    if (in > 1.0f)
    {
        return 1.0f;
    }

    if (in < 0.0f)
    {
        return 0.0f;
    }

    return in;
}

inline float clamp1bp(float in)
{
    if (in > 1.0f)
    {
        return 1.0f;
    }

    if (in < -1.0f)
    {
        return -1.0f;
    }

    return in;
}

// Use custom format (x^3) to represent gain internally, but save as decibel in XML-data
inline float amp_to_linear(float x)
{
    x = std::max(0.f, x);

    return x * x * x;
}

inline float linear_to_amp(float x) { return powf(limit_range(x, 0.0000000001f, 1.f), 1.f / 3.f); }

inline float amp_to_db(float x) { return limit_range((float)(18.f * log2(x)), -192.f, 96.f); }

inline float db_to_amp(float x) { return limit_range(powf(2.f, x / 18.f), 0.f, 2.f); }

inline double sincf(double x)
{
    if (x == 0)
    {
        return 1;
    }

    return (sin(M_PI * x)) / (M_PI * x);
}

inline double sinc(double x)
{
    if (fabs(x) < 0.0000000000000000000001)
    {
        return 1.0;
    }

    return (sin(x) / x);
}

inline double blackman(int i, int n)
{
    return (0.42 - 0.5 * cos(2 * M_PI * i / (n - 1)) + 0.08 * cos(4 * M_PI * i / (n - 1)));
}

inline double symmetric_blackman(double i, int n)
{
    i -= (n / 2);

    return (0.42 - 0.5 * cos(2 * M_PI * i / (n)) + 0.08 * cos(4 * M_PI * i / (n)));
}

inline double blackman(double i, int n)
{
    return (0.42 - 0.5 * cos(2 * M_PI * i / (n - 1)) + 0.08 * cos(4 * M_PI * i / (n - 1)));
}

inline double blackman_harris(int i, int n)
{
    return (0.35875 - 0.48829 * cos(2 * M_PI * i / (n - 1)) +
            0.14128 * cos(4 * M_PI * i / (n - 1)) - 0.01168 * cos(6 * M_PI * i / (n - 1)));
}

inline double symmetric_blackman_harris(double i, int n)
{
    i -= (n / 2);

    return (0.35875 - 0.48829 * cos(2 * M_PI * i / (n)) + 0.14128 * cos(4 * M_PI * i / (n - 1)) -
            0.01168 * cos(6 * M_PI * i / (n)));
}

inline double blackman_harris(double i, int n)
{
    return (0.35875 - 0.48829 * cos(2 * M_PI * i / (n - 1)) +
            0.14128 * cos(4 * M_PI * i / (n - 1)) - 0.01168 * cos(6 * M_PI * i / (n - 1)));
}

inline double hanning(int i, int n)
{
    if (i >= n)
    {
        return 0;
    }

    return 0.5 * (1 - cos(2 * M_PI * i / (n - 1)));
}

inline double hamming(int i, int n)
{
    if (i >= n)
    {
        return 0;
    }

    return 0.54 - 0.46 * cos(2 * M_PI * i / (n - 1));
}

// We use this method when streamimg to a patch to make sure
// floating point values always use dot as a decimal separator!
inline std::string float_to_clocalestr(float value)
{
    return fmt::format(std::locale::classic(), "{:L}", value);
}

float correlated_noise(float lastval, float correlation);
float correlated_noise_mk2(float &lastval, float correlation);
float drift_noise(float &lastval);
float correlated_noise_o2(float lastval, float &lastval2, float correlation);
float correlated_noise_o2mk2(float &lastval, float &lastval2, float correlation);
// alternative version where you supply a uniform RNG on [-1, 1] externally
float correlated_noise_o2mk2_suppliedrng(float &lastval, float &lastval2, float correlation,
                                         std::function<float()> &urng);
class SurgeStorage;
float correlated_noise_o2mk2_storagerng(float &lastval, float &lastval2, float correlation,
                                        SurgeStorage *storage);