#pragma once
// ---------------------------------------------------------------------------
//  AjustesDsp - ajustes do painel que os demoduladores leem a cada bloco
//  (AGC SLOW/MID/FAST/OFF, corte baixo do SSB/AM, tom do CW).
//  Atomics: a tela escreve, a thread do dongle le, sem trava.
// ---------------------------------------------------------------------------
#include <atomic>
#include <cmath>

namespace masdr {

struct AjustesDsp {
    std::atomic<int>   agc{2};          // 0 OFF, 1 SLOW, 2 MID, 3 FAST
    std::atomic<int>   loCutHz{250};    // corte baixo do audio (SSB / AM)
    std::atomic<float> cwTomHz{700.f};  // CW PITCH

    // MB1SDR: o nivel do canal que se OUVE (amplitude antes do AGC, no demodulador): o maior valor
    // desde a ultima leitura. O S-meter do receptor ouvido anda com isto - acompanha o audio, e nao so o espectro.
    std::atomic<float> picoCanal{0.f};

    static AjustesDsp& g() { static AjustesDsp a; return a; }

    // MB1SDR (dois aparelhos): cada aparelho tem a sua thread; so a do receptor ouvido marca o canal
    static bool& mede() { thread_local bool m = true; return m; }
    static void canal(float v) {
        if (!mede()) return;
        std::atomic<float>& p = g().picoCanal;
        float o = p.load(std::memory_order_relaxed);
        while (v > o && !p.compare_exchange_weak(o, v, std::memory_order_relaxed)) {}
    }
    static float lerCanal() { return g().picoCanal.exchange(0.f, std::memory_order_relaxed); }

    // Coeficiente de liberacao do AGC por amostra, para a taxa fs.
    // MID = ~1 s (o valor fixo de antes), SLOW = 3 s, FAST = 0,15 s.
    static float liberacao(float fs, float lentoS = 3.0f, float medioS = 1.0f, float rapidoS = 0.15f) {
        const int m = g().agc.load();
        const float tau = m == 1 ? lentoS : m == 3 ? rapidoS : medioS;
        return 1.f - std::exp(-1.f / (tau * (fs > 1.f ? fs : 48000.f)));
    }
};

} // namespace masdr
