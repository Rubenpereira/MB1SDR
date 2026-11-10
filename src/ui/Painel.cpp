// ---------------------------------------------------------------------------
//  Painel do MB1SDR
//
//  A tela inteira e o painel frontal de um transceptor de HF de mesa no
//  estilo do MB1 (sem a marca do fabricante), medido na foto do radio
//  (mb1.jpg): os dois S-meters analogicos (iluminados em ambar), o VFD
//  com as duas frequencias em 7 segmentos, a tela TFT com o escopo no lugar do
//  mapa-mundi, os botoes do MAIN (METER ... NB-SQL, CONT-DNR, VRF-NTCH), as
//  teclas de MODE / QMB em volta do dial, a fileira do SUB (B) embaixo, os
//  AF GAIN-RF GAIN / SHIFT-WIDTH / CLAR-VFO-B da direita e a tecla CACHOEIRA no
//  lugar do cartao de memoria. As funcoes ft9*() estao no fim deste arquivo.
//  (As funcoes dos paineis antigos - FTdxSDR e IC7610SDR - continuam no arquivo
//  mas nao sao chamadas.)
//
//  Tudo e desenhado aqui (sem imagem), num "papel" de 1500 x 532 unidades
//  que cresce e encolhe com a janela - os botoes ficam sempre no lugar.
//  Cada botao faz a funcao do radio; os de transmissao, que num receptor nao
//  tem sentido, ganharam funcoes de recepcao (a dica de cada um diz qual).
// ---------------------------------------------------------------------------
#include "Ui.h"

#include "../app/Config.h"
#include "../dsp/AjustesDsp.h"
#include "../util/Logger.h"
#include "Idioma.h"
#include "Recursos.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

#include <windows.h>
#include <mmsystem.h>
#include <sapi.h>
#include <shellapi.h>

#include <algorithm>
#include <atomic>
#include <cfloat>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace masdr {

extern int g_paletaCachoeira;   // Ui.cpp
extern bool g_pedirCaptura;     // main.cpp (F12: captura da tela)
extern bool g_fundoChave;       // main.cpp: fundo transparente (cor-chave magenta)
extern float g_papelAlto;       // main.cpp: altura do papel (radio + pes + cachoeira) p/ a proporcao
extern std::string g_ultimaCaptura;

namespace {

constexpr float kLW = 1500.f, kLH = 637.f;   // MB1SDR: a tampa (24) + a frente do MB1 (613), foto endireitada 1500 px
constexpr float kPe = 30.f;
constexpr float kCach = 300.f;                 // a cachoeira em cima do radio (papel), quando ligada                    // os pes do radio, embaixo do painel (papel)     // tamanho do "papel" do painel
constexpr double kPi = 3.14159265358979323846;

// Cores do painel (preto do FTDX, tela TFT azul)
constexpr ImU32 P_CHASSI   = IM_COL32(0x13, 0x1b, 0x1f, 255);
constexpr ImU32 P_CHASSI2  = IM_COL32(0x08, 0x0e, 0x11, 255);
constexpr ImU32 P_ROTULO   = IM_COL32(0xe6, 0xe8, 0xec, 255);
constexpr ImU32 P_ROTULO2  = IM_COL32(0xa4, 0xa8, 0xb0, 255);
constexpr ImU32 P_LINHA    = IM_COL32(0x3a, 0x3d, 0x42, 255);
constexpr ImU32 P_LCD      = IM_COL32(0x02, 0x03, 0x08, 255);
constexpr ImU32 P_AMBAR    = IM_COL32(0xff, 0xb0, 0x30, 255);
constexpr ImU32 P_LARANJA  = IM_COL32(0xff, 0x8a, 0x1a, 255);
constexpr ImU32 P_VERDE    = IM_COL32(0x3c, 0xff, 0x6e, 255);
constexpr ImU32 P_CIANO    = IM_COL32(0x40, 0xc8, 0xff, 255);
constexpr ImU32 P_AMARELO  = IM_COL32(0xff, 0xe0, 0x30, 255);
constexpr ImU32 P_VERMELHO = IM_COL32(0xff, 0x3a, 0x30, 255);
constexpr ImU32 P_BRANCO   = IM_COL32(0xf4, 0xf6, 0xfa, 255);
constexpr ImU32 P_APAGADO  = IM_COL32(0x60, 0x66, 0x70, 255);
constexpr ImU32 P_AZULSUB  = IM_COL32(0x74, 0xd4, 0xff, 255);   // rotulos do SUB (ciano claro, como na foto)
constexpr ImU32 P_TFTAZUL  = IM_COL32(0x1e, 0x46, 0xa8, 255);   // faixas de titulo da tela
constexpr ImU32 P_TFTTXT   = IM_COL32(0x9c, 0xc8, 0xff, 255);   // texto azul-claro da tela
constexpr ImU32 P_TFTCAB   = IM_COL32(0xff, 0x5e, 0x3a, 255);   // cabecalho das celulas (ANT, ATT...)
constexpr ImU32 P_MODO     = IM_COL32(0xe0, 0x48, 0x2c, 255);   // etiqueta do modo (LSB, USB...)
constexpr ImU32 P_DOURADO  = IM_COL32(0xe0, 0xb0, 0x48, 255);

// cor dos rotulos em cima das teclas / botoes (o SUB e azul)
ImU32 g_corRot = IM_COL32(0xf4, 0xf4, 0xf4, 255);

// "Papel" -> tela
struct Mapa {
    float ox = 0, oy = 0, k = 1;
    ImVec2 p(float x, float y) const { return ImVec2(ox + x * k, oy + y * k); }
    float d(float v) const { return v * k; }
};
Mapa M;

// Fontes em varios tamanhos (Fontes::painelT / lcdT) e o aumento dos textos
// pequenos em tela pequena (num notebook de 1366 x 768 os rotulos do painel
// ficariam com 5 px - ilegiveis)
ImFont* g_fp[10] = {};
ImFont* g_fl[12] = {};
float g_txt = 1.f;

float tamReal(float tam) { return tam < 12.f ? std::min(tam * g_txt, std::max(tam, 14.f)) : tam; }
// aperto horizontal do texto (1 = normal): os rotulos do painel usam a letra estreita da foto
float g_cond = 1.f;

ImFont* melhor(ImFont* f, float px)
{
    ImFont** fam = nullptr; int n = 0;
    if (f && f == g_fp[9]) { fam = g_fp; n = 10; }
    else if (f && f == g_fl[11]) { fam = g_fl; n = 12; }
    if (!fam) return f;
    for (int i = 0; i < n; ++i) if (fam[i] && fam[i]->FontSize >= px * 0.96f) return fam[i];
    return fam[n - 1];
}

double agora()
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

ImU32 alfa(ImU32 c, int a) { return (c & ~IM_COL32_A_MASK) | ((ImU32)std::clamp(a, 0, 255) << IM_COL32_A_SHIFT); }

// texto num ponto do papel; alin 0 = esquerda, 0.5 = centro, 1 = direita
void texto(ImFont* f, float tam, float x, float y, ImU32 cor, const char* s, float alin = 0.f)
{
    if (!f || !s) return;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float px = M.d(tamReal(tam));
    const bool doPainel = f == g_fp[9];
    f = melhor(f, px);
    const ImVec2 ts = f->CalcTextSizeA(px, FLT_MAX, 0, s);
    const ImVec2 q = M.p(x, y);
    const ImVec2 pos(std::floor(q.x - ts.x * alin), std::floor(q.y));
    const int v0 = dl->VtxBuffer.Size;
    if (g_cond != 1.f) {     // o ImGui corta as letras pela largura sem aperto: abre o corte na horizontal
        const ImVec2 c0 = dl->GetClipRectMin(), c1 = dl->GetClipRectMax();
        dl->PushClipRect(ImVec2(c0.x - 4000.f, c0.y), ImVec2(c1.x + 4000.f, c1.y), false);
    }
    // sombra escura atras dos rotulos claros: le-se melhor em cima do cinza
    if (doPainel && ((cor >> IM_COL32_A_SHIFT) & 0xff) > 200 && (((cor >> IM_COL32_R_SHIFT) & 0xff) + ((cor >> IM_COL32_G_SHIFT) & 0xff)) > 300)
        dl->AddText(f, px, ImVec2(pos.x + 1, pos.y + 1), IM_COL32(0, 0, 0, 200), s);
    dl->AddText(f, px, pos, cor, s);
    if (g_cond != 1.f) {     // letra apertada na horizontal, em volta do ponto de alinhamento
        for (int i = v0; i < dl->VtxBuffer.Size; ++i) dl->VtxBuffer[i].pos.x = q.x + (dl->VtxBuffer[i].pos.x - q.x) * g_cond;
        dl->PopClipRect();
    }
}
float larguraTexto(ImFont* f, float tam, const char* s)
{
    const float px = M.d(tamReal(tam));
    return melhor(f, px)->CalcTextSizeA(px, FLT_MAX, 0, s).x / M.k * g_cond;
}

// Rotulo do painel como os da foto: letra grande e apertada (0,77). yc = meio da
// altura da letra (papel); maxW > 0 encolhe para caber.
constexpr float ROT_COND = 0.77f;
void rotulo(float tam, float x, float yc, ImU32 cor, const char* s, float alin, float maxW = 0.f)
{
    g_cond = ROT_COND;
    for (int i = 0; i < 12 && maxW > 0.f && tam > 5.f && larguraTexto(g_fp[9], tam, s) > maxW; ++i) tam *= 0.93f;
    texto(g_fp[9], tam, x, yc - tamReal(tam) * 0.548f, cor, s, alin);
    g_cond = 1.f;
}
float larguraRotulo(float tam, const char* s)
{
    g_cond = ROT_COND;
    const float w = larguraTexto(g_fp[9], tam, s);
    g_cond = 1.f;
    return w;
}

// texto de varias linhas centrado numa caixa do papel
void textoCaixa(ImFont* f, float tam, float x, float y, float w, float h, ImU32 cor, const char* s)
{
    std::vector<std::string> linhas;
    std::string a;
    for (const char* c = s; ; ++c) {
        if (*c == '\n' || *c == 0) { linhas.push_back(a); a.clear(); if (!*c) break; }
        else a += *c;
    }
    const float lh = tamReal(tam) * 1.05f;
    float yy = y + (h - lh * linhas.size()) * 0.5f;
    for (auto& l : linhas) { texto(f, tam, x + w * 0.5f, yy, cor, l.c_str(), 0.5f); yy += lh; }
}

void textoCabe(ImFont* f, float tam, float x, float y, ImU32 cor, const char* s, float alin, float maxW)
{
    for (int i = 0; i < 10 && tam > 4.f && larguraTexto(f, tam, s) > maxW; ++i) tam *= 0.92f;
    texto(f, tam, x, y, cor, s, alin);
}

ImVec2 dirAng(float a) { return ImVec2(std::cos(a), std::sin(a)); }
float angKnob(float t) { return (float)((135.0 + 270.0 * std::clamp(t, 0.f, 1.f)) * kPi / 180.0); }

std::string fmtPasso(int p)
{
    char r[24];
    if (p >= 1000) std::snprintf(r, sizeof r, (p % 1000) ? "%.2f kHz" : "%.0f kHz", p / 1000.0);
    else std::snprintf(r, sizeof r, "%d Hz", p);
    return r;
}

// Estados por controle (angulo do arrasto, toque longo ja disparado)
std::unordered_map<ImGuiID, float>& angArrasto() { static std::unordered_map<ImGuiID, float> m; return m; }
std::unordered_map<ImGuiID, int>& longoFeito() { static std::unordered_map<ImGuiID, int> m; return m; }
std::unordered_map<ImGuiID, int>& parteGiro() { static std::unordered_map<ImGuiID, int> m; return m; }

const char* kNomesFil[3] = {"A", "B", "C"};

// As 12 teclas de banda do radio: 1.8 ... 50 e 70/GEN
constexpr int kNB = 12;
struct BandaHam { const char* rot; uint64_t lo, hi, padrao; const char* modo; };
const BandaHam kBandasHam[kNB] = {
    {"1.8",    1800000,  2000000,  1840000, "LSB"},
    {"3.5",    3500000,  4000000,  3710000, "LSB"},
    {"5.0",    5250000,  5450000,  5357000, "USB"},
    {"7.0",    7000000,  7300000,  7100000, "LSB"},
    {"10",    10100000, 10150000, 10120000, "CW"},
    {"14",    14000000, 14350000, 14200000, "USB"},
    {"18",    18068000, 18168000, 18130000, "USB"},
    {"21",    21000000, 21450000, 21250000, "USB"},
    {"24.5",  24890000, 24990000, 24950000, "USB"},
    {"28/29", 28000000, 29700000, 28500000, "USB"},
    {"50",    50000000, 54000000, 50125000, "USB"},
    {"70/GEN",70000000, 70500000, 70200000, "USB"},
};

struct ItemGen { const char* grupo; uint64_t hz; const char* nome; const char* modo; };
const ItemGen kGen1[] = {
    {"Radiodifusão - ondas longas e médias", 198000, "OL - ondas longas", "AM"},
    {nullptr, 1000000, "OM - ondas médias", "AM"},
    {"Radiodifusão - ondas curtas (SWL)", 2400000, "120 m", "AM"},
    {nullptr, 3300000, "90 m", "AM"},
    {nullptr, 3950000, "75 m", "AM"},
    {nullptr, 4900000, "60 m", "AM"},
    {nullptr, 6050000, "49 m", "AM"},
    {nullptr, 7300000, "41 m", "AM"},
    {nullptr, 9650000, "31 m", "AM"},
    {nullptr, 11850000, "25 m", "AM"},
    {nullptr, 13700000, "22 m", "AM"},
    {nullptr, 15400000, "19 m", "AM"},
    {nullptr, 17700000, "16 m", "AM"},
    {nullptr, 18950000, "15 m", "AM"},
    {nullptr, 21600000, "13 m", "AM"},
    {nullptr, 25850000, "11 m", "AM"},
    {"Utilitárias", 5000000, "Hora padrão (5 MHz)", "AM"},
    {nullptr, 10000000, "Hora padrão (10 MHz)", "AM"},
    {nullptr, 6604000, "VOLMET New York (6604 kHz)", "USB"},
    {nullptr, 8414500, "DSC marítimo (8414,5 kHz)", "USB"},
    {nullptr, 27455000, "Faixa do Cidadão, canal 45", "AM"},
};
const ItemGen kGen2[] = {
    {"VHF", 101700000, "FM - radiodifusão (88 a 108)", "WFM"},
    {nullptr, 119450000, "Aviação (118 a 137)", "AM"},
    {nullptr, 144390000, "APRS (144,390)", "NFM"},
    {nullptr, 145570000, "APRS Brasil (145,570)", "NFM"},
    {nullptr, 146000000, "2 m (144 a 148)", "NFM"},
    {nullptr, 156800000, "Marítimo - canal 16", "NFM"},
    {nullptr, 162000000, "AIS (162,000)", "NFM"},
    {"UHF", 261620000, "Satélite militar (UHF)", "NFM"},
    {nullptr, 439400000, "70 cm (430 a 440)", "NFM"},
    {nullptr, 446006250, "PMR 446 - canal 1", "NFM"},
    {nullptr, 462562500, "FRS 462 MHz", "NFM"},
};

// VOICE: fala a frequencia pela voz do Windows (SAPI - existe do 7 ao 11)
std::atomic<bool> g_falando{false};
void falar(const std::wstring& txt)
{
    if (g_falando.exchange(true)) return;
    std::thread([txt] {
        if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
            static const CLSID kVoz = {0x96749377, 0x3391, 0x11D2, {0x9E, 0xE3, 0x00, 0xC0, 0x4F, 0x79, 0x73, 0x96}};
            static const IID kIVoz = {0x6C44DF74, 0x72B9, 0x4992, {0xA1, 0xEC, 0xEF, 0x99, 0x6E, 0x04, 0x22, 0xD4}};
            ISpVoice* v = nullptr;
            if (SUCCEEDED(CoCreateInstance(kVoz, nullptr, CLSCTX_ALL, kIVoz, (void**)&v)) && v) {
                v->Speak(txt.c_str(), 0, nullptr);
                v->Release();
            }
            CoUninitialize();
        }
        g_falando = false;
    }).detach();
}

std::string pastaExe()
{
    char p[MAX_PATH]{};
    GetModuleFileNameA(nullptr, p, MAX_PATH);
    std::string d(p);
    const auto pos = d.find_last_of("\\/");
    return pos == std::string::npos ? std::string(".") : d.substr(0, pos);
}

// mesma curva do ponteiro do Ui.cpp (dB calibrado -> posicao 0..1 da escala)
float dbParaPont(float db)
{
    static const float pts[10][2] = {
        {-110, 0.05f}, {-45, 0.08f}, {-20, 0.145f}, {-14, 0.285f}, {-9, 0.450f},
        {-6, 0.605f}, {-3.5f, 0.675f}, {-1.5f, 0.785f}, {-0.5f, 0.895f}, {0, 0.960f}};
    if (db <= pts[0][0]) return pts[0][1];
    if (db >= pts[9][0]) return pts[9][1];
    for (int i = 0; i < 9; ++i)
        if (db >= pts[i][0] && db <= pts[i + 1][0]) {
            const float f = (db - pts[i][0]) / (pts[i + 1][0] - pts[i][0]);
            return pts[i][1] + f * (pts[i + 1][1] - pts[i][1]);
        }
    return 0.05f;
}

// posicoes das marcas do S-meter (as mesmas do ponteiro)
const float kSPos[10] = {0.06f, 0.117f, 0.17f, 0.224f, 0.282f, 0.34f, 0.392f, 0.444f, 0.495f, 0.546f};
const float kDbPos[6] = {0.62f, 0.693f, 0.77f, 0.845f, 0.92f, 0.996f};

std::string leituraS(float p)
{
    char b[24];
    if (p < 0.546f) { int k = 0; for (int i = 0; i < 10; ++i) if (p >= kSPos[i]) k = i; std::snprintf(b, sizeof b, "S%d", k); }
    else std::snprintf(b, sizeof b, "S9+%d", (int)std::lround((p - 0.546f) / (0.996f - 0.546f) * 60.f / 5.f) * 5);
    return b;
}

// cor do 3DSS: azul (fraco) -> violeta -> magenta -> vermelho -> branco (forte)
ImU32 corNivel3d(float v, int a)
{
    v = std::clamp(v, 0.f, 1.f);
    static const float p[6][4] = {
        {0.00f, 0x08, 0x18, 0x60}, {0.35f, 0x20, 0x40, 0xd8}, {0.55f, 0x90, 0x40, 0xff},
        {0.72f, 0xff, 0x40, 0xd0}, {0.86f, 0xff, 0x30, 0x30}, {1.00f, 0xff, 0xf0, 0xd0}};
    for (int i = 0; i < 5; ++i)
        if (v <= p[i + 1][0]) {
            const float t = (v - p[i][0]) / (p[i + 1][0] - p[i][0]);
            return IM_COL32((int)(p[i][1] + t * (p[i + 1][1] - p[i][1])), (int)(p[i][2] + t * (p[i + 1][2] - p[i][2])),
                            (int)(p[i][3] + t * (p[i + 1][3] - p[i][3])), a);
        }
    return IM_COL32(255, 240, 208, a);
}

// ---------------------------------------------------------------------------
// Posicoes medidas na foto do radio (metade da direita vista de frente,
// 1200 x 756 px) convertidas para o papel: a faixa de bandas comeca em
// x 75 / y 108 da foto e vira x 702 / y 34 do papel.
// ---------------------------------------------------------------------------
inline float FX(float x) { return 702.f + (x - 75.f) * 0.715f; }
inline float FY(float y) { return 34.f + (y - 108.f) * 0.745f; }
inline float FW(float w) { return w * 0.715f; }
inline float FH(float h) { return h * 0.745f; }

// como a tecla desenha o LED: 0 dentro (em cima), 1 janelinha acima da tecla
// (SYNC, SPLIT, FAST, LOCK), 2 barra em cima da borda (RX / TX)
int g_ledFora = 0;
// borracha das teclas sem texto: 0 = pontinhos e oval (NB, DNR, BK-IN...),
// 1 = so pontinhos (MODE, SSB, CW)
int g_borracha = 0;
// tom da face das teclas: 0 normal, 1 clara (a fileira RX / TX pega a luz na foto)
int g_teclaTom = 0;

inline float rad(float g) { return g * (float)kPi / 180.f; }
// "sino" em volta de um angulo (radianos): 1 no centro, cai com a largura
inline float sino(float a, float centro, float larg)
{
    float d = a - centro;
    while (d > kPi) d -= (float)(2 * kPi);
    while (d < -kPi) d += (float)(2 * kPi);
    return std::exp(-0.5f * (d / larg) * (d / larg));
}
// cinza da foto: o escuro puxa para o petroleo (verde-azulado) e o claro fica
// quase neutro, como o aluminio. v = brilho 0..1
ImU32 cinzaFrio(float v, int a = 255)
{
    v = std::clamp(v, 0.f, 1.f);
    const float r = 255.f * v * (0.55f + 0.42f * std::sqrt(v));
    const float g = 255.f * v * (0.93f + 0.05f * v) + 1.f;
    const float b = 255.f * v + 3.f * (1.f - v);
    return IM_COL32((int)std::min(255.f, r), (int)std::min(255.f, g), (int)std::min(255.f, b), a);
}

// A luz do estudio vem de cima, um pouco para a esquerda. Brilho de cada peca do
// dial por angulo, MEDIDO NA FOTO (raio a raio, em volta do centro do anel):
// 36 valores 0..255, um a cada 10 graus a partir de -180 (a esquerda).
// Angulos da tela: -90 = em cima, 0 = a direita, 90 = embaixo.
const unsigned char T_ANEL[36]  = { 51, 82,150,125,118,156,178,203,228,205,125, 72, 54, 47, 51, 66, 58, 33,
                                    27, 18, 10,  7,  6,  8, 10, 12, 16, 21, 25, 30, 38, 43, 36, 24, 18, 32 };  // anel MPVD
const unsigned char T_LINHA[36] = {175,213,238,240,249,253,254,254,254,254,250,229,195,130,125,160,125, 90,
                                   100, 72, 48, 55, 35, 45, 42, 36, 38, 40, 38, 45, 70, 74, 70, 95,118,138 };  // filete prateado
const unsigned char T_SEGM[36]  = {100,160,215,200,155,125,148,195,228,205,155,102, 84,108,170,205,205,175,
                                   105, 72, 55, 58, 40, 62, 58, 62, 88, 80, 78,105,108, 98, 76, 40, 40, 62 };  // tracos do anel
const unsigned char T_CHANF[36] = {156,194,221,236,248,253,253,254,254,254,252,234,214,181,150,143,142,134,
                                   124,106, 98, 74, 45, 46, 46, 36, 37, 37, 41, 48, 72, 74, 65, 72,100,128 };  // chanfro da face
const unsigned char T_ABA[36]   = {  8, 12, 24, 30, 38, 66, 88,100,150,122, 55, 26, 18, 14, 14, 20, 20, 16,
                                    14, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 10,  8 };  // aba lisa do pe do dial
const unsigned char T_SERR[36]  = { 38, 42, 47, 52, 57, 63, 67, 72, 76, 76, 67, 62, 57, 54, 50, 44, 40, 36,
                                    34, 32, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 32, 34, 35, 36 };  // serrilhado do dial
const unsigned char T_LABIO[36] = { 58, 85,150,130, 95, 72, 95,145,170,190, 95, 68, 56, 68,115,175,140,118,
                                    55, 35, 22, 18, 22, 36, 38, 42, 48, 52, 50, 56, 62, 70, 56, 28, 22, 30 };  // labio da gola
// brilho (0..1) de uma tabela no angulo a (radianos, qualquer volta)
float tabela(const unsigned char* t, float a)
{
    float g = std::fmod(a * 180.f / (float)kPi + 540.f, 360.f);
    if (g < 0.f) g += 360.f;
    const float x = g / 10.f;
    int i = (int)x;
    const float f = x - (float)i;
    i %= 36;
    return (t[i] * (1.f - f) + t[(i + 1) % 36] * f) / 255.f;
}
float luzAbaDial(float a)  { return tabela(T_ABA, a); }
float luzSerrDial(float a) { return tabela(T_SERR, a); }
// botoes pequenos (medidos no SHIFT/WIDTH da foto): o chanfro do alto do botao de
// fora brilha forte em cima, a esquerda; o serrilhado e escuro
float luzBisel(float a) { return 0.05f + 1.00f * sino(a, rad(-128), rad(36)) + 0.16f * sino(a, rad(-72), rad(18)) + 0.06f * sino(a, rad(50), rad(25)); }
float luzSerrK(float a) { return 0.04f + 0.24f * sino(a, rad(-122), rad(40)); }
float luzAbaK(float a)  { return 0.03f + 0.08f * sino(a, rad(-120), rad(40)); }
// face escovada (torneado): os "raios" de brilho que nao giram com o dial
float luzFace(float a)   { return 0.045f + 0.46f * sino(a, rad(-162), rad(8)) + 0.36f * sino(a, rad(18), rad(9))
                                 + 0.22f * sino(a, rad(-45), rad(12)) + 0.15f * sino(a, rad(135), rad(13)); }

// Anel (ou pedaco de anel, de a0 a a1) com cor por vertice: degrade liso, sem
// emendas. cor(angulo, raio) devolve a cor daquele ponto. Pixels da tela.
template <class F>
void anelLuz(ImDrawList* dl, ImVec2 c, float r0, float r1, float a0, float a1, int n, F cor)
{
    const ImVec2 uv = dl->_Data->TexUvWhitePixel;
    dl->PrimReserve(n * 6, n * 4);
    for (int k = 0; k < n; ++k) {
        const float b0 = a0 + (a1 - a0) * k / n, b1 = a0 + (a1 - a0) * (k + 1) / n;
        const float c0 = std::cos(b0), s0 = std::sin(b0), c1 = std::cos(b1), s1 = std::sin(b1);
        const ImDrawIdx i0 = (ImDrawIdx)dl->_VtxCurrentIdx;
        dl->PrimWriteVtx(ImVec2(c.x + c0 * r0, c.y + s0 * r0), uv, cor(b0, r0));
        dl->PrimWriteVtx(ImVec2(c.x + c0 * r1, c.y + s0 * r1), uv, cor(b0, r1));
        dl->PrimWriteVtx(ImVec2(c.x + c1 * r1, c.y + s1 * r1), uv, cor(b1, r1));
        dl->PrimWriteVtx(ImVec2(c.x + c1 * r0, c.y + s1 * r0), uv, cor(b1, r0));
        dl->PrimWriteIdx(i0); dl->PrimWriteIdx((ImDrawIdx)(i0 + 1)); dl->PrimWriteIdx((ImDrawIdx)(i0 + 2));
        dl->PrimWriteIdx(i0); dl->PrimWriteIdx((ImDrawIdx)(i0 + 2)); dl->PrimWriteIdx((ImDrawIdx)(i0 + 3));
    }
}

// Preenche um contorno (pixels da tela) em leque a partir de um ponto de
// dentro, com a cor de cada vertice dada por cor(ponto): serve para as formas
// com um lado concavo (as teclas em volta do dial) e para os degrades.
template <class F>
void leque(ImDrawList* dl, const std::vector<ImVec2>& p, ImVec2 centro, F cor)
{
    const int n = (int)p.size();
    if (n < 3) return;
    const ImVec2 uv = dl->_Data->TexUvWhitePixel;
    dl->PrimReserve(n * 3, n + 1);
    const ImDrawIdx b = (ImDrawIdx)dl->_VtxCurrentIdx;
    dl->PrimWriteVtx(centro, uv, cor(centro));
    for (const ImVec2& q : p) dl->PrimWriteVtx(q, uv, cor(q));
    for (int i = 0; i < n; ++i) {
        dl->PrimWriteIdx(b);
        dl->PrimWriteIdx((ImDrawIdx)(b + 1 + i));
        dl->PrimWriteIdx((ImDrawIdx)(b + 1 + (i + 1) % n));
    }
}

// Circulo fino antisserrilhado com a cor por angulo (suaviza a quina dos aneis
// feitos com anelLuz, que nao tem antisserrilhado)
template <class F>
void circuloSuave(ImDrawList* dl, ImVec2 c, float r, int n, float esp, F cor)
{
    ImVec2 ant(c.x + r, c.y);
    for (int k = 1; k <= n; ++k) {
        const float a = (float)(2 * kPi) * k / n;
        const ImVec2 q(c.x + std::cos(a) * r, c.y + std::sin(a) * r);
        dl->AddLine(ant, q, cor(a - (float)kPi / n), esp);
        ant = q;
    }
}

// Um pe do radio (cilindro preto, como na foto original), de x0 a x1 no papel,
// embaixo do painel: brilho vertical (luz de cima/esquerda) e a sombra no chao.
void peDoRadio(ImDrawList* dl, float x0, float x1)
{
    // MB1SDR: cromado escovado (cores tiradas da amostra do autor, da esquerda para a direita)
    static const unsigned char kCromo[33][3] = {
        {146,145,148}, {131,129,133}, {151,150,152}, {171,171,173}, {193,193,195}, {205,203,204}, {216,213,214}, {227,223,222},
        {239,234,231}, {246,241,238}, {247,242,239}, {248,243,243}, {249,245,244}, {251,247,246}, {251,250,248}, {253,251,252},
        {252,252,252}, {253,253,253}, {243,243,243}, {234,234,234}, {226,226,226}, {220,220,220}, {222,222,222}, {227,225,225},
        {232,228,228}, {236,232,231}, {242,236,234}, {245,239,237}, {245,239,237}, {241,236,233}, {229,223,222}, {198,194,194}, {140,138,141}};
    const float y0 = kLH - 2.f, y1 = kLH + kPe - 3.2f, xm = (x0 + x1) * 0.5f;
    if (!g_fundoChave) {
        dl->AddEllipseFilled(M.p(xm, y1 + 2.f), ImVec2(M.d((x1 - x0) * 0.64f), M.d(6.f)), IM_COL32(0, 0, 0, 90), 0.f, 48);
        dl->AddEllipseFilled(M.p(xm, y1 + 1.f), ImVec2(M.d((x1 - x0) * 0.55f), M.d(3.2f)), IM_COL32(0, 0, 0, 150), 0.f, 48);
    }
    auto cor = [](int k, float f) {
        return IM_COL32((int)(kCromo[k][0] * f), (int)(kCromo[k][1] * f), (int)(kCromo[k][2] * f), 255);
    };
    const int n = 32;
    const ImVec2 uv = dl->_Data->TexUvWhitePixel;
    dl->PrimReserve(n * 12, (n + 1) * 3);
    const ImDrawIdx i0 = (ImDrawIdx)dl->_VtxCurrentIdx;
    for (int k = 0; k <= n; ++k) {
        const float u = (float)k / n, x = x0 + (x1 - x0) * u;
        const float yb = y1 + 2.4f * std::sin((float)kPi * u);              // a borda de baixo do cilindro
        dl->PrimWriteVtx(M.p(x, y0), uv, cor(k, 0.42f));                     // sombra embaixo do radio
        dl->PrimWriteVtx(M.p(x, y0 + 6.f), uv, cor(k, 1.0f));
        dl->PrimWriteVtx(M.p(x, yb), uv, cor(k, 0.86f));
    }
    for (int k = 0; k < n; ++k)
        for (int r = 0; r < 2; ++r) {
            const ImDrawIdx a = (ImDrawIdx)(i0 + k * 3 + r), b = (ImDrawIdx)(a + 3);
            dl->PrimWriteIdx(a); dl->PrimWriteIdx(b); dl->PrimWriteIdx((ImDrawIdx)(b + 1));
            dl->PrimWriteIdx(a); dl->PrimWriteIdx((ImDrawIdx)(b + 1)); dl->PrimWriteIdx((ImDrawIdx)(a + 1));
        }
    // o escovado: riscos finos na horizontal
    for (int i = 0; i < 14; ++i) {
        const float y = y0 + 7.f + i * (y1 - y0 - 8.f) / 14.f;
        const int a = 10 + (int)((i * 37) % 16);
        dl->AddLine(M.p(x0 + 2.f, y), M.p(x1 - 2.f, y), (i & 1) ? IM_COL32(0, 0, 0, a) : IM_COL32(255, 255, 255, a), 1.f);
    }
}

// Disco de metal torneado (face do dial, tampa dos botoes): o brilho depende
// so do angulo (os raios de luz), com os sulcos finos do torno por cima.
void discoEscovado(ImDrawList* dl, ImVec2 c, float r, float ganho, int sulcos)
{
    const int n = 180;
    const ImVec2 uv = dl->_Data->TexUvWhitePixel;
    dl->PrimReserve(n * 3, n * 3);
    for (int k = 0; k < n; ++k) {
        const float a0 = k * (float)(2 * kPi / n), a1 = (k + 1) * (float)(2 * kPi / n);
        const float l0 = luzFace(a0) * ganho, l1 = luzFace(a1) * ganho;
        const ImDrawIdx b = (ImDrawIdx)dl->_VtxCurrentIdx;
        dl->PrimWriteVtx(c, uv, cinzaFrio((l0 + l1) * 0.5f));
        dl->PrimWriteVtx(ImVec2(c.x + std::cos(a0) * r, c.y + std::sin(a0) * r), uv, cinzaFrio(l0));
        dl->PrimWriteVtx(ImVec2(c.x + std::cos(a1) * r, c.y + std::sin(a1) * r), uv, cinzaFrio(l1));
        dl->PrimWriteIdx(b); dl->PrimWriteIdx((ImDrawIdx)(b + 1)); dl->PrimWriteIdx((ImDrawIdx)(b + 2));
    }
    for (int i = 1; i <= sulcos; ++i)
        dl->AddCircle(c, r * i / (sulcos + 1.f), IM_COL32(0, 0, 0, (i & 1) ? 26 : 10), 0, 1.0f);
    dl->AddCircle(c, r, cinzaFrio(luzFace(rad(-162)) * ganho * 0.6f, 200), 0, 1.0f);   // borda lisa (antisserrilhada)
}

// Lateral de um botao visto de cima, a esquerda (a perspectiva da foto): o pe
// fica no painel (centro cb, raio rb) e o alto sai deslocado para baixo/direita
// (centro ct, raio rt). So aparece a parte virada para a camera. Em t (0 = pe,
// 1 = alto): aba lisa ate tAba e o serrilhado dai para cima; nd dentes que
// giram com rot, cada um com uma face clara e outra escura e, se ponta > 0, a
// ponta em V descendo para a aba (como o dial da foto).
void lateralBotao(ImDrawList* dl, ImVec2 cb, float rb, ImVec2 ct, float rt, float tAba, int nd, float rot,
                  float (*luzAba)(float), float (*luzSerr)(float), float ganho, float ponta)
{
    const float dx = ct.x - cb.x, dy = ct.y - cb.y;
    const float dd = std::sqrt(dx * dx + dy * dy);
    if (dd < 0.3f) return;
    const float TWO_PI = (float)(2 * kPi);
    const float aOff = std::atan2(dy, dx);
    const float phiT = std::acos(std::clamp((rb - rt) / dd, -1.f, 1.f));
    const float a0 = aOff + phiT, a1 = aOff + TWO_PI - phiT;     // so as geratrizes viradas para a camera
    if (a1 <= a0 + 0.01f) return;
    auto P = [&](float a, float t) {
        const float ca = std::cos(a), sa = std::sin(a);
        const float xb = cb.x + ca * rb, yb = cb.y + sa * rb;
        return ImVec2(xb + (ct.x + ca * rt - xb) * t, yb + (ct.y + sa * rt - yb) * t);
    };
    // perto do contorno (onde a lateral "vira") escurece
    auto borda = [&](float a) { return 0.4f + 0.6f * std::clamp(std::min(a - a0, a1 - a) / 0.35f, 0.f, 1.f); };
    const ImVec2 uv = dl->_Data->TexUvWhitePixel;
    const int n = std::max(16, (int)((a1 - a0) * 30.f));
    const bool comPonta = ponta > 0.f;
    // faixas em t e o brilho de cada uma (fracao do serrilhado; a primeira e a aba)
    const int nf = comPonta ? 4 : 3;
    const float tsP[5] = {0.f, tAba, 0.55f, 0.92f, 1.f}, gsP[4] = {0.f, 0.70f, 0.95f, 1.0f};
    const float tsS[5] = {0.f, tAba, 0.965f, 1.f, 1.f}, gsS[4] = {0.f, 0.80f, 0.50f, 0.f};
    const float* ts = comPonta ? tsP : tsS;
    const float* gs = comPonta ? gsP : gsS;
    for (int fx = 0; fx < nf; ++fx) {
        dl->PrimReserve(n * 6, n * 4);
        for (int k = 0; k < n; ++k) {
            const float b0 = a0 + (a1 - a0) * k / n, b1 = a0 + (a1 - a0) * (k + 1) / n;
            auto cor = [&](float a) {
                const float l = fx == 0 ? luzAba(a) : luzSerr(a) * gs[fx];
                return cinzaFrio(l * ganho * borda(a));
            };
            const ImU32 c0 = cor(b0), c1 = cor(b1);
            const ImDrawIdx i0 = (ImDrawIdx)dl->_VtxCurrentIdx;
            dl->PrimWriteVtx(P(b0, ts[fx]), uv, c0);
            dl->PrimWriteVtx(P(b0, ts[fx + 1]), uv, c0);
            dl->PrimWriteVtx(P(b1, ts[fx + 1]), uv, c1);
            dl->PrimWriteVtx(P(b1, ts[fx]), uv, c1);
            dl->PrimWriteIdx(i0); dl->PrimWriteIdx((ImDrawIdx)(i0 + 1)); dl->PrimWriteIdx((ImDrawIdx)(i0 + 2));
            dl->PrimWriteIdx(i0); dl->PrimWriteIdx((ImDrawIdx)(i0 + 2)); dl->PrimWriteIdx((ImDrawIdx)(i0 + 3));
        }
    }
    // os dentes. Com ponta (o dial): um triangulo comprido por dente, base larga perto
    // da face e a ponta para fora entrando na aba, uma face clara e a outra escura (como
    // na foto). Sem ponta (os botoes): o serrilhado fino, duas faces por dente.
    const float passo = TWO_PI / (float)nd;
    const float meia = comPonta ? passo * 0.49f : passo * 0.40f;
    const float aLuz = rad(-125);
    const float tPonta = std::max(0.f, tAba - ponta), tBase = 0.55f, tt = 0.95f;
    for (int k = 0; k < nd; ++k) {
        float m = std::fmod(rot + k * passo - a0, TWO_PI);
        if (m < 0.f) m += TWO_PI;
        const float an = a0 + m;
        if (an - meia < a0 || an + meia > a1) continue;
        const float l = luzSerr(an) * ganho * borda(an);
        const bool esqClara = std::sin(an - aLuz) > 0.f;          // a face virada para a luz e a clara
        if (comPonta) {
            // a serra: triangulo curto (a ponta entra na aba); dai ate perto da face so um risco leve
            const ImVec2 b0 = P(an - meia, tBase), bm = P(an, tBase), b1 = P(an + meia, tBase), bico = P(an, tPonta);
            const ImU32 cc = cinzaFrio(l * 1.58f + 0.015f), ce = cinzaFrio(l * 0.62f);
            dl->AddTriangleFilled(b0, bm, bico, esqClara ? cc : ce);
            dl->AddTriangleFilled(bm, b1, bico, esqClara ? ce : cc);
            const ImVec2 r0 = P(an - meia, 0.92f), rm = P(an, 0.92f), r1 = P(an + meia, 0.92f);
            const ImU32 rc = cinzaFrio(l * 1.06f), re = cinzaFrio(l * 0.84f);
            dl->AddQuadFilled(b0, r0, rm, bm, esqClara ? rc : re);
            dl->AddQuadFilled(bm, rm, r1, b1, esqClara ? re : rc);
        } else {
            const ImVec2 p0 = P(an - meia, tAba), p1 = P(an - meia, tt), p2 = P(an, tt), p3 = P(an, tAba);
            const ImVec2 q0 = P(an + meia, tAba), q1 = P(an + meia, tt);
            const ImU32 cc = cinzaFrio(l * 1.30f + 0.012f), ce = cinzaFrio(l * 0.62f);
            dl->AddQuadFilled(p0, p1, p2, p3, esqClara ? cc : ce);
            dl->AddQuadFilled(p3, p2, q1, q0, esqClara ? ce : cc);
        }
    }
}

// Arredonda os cantos (indices em "cantos") de um contorno fechado com raio
// rc: corta rc antes e depois de cada canto e poe uma curva no lugar.
std::vector<ImVec2> arredondar(const std::vector<ImVec2>& p, const std::vector<int>& cantos, float rc)
{
    const int n = (int)p.size();
    std::vector<float> s(n + 1, 0.f);
    for (int i = 0; i < n; ++i) {
        const ImVec2 d(p[(i + 1) % n].x - p[i].x, p[(i + 1) % n].y - p[i].y);
        s[i + 1] = s[i] + std::sqrt(d.x * d.x + d.y * d.y);
    }
    const float total = std::max(1e-3f, s[n]);
    auto pontoEm = [&](float t) {               // o ponto do contorno a distancia t do comeco
        t = std::fmod(t + total * 4.f, total);
        int i = 0;
        while (i < n - 1 && s[i + 1] < t) ++i;
        const float f = (t - s[i]) / std::max(1e-6f, s[i + 1] - s[i]);
        const ImVec2 a = p[i], b = p[(i + 1) % n];
        return ImVec2(a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f);
    };
    std::vector<ImVec2> out;
    for (int i = 0; i < n; ++i) {
        bool canto = false, cortado = false;
        for (int c : cantos) {
            if (c == i) { canto = true; continue; }
            float d = std::fabs(s[i] - s[c]);
            d = std::min(d, total - d);
            if (d < rc) cortado = true;
        }
        if (cortado && !canto) continue;
        if (!canto) { out.push_back(p[i]); continue; }
        const ImVec2 a = pontoEm(s[i] - rc), b = pontoEm(s[i] + rc), q = p[i];
        for (int k = 0; k <= 6; ++k) {            // curva (Bezier quadratica) a -> q -> b
            const float t = k / 6.f, u = 1.f - t;
            out.push_back(ImVec2(u * u * a.x + 2 * u * t * q.x + t * t * b.x, u * u * a.y + 2 * u * t * q.y + t * t * b.y));
        }
    }
    return out;
}

bool dentroPoligono(const std::vector<ImVec2>& p, ImVec2 q)
{
    bool d = false;
    for (size_t i = 0, j = p.size() - 1; i < p.size(); j = i++)
        if (((p[i].y > q.y) != (p[j].y > q.y)) &&
            (q.x < (p[j].x - p[i].x) * (q.y - p[i].y) / (p[j].y - p[i].y) + p[i].x))
            d = !d;
    return d;
}

// Reticula (meio-tom) do painel do radio: pontinhos numa grade alternada.
// sentido +1: maiores em cima, somem embaixo (fileira do SUB); -1: pequenos
// em cima, maiores embaixo (fileira do MAIN). Os lados esmaecem.
void pontilhado(float x0, float y0, float x1, float y1, float passo, ImU32 cor, int sentido)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float rMax = passo * 0.32f;
    const int a0 = (cor >> IM_COL32_A_SHIFT) & 0xff;
    int lin = 0;
    for (float y = y0; y <= y1; y += passo * 0.866f, ++lin) {
        float t = (y - y0) / std::max(1.f, y1 - y0);
        if (sentido < 0) t = 1.f - t;
        float fv = 1.f - 0.45f * t;                              // diminui ao longe
        if (t < 0.04f) fv *= t / 0.04f;                          // entra suave na borda
        if (t > 0.70f) fv *= std::max(0.f, (1.f - t) / 0.30f);   // e some
        if (fv <= 0.03f) continue;
        for (float x = x0 + ((lin & 1) ? passo * 0.5f : 0.f); x <= x1; x += passo) {
            const float e = std::min(x - x0, x1 - x);
            const float fh = std::clamp(e / 12.f, 0.f, 1.f);
            const float rr = rMax * fv * (0.45f + 0.55f * fh);
            const float px = M.d(rr);
            const ImVec2 q = M.p(x, y);
            const float fa = std::min(1.f, 0.50f + fv) * fh;
            if (px < 0.6f)          // tela pequena: ponto de 1 px mais fraco
                dl->AddRectFilled(ImVec2(q.x - 0.5f, q.y - 0.5f), ImVec2(q.x + 0.5f, q.y + 0.5f), alfa(cor, (int)(a0 * fa * px / 0.6f)));
            else
                dl->AddCircleFilled(q, px, alfa(cor, (int)(a0 * fa)), px < 1.6f ? 4 : 8);
        }
    }
}

// rotulo com o simbolo de botao duplo no meio: "SHIFT (o) WIDTH"
void rotuloDuplo(ImFont* f, float tam, float cx, float y, ImU32 cor, const char* a, const char* b)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (b && *b) {
        texto(f, tam, cx - 5.f, y, cor, a, 1.f);
        texto(f, tam, cx + 5.f, y, cor, b, 0.f);
        const float yc = y + tamReal(tam) * 0.55f;
        dl->AddCircle(M.p(cx, yc), M.d(2.4f), cor, 12, M.d(0.7f));
        dl->AddCircleFilled(M.p(cx, yc), M.d(1.0f), cor, 8);
    } else texto(f, tam, cx, y, cor, a, 0.5f);
}

// nome do modo como o radio mostra
std::string nomeModo(const std::string& m)
{
    if (m == "CW") return "CW-U";
    if (m == "NFM") return "FM-N";
    if (m == "WFM") return "FM-W";
    return m;
}

// ===========================================================================
//  MB1SDR: o painel do MB1. Tudo foi medido na foto do radio
//  (ic_7610.jpg, 2400 x 1011 px): x 75..2342 e y 104..943 da foto viram o
//  papel 1500 x 555. As funcoes abaixo recebem as posicoes da FOTO.
// ===========================================================================
constexpr float kK = 1.f;                         // MB1SDR: mb1.jpg, x 6..1894, y 3..673
inline float PX(float x) { return x * kK; }
inline float PY(float y) { return (y + 24.f) * kK; }   // MB1SDR: 24 = a tampa em cima da frente
inline ImVec2 Q(float x, float y) { return M.p(PX(x), PY(y)); }
inline float QD(float d) { return M.d(d * kK); }
inline ImFont* fP() { return g_fp[9]; }     // rotulos do painel (Segoe UI negrito)
inline ImFont* fL() { return g_fl[11]; }    // texto da tela (Bahnschrift)

ImU32 mistura(ImU32 a, ImU32 b, float t)
{
    t = std::clamp(t, 0.f, 1.f);
    auto c = [&](int sh) { return (ImU32)std::lround(((a >> sh) & 0xff) * (1.f - t) + ((b >> sh) & 0xff) * t) << sh; };
    return c(IM_COL32_R_SHIFT) | c(IM_COL32_G_SHIFT) | c(IM_COL32_B_SHIFT) | c(IM_COL32_A_SHIFT);
}
// cinza neutro (o MB1 e preto / prata, sem o tom petroleo do FTDX)
ImU32 cinzaN(float v, int a = 255)
{
    v = std::clamp(v, 0.f, 1.f);
    return IM_COL32((int)(v * 255.f), (int)(v * 255.f), (int)std::min(255.f, v * 255.f * 1.02f + 1.f), a);
}

// tamanho em pixels da fonte f para que as MAIUSCULAS tenham "cap" px da foto,
// e a altura (fracao do tamanho) do meio das maiusculas
float pxCap(ImFont* f, float cap, float* centro = nullptr)
{
    ImFont* f0 = melhor(f, 40.f);
    float rc = 0.70f, cc = 0.55f;
    if (f0 && f0->FontSize > 0.f)
        if (const ImFontGlyph* g = f0->FindGlyphNoFallback((ImWchar)'H'))
            if (g->Y1 > g->Y0) { rc = (g->Y1 - g->Y0) / f0->FontSize; cc = (g->Y0 + g->Y1) * 0.5f / f0->FontSize; }
    if (centro) *centro = cc;
    return std::max(QD(cap) * 1.2f / std::max(0.3f, rc), 5.f);   // 1,2: medido na captura x foto
}

// texto: cap = altura das maiusculas (px da foto), yc = o meio delas (foto);
// alin 0 esquerda / 0.5 centro / 1 direita; cond < 1 aperta, > 1 alarga a letra;
// maxW > 0 (px da foto) encolhe para caber
void tx(ImFont* f, float cap, float x, float yc, ImU32 cor, const char* s, float alin = 0.f, float cond = 1.f, float maxW = 0.f)
{
    if (!f || !s || !*s) return;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float cc = 0.55f;
    float px = pxCap(f, cap, &cc);
    ImFont* fo = melhor(f, px);
    ImVec2 ts = fo->CalcTextSizeA(px, FLT_MAX, 0, s);
    if (maxW > 0.f && ts.x * cond > QD(maxW) && ts.x > 0.f) {
        px = std::max(5.f, px * QD(maxW) / (ts.x * cond));
        fo = melhor(f, px);
        ts = fo->CalcTextSizeA(px, FLT_MAX, 0, s);
    }
    const ImVec2 q = Q(x, yc);
    const ImVec2 pos(std::floor(q.x - ts.x * alin + 0.5f), std::floor(q.y - px * cc + 0.5f));
    const int v0 = dl->VtxBuffer.Size;
    if (cond != 1.f) {
        const ImVec2 c0 = dl->GetClipRectMin(), c1 = dl->GetClipRectMax();
        dl->PushClipRect(ImVec2(c0.x - 4000.f, c0.y), ImVec2(c1.x + 4000.f, c1.y), false);
    }
    dl->AddText(fo, px, pos, cor, s);
    if (cond != 1.f) {
        for (int i = v0; i < dl->VtxBuffer.Size; ++i) dl->VtxBuffer[i].pos.x = q.x + (dl->VtxBuffer[i].pos.x - q.x) * cond;
        dl->PopClipRect();
    }
}
// largura de um texto (px da foto)
float txW(ImFont* f, float cap, const char* s, float cond = 1.f)
{
    const float px = pxCap(f, cap);
    return melhor(f, px)->CalcTextSizeA(px, FLT_MAX, 0, s).x * cond / std::max(1e-3f, M.k * kK);
}

// retangulo arredondado com degrade de cima (c0) para baixo (c1), em pixels da tela
void retGrad(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, ImU32 c0, ImU32 c1)
{
    dl->PathRect(a, b, r);
    std::vector<ImVec2> p(dl->_Path.Data, dl->_Path.Data + dl->_Path.Size);
    dl->PathClear();
    if (p.size() < 3) return;
    const float h = std::max(1.f, b.y - a.y);
    leque(dl, p, ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f), [&](ImVec2 q) { return mistura(c0, c1, (q.y - a.y) / h); });
}

// quadrilatero com uma cor por vertice (sem antisserrilhado: bom para o fundo com cor-chave)
void quadCores(ImDrawList* dl, ImVec2 a, ImVec2 b, ImVec2 c, ImVec2 d, ImU32 ca, ImU32 cb, ImU32 cc, ImU32 cd)
{
    const ImVec2 uv = dl->_Data->TexUvWhitePixel;
    dl->PrimReserve(6, 4);
    const ImDrawIdx i0 = (ImDrawIdx)dl->_VtxCurrentIdx;
    dl->PrimWriteVtx(a, uv, ca); dl->PrimWriteVtx(b, uv, cb); dl->PrimWriteVtx(c, uv, cc); dl->PrimWriteVtx(d, uv, cd);
    dl->PrimWriteIdx(i0); dl->PrimWriteIdx((ImDrawIdx)(i0 + 1)); dl->PrimWriteIdx((ImDrawIdx)(i0 + 2));
    dl->PrimWriteIdx(i0); dl->PrimWriteIdx((ImDrawIdx)(i0 + 2)); dl->PrimWriteIdx((ImDrawIdx)(i0 + 3));
}

// o simbolo de botao duplo dos rotulos ("AF -(o)- RF/SQL"): yc em px da foto
float simboloDuplo(float x, float yc, ImU32 cor, float esc = 1.f)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float r = 5.6f * esc, l = 5.f * esc;
    dl->AddLine(Q(x, yc), Q(x + l, yc), cor, std::max(1.f, QD(2.f)));
    dl->AddCircle(Q(x + l + r, yc), QD(r), cor, 16, std::max(1.f, QD(2.f)));
    dl->AddCircleFilled(Q(x + l + r, yc), std::max(1.f, QD(2.f * esc)), cor, 10);
    dl->AddLine(Q(x + l + 2 * r, yc), Q(x + 2 * l + 2 * r, yc), cor, std::max(1.f, QD(2.f)));
    return 2 * l + 2 * r;
}

// rotulo dos botoes duplos: "MAIN:AF" + simbolo + "RF/SQL", centrado em cx
void rotuloDuploIc(float cx, float yc, float cap, ImU32 cor, const char* a, const char* b)
{
    const float wa = txW(fP(), cap, a, 0.94f), wb = txW(fP(), cap, b, 0.94f), ws = 21.f * cap / 13.f;
    float x = cx - (wa + wb + ws + 4.f) * 0.5f;
    tx(fP(), cap, x, yc, cor, a, 0.f, 0.94f);
    x += wa + 2.f;
    x += simboloDuplo(x, yc, cor, cap / 13.f) + 2.f;
    tx(fP(), cap, x, yc, cor, b, 0.f, 0.94f);
}

ImU32 corLedIc(int ld)
{
    switch (ld) {
    case 2: return IM_COL32(0x5c, 0xff, 0x6a, 255);
    case 3: return IM_COL32(0xff, 0x9a, 0x30, 255);
    case 4: return IM_COL32(0xff, 0x34, 0x2c, 255);
    case 5: return IM_COL32(0xf4, 0xf6, 0xff, 255);
    case 6: return IM_COL32(0x8c, 0xf4, 0xff, 255);
    case 7: return IM_COL32(0xd8, 0xff, 0x3c, 255);
    default: return IM_COL32(0x2c, 0x2e, 0x30, 255);
    }
}

// face conica do dial (escovado em leque a partir do centro): brilho por angulo
float luzCone(float a)
{
    return 0.20f + 0.19f * sino(a, rad(160), rad(42)) + 0.10f * sino(a, rad(105), rad(22)) + 0.07f * sino(a, rad(-35), rad(28))
         + 0.05f * sino(a, rad(-120), rad(20));
}
void discoCone(ImDrawList* dl, ImVec2 c, float r, float (*luz)(float), float ganho)
{
    const int n = 180;
    const ImVec2 uv = dl->_Data->TexUvWhitePixel;
    dl->PrimReserve(n * 3, n * 3);
    for (int k = 0; k < n; ++k) {
        const float a0 = k * (float)(2 * kPi / n), a1 = (k + 1) * (float)(2 * kPi / n);
        const float l0 = luz(a0) * ganho, l1 = luz(a1) * ganho;
        const ImDrawIdx b = (ImDrawIdx)dl->_VtxCurrentIdx;
        dl->PrimWriteVtx(c, uv, cinzaN((l0 + l1) * 0.5f));
        dl->PrimWriteVtx(ImVec2(c.x + std::cos(a0) * r, c.y + std::sin(a0) * r), uv, cinzaN(l0));
        dl->PrimWriteVtx(ImVec2(c.x + std::cos(a1) * r, c.y + std::sin(a1) * r), uv, cinzaN(l1));
        dl->PrimWriteIdx(b); dl->PrimWriteIdx((ImDrawIdx)(b + 1)); dl->PrimWriteIdx((ImDrawIdx)(b + 2));
    }
}

// as 10 bandas de radioamador das teclas (indices de kBandasHam)
const int kBandaIc[3][3] = {{0, 1, 3}, {4, 5, 6}, {7, 8, 9}};

// hash simples (pontinhos da textura do painel, sempre nos mesmos lugares)
inline uint32_t hashIc(uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
    return x;
}

} // namespace

// ===========================================================================
//  Pecas desenhadas: tecla, botao duplo, encoder, dial
// ===========================================================================

// Tecla do radio. led: 0 sem LED, 1 apagado, 2 verde, 3 laranja, 4 vermelho,
// 5 ambar. Devolve 1 = clique, 2 = toque longo (SEL), 0 = nada.
// O LED sai dentro da tecla, numa janelinha acima ou sobre a borda conforme
// g_ledFora; as teclas sem texto levam a borracha do radio (g_borracha).
int Ui::tecla(const char* id, const char* txt, float x, float y, float w, float h, int led,
              const char* acima, const char* dica, unsigned corTxt, bool fkey, int ledBaixo)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = M.p(x, y), b = M.p(x + w, y + h);
    ImGui::SetCursorScreenPos(a);
    ImGui::PushID(id);
    ImGui::InvisibleButton("##t", ImVec2(std::max(1.f, b.x - a.x), std::max(1.f, b.y - a.y)));
    const ImGuiID iid = ImGui::GetItemID();
    const bool sobre = ImGui::IsItemHovered(), ativo = ImGui::IsItemActive();
    int res = 0;
    if (ImGui::IsItemActivated()) longoFeito()[iid] = 0;
    if (ativo && ImGui::GetIO().MouseDownDuration[0] > 0.6f && longoFeito()[iid] == 0) { longoFeito()[iid] = 1; res = 2; }
    if (ImGui::IsItemDeactivated() && sobre && longoFeito()[iid] == 0) res = 1;
    ImGui::PopID();

    const bool temTexto = txt && *txt;
    const float r = M.d(2.8f);
    // rebaixo preto em volta: a tecla fica afundada no painel
    {
        const float g = M.d(2.4f);
        dl->AddRectFilled(ImVec2(a.x - g, a.y - g), ImVec2(b.x + g, b.y + g * 1.1f), IM_COL32(0x02, 0x03, 0x04, 255), r + g);
        dl->AddRect(ImVec2(a.x - g, a.y - g), ImVec2(b.x + g, b.y + g * 1.1f), IM_COL32(0x30, 0x36, 0x3c, 150), r + g, 0, M.d(0.6f));
    }
    ImU32 c1, c2;
    if (fkey) {          // QMB: azul
        c1 = ativo ? IM_COL32(0x1c, 0x3c, 0x9c, 255) : sobre ? IM_COL32(0x60, 0x94, 0xff, 255) : IM_COL32(0x48, 0x7c, 0xf4, 255);
        c2 = ativo ? IM_COL32(0x10, 0x24, 0x60, 255) : IM_COL32(0x1c, 0x42, 0xb4, 255);
    } else if (g_teclaTom == 1) {   // fileira RX / TX: cinza claro (pega a luz, como na foto)
        c1 = ativo ? IM_COL32(0x3c, 0x46, 0x4c, 255) : sobre ? IM_COL32(0x9c, 0xac, 0xb4, 255) : IM_COL32(0x8c, 0x9c, 0xa4, 255);
        c2 = ativo ? IM_COL32(0x22, 0x28, 0x2c, 255) : IM_COL32(0x56, 0x64, 0x6c, 255);
    } else if (temTexto) {   // teclas com nome: face cinza com o alto mais claro (brilho)
        c1 = ativo ? IM_COL32(0x1e, 0x21, 0x25, 255) : sobre ? IM_COL32(0x5a, 0x60, 0x68, 255) : IM_COL32(0x4a, 0x50, 0x58, 255);
        c2 = ativo ? IM_COL32(0x0e, 0x0f, 0x11, 255) : IM_COL32(0x1a, 0x1d, 0x21, 255);
    } else {                 // borracha
        c1 = ativo ? IM_COL32(0x1a, 0x1c, 0x1f, 255) : sobre ? IM_COL32(0x48, 0x4d, 0x54, 255) : IM_COL32(0x3a, 0x3f, 0x45, 255);
        c2 = ativo ? IM_COL32(0x0c, 0x0d, 0x0e, 255) : IM_COL32(0x17, 0x19, 0x1c, 255);
    }
    dl->AddRectFilled(a, b, c2, r);
    dl->AddRectFilledMultiColor(ImVec2(a.x + M.d(0.8f), a.y + M.d(0.8f)), ImVec2(b.x - M.d(0.8f), b.y - M.d(0.8f)), c1, c1, c2, c2);
    // chanfro claro em volta (pega a luz) e o brilho na beirada de cima
    dl->AddRect(a, b, fkey ? IM_COL32(0x9c, 0xc0, 0xff, 150) : IM_COL32(0x96, 0x9e, 0xa8, temTexto ? 150 : 95), r, 0, M.d(0.7f));
    dl->AddLine(ImVec2(a.x + r, a.y + M.d(1.0f)), ImVec2(b.x - r, a.y + M.d(1.0f)), IM_COL32(255, 255, 255, fkey ? 120 : 80), M.d(0.7f));
    auto corLed = [](int ld) {
        if (ld == 2) return P_VERDE;
        if (ld == 3) return P_LARANJA;
        if (ld == 4) return P_VERMELHO;
        if (ld == 5) return P_AMBAR;
        return IM_COL32(0x26, 0x2a, 0x28, 255);
    };
    auto desenhaLed = [&](int ld, float y0) {
        const ImU32 cl = corLed(ld);
        const float lw = w * 0.42f;
        const ImVec2 la = M.p(x + (w - lw) * 0.5f, y0), lb = M.p(x + (w + lw) * 0.5f, y0 + 1.8f);
        if (ld >= 2) dl->AddRectFilled(ImVec2(la.x - M.d(1.5f), la.y - M.d(1.2f)), ImVec2(lb.x + M.d(1.5f), lb.y + M.d(1.2f)), alfa(cl, 70), M.d(1.5f));
        dl->AddRectFilled(la, lb, cl, M.d(0.8f));
    };
    if (led > 0) {
        if (g_ledFora == 1) {          // janelinha do LED acima da tecla (SYNC, SPLIT, FAST, LOCK)
            const float lw = std::min(w * 0.30f, 11.f);
            const ImVec2 la = M.p(x + (w - lw) * 0.5f, y - 10.f), lb = M.p(x + (w + lw) * 0.5f, y - 6.f);
            const float g = M.d(1.1f);
            dl->AddRectFilled(ImVec2(la.x - g, la.y - g), ImVec2(lb.x + g, lb.y + g), IM_COL32(0x02, 0x02, 0x03, 255), M.d(2.6f));
            dl->AddRect(ImVec2(la.x - g, la.y - g), ImVec2(lb.x + g, lb.y + g), IM_COL32(0x5a, 0x60, 0x68, 200), M.d(2.6f), 0, M.d(0.6f));
            if (led >= 2) dl->AddRectFilled(ImVec2(la.x - g * 2, la.y - g * 2), ImVec2(lb.x + g * 2, lb.y + g * 2), alfa(corLed(led), 60), M.d(3.f));
            dl->AddRectFilled(la, lb, led >= 2 ? corLed(led) : IM_COL32(0x12, 0x14, 0x16, 255), M.d(1.6f));
        } else if (g_ledFora == 2) {   // barra sobre a borda de cima (RX / TX)
            const float lw = w * 0.44f;
            const ImVec2 la = M.p(x + (w - lw) * 0.5f, y - 1.8f), lb = M.p(x + (w + lw) * 0.5f, y + 1.0f);
            if (led >= 2) dl->AddRectFilled(ImVec2(la.x - M.d(1.5f), la.y - M.d(1.5f)), ImVec2(lb.x + M.d(1.5f), lb.y + M.d(1.5f)), alfa(corLed(led), 80), M.d(2.f));
            dl->AddRectFilled(la, lb, led >= 2 ? corLed(led) : IM_COL32(0x2a, 0x30, 0x34, 255), M.d(1.4f));
        } else desenhaLed(led, y + 1.6f);
    }
    if (ledBaixo > 0) desenhaLed(ledBaixo, y + h - 3.4f);
    const bool ledDentro = led > 0 && g_ledFora == 0;
    const bool doisLeds = ledDentro && ledBaixo > 0;
    if (!temTexto && g_teclaTom == 0 && w >= 14 && h >= 11) {      // borracha das teclas sem texto (como no radio)
        if (fkey) {                              // QMB azul: grade de pontinhos
            for (int lx = 0; lx < 3; ++lx)
                for (int ly = 0; ly < 7; ++ly) {
                    const float px = x + w * (0.30f + 0.20f * lx) + ((ly & 1) ? w * 0.10f : 0.f), py = y + h * (0.20f + 0.10f * ly);
                    if (px > x + w * 0.82f) continue;
                    dl->AddCircleFilled(M.p(px, py), std::max(0.7f, M.d(0.9f)), IM_COL32(0xa8, 0xc8, 0xff, 190), 8);
                }
        } else {
            const ImU32 cp = IM_COL32(0x10, 0x11, 0x13, 235), cl = IM_COL32(0x6a, 0x70, 0x78, 120);
            const float ym = y + h * 0.54f;
            auto ponto = [&](float px, float py) {
                dl->AddCircleFilled(M.p(px, py + 0.35f), std::max(0.6f, M.d(0.85f)), cl, 8);   // a luz pega embaixo
                dl->AddCircleFilled(M.p(px, py), std::max(0.6f, M.d(0.80f)), cp, 8);
            };
            const float px[3] = {0.24f, 0.50f, 0.76f};
            const float dy = std::min(3.6f, h * 0.24f);
            for (int k = 0; k < 3; ++k) { ponto(x + w * px[k], ym - dy); ponto(x + w * px[k], ym + dy); }
            if (g_borracha == 1) { ponto(x + w * 0.37f, ym); ponto(x + w * 0.63f, ym); }
            else {
                dl->AddRectFilled(M.p(x + w * 0.36f, ym - 0.9f + 0.35f), M.p(x + w * 0.64f, ym + 0.9f + 0.35f), cl, M.d(1.f));
                dl->AddRectFilled(M.p(x + w * 0.36f, ym - 0.9f), M.p(x + w * 0.64f, ym + 0.9f), cp, M.d(1.f));
            }
        }
    }
    if (temTexto) {
        const bool duas = std::strchr(txt, '\n') != nullptr;
        float tam = std::min(h * (duas ? 0.40f : 0.62f), 11.f);
        std::string maior;
        { std::string s; for (const char* c = txt; ; ++c) { if (*c == '\n' || !*c) { if (s.size() > maior.size()) maior = s; s.clear(); if (!*c) break; } else s += *c; } }
        for (int i = 0; i < 12 && tam > 4.f; ++i) {      // encolhe so o que precisa para caber
            const float lim = h * (doisLeds ? (duas ? 0.36f : 0.56f) : (duas ? 0.47f : 0.80f));
            if (larguraTexto(f_.painel, tam, maior.c_str()) <= w - 4.f && tamReal(tam) <= lim + 0.01f) break;
            tam *= 0.92f;
        }
        textoCaixa(f_.painel, tam, x, y + (ledDentro && !doisLeds ? 1.2f : 0.f), w, h,
                   corTxt ? (ImU32)corTxt : IM_COL32(0xff, 0xff, 0xff, 255), txt);
    }
    if (acima && *acima) rotulo(17.5f, x + w * 0.5f, y - 8.3f, g_corRot, acima, 0.5f, w + 16.f);
    if (sobre && dica && *dica && !ativo) ImGui::SetTooltip("%s", dica);
    return res;
}

// Botao giratorio duplo (como o AF-RF): o miolo e o anel de fora sao dois
// controles. Arrastar para cima/direita aumenta, a roda tambem. Comecou o
// arrasto em cima do miolo -> mexe no miolo; no anel -> no anel.
// Desenho como o da foto: disco preto com 12 pontinhos e os 2 furinhos por
// fora; o botao de fora (serrilhado, com o chanfro claro no alto) e o miolo
// (serrilhado fino e a tampa torneada), em perspectiva: a camera esta em cima,
// a esquerda, entao o alto de cada degrau aparece deslocado para baixo/direita
// e a lateral serrilhada aparece em cima, a esquerda.
bool Ui::giro(const char* id, float cx, float cy, float r, const char* rotE, const char* rotD,
              float tIn, float tOut, float& dIn, float& dOut, const char* dica, bool duplo, float yRot)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    dIn = dOut = 0;
    const ImVec2 c = M.p(cx, cy);
    const float R = M.d(r);
    const float TWO_PI = (float)(2 * kPi);
    {   // disco preto, os 12 pontinhos e os 2 furinhos
        const float rp = R * 1.47f;
        dl->AddCircleFilled(c, rp, cinzaFrio(0.018f), 64);
        anelLuz(dl, c, rp - std::max(1.f, M.d(0.8f)), rp, 0.f, TWO_PI, 64, [](float a, float) {
            return cinzaFrio(0.06f + 0.16f * sino(a, rad(-115), rad(45)));
        });
        for (int i = 0; i < 12; ++i) {
            const ImVec2 d = dirAng(i * (float)(2 * kPi / 12));
            dl->AddCircleFilled(ImVec2(c.x + d.x * R * 1.32f, c.y + d.y * R * 1.32f), std::max(0.8f, M.d(1.0f)), IM_COL32(0xc8, 0xec, 0xff, 240), 10);
        }
        for (int s2 = -1; s2 <= 1; s2 += 2) {
            const ImVec2 d = dirAng((float)(-kPi / 2 + s2 * rad(46)));
            const ImVec2 f(c.x + d.x * R * 1.74f, c.y + d.y * R * 1.74f);
            const float rf = std::max(1.f, M.d(2.1f));
            dl->AddCircleFilled(f, rf * 1.25f, IM_COL32(0x02, 0x02, 0x03, 255), 14);
            dl->AddCircleFilled(f, rf, cinzaFrio(0.05f), 14);
            dl->AddCircle(f, rf * 1.25f, cinzaFrio(0.24f, 200), 14, M.d(0.6f));
        }
    }
    if (rotE) {                               // rotulo "AF (o) RF": letra grande e apertada, como na foto
        const float yc = (yRot >= 0.f ? yRot : cy - r * 1.47f - 9.f) + 4.6f;     // meio da letra
        if (rotD && *rotD) {
            // as duas metades com a mesma letra, e a da direita sem passar da beira do painel
            const float wE = r * 1.8f, wD = std::min(r * 1.8f, kLW - 5.f - (cx + 8.2f));
            float S = 18.5f;
            for (int i = 0; i < 14 && S > 6.f && (larguraRotulo(S, rotE) > wE || larguraRotulo(S, rotD) > wD); ++i) S *= 0.95f;
            rotulo(S, cx - 8.2f, yc, g_corRot, rotE, 1.f);
            rotulo(S, cx + 8.2f, yc, g_corRot, rotD, 0.f);
            dl->AddCircle(M.p(cx, yc), M.d(3.6f), g_corRot, 16, M.d(1.2f));
            dl->AddCircleFilled(M.p(cx, yc), M.d(1.5f), g_corRot, 10);
            dl->AddLine(M.p(cx - 7.4f, yc), M.p(cx - 3.6f, yc), g_corRot, M.d(1.2f));
            dl->AddLine(M.p(cx + 3.6f, yc), M.p(cx + 7.4f, yc), g_corRot, M.d(1.2f));
        } else rotulo(18.5f, cx, yc, g_corRot, rotE, 0.5f, r * 4.f);
    }
    // geometria em perspectiva (medida no SHIFT/WIDTH da foto)
    const float rB1 = R * 1.12f, rT1 = R * 1.00f, rB2 = R * 0.64f, rT2 = R * 0.55f;
    const ImVec2 cT1(c.x + R * 0.11f, c.y + R * 0.12f);          // alto do botao de fora
    const ImVec2 cT2(c.x + R * 0.30f, c.y + R * 0.26f);          // tampa do miolo
    const ImVec2 cTampa = duplo ? cT2 : cT1;
    const float rTampa = duplo ? rT2 : rT1 * 0.80f;
    ImGui::SetCursorScreenPos(ImVec2(c.x - R * 1.15f, c.y - R * 1.15f));
    ImGui::PushID(id);
    ImGui::InvisibleButton("##g", ImVec2(R * 2.3f, R * 2.3f));
    const ImGuiID iid = ImGui::GetItemID();
    const bool sobre = ImGui::IsItemHovered(), ativo = ImGui::IsItemActive();
    ImGui::PopID();
    const float dxm = io.MousePos.x - cTampa.x, dym = io.MousePos.y - cTampa.y;
    const bool noMiolo = !duplo || std::sqrt(dxm * dxm + dym * dym) <= rTampa * 1.10f;
    if (ImGui::IsItemActivated()) parteGiro()[iid] = noMiolo ? 0 : 1;
    if (ativo) {
        const float dv = (-io.MouseDelta.y + io.MouseDelta.x) / M.d(150.f);
        if (parteGiro()[iid] == 0) dIn = dv; else dOut = dv;
    }
    if (sobre && io.MouseWheel != 0) {
        const float dv = io.MouseWheel * 0.02f;
        if (noMiolo) dIn = dv; else dOut = dv;
    }
    const float rotFora = angKnob(tOut), rotIn = angKnob(tIn);
    // sombra no disco (a luz vem de cima, a esquerda)
    dl->AddCircleFilled(ImVec2(c.x + R * 0.12f, c.y + R * 0.17f), rB1 * 1.03f, IM_COL32(0, 0, 0, 140), 48);
    // botao de fora: o pe, a lateral serrilhada (gira com o valor) e o alto
    dl->AddCircleFilled(c, rB1, cinzaFrio(0.02f), 64);
    lateralBotao(dl, c, rB1, cT1, rT1, 0.12f, 72, rotFora, luzAbaK, luzSerrK, 1.0f, 0.f);
    dl->AddCircleFilled(cT1, rT1, cinzaFrio(0.03f), 64);
    anelLuz(dl, cT1, rB2, rT1 * 0.80f, 0.f, TWO_PI, 72, [&](float a, float rr) {     // o plano escuro do alto
        return cinzaFrio(rr > rB2 * 1.05f ? 0.03f + 0.06f * sino(a, rad(-128), rad(38)) : 0.03f);
    });
    // chanfro do alto: brilha forte em cima, a esquerda (claro no meio da quina)
    anelLuz(dl, cT1, rT1 * 0.80f, rT1 * 0.90f, 0.f, TWO_PI, 72, [&](float a, float rr) {
        const float l = luzBisel(a); return cinzaFrio(rr > rT1 * 0.85f ? l : l * 0.20f);
    });
    anelLuz(dl, cT1, rT1 * 0.90f, rT1, 0.f, TWO_PI, 72, [&](float a, float rr) {
        const float l = luzBisel(a); return cinzaFrio(rr > rT1 * 0.95f ? l * 0.38f : l);
    });
    if (duplo) {
        {   // marquinha branca do botao de fora (no plano do alto)
            const ImVec2 d = dirAng(rotFora);
            dl->AddLine(ImVec2(cT1.x + d.x * rT1 * 0.66f, cT1.y + d.y * rT1 * 0.66f), ImVec2(cT1.x + d.x * rT1 * 0.79f, cT1.y + d.y * rT1 * 0.79f),
                        (ativo && parteGiro()[iid] == 1) ? P_LARANJA : IM_COL32(0xe8, 0xee, 0xf4, 230), std::max(1.f, M.d(1.3f)));
        }
        // o miolo: pe no alto do botao de fora, lateral serrilhada fina e a tampa
        dl->AddCircleFilled(ImVec2(cT1.x + R * 0.06f, cT1.y + R * 0.09f), rB2 * 1.05f, IM_COL32(0, 0, 0, 120), 40);
        dl->AddCircleFilled(cT1, rB2, cinzaFrio(0.02f), 48);
        lateralBotao(dl, cT1, rB2, cT2, rT2, 0.10f, 30, rotIn, luzAbaK, luzSerrK, 1.0f, 0.f);
    }
    // tampa torneada: chanfro claro e o disco escovado (os raios de luz nao giram)
    anelLuz(dl, cTampa, rTampa * 0.92f, rTampa, 0.f, TWO_PI, 72, [&](float a, float rr) {
        const float l = tabela(T_CHANF, a) * 0.85f;
        return cinzaFrio(rr > rTampa * 0.97f ? l * 0.55f : l);
    });
    discoEscovado(dl, cTampa, rTampa * 0.92f, 1.50f, std::max(3, (int)(rTampa / 2.6f)));
    {
        const ImVec2 d = dirAng(rotIn);
        const ImVec2 pd(cTampa.x + d.x * rTampa * 0.62f, cTampa.y + d.y * rTampa * 0.62f);
        dl->AddCircleFilled(pd, std::max(1.1f, rTampa * 0.085f), (ativo && parteGiro()[iid] == 0) ? P_LARANJA : IM_COL32(0xe4, 0xe8, 0xee, 220), 12);
    }
    if (ativo) dl->AddCircle(c, R * 1.12f, alfa(P_LARANJA, 120), 48, M.d(1.2f));
    if ((sobre || ativo) && dica) ImGui::SetTooltip("%s", dica);
    return dIn != 0 || dOut != 0;
}

// Encoder sem fim (o MULTI): gira arrastando em volta; cada 15 graus e um
// passo. Roda: um passo por dente. Devolve os passos do quadro.
int Ui::encoder(const char* id, float cx, float cy, float r, float& ang, const char* dica, bool* clique)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 c = M.p(cx, cy);
    const float R = M.d(r);
    ImGui::SetCursorScreenPos(ImVec2(c.x - R, c.y - R));
    ImGui::PushID(id);
    ImGui::InvisibleButton("##e", ImVec2(R * 2, R * 2));
    const ImGuiID iid = ImGui::GetItemID();
    const bool sobre = ImGui::IsItemHovered(), ativo = ImGui::IsItemActive();
    ImGui::PopID();
    int passos = 0;
    static std::unordered_map<ImGuiID, float> resto;
    static std::unordered_map<ImGuiID, float> andou;
    const float aM = std::atan2(io.MousePos.y - c.y, io.MousePos.x - c.x);
    if (ImGui::IsItemActivated()) { angArrasto()[iid] = aM; andou[iid] = 0; }
    if (ativo) {
        float d = aM - angArrasto()[iid];
        while (d > kPi) d -= (float)(2 * kPi);
        while (d < -kPi) d += (float)(2 * kPi);
        angArrasto()[iid] = aM;
        ang += d;
        andou[iid] += std::fabs(d);
        resto[iid] += d;
        const float passo = (float)(15.0 * kPi / 180.0);
        while (resto[iid] >= passo) { ++passos; resto[iid] -= passo; }
        while (resto[iid] <= -passo) { --passos; resto[iid] += passo; }
    }
    if (ImGui::IsItemDeactivated() && clique && andou[iid] < 0.08f && sobre) *clique = true;
    if (sobre && io.MouseWheel != 0) {
        int n = (int)std::lround(io.MouseWheel);
        if (n == 0) n = io.MouseWheel > 0 ? 1 : -1;
        passos += n; ang += n * 0.26f;
    }
    // botao serrilhado com a tampa torneada (como o MULTI do radio), em perspectiva
    const float TWO_PI = (float)(2 * kPi);
    const float rB = R * 0.98f, rT = R * 0.90f;
    const ImVec2 cT(c.x + R * 0.10f, c.y + R * 0.12f);
    dl->AddCircleFilled(ImVec2(c.x + R * 0.12f, c.y + R * 0.18f), rB * 1.03f, IM_COL32(0, 0, 0, 150), 48);
    dl->AddCircleFilled(c, rB, cinzaFrio(0.02f), 64);
    lateralBotao(dl, c, rB, cT, rT, 0.10f, 40, ang, luzAbaK, luzSerrK, 1.1f, 0.f);
    dl->AddCircleFilled(cT, rT, cinzaFrio(0.03f), 64);
    anelLuz(dl, cT, rT * 0.80f, rT * 0.90f, 0.f, TWO_PI, 72, [&](float a, float rr) {
        const float l = luzBisel(a); return cinzaFrio(rr > rT * 0.85f ? l : l * 0.20f);
    });
    anelLuz(dl, cT, rT * 0.90f, rT, 0.f, TWO_PI, 72, [&](float a, float rr) {
        const float l = luzBisel(a); return cinzaFrio(rr > rT * 0.95f ? l * 0.38f : l);
    });
    anelLuz(dl, cT, rT * 0.70f, rT * 0.76f, 0.f, TWO_PI, 72, [](float a, float rr) {
        (void)rr; return cinzaFrio(tabela(T_CHANF, a) * 0.80f);
    });
    discoEscovado(dl, cT, rT * 0.70f, 0.95f, std::max(3, (int)(rT * 0.7f / 2.6f)));
    if (ativo) dl->AddCircle(c, R * 1.06f, alfa(P_LARANJA, 160), 48, M.d(1.2f));
    if ((sobre || ativo) && dica) ImGui::SetTooltip("%s", dica);
    return passos;
}

// Dial principal com o anel MPVD em volta (como no radio): o disco do meio
// sintoniza (100 passos por volta, acelera girando rapido); o anel de fora
// faz o que as teclas CLAR / VC TUNE / C.S / MAIN-SUB escolheram.
// rDial = 1,17 x o raio da face; rAnel = borda de fora do anel MPVD.
// Desenho tirado da foto (medido raio a raio): o anel de aluminio acetinado
// com 24 tracos e o filete prateado, o vao preto, o dial em perspectiva (a
// face sai para baixo/direita e a lateral serrilhada aparece em cima, a
// esquerda) e a covinha do dedo.
int Ui::dialMpvd(const char* id, float cx, float cy, float rDial, float rAnel, float& angD, float& angA,
                 bool travado, int& passosAnel, const char* dicaDial, const char* dicaAnel)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 c = M.p(cx, cy);
    const float F = M.d(rDial) / 1.17f;               // raio da face (pixels)
    const float R = F * 1.21f, RA = M.d(rAnel);       // R: ate onde o clique e do dial
    passosAnel = 0;
    ImGui::SetCursorScreenPos(ImVec2(c.x - RA - M.d(4), c.y - RA - M.d(4)));
    ImGui::PushID(id);
    ImGui::InvisibleButton("##mpvd", ImVec2(2 * (RA + M.d(4)), 2 * (RA + M.d(4))));
    const ImGuiID iid = ImGui::GetItemID();
    const bool sobreB = ImGui::IsItemHovered(), ativo = ImGui::IsItemActive();
    ImGui::PopID();
    const float dist = std::sqrt((io.MousePos.x - c.x) * (io.MousePos.x - c.x) + (io.MousePos.y - c.y) * (io.MousePos.y - c.y));
    const bool sobreDial = sobreB && dist <= R, sobreAnel = sobreB && dist > R && dist <= RA + M.d(4);
    int passos = 0;
    static std::unordered_map<ImGuiID, float> resto, restoA;
    const float aM = std::atan2(io.MousePos.y - c.y, io.MousePos.x - c.x);
    if (ImGui::IsItemActivated()) {
        angArrasto()[iid] = aM;
        parteGiro()[iid] = dist <= R ? 0 : (dist <= RA + M.d(4) ? 1 : 2);
        // clique fora do circulo (nos cantos do quadrado): nao segura o mouse - e das teclas em volta
        if (parteGiro()[iid] == 2) ImGui::ClearActiveID();
    }
    const int parte = parteGiro()[iid];
    if (ativo && parte < 2) {
        float d = aM - angArrasto()[iid];
        while (d > kPi) d -= (float)(2 * kPi);
        while (d < -kPi) d += (float)(2 * kPi);
        angArrasto()[iid] = aM;
        if (parte == 0 && !travado) {
            angD += d;
            const float vel = std::fabs(d) / std::max(0.001f, io.DeltaTime) / (float)(2 * kPi);
            const float mult = vel > 2.0f ? 10.f : vel > 0.8f ? 4.f : 1.f;   // girando rapido o passo multiplica
            resto[iid] += d * mult;
            const float passo = (float)(2 * kPi / 100.0);
            while (resto[iid] >= passo) { ++passos; resto[iid] -= passo; }
            while (resto[iid] <= -passo) { --passos; resto[iid] += passo; }
        } else if (parte == 1) {
            angA += d;
            restoA[iid] += d;
            const float passo = (float)(10.0 * kPi / 180.0);                  // anel: um passo a cada 10 graus
            while (restoA[iid] >= passo) { ++passosAnel; restoA[iid] -= passo; }
            while (restoA[iid] <= -passo) { --passosAnel; restoA[iid] += passo; }
        }
    }
    if (io.MouseWheel != 0 && !ativo) {
        int n = (int)std::lround(io.MouseWheel);
        if (n == 0) n = io.MouseWheel > 0 ? 1 : -1;
        if (sobreDial && !travado) { passos += n * 5; angD += n * 5 * (float)(2 * kPi / 100.0); }
        else if (sobreAnel) { passosAnel += n; angA += n * 0.17f; }
    }

    // --- desenho, de fora para dentro ---
    const float TWO_PI = (float)(2 * kPi);
    // a marca branca em cima (na gola)
    {
        const ImVec2 a(c.x - F * 0.052f, c.y - F * 1.588f), b(c.x + F * 0.052f, c.y - F * 1.588f), p(c.x, c.y - F * 1.50f);
        dl->AddTriangleFilled(ImVec2(a.x - 1, a.y - 0.6f), ImVec2(b.x + 1, b.y - 0.6f), ImVec2(p.x, p.y + 1.2f), cinzaFrio(0.02f));
        dl->AddTriangleFilled(a, b, p, IM_COL32(0xe4, 0xe8, 0xee, 255));
        dl->AddLine(a, b, IM_COL32(255, 255, 255, 255), 1.0f);
    }
    // anel MPVD: aluminio acetinado; o brilho so depende do angulo (a luz e fixa)
    anelLuz(dl, c, F * 1.305f, RA, 0.f, TWO_PI, 180, [](float a, float) { return cinzaFrio(tabela(T_ANEL, a) * 1.08f); });
    anelLuz(dl, c, RA - F * 0.013f, RA, 0.f, TWO_PI, 180, [](float a, float) { return cinzaFrio(std::min(1.f, tabela(T_ANEL, a) * 1.10f + 0.02f)); });
    anelLuz(dl, c, RA, RA + F * 0.022f, 0.f, TWO_PI, 180, [&](float a, float rr) {          // a quina que cai para o vao
        return cinzaFrio(rr < RA + 0.5f ? tabela(T_ANEL, a) * 0.35f : 0.022f);
    });
    if (ativo && parte == 1) dl->AddCircle(c, (RA + F * 1.305f) * 0.5f, alfa(P_LARANJA, 60), 0, RA - F * 1.305f);
    // sulco entre o anel e os tracos; a faixa dos tracos e os 24 tracos (giram com o anel)
    anelLuz(dl, c, F * 1.287f, F * 1.305f, 0.f, TWO_PI, 180, [](float a, float) { return cinzaFrio(tabela(T_ANEL, a) * 0.22f + 0.015f); });
    anelLuz(dl, c, F * 1.236f, F * 1.287f, 0.f, TWO_PI, 180, [](float a, float) { return cinzaFrio(tabela(T_SEGM, a) * 0.14f + 0.015f); });
    {
        const int seg = 24;
        for (int k = 0; k < seg; ++k) {
            const float b0 = angA + k * (TWO_PI / seg), b1 = b0 + (TWO_PI / seg) * 0.92f;
            anelLuz(dl, c, F * 1.247f, F * 1.284f, b0, b1, 10, [&](float a, float rr) {
                const float l = std::min(1.f, tabela(T_SEGM, a) * 1.15f);
                return cinzaFrio(rr < F * 1.26f ? l : l * 0.86f);
            });
        }
    }
    // filete prateado (beirada de dentro do anel) e a parede que desce para o vao
    anelLuz(dl, c, F * 1.199f, F * 1.207f, 0.f, TWO_PI, 180, [](float a, float) { return cinzaFrio(tabela(T_LINHA, a) * 0.55f); });
    anelLuz(dl, c, F * 1.207f, F * 1.231f, 0.f, TWO_PI, 180, [](float a, float) { return cinzaFrio(tabela(T_LINHA, a)); });
    anelLuz(dl, c, F * 1.231f, F * 1.237f, 0.f, TWO_PI, 180, [](float a, float) { return cinzaFrio(tabela(T_LINHA, a) * 0.50f); });
    circuloSuave(dl, c, F * 1.233f, 180, 1.0f, [](float a) { return cinzaFrio(tabela(T_LINHA, a) * 0.55f, 200); });
    circuloSuave(dl, c, F * 1.203f, 180, 1.0f, [](float a) { return cinzaFrio(tabela(T_LINHA, a) * 0.55f, 200); });
    circuloSuave(dl, c, RA, 180, 1.0f, [](float a) { return cinzaFrio(tabela(T_ANEL, a) * 0.60f, 200); });
    dl->AddCircleFilled(c, F * 1.122f, cinzaFrio(0.012f), 180);               // o vao preto
    anelLuz(dl, c, F * 1.12f, F * 1.17f, 0.f, TWO_PI, 180, [&](float a, float rr) {     // a parede que desce
        return cinzaFrio(rr > F * 1.15f ? tabela(T_LINHA, a) * 0.17f : 0.012f);
    });
    anelLuz(dl, c, F * 1.17f, F * 1.199f, 0.f, TWO_PI, 180, [&](float a, float rr) {
        return cinzaFrio(tabela(T_LINHA, a) * (rr > F * 1.19f ? 0.30f : 0.17f));
    });
    if (ativo && parte == 0) dl->AddCircle(c, F * 1.17f, alfa(P_LARANJA, 130), 0, M.d(1.5f));
    // o dial em perspectiva: o pe da saia fica em c; a face sobe e aparece
    // deslocada para baixo, a direita - a lateral serrilhada fica a mostra em
    // cima, a esquerda (aba lisa embaixo, dentes com a ponta em V)
    const ImVec2 cf(c.x + F * 0.11f, c.y + F * 0.145f);
    const float rPe = F * 1.09f, rAlto = F * 1.03f;
    dl->AddCircleFilled(c, rPe, cinzaFrio(0.02f), 180);
    lateralBotao(dl, c, rPe, cf, rAlto, 0.32f, 100, angD, luzAbaDial, luzSerrDial, 1.0f, 0.04f);
    // chanfro claro da face e a face de aluminio torneado (os raios de luz sao fixos)
    anelLuz(dl, cf, F * 0.985f, rAlto, 0.f, TWO_PI, 180, [&](float a, float rr) {
        const float l = tabela(T_CHANF, a);
        return cinzaFrio(rr > F * 1.02f ? l * 0.70f : l);
    });
    circuloSuave(dl, cf, rAlto, 180, 1.0f, [](float a) { return cinzaFrio(tabela(T_CHANF, a) * 0.45f, 210); });
    discoEscovado(dl, cf, F * 0.985f, 1.0f, std::max(8, (int)(F / 2.4f)));
    {   // covinha do dedo: tigela conica brilhante (gira com o dial; a luz nao)
        const ImVec2 d = dirAng(angD + (float)(kPi * 0.5));
        const ImVec2 f(cf.x + d.x * F * 0.615f, cf.y + d.y * F * 0.615f);
        const float rf = F * 0.28f;
        auto luzCov = [](float a) {      // os reflexos do cone (medidos na foto)
            return 0.03f + 0.85f * sino(a, rad(15), rad(13)) + 0.38f * sino(a, rad(140), rad(16))
                         + 0.22f * sino(a, rad(-155), rad(6)) + 0.10f * sino(a, rad(-48), rad(9));
        };
        auto fundo = [](float a) { return 0.015f + 0.05f * sino(a, rad(95), rad(55)); };   // mais claro embaixo
        auto luzAro = [](float a) { return 0.04f + 0.62f * sino(a, rad(118), rad(34)) + 0.42f * sino(a, rad(30), rad(22))
                                           + 0.18f * sino(a, rad(75), rad(20)); };
        dl->AddCircleFilled(f, rf * 1.045f, cinzaFrio(0.008f, 220), 48);
        anelLuz(dl, f, 0.f, rf * 0.22f, 0.f, TWO_PI, 120, [&](float a, float rr) { return cinzaFrio(rr < 0.5f ? 0.12f : luzCov(a)); });
        anelLuz(dl, f, rf * 0.22f, rf * 0.50f, 0.f, TWO_PI, 120, [&](float a, float rr) {
            const float l = luzCov(a); return cinzaFrio(rr < rf * 0.3f ? l : l * 0.45f + fundo(a));
        });
        anelLuz(dl, f, rf * 0.50f, rf * 0.90f, 0.f, TWO_PI, 120, [&](float a, float rr) {
            return cinzaFrio(rr < rf * 0.6f ? luzCov(a) * 0.45f + fundo(a) : fundo(a));
        });
        anelLuz(dl, f, rf * 0.90f, rf * 0.965f, 0.f, TWO_PI, 120, [&](float a, float rr) { return cinzaFrio(rr < rf * 0.93f ? fundo(a) : luzAro(a)); });
        anelLuz(dl, f, rf * 0.965f, rf, 0.f, TWO_PI, 120, [&](float a, float rr) { return cinzaFrio(rr < rf * 0.98f ? luzAro(a) : 0.01f); });
        for (int i2 = 3; i2 >= 1; --i2)    // o brilho forte a direita do bico
            dl->AddCircleFilled(ImVec2(f.x + rf * 0.30f, f.y + rf * 0.07f), rf * 0.045f * i2, IM_COL32(235, 242, 250, 35 + 30 * (3 - i2)), 16);
    }
    if (travado) texto(f_.painel, 9.f, cx + 0.11f * rDial / 1.17f, cy - 0.30f * rDial / 1.17f, alfa(P_VERMELHO, 220), "LOCK", 0.5f);
    if (!ativo) {
        if (sobreDial && dicaDial) ImGui::SetTooltip("%s", dicaDial);
        else if (sobreAnel && dicaAnel) ImGui::SetTooltip("%s", dicaAnel);
    }
    return travado ? 0 : passos;
}

// Contorno das teclas grandes em volta do dial (CLAR, VC TUNE, FINE TUNING,
// MAIN/SUB), em pixels: cantos arredondados e o lado de dentro concavo,
// acompanhando o aro. sx / sy = -1 ou +1 escolhem o canto (-1,-1 = em cima a
// esquerda). moldura = o sulco preto em volta (um pouco maior, encosta no aro).
static std::vector<ImVec2> contornoPetala(ImVec2 c, float F, int sx, int sy, bool moldura)
{
    // forma da tecla de cima a esquerda, em raios da face (medida na foto)
    float A[2] = {-1.740f, -1.220f}, D[2] = {-1.900f, -0.800f};
    float rIn = 1.640f, yTopo = -1.220f;
    float C[2] = {-1.486f, -0.692f};
    float rc = 0.07f;
    if (moldura) {
        A[0] = -1.771f; A[1] = -1.265f; D[0] = -1.960f; D[1] = -0.769f;
        rIn = 1.575f; yTopo = -1.265f; C[0] = -1.442f; C[1] = -0.634f;
        rc = 0.11f;
    }
    const float bx = -std::sqrt(std::max(0.f, rIn * rIn - yTopo * yTopo));
    const float aB = std::atan2(yTopo, bx), aC = std::atan2(C[1], C[0]);
    std::vector<ImVec2> p;
    auto add = [&](float u, float v) { p.push_back(ImVec2(c.x + u * F * (float)(-sx), c.y + v * F * (float)(-sy))); };
    add(A[0], A[1]);                                // 0: canto de fora, em cima
    const int iB = (int)p.size();
    const int nArco = 14;
    for (int k = 0; k <= nArco; ++k) {              // B .. C: o lado de dentro, no aro
        const float a = aB + (aC - aB) * k / nArco;
        add(std::cos(a) * rIn, std::sin(a) * rIn);
    }
    const int iC = (int)p.size() - 1;
    add(D[0], D[1]);                                // canto de fora, embaixo
    const int iD = (int)p.size() - 1;
    return arredondar(p, {0, iB, iC, iD}, rc * F);
}

// Tecla grande em volta do dial. Devolve 1 = clique, 2 = segurar.
// O clique e feito a mao (pela forma, sem botao do ImGui): os cantos do
// quadrado do dial passam por cima e o botao do dial nao pode roubar o clique.
int Ui::teclaPetala(const char* id, const char* txt, float cx, float cy, float Fp, int sx, int sy, int led, const char* dica)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 c = M.p(cx, cy);
    const float F = M.d(Fp);
    const std::vector<ImVec2> forma = contornoPetala(c, F, sx, sy, false);
    const ImGuiID gid = ImGui::GetID(id);
    static std::unordered_map<ImGuiID, int> estado;       // 0 solto, 1 apertado, 2 segurar ja disparou
    static std::unordered_map<ImGuiID, double> t0;
    const bool dentro = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && dentroPoligono(forma, io.MousePos);
    int& st = estado[gid];
    int res = 0;
    if (dentro && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { st = 1; t0[gid] = agora(); }
    if (st == 1 && ImGui::IsMouseDown(ImGuiMouseButton_Left) && agora() - t0[gid] > 0.6) { st = 2; res = 2; }
    if (st != 0 && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) { if (st == 1 && dentro) res = 1; st = 0; }
    const bool apertado = st != 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left);
    // face: cinza escuro, mais claro em cima (as de cima pegam mais luz)
    float yMin = 1e9f, yMax = -1e9f, xs = 0, ys = 0;
    for (const ImVec2& q : forma) { yMin = std::min(yMin, q.y); yMax = std::max(yMax, q.y); xs += q.x; ys += q.y; }
    const ImVec2 meio(xs / forma.size(), ys / forma.size());
    float xMin = 1e9f, xMax = -1e9f;
    for (const ImVec2& q : forma) { xMin = std::min(xMin, q.x); xMax = std::max(xMax, q.x); }
    // brilho largo da foto: mais claro em cima, um pouco a esquerda; as de cima pegam mais luz
    const float base = (sy < 0 ? 0.17f : 0.11f) + (dentro ? 0.04f : 0.f) - (apertado ? 0.07f : 0.f);
    leque(dl, forma, meio, [&](ImVec2 q) {
        const float t = (q.y - yMin) / std::max(1.f, yMax - yMin);
        const float u = (q.x - xMin) / std::max(1.f, xMax - xMin);
        const float brilho = 0.09f * std::clamp(1.2f - t - 0.6f * std::fabs(u - 0.4f), 0.f, 1.f);
        return cinzaFrio(base + brilho + 0.05f * (1.f - t) - 0.05f * t);
    });
    dl->AddPolyline(forma.data(), (int)forma.size(), cinzaFrio(base), ImDrawFlags_Closed, 1.0f);
    {   // chanfro claro na beirada de cima e na de fora (onde a luz bate)
        std::vector<ImVec2> b;
        for (size_t i = 0; i < forma.size(); ++i) {
            const ImVec2 q = forma[i], n = forma[(i + 1) % forma.size()];
            const ImVec2 m((q.x + n.x) * 0.5f, (q.y + n.y) * 0.5f);
            const float dx = m.x - c.x, dy = m.y - c.y;
            const float dd = std::sqrt(dx * dx + dy * dy);
            const bool beiradaDeFora = dd > F * 1.66f;
            if (!beiradaDeFora) continue;
            dl->AddLine(q, n, IM_COL32(0xb4, 0xbe, 0xca, sy < 0 ? 120 : 70), 1.0f);
        }
    }
    if (led > 0) {   // LED pequeno na moldura, ao lado do aro
        const float ang = std::atan2(-0.95f * (float)(-sy), -1.30f * (float)(-sx));
        const ImVec2 p(c.x + std::cos(ang) * F * 1.545f, c.y + std::sin(ang) * F * 1.545f);
        const float rl = std::max(1.2f, F * 0.022f);
        ImU32 cl = IM_COL32(0x16, 0x18, 0x1a, 255);
        if (led == 2) cl = P_VERDE; else if (led == 3) cl = P_LARANJA; else if (led == 4) cl = P_VERMELHO; else if (led == 5) cl = P_AMBAR;
        if (led >= 2) dl->AddCircleFilled(p, rl * 2.4f, alfa(cl, 70), 16);
        dl->AddCircleFilled(p, rl, cl, 12);
        dl->AddCircle(p, rl * 1.2f, IM_COL32(0x50, 0x56, 0x5e, 200), 12, 1.0f);
    }
    {
        const ImVec2 mp((meio.x - M.ox) / M.k, (meio.y - M.oy) / M.k);
        textoCaixa(f_.painel, 8.6f, mp.x - 30, mp.y - 12, 60, 24, IM_COL32(0xf4, 0xf6, 0xfa, 255), txt);
    }
    if (dentro && !apertado && dica) ImGui::SetTooltip("%s", dica);
    return res;
}

// Tecla MAIN / SUB grande (prata, trapezio de cantos redondos, como no radio).
// aceso = banda de operacao (o receptor que o dial, o som e os botoes comandam):
// fica azul-claro, como a tecla acesa do radio.
int Ui::teclaCunha(const char* id, const char* txt, float x0t, float x1t, float x0b, float x1b, float yt, float yb,
                   bool aceso, const char* dica)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float xMin = std::min(x0t, x0b), xMax = std::max(x1t, x1b);
    ImGui::SetCursorScreenPos(M.p(xMin, yt));
    ImGui::PushID(id);
    ImGui::InvisibleButton("##cunha", ImVec2(M.d(xMax - xMin), M.d(yb - yt)));
    const bool sobre = ImGui::IsItemHovered(), at = ImGui::IsItemActive();
    const int res = ImGui::IsItemClicked() ? 1 : 0;
    ImGui::PopID();
    std::vector<ImVec2> p = {M.p(x0t, yt), M.p(x1t, yt), M.p(x1b, yb), M.p(x0b, yb)};
    p = arredondar(p, {0, 1, 2, 3}, M.d(6.f));
    {   // rebaixo preto (a tecla sai de um furo do painel)
        std::vector<ImVec2> f = arredondar({M.p(x0t - 3, yt - 3), M.p(x1t + 3, yt - 3), M.p(x1b + 3, yb + 3), M.p(x0b - 3, yb + 3)}, {0, 1, 2, 3}, M.d(8.f));
        leque(dl, f, M.p((x0t + x1t) * 0.5f, (yt + yb) * 0.5f), [](ImVec2) { return IM_COL32(0x02, 0x03, 0x04, 255); });
        dl->AddPolyline(f.data(), (int)f.size(), IM_COL32(0x2c, 0x32, 0x3a, 200), ImDrawFlags_Closed, 1.0f);
    }
    if (aceso) {     // a tecla acesa: filete azul embaixo, no rebaixo (como na foto)
        const ImVec2 a = M.p(x0b + 2, yb + 1.9f), b = M.p(x1b - 2, yb + 1.9f);
        dl->AddLine(a, b, IM_COL32(0x30, 0x80, 0xff, 90), M.d(2.6f));
        dl->AddLine(a, b, IM_COL32(0x58, 0xa8, 0xff, 230), M.d(0.9f));
    }
    const float yT = M.p(0, yt).y, yB = M.p(0, yb).y;
    leque(dl, p, M.p((x0t + x1t) * 0.5f, (yt + yb) * 0.5f), [&](ImVec2 q) {
        float t = (q.y - yT) / std::max(1.f, yB - yT);
        if (at) t = t * 0.6f + 0.4f;
        // prata: a acesa e clara (como a MAIN da foto); a apagada, cinza (como a SUB)
        const float topo = aceso ? 0.90f : 0.66f, fundo = aceso ? 0.70f : 0.42f;
        const float l = topo + (fundo - topo) * t + (sobre ? 0.04f : 0.f);
        const int v = (int)(255.f * std::clamp(l, 0.f, 1.f));
        return IM_COL32(std::max(0, v - 22), std::max(0, v - 10), std::min(255, v + 4), 255);
    });
    dl->AddPolyline(p.data(), (int)p.size(), IM_COL32(0x18, 0x1c, 0x22, 255), ImDrawFlags_Closed, M.d(0.9f));
    {   // brilho na beirada de cima
        const ImVec2 a = M.p(x0t + 5, yt + 1.2f), b = M.p(x1t - 5, yt + 1.2f);
        dl->AddLine(a, b, IM_COL32(255, 255, 255, 170), M.d(0.8f));
    }
    // nome em branco com contorno escuro (relevo, como no radio)
    const float xm = (x0t + x1t + x0b + x1b) * 0.25f, ym = (yt + yb) * 0.5f - 6.2f;
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if (dx || dy) texto(f_.painel, 10.f, xm + dx * 0.45f, ym + dy * 0.45f, IM_COL32(0x40, 0x48, 0x56, 200), txt, 0.5f);
    texto(f_.painel, 10.f, xm, ym, IM_COL32(0xff, 0xff, 0xff, 255), txt, 0.5f);
    if (sobre && dica && !at) ImGui::SetTooltip("%s", dica);
    return res;
}

// ===========================================================================
//  Logica do painel
// ===========================================================================
int Ui::grupoModo(const std::string& m) const
{
    if (m == "LSB" || m == "USB") return 0;
    if (m == "CW") return 1;
    if (m == "AM") return 2;
    if (m == "WFM") return 4;
    return 3;     // FM / NFM
}

std::string Ui::fmtFreqLcd(uint64_t hz) const
{
    char b[32];
    const unsigned long long mhz = hz / 1000000, khz = hz / 1000 % 1000, h = hz % 1000;
    std::snprintf(b, sizeof b, "%llu.%03llu.%03llu", mhz, khz, h);
    return b;
}

void Ui::aviso(const std::string& t, double seg) { avisoPainel_ = t; avisoAte_ = agora() + seg; }

void Ui::sincronizarAtivo()
{
    Receptor& R = rx_[ativo_];
    R.f = r_->vfo();
    R.modo = r_->modo();
    // o squelch automatico (VOX) mexe no sql_ direto: guarda no receptor
    if (sqlAplicado_ > -900 && sql_ != sqlAplicado_) R.sql = sql_;
    // MB1SDR: cores, LEVEL e CONTRAST da cachoeira sao de cada receptor
    R.paleta = paleta_; R.brilho = wfBrilho_; R.range = wfRange_; R.acima = wfAcimaPiso_; R.rangeCal = wfRangeCal_;
}

// Aplica a cada quadro o que e do receptor ativo (barato: so muda o que mudou)
void Ui::aplicarReceptor()
{
    Receptor& R = rx_[ativo_];
    vol_ = R.af;
    sql_ = R.sql; sqlAplicado_ = sql_;
    mudo_ = R.mute;
    nb_ = R.nb1 ? nbNivel_ : 0.f;
    nr_ = R.nr1 ? nrNivel_ : 0.f;
    nrEsp_ = R.nr2;
    r_->setNb2(R.nb2, nb2Nivel_);
    AjustesDsp::g().agc = R.agcOff ? 0 : std::clamp(R.agc, 1, 3);
    // RF: so manda ao dongle quando o botao mexeu (a CONFIGURACAO tambem ajusta)
    if (R.rf != rfAplicado_) {
        rfAplicado_ = R.rf;
        ganhoRf_ = 100.f - R.rf;
        r_->setGanho((int)std::lround(4.96f * R.rf));
    }
    r_->setGanhoExtraDb(-6.f * att_);
    r_->setPreamp(pamp_);
    // CW: SHIFT desloca a escuta e compensa no tom - a batida continua no PITCH
    const int g = grupoModo(r_->modo());
    int shift = 0;
    if (g == 1) shift = (int)std::lround(filLo_[1][R.fil] / 10.f) * 10;
    AjustesDsp::g().cwTomHz = std::clamp(cwPitch_ + (float)shift, 250.f, 1400.f);
    r_->setRit((R.ritOn ? R.rit : 0) + shift);
    // largura mexida fora do painel (arrasto da borda no espectro, decoders)
    if (g != grupoAplicado_) { grupoAplicado_ = g; bwAplicado_ = r_->banda(); }
    else if (r_->banda() != bwAplicado_) {
        const int bw = r_->banda();
        filHi_[g][R.fil] = (float)(g == 2 ? bw / 2 : bw);
        bwAplicado_ = bw;
    }
}

void Ui::aplicarFiltro()
{
    const Receptor& R = rx_[ativo_];
    const int g = grupoModo(r_->modo());
    const float hi = filHi_[g][R.fil], lo = filLo_[g][R.fil];
    int bw = (int)std::lround(hi);
    int loCut = 250;
    if (g == 0) loCut = (int)std::lround(lo);
    else if (g == 2) { bw = (int)std::lround(hi * 2); loCut = (int)std::lround(lo); }
    bw = std::max(50, bw);
    r_->setBanda(bw);
    AjustesDsp::g().loCutHz = std::clamp(loCut, 0, 1500);
    bwAplicado_ = bw; grupoAplicado_ = g;
}

void Ui::ativar(int i)
{
    if (i == ativo_ || i < 0 || i > 1) return;
    sincronizarAtivo();
    if (duas_) {
        // MB1SDR (dois aparelhos): o som e os botoes passam para o aparelho do outro VFO.
        // O decodificador roda no aparelho ouvido: fecha antes de trocar.
        if (decTipo_ != Decoders::NENHUM && r_->decoders().tipo() != Decoders::NENHUM) {
            escolherDecoder(Decoders::NENHUM, false);
            aviso("Decodificador fechado: ele roda no VFO ouvido", 3.0);
        }
        r_->setMedirCanal(false);
        r_ = (i == 0) ? ra_ : rb_;
        r_->setMedirCanal(true);
        std::swap(fft_, fftO_);
        std::swap(suave_, suaveO_);
        std::swap(pisoSqlA_, pisoSqlO_);
        std::swap(pisoMed_, pisoO_);
        quadrosOutro_ = 0;
        picoHold_.assign(suave_.size(), -200.f);
        ajPrimeiro_ = true; ajOutroPrimeiro_ = true;
        rfOutroAplicado_ = -1.f;
        canalLento_ = 1e9f; pkLento_ = 1e9f;
        if (!r_->aoVivo()) { centroMostrado_ = r_->centro(); taxaMostrada_ = r_->taxa(); }
    }
    ativo_ = i;
    const Receptor& R = rx_[i];
    if (R.paleta >= 0) {          // a aparencia gravada neste receptor
        paleta_ = R.paleta; g_paletaCachoeira = paleta_;
        if (R.brilho >= 0.f) wfBrilho_ = R.brilho;
        if (R.range > 0.f) wfRange_ = R.range;
        if (R.acima > -900.f && R.acima != -1.f) wfAcimaPiso_ = R.acima;
        if (R.rangeCal > 0.f) wfRangeCal_ = R.rangeCal;
        lutRef_ = 1e9f; lutRange_ = 1e9f;   // a cachoeira inteira muda de cor
        autoWf_ = false;
    }
    if (R.modo != r_->modo()) r_->setModo(R.modo);
    sintonizar(R.f, false);
    rfAplicado_ = -1;             // o RF deste receptor vale agora
    aplicarFiltro();
    muteAte_ = agora() + 0.25;
}

void Ui::mudarModoPainel(const std::string& m)
{
    if (m != r_->modo()) { r_->setModo(m); muteAte_ = agora() + 0.25; }
    rx_[ativo_].modo = m;
    aplicarFiltro();
    if (zoom_ > 0) centrarParaZoom(true);
}

// Roda do mouse em cima de um digito de uma frequencia do painel: aquele
// digito sobe/desce (como no RXSDR). s e o texto desenhado alinhado a direita
// em xDir; pesoUltimo = quanto vale o ultimo digito (1 Hz, ou 1 kHz quando a
// parte dos Hz e desenhada separada). O digito embaixo do mouse fica sublinhado.
void Ui::rodaDigitos(ImFont* f, float tam, float xDir, float y, const std::string& s, int rx, uint64_t pesoUltimo)
{
    if (!ImGui::IsWindowHovered()) return;          // outra janela (decoders, menu) por cima
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float xr = xDir;
    uint64_t peso = pesoUltimo;
    for (int j = (int)s.size() - 1; j >= 0; --j) {
        const char c[2] = {s[(size_t)j], 0};
        const float wc = larguraTexto(f, tam, c);
        if (c[0] >= '0' && c[0] <= '9') {
            const float tr = tamReal(tam);              // o mesmo tamanho que o texto() usou
            const ImVec2 a = M.p(xr - wc, y + tr * 0.08f), b = M.p(xr, y + tr * 0.92f);
            if (ImGui::IsMouseHoveringRect(a, b, false)) {
                dl->AddLine(ImVec2(a.x + 1, b.y + M.d(0.6f)), ImVec2(b.x - 1, b.y + M.d(0.6f)), P_CIANO, std::max(1.5f, M.d(1.f)));
                if (io.MouseWheel != 0) {
                    int n = (int)std::lround(io.MouseWheel);
                    if (n == 0) n = io.MouseWheel > 0 ? 1 : -1;
                    mudarFreqRx(rx, (int64_t)n * (int64_t)peso);
                    io.MouseWheel = 0;                  // a roda e deste digito (nao rola mais nada)
                }
            }
            if (peso < 1000000000ULL) peso *= 10;
        }
        xr -= wc;
    }
}

void Ui::mudarFreqRx(int rx, int64_t d)
{
    if (rx < 0 || rx > 1 || d == 0) return;
    Receptor& R = rx_[rx];
    if (R.lock || R.memModo) return;
    const uint64_t f = rx == ativo_ ? r_->vfo() : R.f;
    const int64_t nf = (int64_t)f + d;
    if (nf < 1000 || nf > 2000000000LL) return;
    if (rx == ativo_) sintonizar((uint64_t)nf, false);
    R.f = (uint64_t)nf;
}

// M/S: so a "banda" (frequencia, modo, filtro, memoria) troca de lado; os
// knobs e teclas de cada coluna (AF/RF, SQL, NB, NR, NOTCH, AGC, MUTE...) sao
// fisicos e continuam no lugar - antes o squelch fechado de um ia para o outro
void Ui::trocarMainSub()
{
    sincronizarAtivo();
    std::swap(rx_[0].f, rx_[1].f);
    std::swap(rx_[0].modo, rx_[1].modo);
    std::swap(rx_[0].fil, rx_[1].fil);
    std::swap(rx_[0].memModo, rx_[1].memModo);
    std::swap(rx_[0].memCanal, rx_[1].memCanal);
    r_->setModo(rx_[ativo_].modo);
    sintonizar(rx_[ativo_].f, false);
    aplicarFiltro();
    muteAte_ = agora() + 0.25;
}

// Gira um dial: delta em passos (10 Hz, ou 1 Hz com FINE; mais largos em AM/FM)
void Ui::sintonizarRx(int i, int64_t delta)
{
    if (delta == 0) return;
    Receptor& R = rx_[i];
    if (R.lock) return;
    if (R.memModo) {          // em memoria o dial passa os canais
        if (!mem_.empty()) {
            const int64_t n = (int64_t)mem_.size();
            R.memCanal = (int)((((int64_t)R.memCanal + delta) % n + n) % n);
            if (i == ativo_) sintonizarMemoria((size_t)R.memCanal);
            else { R.f = mem_[R.memCanal].f; R.modo = mem_[R.memCanal].modo; }
        }
        return;
    }
    const std::string md = (i == ativo_) ? r_->modo() : R.modo;
    const int g = grupoModo(md);
    int64_t passo = R.fine ? 1 : (g == 2 ? 100 : 10);
    if (g == 3) passo = R.fine ? 10 : 500;
    if (g == 4) passo = R.fine ? 100 : 5000;
    if (fast_ && !R.fine) passo *= 10;          // FAST: dez vezes mais rapido
    const int64_t df = delta * passo;
    auto mover = [&](int k, int64_t d) {
        Receptor& X = rx_[k];
        if (k == ativo_) {
            const int64_t nf = std::max<int64_t>(1000, (int64_t)r_->vfo() + d);
            sintonizar((uint64_t)nf, false);
            X.f = (uint64_t)nf;
            if (modoCentro_ && std::llabs((int64_t)r_->centro() - nf) > (int64_t)(r_->taxa() / (1.0 + zoom_) * 0.12))
                r_->centralizar(r_->centroSemDc((uint64_t)nf));
        } else {
            X.f = (uint64_t)std::max<int64_t>(1000, (int64_t)X.f + d);
        }
    };
    mover(i, df);
    if (tracking_) mover(1 - i, df);
}

void Ui::irBanda(int b)
{
    if (b < 0 || b >= kNB) return;
    Receptor& R = rx_[ativo_];
    if ((int)bandaMem_.size() < kNB) bandaMem_.resize(kNB, Rapida{0, ""});
    const uint64_t f = r_->vfo();       // guarda onde estava, se estava numa banda
    for (int k = 0; k < kNB; ++k)
        if (f >= kBandasHam[k].lo && f <= kBandasHam[k].hi) bandaMem_[k] = {f, r_->modo()};
    R.memModo = false;
    const Rapida& m = bandaMem_[b];
    const uint64_t alvo = m.f ? m.f : kBandasHam[b].padrao;
    const std::string modo = m.modo.empty() ? std::string(kBandasHam[b].modo) : m.modo;
    Config::instance().set("qmode", std::string("auto"));
    mudarModoPainel(modo);
    sintonizar(alvo, false);
    muteAte_ = agora() + 0.25;
}

void Ui::digito(int d)
{
    if (!entrando_) { entrando_ = true; entrada_.clear(); }
    if (entrada_.size() < 8) entrada_ += (char)('0' + d);
}

void Ui::confirmarEntrada()
{
    entrando_ = false;
    if (entrada_.empty()) return;
    const double khz = std::atof(entrada_.c_str());
    entrada_.clear();
    if (khz < 1 || khz > 2000000) { aviso("Frequência fora da faixa"); return; }
    rx_[ativo_].memModo = false;
    sintonizar((uint64_t)std::llround(khz * 1000.0), false);
}

// CW T. (sintonia automatica): acha o pico perto do VFO e poe o VFO nele
void Ui::cwTune()
{
    const int nb = (int)fft_.bins.size();
    if (nb < 16 || !r_->aoVivo() || suave_.size() != (size_t)nb) { aviso("Sem sinal para sintonizar"); return; }
    const std::string m = r_->modo();
    if (m == "USB" || m == "LSB") { aviso("CW T.: use em CW, AM ou FM"); return; }
    const double sr = fft_.taxa ? fft_.taxa : r_->taxa();
    const double vfo = (double)r_->vfo();
    const double janela = (m == "CW") ? 800.0 : std::max(1500.0, r_->banda() * 0.6);
    auto bin = [&](double f) { return (int)std::lround((f - (double)fft_.centro) / sr * nb + nb / 2.0); };
    const int a = std::max(1, bin(vfo - janela)), b = std::min(nb - 2, bin(vfo + janela));
    if (b <= a) return;
    int mx = a; float pv = -200;
    for (int i = a; i <= b; ++i) {
        if (std::abs(i - nb / 2) <= 1) continue;               // o DC nao e estacao
        if (suave_[i] > pv) { pv = suave_[i]; mx = i; }
    }
    double off = 0;                    // refina pelo vertice da parabola
    const double y0 = suave_[mx - 1], y1 = suave_[mx], y2 = suave_[mx + 1];
    const double den = y0 - 2 * y1 + y2;
    if (std::fabs(den) > 1e-6) off = std::clamp(0.5 * (y0 - y2) / den, -0.5, 0.5);
    const double f = (double)fft_.centro + ((mx + off) - nb / 2.0) * sr / nb;
    sintonizar((uint64_t)std::llround(f), false);
    aviso("CW T.: sintonizado no pico");
}

void Ui::falarFrequencia(int i)
{
    const uint64_t f = (i == ativo_) ? r_->vfo() : rx_[i].f;
    const std::string md = (i == ativo_) ? r_->modo() : rx_[i].modo;
    const std::string s = fmtFreqLcd(f);
    std::wstring w;
    for (char c : s) {
        if (c == '.') w += emIngles() ? L" point " : L" ponto ";
        else { w += (wchar_t)c; w += L' '; }
    }
    w += L", ";
    for (char c : md) { w += (wchar_t)c; w += L' '; }
    falar(w);
}

void Ui::tocarUltimaGravacao()
{
    std::string arq = ultimaGravacao_;
    if (arq.empty()) {      // a mais nova da pasta Gravacoes
        WIN32_FIND_DATAA fd;
        const std::string pasta = pastaExe() + "\\Gravacoes\\";
        HANDLE h = FindFirstFileA((pasta + "*.wav").c_str(), &fd);
        FILETIME melhor{}; std::string nome;
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if (CompareFileTime(&fd.ftLastWriteTime, &melhor) > 0) { melhor = fd.ftLastWriteTime; nome = fd.cFileName; }
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }
        if (!nome.empty()) arq = pasta + nome;
    }
    if (arq.empty()) { aviso("Nenhuma gravação ainda (REC grava)"); return; }
    if (r_->som().gravando()) { aviso("Pare a gravação antes (STOP)"); return; }
    PlaySoundA(arq.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
    aviso("PLAY: " + arq.substr(arq.find_last_of('\\') + 1), 4);
}

// ---------------------------------------------------------------------------
//  Guardar / ler o estado do painel no MB1SDR.ini
// ---------------------------------------------------------------------------
void Ui::carregarPainel()
{
    auto& c = Config::instance();
    // filtros padrao (A / B / C) por grupo: SSB, CW, AM, FM, WFM
    const float hi0[5][3] = {{2700, 2400, 3600}, {500, 250, 1200}, {4000, 3000, 5000}, {12500, 10000, 25000}, {200000, 150000, 250000}};
    // instalacao nova: SSB FIL A = HI 2,7 kHz / LO 200 Hz (knob HI/SHIFT-LO/WIDTH em ~4 e 2)
    const float lo0[5][3] = {{200, 400, 100}, {0, 0, 0}, {200, 250, 100}, {0, 0, 0}, {0, 0, 0}};
    for (int g = 0; g < 5; ++g)
        for (int k = 0; k < 3; ++k) {
            char ch[32]; std::snprintf(ch, sizeof ch, "fil_%d_%d", g, k);
            const std::string v = c.str(ch);
            float h = hi0[g][k], l = lo0[g][k];
            if (!v.empty()) std::sscanf(v.c_str(), "%f,%f", &h, &l);
            filHi_[g][k] = h; filLo_[g][k] = l;
        }
    for (int i = 0; i < 2; ++i) {
        Receptor& R = rx_[i];
        R.rf = std::clamp(c.gainTenths() / 4.96f, 0.f, 100.f);
        R.af = vol_;
        if (i == 1) { R.f = 14200000; R.modo = "USB"; }
        const std::string v = c.str(i == 0 ? "rx_main" : "rx_sub");
        if (!v.empty()) {
            std::stringstream ss(v);
            std::string t; std::vector<std::string> p;
            while (std::getline(ss, t, ',')) p.push_back(t);
            if (p.size() >= 24) {
                size_t k = 0;
                auto bo = [&]() { return p[k++] == "1"; };
                R.f = std::strtoull(p[k++].c_str(), nullptr, 10); R.modo = p[k++];
                R.fil = std::clamp(std::atoi(p[k++].c_str()), 0, 2);
                R.af = (float)std::atof(p[k++].c_str()); R.rf = (float)std::atof(p[k++].c_str()); R.sql = (float)std::atof(p[k++].c_str());
                R.nb1 = bo(); R.nb2 = bo(); R.nr1 = bo(); R.nr2 = bo(); R.nch = bo(); R.nchLargo = bo();
                R.bef = bo(); R.anch = bo(); R.bc = bo(); R.apf = bo(); R.mute = bo();
                R.notch = (float)std::atof(p[k++].c_str());
                R.agc = std::clamp(std::atoi(p[k++].c_str()), 1, 3); R.agcOff = bo();
                R.rit = std::atoi(p[k++].c_str()); R.ritOn = bo();
                if (k < p.size()) R.lock = bo();
                if (k < p.size()) R.fine = bo();
                if (k < p.size()) R.cont = std::clamp((float)std::atof(p[k++].c_str()), 0.f, 1.f);
                if (k + 4 < p.size()) {   // MB1SDR: aparencia da cachoeira deste receptor
                    R.paleta = std::clamp(std::atoi(p[k++].c_str()), -1, 5);
                    R.brilho = (float)std::atof(p[k++].c_str()); R.range = (float)std::atof(p[k++].c_str());
                    R.acima = (float)std::atof(p[k++].c_str()); R.rangeCal = (float)std::atof(p[k++].c_str());
                }
            }
        }
        if (v.empty()) R.nr2 = true;          // instalacao nova: NR ESPECTRAL ligado (forca 10)
        if (R.f < 1000) R.f = 7100000;
        if (R.modo.empty()) R.modo = "LSB";
    }
    rx_[0].f = r_->vfo(); rx_[0].modo = r_->modo();      // o MAIN e o que o radio tinha
    ativo_ = 0;
    if (c.num("sql_rel", 0) < 1) {
        // o squelch mudou de escala (10/11/2026: dB acima do ruido - 60): o valor antigo nao vale mais;
        // quem tinha squelch ligado fica com 10 dB acima do ruido
        for (auto& R : rx_) if (R.sql > -90.f) R.sql = -50.f;
        c.set("sql_rel", 1);
    }
    txVfo_ = (int)std::clamp(c.num("p_tx", 0), 0LL, 1LL);
    att_ = (int)std::clamp(c.num("p_att", 0), 0LL, 3LL);
    pamp_ = (int)std::clamp(c.num("p_pamp", 0), 0LL, 2LL);
    medidorModo_ = (int)std::clamp(c.num("p_meter", 0), 0LL, 2LL);
    dim_ = (int)std::clamp(c.num("p_dim", 0), 0LL, 3LL);
    sDisp_ = (int)std::clamp(c.num("p_sdisp", 0), 0LL, 2LL);
    scp_ = (int)std::clamp(c.num("p_scp", 0), 0LL, 2LL);
    modoCentro_ = c.flag("p_centro", false);
    tracking_ = c.flag("p_track", false);
    proc_ = c.flag("p_proc", false);
    nbNivel_ = (float)c.dbl("p_nb1", 40); nb2Nivel_ = (float)c.dbl("p_nb2", 0.5);
    nrNivel_ = (float)c.dbl("p_nr1", 40);
    procNivel_ = (float)c.dbl("p_procn", 0.5);
    cwPitch_ = (float)std::clamp(c.dbl("p_pitch", 700), 300.0, 1000.0);
    sqlMargem_ = (float)c.dbl("p_sqlm", 2); sqlHang_ = (float)c.dbl("p_sqlh", 0.3);
    sqlAuto_[0] = c.flag("p_sqlauto0", false); sqlAuto_[1] = c.flag("p_sqlauto1", false);
    paleta_ = (int)std::clamp(c.num("p_paleta", 5), 0LL, 5LL); g_paletaCachoeira = paleta_;   // MB1SDR: 5 = o azul do MB1 //   // 4 = azul do MB1
    dual_ = c.flag("p_dual", false);
    autoInicial_ = c.str("wf_brilho").empty();
    if (c.num("mb1_escala", 0) < 2) {
        // MB1SDR 1.1.1: o topo do espectro / cachoeira ficava 45 dB acima do ruido - o ruido colado
        // embaixo e os sinais baixos ("pouco ganho"). Agora 40 dB: desce o LEVEL 5 dB uma vez so.
        for (auto& R : rx_)
            if (R.acima > 41.f) {
                if (R.brilho >= 0.f) R.brilho = std::clamp(R.brilho - (R.acima - 40.f) * 2.f, 0.f, 200.f);
                R.acima = 40.f;
            }
        if (wfAcimaPiso_ > 41.f) {
            wfBrilho_ = std::clamp(wfBrilho_ - (wfAcimaPiso_ - 40.f) * 2.f, 0.f, 200.f);
            wfAcimaPiso_ = 40.f;
            c.set("wf_acima_piso", 40.0);
        }
        c.set("mb1_escala", 2);
    }
    {   // MB1SDR: o radio abre no MAIN - com a aparencia de cachoeira gravada nele
        const Receptor& R0 = rx_[0];
        if (R0.paleta >= 0) {
            paleta_ = R0.paleta; g_paletaCachoeira = paleta_;
            if (R0.brilho >= 0.f) wfBrilho_ = R0.brilho;
            if (R0.range > 0.f) wfRange_ = R0.range;
            if (R0.acima != -1.f) wfAcimaPiso_ = R0.acima;
            if (R0.rangeCal > 0.f) wfRangeCal_ = R0.rangeCal;
        }
    }
    espCheio_ = c.flag("p_esp_cheio", true);        // MB1SDR: o espectro verde preenchido, como no radio
    paginaSoft_ = (int)std::clamp(c.num("p_pagina", 0), 0LL, 1LL);
    rxeq_ = (int)std::clamp(c.num("p_rxeq", 0), 0LL, 4LL);
    painelFrac_ = (float)std::clamp(c.dbl("p_painel_frac", 0.66), 0.3, 1.0);
    multiFunc_ = (int)std::clamp(c.num("p_multi", 0), 0LL, 8LL);
    csFunc_ = (int)std::clamp(c.num("p_cs", 4), 0LL, 8LL);
    rfSql_ = (int)std::clamp(c.num("p_rfsql", 0), 0LL, 2LL);
    modo3d_ = c.flag("p_3dss", false);
    mono_ = c.flag("p_mono", false);
    expand_ = c.flag("p_expand", false);
    reguaN_ = (float)std::clamp(c.num("p_regua_n", 398), 280LL, 427LL);   // MB1SDR: a regua arrastavel do escopo
    reguaE_ = (float)std::clamp(c.num("p_regua_e", 280), 139LL, 427LL);
    cimaAtivo_ = false;   // MB1SDR: a tela do radio ja tem a cachoeira (sem a cachoeira separada) //       // desligada por padrao
    cimaAplicado_ = cimaAtivo_ ? 1 : 0;              // o main.cpp ja abre a janela no tamanho certo
    fast_ = c.flag("p_fast", false);
    fracBaixo_ = (float)std::clamp(c.dbl("p_esp_baixo", 0.35), 0.1, 0.85);
    bandaMem_.assign(kNB, Rapida{0, ""});
    for (int b = 0; b < kNB; ++b) {
        char ch[16]; std::snprintf(ch, sizeof ch, "banda_%d", b);
        const std::string v = c.str(ch);
        const auto p = v.find(',');
        if (p != std::string::npos) bandaMem_[b] = {std::strtoull(v.c_str(), nullptr, 10), v.substr(p + 1)};
    }
    qm_.clear();
    for (int q = 0; q < 10; ++q) {
        char ch[16]; std::snprintf(ch, sizeof ch, "qm_%d", q);
        const std::string v = c.str(ch);
        const auto p = v.find(',');
        if (p != std::string::npos) qm_.push_back({std::strtoull(v.c_str(), nullptr, 10), v.substr(p + 1)});
    }
    aplicarFiltro();
}

void Ui::salvarPainel()
{
    auto& c = Config::instance();
    sincronizarAtivo();
    for (int g = 0; g < 5; ++g)
        for (int k = 0; k < 3; ++k) {
            char ch[32], v[64];
            std::snprintf(ch, sizeof ch, "fil_%d_%d", g, k);
            std::snprintf(v, sizeof v, "%.0f,%.0f", filHi_[g][k], filLo_[g][k]);
            c.set(ch, std::string(v));
        }
    for (int i = 0; i < 2; ++i) {
        const Receptor& R = rx_[i];
        std::ostringstream o;
        auto b = [](bool x) { return x ? "1" : "0"; };
        o << R.f << ',' << R.modo << ',' << R.fil << ',' << R.af << ',' << R.rf << ',' << R.sql << ','
          << b(R.nb1) << ',' << b(R.nb2) << ',' << b(R.nr1) << ',' << b(R.nr2) << ',' << b(R.nch) << ',' << b(R.nchLargo) << ','
          << b(R.bef) << ',' << b(R.anch) << ',' << b(R.bc) << ',' << b(R.apf) << ',' << b(R.mute) << ','
          << R.notch << ',' << R.agc << ',' << b(R.agcOff) << ',' << R.rit << ',' << b(R.ritOn) << ','
          << b(R.lock) << ',' << b(R.fine) << ',' << R.cont
          << ',' << R.paleta << ',' << R.brilho << ',' << R.range << ',' << R.acima << ',' << R.rangeCal;
        c.set(i == 0 ? "rx_main" : "rx_sub", o.str());
    }
    // o radio volta a abrir no MAIN
    if (ativo_ == 1) { c.set("vfo", (long long)rx_[0].f); c.set("modo", rx_[0].modo); }
    c.set("p_tx", txVfo_); c.set("p_att", att_); c.set("p_pamp", pamp_); c.set("p_meter", medidorModo_);
    c.set("p_dim", dim_); c.set("p_sdisp", sDisp_); c.set("p_scp", scp_);
    c.set("p_centro", modoCentro_); c.set("p_track", tracking_); c.set("p_proc", proc_);
    c.set("p_nb1", (double)nbNivel_); c.set("p_nb2", (double)nb2Nivel_); c.set("p_nr1", (double)nrNivel_);
    c.set("p_procn", (double)procNivel_); c.set("p_pitch", (double)cwPitch_);
    c.set("p_sqlm", (double)sqlMargem_); c.set("p_sqlh", (double)sqlHang_);
    c.set("p_sqlauto0", sqlAuto_[0]); c.set("p_sqlauto1", sqlAuto_[1]);
    c.set("p_paleta", paleta_); c.set("p_rxeq", rxeq_);
    c.set("p_painel_frac", (double)painelFrac_); c.set("p_esp_baixo", (double)fracBaixo_);
    c.set("p_multi", multiFunc_); c.set("p_cs", csFunc_); c.set("p_rfsql", rfSql_);
    c.set("p_3dss", modo3d_); c.set("p_mono", mono_); c.set("p_expand", expand_); c.set("p_regua_n", (int)std::lround(reguaN_)); c.set("p_regua_e", (int)std::lround(reguaE_)); c.set("p_cachoeira", cimaAtivo_); c.set("p_fast", fast_);
    c.set("p_dual", dual_); c.set("p_pagina", paginaSoft_); c.set("p_esp_cheio", espCheio_);
    for (int b = 0; b < kNB && b < (int)bandaMem_.size(); ++b) {
        if (!bandaMem_[b].f) continue;
        char ch[16]; std::snprintf(ch, sizeof ch, "banda_%d", b);
        c.set(ch, std::to_string(bandaMem_[b].f) + "," + bandaMem_[b].modo);
    }
    for (int q = 0; q < (int)qm_.size(); ++q) {
        char ch[16]; std::snprintf(ch, sizeof ch, "qm_%d", q);
        c.set(ch, std::to_string(qm_[q].f) + "," + qm_[q].modo);
    }
}

// FUNC > Escopo em tela cheia: o escopo na janela inteira, com a frequencia em cima (como o
// monitor externo que o radio de verdade aceita). ESC ou o botao volta.
void Ui::telaCheia()
{
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float W = io.DisplaySize.x, H = io.DisplaySize.y;
    // aqui o "papel" e a janela inteira em largura
    M.k = W / kLW; M.ox = 0; M.oy = 0;
    const float alt = H / M.k;
    dl->AddRectFilled(ImVec2(0, 0), ImVec2(W, H), P_LCD);
    const Receptor& O = rx_[1 - ativo_];
    char b[160];
    texto(f_.painel, 11.f, 12, 6, ativo_ == 0 ? P_BRANCO : P_AMARELO, ativo_ == 0 ? "MAIN" : "SUB");
    texto(f_.painel, 11.f, 70, 6, P_BRANCO, r_->modo().c_str());
    texto(f_.lcd, 26.f, 130, 2, r_->ligado() ? P_BRANCO : P_APAGADO, fmtFreqLcd(r_->vfo()).c_str());
    std::snprintf(b, sizeof b, "%s  %s  %s", ativo_ == 0 ? "SUB" : "MAIN", O.modo.c_str(), fmtFreqLcd(O.f).c_str());
    texto(f_.painel, 9.f, 420, 10, IM_COL32(0xb8, 0xc0, 0xcc, 255), b);
    std::snprintf(b, sizeof b, "S-meter %.0f dB   SPAN %.0fk   %s", peakDb_, r_->taxa() / (1.0 + zoom_) / 1000.0, modoCentro_ ? "CENTER" : "FIXED");
    texto(f_.painel, 9.f, 700, 10, P_CIANO, b);
    if (tecla("kScpVolta", "VOLTAR AO PAINEL  (ESC)", 1290, 5, 200, 20, 3, nullptr, "Volta ao painel do rádio")) scp_ = 0;
    escopo(6, 32, kLW - 12, alt - 38);
    if (!avisoPainel_.empty() && agora() < avisoAte_) texto(f_.painel, 9.f, 750, 40, P_AMBAR, avisoPainel_.c_str(), 0.5f);
}

// Band Scope: espectro + regua + cachoeira dentro de um retangulo do papel
void Ui::escopo(float x, float y, float w, float h)
{
    const ImVec2 a = M.p(x, y), b = M.p(x + w, y + h);
    wfVelDesenho_ = wfSpeedEsc_;          // cachoeira do Band Scope (e o IF dentro dele)
    ImGui::GetWindowDrawList()->PushClipRect(a, b, true);   // os nomes das memorias nao saem da tela
    escopoTela(a.x, a.y, b.x - a.x, b.y - a.y, espFrac_, "##lcdEsc", ifOn_ && !baixoAtivo_);
    ImGui::GetWindowDrawList()->PopClipRect();
}

// O Band Scope em coordenadas da tela (serve para o da tela do radio e para a
// cachoeira grande de baixo). Cada um com o seu ID: os dois podem estar na tela.
void Ui::escopoTela(float sx, float sy, float sw, float shh, float& frac, const char* id, bool comIf)
{
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushID(id);
    dl->AddRectFilled(ImVec2(sx, sy), ImVec2(sx + sw, sy + shh), IM_COL32(0, 0, 0, 255));
    float xm = sx;
    if (comIf) {
        const float wIf = std::floor(sw * 0.24f);
        ifDisplay(sx, sy, wIf, shh);
        xm = sx + wIf + 2.f * s_;
    }
    const float wm = sx + sw - xm;
    const float hEsc = std::max(14.f * s_, M.d(10));
    const float hEsp = std::floor(std::clamp(frac, 0.10f, 0.85f) * (shh - hEsc));
    // PAUSE: o espectro fica parado (troca a curva pela congelada so para desenhar)
    const bool troca = pausa_ && congelado_.size() == suave_.size() && !suave_.empty();
    if (troca) suave_.swap(congelado_);
    espectro(xm, sy, wm, hEsp);
    if (troca) suave_.swap(congelado_);
    escala(xm, sy + hEsp, wm, hEsc);
    ImGui::SetCursorScreenPos(ImVec2(xm, sy + hEsp));
    ImGui::InvisibleButton("##div990", ImVec2(std::max(1.f, wm), hEsc));
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ImGui::IsItemActive()) frac = std::clamp((io.MousePos.y - sy - hEsc * 0.5f) / (shh - hEsc), 0.10f, 0.85f);
    cachoeira(xm, sy + hEsp + hEsc, wm, shh - hEsp - hEsc);
    // marca do outro VFO e do NOTCH no espectro
    const float u = std::max(1.f, s_);
    dl->PushClipRect(ImVec2(xm, sy), ImVec2(xm + wm, sy + hEsp), true);
    {
        const Receptor& O = rx_[1 - ativo_];
        const float xo = (float)hzParaX((double)O.f, xm, wm);
        const ImU32 co = ativo_ == 0 ? P_AZULSUB : P_BRANCO;
        if (!mono_) {          // DUAL: a marca do outro receptor; MONO: so a do que se ouve
            dl->AddLine(ImVec2(xo, sy), ImVec2(xo, sy + hEsp), alfa(co, 140), u);
            dl->AddTriangleFilled(ImVec2(xo - 4 * u, sy), ImVec2(xo + 4 * u, sy), ImVec2(xo, sy + 5 * u), co);
        }
        const Receptor& R = rx_[ativo_];
        if ((R.nch || R.bef) && (r_->modo() == "USB" || r_->modo() == "LSB")) {
            const double nh = 150.0 * std::pow(3500.0 / 150.0, std::clamp(R.notch, 0.f, 1.f));
            const double fn = r_->modo() == "USB" ? (double)r_->vfo() + nh : (double)r_->vfo() - nh;
            const float xn = (float)hzParaX(fn, xm, wm);
            dl->AddLine(ImVec2(xn, sy + 5 * u), ImVec2(xn, sy + hEsp), alfa(P_CIANO, 200), R.bef ? 3.f * u : 1.5f * u);
            dl->AddTriangleFilled(ImVec2(xn - 4 * u, sy), ImVec2(xn + 4 * u, sy), ImVec2(xn, sy + 5 * u), P_CIANO);
        }
    }
    dl->PopClipRect();
    if (pausa_) {
        ImGui::PushFont(f_.negrito);
        const ImVec2 ts = ImGui::CalcTextSize("PAUSE");
        dl->AddRectFilled(ImVec2(sx + sw - ts.x - 12 * u, sy + 2 * u), ImVec2(sx + sw - 2 * u, sy + ts.y + 6 * u), IM_COL32(0, 0, 0, 200), 3 * u);
        dl->AddText(ImVec2(sx + sw - ts.x - 7 * u, sy + 4 * u), P_AMBAR, "PAUSE");
        ImGui::PopFont();
    }
    ImGui::PopID();
}

// (MB1SDR: ainda nao usada - a cachoeira de baixo vem depois)
// Embaixo do radio: barra de botoes (como a do RXSDR), IF DISPLAY a esquerda e
// a cachoeira grande. A barra fina entre o radio e a cachoeira sobe e desce.
void Ui::areaBaixo(float x, float y, float w, float h)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float u = std::max(1.f, s_);
    const float hb = 28 * u;
    dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + hb), IM_COL32(0x0a, 0x14, 0x0d, 255));
    dl->AddLine(ImVec2(x, y + hb - 1), ImVec2(x + w, y + hb - 1), IM_COL32(0x1c, 0x36, 0x24, 255));
    Receptor& R = rx_[ativo_];
    float bx = x + 8 * u;
    const float by = y + 3 * u, bh = hb - 6 * u;
    auto bt = [&](const char* rot, float bw, bool on, unsigned cor = 0) {
        const bool c = botao(rot, bx, by, bw * u, bh, on, cor);
        bx += bw * u + 4 * u;
        return c;
    };
    if (bt("IF DISPLAY", 92, ifOn_, IM_COL32(0xff, 0x9a, 0x1f, 255))) ifOn_ = !ifOn_;
    if (bt("NR ESPECTRAL", 112, R.nr2, IM_COL32(0x40, 0xff, 0x60, 255))) R.nr2 = !R.nr2;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("NR ESPECTRAL (o mesmo do RXSDR): tira o chiado frequência por frequência.\nNo painel é a tecla NR2; a força fica no anel do botão NR1/NR2 ou na setinha ao lado.");
    if (bt("v", 22, false)) ImGui::OpenPopup("##forcaNr");
    if (ImGui::BeginPopup("##forcaNr")) {
        ImGui::Text("Força do NR ESPECTRAL: %d", (int)nrEspForca_);
        ImGui::SetNextItemWidth(260 * u);
        ImGui::SliderFloat("##fz990", &nrEspForca_, 0, 100, "%.0f");
        ImGui::EndPopup();
    }
    if (bt("NR1", 46, R.nr1)) R.nr1 = !R.nr1;
    if (bt("NB1", 46, R.nb1)) R.nb1 = !R.nb1;
    bx += 8 * u;
    if (bt("AUTO", 56, autoWf_)) autoCachoeira();
    if (bt("PADRÃO", 66, false)) padrao();
    if (bt("ZOOM -", 62, false)) { zoom_ = std::max(0.f, zoom_ <= 1 ? 0.f : (zoom_ + 1) / 2 - 1); centrarParaZoom(true); }
    if (bt("ZOOM +", 62, zoom_ > 0)) { zoom_ = std::min(31.f, (zoom_ + 1) * 2 - 1); centrarParaZoom(true); }
    if (bt("MUTE", 56, R.mute, IM_COL32(0xff, 0x50, 0x50, 255))) R.mute = !R.mute;
    bx += 8 * u;
    {   // BW: largura do filtro do receptor ouvido (o mesmo do knob HI/SHIFT - LO/WIDTH)
        const int g = grupoModo(r_->modo());
        float& hi = filHi_[g][R.fil];
        const float lo = filLo_[g][R.fil];
        const float bwAtual = g == 2 ? hi * 2.f : hi;          // AM: as duas bandas laterais
        auto fmtBwK = [](float v) {
            char b[24];
            if (v >= 10000) std::snprintf(b, sizeof b, "%.0fk", v / 1000.f);
            else if (v >= 1000) std::snprintf(b, sizeof b, "%.2fk", v / 1000.f);
            else std::snprintf(b, sizeof b, "%.0f", v);
            return std::string(b);
        };
        auto aplicarBw = [&](float v) {
            static const float mn[5] = {1000, 50, 4000, 6000, 100000}, mx[5] = {5000, 2500, 16000, 30000, 250000};
            v = std::clamp(v, mn[g], mx[g]);
            hi = g == 2 ? v / 2.f : v;
            if (g == 0) hi = std::max(hi, lo + 400.f);
            aplicarFiltro();
        };
        const std::string rot = "BW " + fmtBwK(bwAtual);
        if (bt(rot.c_str(), 84, false, IM_COL32(0x40, 0xc8, 0xff, 255))) ImGui::OpenPopup("##bw990");
        const bool sobre = ImGui::IsItemHovered();
        static const float passo[5] = {100, 50, 1000, 2500, 10000};
        if (sobre && ImGui::GetIO().MouseWheel != 0) aplicarBw(bwAtual + (ImGui::GetIO().MouseWheel > 0 ? passo[g] : -passo[g]));
        if (sobre) ImGui::SetTooltip("BW: largura do filtro (%s  -  FIL %s)\nClique: lista de larguras.  Roda do mouse: ajusta.\n"
                                     "O mesmo do knob HI/SHIFT · LO/WIDTH (coluna MAIN RECEIVER) e da borda do filtro no espectro.",
                                     r_->modo().c_str(), kNomesFil[R.fil]);
        if (ImGui::BeginPopup("##bw990")) {
            static const float kSsb[] = {1800, 2100, 2400, 2700, 3000, 3600, 4000, 5000};
            static const float kCw[] = {50, 100, 200, 300, 500, 800, 1000, 2000};
            static const float kAm[] = {4000, 6000, 8000, 10000, 12000, 16000};
            static const float kFm[] = {6000, 8000, 10000, 12500, 15000, 20000, 25000};
            static const float kWfm[] = {100000, 150000, 180000, 200000, 250000};
            const float* l = g == 0 ? kSsb : g == 1 ? kCw : g == 2 ? kAm : g == 3 ? kFm : kWfm;
            const int n = g == 0 ? 8 : g == 1 ? 8 : g == 2 ? 6 : g == 3 ? 7 : 5;
            ImGui::TextDisabled("Largura (%s, FIL %s)", r_->modo().c_str(), kNomesFil[R.fil]);
            for (int i = 0; i < n; ++i) {
                const std::string t = fmtBwK(l[i]) + (l[i] < 1000 ? " Hz" : "Hz");
                if (ImGui::Selectable(t.c_str(), std::fabs(bwAtual - l[i]) < 1.f)) aplicarBw(l[i]);
            }
            ImGui::EndPopup();
        }
    }
    {   // VEL: velocidade das cachoeiras (a de baixo e a do Band Scope)
        char rv[24];
        std::snprintf(rv, sizeof rv, "VEL %.0fx", wfSpeed_);
        if (bt(rv, 70, false, IM_COL32(0x40, 0xc8, 0xff, 255))) ImGui::OpenPopup("##vel990");
        const bool sobre = ImGui::IsItemHovered();
        if (sobre && ImGui::GetIO().MouseWheel != 0)
            wfSpeed_ = std::clamp(wfSpeed_ + (ImGui::GetIO().MouseWheel > 0 ? 1.f : -1.f), 1.f, 10.f);
        if (sobre) ImGui::SetTooltip("VEL: velocidade da cachoeira de baixo (%.0fx)  -  roda do mouse ajusta\n"
                                     "Clique: as duas (a de baixo e a do Band Scope, que também está no KEY SPEED)", wfSpeed_);
        if (ImGui::BeginPopup("##vel990")) {
            ImGui::TextDisabled("Velocidade das cachoeiras (1 = devagar, 10 = rápida)");
            int vb = (int)std::lround(wfSpeed_), ve = (int)std::lround(wfSpeedEsc_);
            ImGui::SetNextItemWidth(220 * u);
            if (ImGui::SliderInt("Cachoeira de baixo", &vb, 1, 10, "%dx")) wfSpeed_ = (float)vb;
            ImGui::SetNextItemWidth(220 * u);
            if (ImGui::SliderInt("Band Scope (KEY SPEED)", &ve, 1, 10, "%dx")) wfSpeedEsc_ = (float)ve;
            ImGui::EndPopup();
        }
    }
    // a direita: receptor, modo, frequencia e hora UTC
    {
        SYSTEMTIME st; GetSystemTime(&st);
        char t[96];
        std::snprintf(t, sizeof t, "%s  %s  %s      %02d:%02d UTC", ativo_ == 0 ? "MAIN" : "SUB", r_->modo().c_str(),
                      fmtFreqLcd(r_->vfo()).c_str(), st.wHour, st.wMinute);
        ImGui::PushFont(f_.freqMouse ? f_.freqMouse : f_.negrito);
        const ImVec2 ts = ImGui::CalcTextSize(t);
        if (x + w - ts.x - 12 * u > bx) dl->AddText(ImVec2(x + w - ts.x - 12 * u, y + (hb - ts.y) * 0.5f), IM_COL32(0x40, 0xff, 0x70, 255), t);
        ImGui::PopFont();
    }
    // IF DISPLAY + cachoeira (na velocidade da cachoeira de baixo)
    wfVelDesenho_ = wfSpeed_;
    const float yy = y + hb + 2 * u, hh = h - hb - 2 * u;
    float xx = x;
    if (ifOn_) {
        const float largIf[3] = {220.f, 300.f, 420.f};          // estreita / media / larga (o menu do IF DISPLAY)
        const float wIf = std::min(largIf[std::clamp(ifTam_, 0, 2)] * u, w * 0.45f);
        ifDisplay(x + 2 * u, yy, wIf, hh);
        xx = x + wIf + 6 * u;
    }
    escopoTela(xx, yy, x + w - xx, hh, fracBaixo_, "##baixoEsc", false);
}

// ===========================================================================
//  O painel inteiro
// ===========================================================================
void Ui::painel()
{
    ImGuiIO& io = ImGui::GetIO();
    const float W = io.DisplaySize.x, H = io.DisplaySize.y;
    // Por enquanto so o radio, centrado na janela (a cachoeira de baixo vem
    // depois: areaBaixo() continua pronta no codigo, so nao e chamada).
    baixoAtivo_ = cimaAtivo_;
    // cachoeira ligada: o radio na largura da janela (se sobrar ao menos 120 px para a cachoeira)
    // e a janela livre; desligada: so o radio com os pes, na proporcao
    g_papelAlto = cimaAtivo_ ? 0.f : kLH + kPe;
    M.k = cimaAtivo_ ? std::max(0.1f, std::min(W / kLW, (H - 120.f) / (kLH + kPe))) : std::min(W / kLW, H / (kLH + kPe));
    M.ox = std::floor((W - kLW * M.k) * 0.5f);
    M.oy = std::floor(H - (kLH + kPe) * M.k);        // encostado embaixo: so os pes abaixo do radio
    for (int i = 0; i < 10; ++i) g_fp[i] = f_.painelT[i] ? f_.painelT[i] : f_.painel;
    for (int i = 0; i < 12; ++i) g_fl[i] = f_.lcdT[i] ? f_.lcdT[i] : f_.lcd;
    g_txt = std::clamp(1.50f / M.k, 1.0f, 1.65f);
    g_corRot = IM_COL32(0xf4, 0xf4, 0xf4, 255);
    // fundo: transparente (a cor-chave magenta some na tela - ficam so o radio e os pes) ou,
    // com fundo_transparente=0 no INI, o cinza de estudio
    if (g_fundoChave) ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(W, H), IM_COL32(255, 0, 255, 255));
    else ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(W, H), IM_COL32(0x3c, 0x3f, 0x44, 255), IM_COL32(0x3c, 0x3f, 0x44, 255),
                                                             IM_COL32(0x2a, 0x2c, 0x30, 255), IM_COL32(0x2a, 0x2c, 0x30, 255));

    sincronizarAtivo();
    aplicarReceptor();
    if (autoInicial_ && r_->aoVivo() && !suave_.empty()) {      // instalacao nova: o AUTO do escopo uma vez
        if (autoInicialT_ == 0) autoInicialT_ = agora();
        else if (agora() - autoInicialT_ > 2.5) { autoCachoeira(); autoInicial_ = false; }
    }
    if (sleepAte_ > 0 && agora() > sleepAte_) {
        sleepAte_ = 0;
        std::string e; r_->ligar(false, e);
        aviso("TIMER: rádio desligado", 5);
    }
    {   // S-meter do outro receptor: lido no espectro, no passa-faixa dele
        const float alvo = (r_->ligado() && r_->aoVivo()) ? ponteiroRx(1 - ativo_) : 0.01f;
        const float dt = std::max(0.001f, io.DeltaTime);
        const float a = alvo > smOutro_ ? 1.f - std::pow(1.f - 0.28f, dt * 60.f) : 1.f - std::exp(-dt / std::max(0.05f, smRetorno_));
        smOutro_ += (alvo - smOutro_) * a;
    }
    if (scp_ == 2) {                 // escopo em tela cheia (FUNC)
        telaCheia();
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) scp_ = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_F12)) g_pedirCaptura = true;
        return;
    }

    {   // os dois pes da frente (embaixo da tela e embaixo dos botoes da direita), como no radio
        ImDrawList* dl = ImGui::GetWindowDrawList();
        peDoRadio(dl, 100.f, 225.f);      // MB1SDR: os pes como na foto, a mesma distancia das laterais      // MB1SDR: pes cromados, a mesma distancia das laterais (121), como na foto
        peDoRadio(dl, 1275.f, 1400.f);
    }
    // ligou / desligou a cachoeira: a janela cresce para cima (ou volta), o pe fica no lugar
    if ((cimaAtivo_ ? 1 : 0) != cimaAplicado_) {
        HWND hw = (HWND)ImGui::GetMainViewport()->PlatformHandleRaw;
        if (hw && !IsZoomed(hw) && !IsIconic(hw)) {
            RECT wr, cr; GetWindowRect(hw, &wr); GetClientRect(hw, &cr);
            const int ncw = (wr.right - wr.left) - (cr.right - cr.left), nch = (wr.bottom - wr.top) - (cr.bottom - cr.top);
            MONITORINFO mi = {sizeof mi};
            RECT t = wr;
            if (GetMonitorInfoW(MonitorFromWindow(hw, MONITOR_DEFAULTTONEAREST), &mi)) t = mi.rcWork;
            if (cimaAtivo_) {        // ligou: a tela inteira (area de trabalho) - radio largo embaixo, cachoeira em cima
                SetWindowPos(hw, nullptr, t.left, t.top, t.right - t.left, t.bottom - t.top, SWP_NOZORDER | SWP_NOACTIVATE);
            } else {                 // desligou: so o radio, na proporcao, com o pe onde estava
                int ww = wr.right - wr.left, left = wr.left;
                int wh = (int)((cr.right - cr.left) * (kLH + kPe) / kLW + 0.5f) + nch;
                if (wh > t.bottom - t.top) {
                    wh = t.bottom - t.top;
                    const int ww2 = (int)((wh - nch) * kLW / (kLH + kPe) + 0.5f) + ncw;
                    left += (ww - ww2) / 2; ww = ww2;
                }
                const int top = std::clamp((int)(wr.bottom - wh), (int)t.top, std::max((int)t.top, (int)t.bottom - wh));
                SetWindowPos(hw, nullptr, left, top, ww, wh, SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        cimaAplicado_ = cimaAtivo_ ? 1 : 0;
    }
    // a cachoeira com espectro em cima do radio (na largura do radio)
    if (cimaAtivo_) {
        const float x0 = M.ox, x1 = M.ox + kLW * M.k, y1 = M.oy - std::max(2.f, M.d(4.f));
        if (y1 > 80.f) {
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(x0, 0), ImVec2(x1, y1), IM_COL32(0x04, 0x07, 0x05, 255));
            areaBaixo(x0, 0, x1 - x0, y1);
        }
    }
    // MB1SDR: o painel do MB1
    mb1Chassi();
    mb1Esquerda();
    mb1Tela();
    mb1Teclado();
    mb1Direita();
    icPopups();
    mb1Popups();
    popupsPainel();

    // teclado do PC: numeros digitam a frequencia (kHz), Enter confirma, Esc cancela
    if (!io.WantTextInput && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId)) {
        for (int i = 0; i < io.InputQueueCharacters.Size; ++i) {
            const ImWchar ch = io.InputQueueCharacters[i];
            if (ch >= '0' && ch <= '9') digito((int)(ch - '0'));
            if ((ch == '.' || ch == ',') && entrando_ && entrada_.find('.') == std::string::npos) entrada_ += '.';
        }
        if (entrando_ && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))) confirmarEntrada();
        if (entrando_ && ImGui::IsKeyPressed(ImGuiKey_Backspace) && !entrada_.empty()) entrada_.pop_back();
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) { entrando_ = false; entrada_.clear(); }
        if (ImGui::IsKeyPressed(ImGuiKey_F12)) { g_pedirCaptura = true; }
    }
    // aviso curto do painel (no meio da tela TFT)
    if (!avisoPainel_.empty() && agora() < avisoAte_) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float tw = larguraTexto(f_.painel, 9.f, avisoPainel_.c_str());
        const float cx = PX(517.f), cy = PY(300.f);       // MB1SDR: no meio do escopo
        dl->AddRectFilled(M.p(cx - tw * 0.5f - 10, cy - 4), M.p(cx + tw * 0.5f + 10, cy + 14), IM_COL32(0, 0, 0, 215), M.d(4));
        dl->AddRect(M.p(cx - tw * 0.5f - 10, cy - 4), M.p(cx + tw * 0.5f + 10, cy + 14), alfa(P_AMBAR, 200), M.d(4));
        texto(f_.painel, 9.f, cx, cy - 1, P_AMBAR, avisoPainel_.c_str(), 0.5f);
    }
}

// Caixa do radio: chapa preta, aba de cima, placa do modelo e a reticula
void Ui::chassi()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = M.p(0, 0), b = M.p(kLW, kLH);
    {
        const ImDrawListFlags fl = dl->Flags;
        if (g_fundoChave) dl->Flags &= ~ImDrawListFlags_AntiAliasedFill;
        dl->AddRectFilled(ImVec2(a.x - M.d(2), a.y - M.d(1)), ImVec2(b.x + M.d(2), b.y + M.d(1)), IM_COL32(0, 0, 0, 255), M.d(12));
        dl->Flags = fl;
    }
    dl->AddRectFilledMultiColor(a, b, P_CHASSI, P_CHASSI, P_CHASSI2, P_CHASSI2);
    dl->AddRect(a, b, IM_COL32(0x38, 0x3e, 0x44, 255), M.d(10), 0, M.d(1.5f));
    // aba de cima (a tampa do radio vista de frente)
    dl->AddRectFilledMultiColor(M.p(2, 2), M.p(kLW - 2, 28), IM_COL32(0x26, 0x2a, 0x2e, 255), IM_COL32(0x26, 0x2a, 0x2e, 255),
                                IM_COL32(0x13, 0x16, 0x19, 255), IM_COL32(0x13, 0x16, 0x19, 255));
    dl->AddLine(M.p(4, 28.5f), M.p(kLW - 4, 28.5f), IM_COL32(0, 0, 0, 255), M.d(1.6f));
    dl->AddLine(M.p(4, 30), M.p(kLW - 4, 30), IM_COL32(255, 255, 255, 26), M.d(0.8f));
    dl->AddLine(M.p(10, 3), M.p(kLW - 10, 3), IM_COL32(255, 255, 255, 40), M.d(0.8f));
    // placa do modelo (em cima, a direita - na foto: x 702..940 e 950..1145)
    {
        const char* placa = "HF/VHF/UHF  SDR RECEIVER";
        const float xp0 = FX(702), xp1 = FX(940);
        dl->AddRectFilled(M.p(xp0, 9), M.p(xp1, 23.5f), IM_COL32(0x0a, 0x0b, 0x0d, 255), M.d(2.5f));
        dl->AddRect(M.p(xp0, 9), M.p(xp1, 23.5f), P_DOURADO, M.d(2.5f), 0, M.d(0.9f));
        textoCabe(f_.painel, 6.8f, (xp0 + xp1) * 0.5f, 10.6f, IM_COL32(0xf0, 0xf0, 0xf2, 255), placa, 0.5f, xp1 - xp0 - 8.f);
        // "MB1SDR" dourado: FT grande, dx pequeno, SDR grande
        ImFont* fm = f_.gabriola ? f_.gabriola : f_.painel;
        const float tg = 21.f, tp = 13.f;
        const float w1 = larguraTexto(fm, tg, "FT"), w2 = larguraTexto(fm, tp, "dx");
        float xx = 16.f;
        const float yy = 3.f;
        auto dourado = [&](float t, float x, float y, const char* s) {
            texto(fm, t, x + 0.8f, y + 0.8f, IM_COL32(0x20, 0x14, 0x00, 230), s);
            texto(fm, t, x, y, P_DOURADO, s);
        };
        dourado(tg, xx, yy, "FT"); xx += w1 + 1.f;
        dourado(tp, xx, yy + (tg - tp) * 0.62f, "dx"); xx += w2 + 1.f;
        dourado(tg, xx, yy, "SDR");
        xx += larguraTexto(fm, tg, "SDR") + 10.f;
        texto(f_.painel, 12.f, xx + 0.7f, yy + 7.6f + 0.7f, IM_COL32(0x20, 0x14, 0x00, 230), "v" RXSDR_VERSAO);
        texto(f_.painel, 12.f, xx, yy + 7.6f, P_DOURADO, "v" RXSDR_VERSAO);
    }
    // a "barra de titulo" na tampa: versao a esquerda; minimizar / maximizar / fechar depois do
    // MB1SDR dourado; arrastar a tampa move a janela e o duplo clique maximiza / volta
    {
        HWND hw = (HWND)ImGui::GetMainViewport()->PlatformHandleRaw;
        if (!hw) hw = GetActiveWindow();
        const float bw = 22.f, bh = 16.f, gap = 3.f, by = 6.f;
        const float bx0 = kLW - 10.f - 3 * bw - 2 * gap;
        const bool max = hw && IsZoomed(hw);
        {   // CACHOEIRA: liga / desliga a cachoeira com espectro em cima do radio
            const float w = 66.f, x = bx0 - 10.f - w;
            const ImVec2 a = M.p(x, by), b = M.p(x + w, by + bh);
            ImGui::SetCursorScreenPos(a);
            ImGui::PushID(9099);
            if (ImGui::InvisibleButton("##cach", ImVec2(b.x - a.x, b.y - a.y))) cimaAtivo_ = !cimaAtivo_;
            const bool sob = ImGui::IsItemHovered(), at = ImGui::IsItemActive();
            ImGui::PopID();
            dl->AddRectFilled(a, b, at ? IM_COL32(0x10, 0x12, 0x14, 255) : sob ? IM_COL32(0x3a, 0x40, 0x48, 255) : IM_COL32(0x1a, 0x1d, 0x21, 255), M.d(2.5f));
            dl->AddRect(a, b, cimaAtivo_ ? alfa(P_LARANJA, 230) : IM_COL32(0x4a, 0x52, 0x5c, 200), M.d(2.5f), 0, M.d(0.8f));
            const float tt = std::min(7.6f, bh * 0.62f);
            textoCabe(f_.painel, tt, x + w * 0.5f, by + (bh - std::min(tamReal(tt), bh * 0.9f)) * 0.5f, cimaAtivo_ ? P_LARANJA : IM_COL32(0xe0, 0xe4, 0xea, 255), "CACHOEIRA", 0.5f, w - 6.f);
            if (sob && !at) ImGui::SetTooltip("%s", cimaAtivo_ ? "Cachoeira com espectro em cima do rádio: DESLIGAR" : "Cachoeira com espectro em cima do rádio: LIGAR");
        }
        for (int i = 0; i < 3; ++i) {
            const float x = bx0 + i * (bw + gap);
            const ImVec2 a = M.p(x, by), b = M.p(x + bw, by + bh);
            ImGui::SetCursorScreenPos(a);
            ImGui::PushID(9100 + i);
            const bool clicou = ImGui::InvisibleButton("##jan", ImVec2(b.x - a.x, b.y - a.y));
            const bool sob = ImGui::IsItemHovered(), at = ImGui::IsItemActive();
            ImGui::PopID();
            ImU32 fundo = at ? IM_COL32(0x10, 0x12, 0x14, 255) : sob ? IM_COL32(0x3a, 0x40, 0x48, 255) : IM_COL32(0x1a, 0x1d, 0x21, 255);
            if (i == 2 && (sob || at)) fundo = at ? IM_COL32(0x90, 0x1c, 0x14, 255) : IM_COL32(0xc4, 0x2b, 0x1c, 255);
            dl->AddRectFilled(a, b, fundo, M.d(2.5f));
            dl->AddRect(a, b, IM_COL32(0x4a, 0x52, 0x5c, 200), M.d(2.5f), 0, M.d(0.7f));
            const ImU32 cs = IM_COL32(0xe8, 0xec, 0xf0, 255);
            const float cx = x + bw * 0.5f, cy = by + bh * 0.5f, t = std::max(1.f, M.d(1.1f));
            if (i == 0) dl->AddLine(M.p(cx - 4.5f, cy + 2.5f), M.p(cx + 4.5f, cy + 2.5f), cs, t);
            else if (i == 1) {
                if (max) {   // duas janelas sobrepostas = voltar ao tamanho anterior
                    dl->AddRect(M.p(cx - 4.f, cy - 2.f), M.p(cx + 2.5f, cy + 4.5f), cs, 0.f, 0, t);
                    dl->AddLine(M.p(cx - 1.5f, cy - 4.5f), M.p(cx + 4.5f, cy - 4.5f), cs, t);
                    dl->AddLine(M.p(cx + 4.5f, cy - 4.5f), M.p(cx + 4.5f, cy + 2.f), cs, t);
                } else dl->AddRect(M.p(cx - 4.5f, cy - 4.f), M.p(cx + 4.5f, cy + 4.f), cs, 0.f, 0, t);
            } else {
                dl->AddLine(M.p(cx - 4.f, cy - 4.f), M.p(cx + 4.f, cy + 4.f), cs, t);
                dl->AddLine(M.p(cx + 4.f, cy - 4.f), M.p(cx - 4.f, cy + 4.f), cs, t);
            }
            if (sob && !at) ImGui::SetTooltip("%s", i == 0 ? "Minimizar" : i == 1 ? (max ? "Restaurar" : "Maximizar") : "Fechar");
            if (clicou && hw) {
                if (i == 0) ShowWindow(hw, SW_MINIMIZE);
                else if (i == 1) ShowWindow(hw, max ? SW_RESTORE : SW_MAXIMIZE);
                else PostMessageW(hw, WM_CLOSE, 0, 0);
            }
        }
        // arrastar pela tampa (fora dos botoes)
        const ImVec2 t0 = M.p(0, 0), t1 = M.p(kLW, 29.f);
        const ImVec2 mp = ImGui::GetIO().MousePos;
        const bool naTampa = mp.x >= t0.x && mp.x < t1.x && mp.y >= t0.y && mp.y < t1.y && !ImGui::IsAnyItemHovered() &&
                             ImGui::IsWindowHovered(ImGuiHoveredFlags_None);
        if (naTampa && hw) {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) ShowWindow(hw, max ? SW_RESTORE : SW_MAXIMIZE);
            else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                ReleaseCapture();
                SendMessageW(hw, WM_NCLBUTTONDOWN, HTCAPTION, 0);      // o Windows move a janela
                ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, false);   // o botao ja foi solto
            }
        }
    }
    // reticula em volta dos botoes (como no radio): na fileira do SUB (em cima)
    // azul-violeta, maior em cima; na do MAIN (embaixo) verde-azulada, maior embaixo
    const ImU32 cSub = IM_COL32(0x5c, 0x6e, 0xc8, 225), cMain = IM_COL32(0x52, 0x7c, 0x8c, 215);
    pontilhado(744, 220, 886, 344, 3.9f, cSub, +1);
    pontilhado(744, 381, 1499, 537, 3.9f, cMain, -1);     // passa por baixo do VFO
    pontilhado(1246, 219, 1499, 340, 3.9f, cSub, +1);
    // botao direito no chassi = FUNC
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##funcFt");
}

// Coluna da esquerda: ON/OFF, USB, KEY, PHONES, MIC
void Ui::colunaEsquerda()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool lig = r_->ligado();
    texto(f_.painel, 6.6f, 40, 38, P_ROTULO, "ON/OFF", 0.5f);
    {   // aro azul em volta da tecla (aceso com o radio ligado)
        const ImU32 azul = lig ? IM_COL32(0x2e, 0x86, 0xff, 255) : IM_COL32(0x1c, 0x3a, 0x70, 255);
        if (lig) dl->AddRect(M.p(13, 49), M.p(67, 83), IM_COL32(0x2e, 0x86, 0xff, 70), M.d(7), 0, M.d(4));
        dl->AddRect(M.p(14, 50), M.p(66, 82), azul, M.d(6), 0, M.d(2.2f));
        if (tecla("kPwr", "", 18, 54, 44, 24, 0, nullptr, lig ? "ON/OFF: desligar o rádio (para o dongle)" : "ON/OFF: ligar o rádio (abre o dongle)"))
            ligarDesligar();
        const ImVec2 c = M.p(40, 66);
        const float r = M.d(5.5f);
        const ImU32 cs = lig ? IM_COL32(0xff, 0x5a, 0x40, 255) : IM_COL32(0x90, 0x50, 0x48, 255);
        dl->PathArcTo(c, r, (float)(-kPi * 0.5 + 0.7), (float)(kPi * 1.5 - 0.7), 24);
        dl->PathStroke(cs, 0, M.d(1.5f));
        dl->AddLine(ImVec2(c.x, c.y - r - M.d(1)), ImVec2(c.x, c.y + M.d(0.5f)), cs, M.d(1.5f));
    }
    texto(f_.painel, 6.2f, 40, 96, P_ROTULO2, "USB", 0.5f);
    for (int i = 0; i < 2; ++i) {
        const float y = 108 + i * 22.f;
        dl->AddRectFilled(M.p(20, y), M.p(60, y + 15), IM_COL32(0x2a, 0x2c, 0x30, 255), M.d(2));
        dl->AddRectFilled(M.p(23, y + 3), M.p(57, y + 12), IM_COL32(0x04, 0x04, 0x05, 255), M.d(1));
        dl->AddRectFilled(M.p(26, y + 5), M.p(54, y + 8), IM_COL32(0x8a, 0x8e, 0x96, 255), M.d(0.5f));
    }
    auto jaque = [&](float y, const char* rot) {
        texto(f_.painel, 6.6f, 40, y - 25, P_ROTULO, rot, 0.5f);
        const ImVec2 c = M.p(40, y);
        dl->AddCircleFilled(c, M.d(12), IM_COL32(0x2c, 0x2e, 0x32, 255), 32);
        dl->AddCircle(c, M.d(12), IM_COL32(0x60, 0x62, 0x68, 255), 32, M.d(1.2f));
        dl->AddCircleFilled(c, M.d(8), IM_COL32(0x9a, 0x9c, 0xa2, 255), 32);
        dl->AddCircleFilled(c, M.d(4.2f), IM_COL32(0, 0, 0, 255), 16);
    };
    jaque(194, "KEY");
    jaque(252, "PHONES");
    texto(f_.painel, 6.6f, 40, 292, P_ROTULO, "MIC", 0.5f);
    {
        const ImVec2 c = M.p(40, 336);
        dl->AddCircleFilled(c, M.d(25), IM_COL32(0xa4, 0xa6, 0xac, 255), 48);
        dl->AddCircle(c, M.d(25), IM_COL32(0xd8, 0xda, 0xe0, 255), 48, M.d(1));
        for (int i = 0; i < 24; ++i) {
            const ImVec2 d = dirAng((float)(i * kPi / 12));
            dl->AddLine(ImVec2(c.x + d.x * M.d(21), c.y + d.y * M.d(21)), ImVec2(c.x + d.x * M.d(25), c.y + d.y * M.d(25)), IM_COL32(0x70, 0x72, 0x78, 255), M.d(0.8f));
        }
        dl->AddCircleFilled(c, M.d(19), IM_COL32(0x1a, 0x1b, 0x1e, 255), 40);
        for (int i = 0; i < 7; ++i) {
            const ImVec2 d = dirAng((float)(i * 2 * kPi / 7 - kPi / 2));
            dl->AddCircleFilled(ImVec2(c.x + d.x * M.d(11), c.y + d.y * M.d(11)), M.d(2), IM_COL32(0xc0, 0xa8, 0x70, 255), 10);
        }
        dl->AddCircleFilled(c, M.d(2), IM_COL32(0xc0, 0xa8, 0x70, 255), 10);
    }
}

// Faixa do receptor rx (o que ele ouve): f0..f1 em Hz
void Ui::faixaRx(int rx, double& f0, double& f1) const
{
    const Receptor& R = rx_[rx];
    const bool at = rx == ativo_;
    const double v = at ? (double)r_->vfo() : (double)R.f;
    const std::string m = at ? r_->modo() : R.modo;
    double bw;
    if (at) bw = r_->banda();
    else {
        const int g = grupoModo(m);
        bw = g == 2 ? filHi_[g][R.fil] * 2.0 : filHi_[g][R.fil];
    }
    bw = std::max(50.0, bw);
    if (m == "USB") { f0 = v; f1 = v + bw; }
    else if (m == "LSB") { f0 = v - bw; f1 = v; }
    else { f0 = v - bw / 2; f1 = v + bw / 2; }
}

// Posicao do ponteiro (0..1) do receptor rx lida no espectro (o mesmo calculo
// do S-meter do Ui.cpp, no passa-faixa dele). Fora da faixa do dongle: zero.
float Ui::ponteiroRx(int rx) const
{
    // MB1SDR: com dois aparelhos, o VFO que nao se ouve e lido no espectro do aparelho dele
    const bool deOutro = duas_ && rx != ativo_;
    const QuadroFft& fft_ = deOutro ? fftO_ : this->fft_;
    const std::vector<float>& suave_ = deOutro ? suaveO_ : this->suave_;
    const float pisoMed_ = deOutro ? pisoO_ : this->pisoMed_;
    const int nb = (int)suave_.size();
    if (nb < 16 || fft_.bins.size() != (size_t)nb) return 0.05f;
    double f0, f1;
    faixaRx(rx, f0, f1);
    const double sr = fft_.taxa ? fft_.taxa : r_->taxa();
    const double c0 = (double)fft_.centro - sr / 2, c1 = (double)fft_.centro + sr / 2;
    if (f1 < c0 || f0 > c1) return 0.02f;
    auto bin = [&](double f) { return std::clamp((int)std::lround((f - (double)fft_.centro) / sr * nb + nb / 2.0), 0, nb - 1); };
    int a = bin(f0), b = bin(f1);
    if (a > b) std::swap(a, b);
    const int cb = nb / 2;
    float pk = -200;
    for (int i = a; i <= b; ++i) {
        if (std::abs(i - cb) <= 1 && (b - a) > 2) continue;
        pk = std::max(pk, (float)fft_.bins[i]);    // o mesmo dado do ponteiro do receptor ouvido
    }
    if (pk <= -199.f) return 0.05f;
    const auto& c = Config::instance();
    const double v = (f0 + f1) * 0.5;
    const bool hf = r_->qAtivo() || v < 30e6;
    const float s9Ref = (float)(hf ? c.num("s9_hf", -94) : c.num("s9_vhf", -93));
    // MB1SDR: como o do receptor ouvido (Ui.cpp): dB bruto acima do piso, com a Referencia S9
    const float desloc = s9Ref - (hf ? -94.f : -93.f);
    return pisoMed_ < 1e8f ? posMedidor((pk - pisoMed_) * std::pow(10.f, -desloc / 64.f)) : 0.05f;   // como no Ui.cpp
}

// S-meter da tela TFT (um para cada receptor): fundo preto, escalas brancas e
// azuis em arco, ponteiro laranja. Em RX so o S anda; as outras escalas sao as
// do radio (PO / COMP / TEMP no MAIN, ALC / ID / SWR no SUB).
void Ui::medidorFt(float x, float y, float w, float h, int rx)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool lig = r_->ligado();
    // faixa do titulo
    dl->AddRectFilledMultiColor(M.p(x + 24, y), M.p(x + w - 24, y + 9), IM_COL32(0x2a, 0x5c, 0xc8, 255), IM_COL32(0x2a, 0x5c, 0xc8, 255),
                                IM_COL32(0x14, 0x30, 0x80, 255), IM_COL32(0x14, 0x30, 0x80, 255));
    texto(f_.painel, 5.8f, x + w * 0.5f, y + 0.6f, IM_COL32(0xe8, 0xf0, 0xff, 255), rx == 0 ? "MAIN  S METER" : "SUB  S METER", 0.5f);
    const float y0 = y + 10, hh = h - 10;
    dl->PushClipRect(M.p(x, y0), M.p(x + w, y0 + hh), true);
    const float R0 = w * 0.70f;
    const float cx = x + w * 0.5f, cy = y0 + 14.f + R0;
    const float meio = 33.f;     // graus para cada lado
    auto ponto = [&](float pos, float raio) {
        const float an = (float)((-90.0 + (pos - 0.5) * 2.0 * meio) * kPi / 180.0);
        return ImVec2(cx + raio * std::cos(an), cy + raio * std::sin(an));
    };
    auto arco = [&](float p0, float p1, float raio, ImU32 cor, float esp) {
        const int n = 36;
        for (int k = 0; k < n; ++k) {
            const ImVec2 q0 = ponto(p0 + (p1 - p0) * k / n, raio), q1 = ponto(p0 + (p1 - p0) * (k + 1) / n, raio);
            dl->AddLine(M.p(q0.x, q0.y), M.p(q1.x, q1.y), cor, M.d(esp));
        }
    };
    auto marca = [&](float pos, float r0, float r1, ImU32 cor, float esp) {
        const ImVec2 q0 = ponto(pos, r0), q1 = ponto(pos, r1);
        dl->AddLine(M.p(q0.x, q0.y), M.p(q1.x, q1.y), cor, M.d(esp));
    };
    auto txMed = [&](float tam, float bx, float by, ImU32 cor, const char* t, float alin) {
        const float px = std::max(M.d(tam), 6.0f);
        ImFont* fo = melhor(f_.painel, px);
        const ImVec2 ts = fo->CalcTextSizeA(px, FLT_MAX, 0, t);
        const ImVec2 q = M.p(bx, by);
        dl->AddText(fo, px, ImVec2(std::floor(q.x - ts.x * alin), std::floor(q.y - ts.y * 0.5f)), cor, t);
    };
    const ImU32 branco = lig ? IM_COL32(0xf0, 0xf4, 0xff, 255) : IM_COL32(0x70, 0x74, 0x80, 255);
    const ImU32 azul = lig ? IM_COL32(0x4c, 0xa0, 0xff, 255) : IM_COL32(0x30, 0x48, 0x70, 255);
    const ImU32 cinza = lig ? IM_COL32(0xa8, 0xbc, 0xdc, 255) : IM_COL32(0x50, 0x58, 0x68, 255);
    // S: 1 3 5 7 9 brancos, +20 +40 +60 azuis
    arco(kSPos[0], kSPos[9], R0, branco, 1.1f);
    arco(kSPos[9], kDbPos[5], R0, azul, 2.2f);
    for (int i = 0; i < 10; ++i) {
        marca(kSPos[i], R0, R0 + ((i % 2) ? 6.f : 3.5f), branco, 0.9f);
        if (i % 2 == 1) {
            char t[4]; std::snprintf(t, sizeof t, "%d", i);
            const ImVec2 q = ponto(kSPos[i], R0 + 11.f);
            txMed(6.4f, q.x, q.y, branco, t, 0.5f);
        }
    }
    for (int i = 0; i < 6; ++i) {
        marca(kDbPos[i], R0, R0 + ((i % 2) ? 6.f : 3.5f), azul, 0.9f);
        if (i % 2 == 1) {
            const char* t[] = {"", "+20", "", "+40", "", "+60"};
            const ImVec2 q = ponto(kDbPos[i], R0 + 11.f);
            txMed(5.6f, q.x, q.y, azul, t[i], 0.5f);
        }
    }
    { const ImVec2 q = ponto(0.0f, R0 + 9.f); txMed(7.f, q.x - 2, q.y, branco, "S", 1.f); }
    { const ImVec2 q = ponto(1.0f, R0 + 3.f); txMed(5.f, q.x + 2, q.y, azul, "dB", 0.f); }
    // as escalas de enfeite (TX)
    struct Esc { const char* nome; const char* fim; float r; std::vector<const char*> rot; };
    const Esc escM[3] = {{"PO", "W", R0 - 13, {"0", "10", "50", "100", "150", "200", "250"}},
                         {"COMP", "dB", R0 - 24, {"0", "5", "10", "15", "20"}},
                         {"TEMP", "\xC2\xB0" "C", R0 - 35, {"0", "20", "40", "60", "80", "100"}}};
    const Esc escS[3] = {{"ALC", "", R0 - 13, {"", "", "", "", ""}},
                         {"ID", "A", R0 - 24, {"0", "5", "10", "15"}},
                         {"SWR", "", R0 - 35, {"1", "1.5", "2", "3", "5", "oo"}}};
    const Esc* es = rx == 0 ? escM : escS;
    for (int k = 0; k < 3; ++k) {
        const float p0 = 0.06f + k * 0.035f, p1 = 0.96f - k * 0.035f;
        arco(p0, p1, es[k].r, cinza, 0.8f);
        const int n = (int)es[k].rot.size();
        for (int i = 0; i < n; ++i) {
            const float pos = p0 + (p1 - p0) * i / std::max(1, n - 1);
            marca(pos, es[k].r, es[k].r + 3.f, cinza, 0.7f);
            if (es[k].rot[i][0]) { const ImVec2 q = ponto(pos, es[k].r - 4.6f); txMed(4.4f, q.x, q.y, cinza, es[k].rot[i], 0.5f); }
        }
        if (k == 0 && rx == 1) arco(0.06f, 0.30f, es[k].r + 1.2f, lig ? IM_COL32(0xff, 0x50, 0x40, 255) : cinza, 1.6f);   // faixa do ALC
        { const ImVec2 q = ponto(p0, es[k].r); txMed(4.8f, q.x - 2, q.y, cinza, es[k].nome, 1.f); }
        if (es[k].fim[0]) { const ImVec2 q = ponto(p1, es[k].r); txMed(4.6f, q.x + 2, q.y, cinza, es[k].fim, 0.f); }
    }
    // ponteiro
    const float pos = std::clamp(rx == ativo_ ? smAtual_ : smOutro_, 0.f, 1.02f);
    {
        const ImVec2 p0 = ponto(pos, R0 - 52.f), p1 = ponto(pos, R0 + 4.f);
        dl->AddLine(M.p(p0.x, p0.y), M.p(p1.x, p1.y), lig ? IM_COL32(0xff, 0x5a, 0x1e, 255) : IM_COL32(0x80, 0x40, 0x30, 255), M.d(1.5f));
    }
    dl->PopClipRect();
    // leitura em texto (canto de baixo)
    if (lig) {
        char b[32];
        if (rx == ativo_ && medidorModo_ == 1) std::snprintf(b, sizeof b, "%.0f dB", peakDb_);
        else if (rx == ativo_ && medidorModo_ == 2) std::snprintf(b, sizeof b, "PK %.0f", peakDbSql_);
        else std::snprintf(b, sizeof b, "%s", leituraS(pos).c_str());
        texto(f_.painel, 5.6f, x + w - 2, y + h - 8.5f, IM_COL32(0x80, 0xb0, 0xf0, 255), b, 1.f);
    }
    // clique no medidor: troca a leitura (S / dB / PEAK)
    ImGui::SetCursorScreenPos(M.p(x, y0));
    char id[16]; std::snprintf(id, sizeof id, "##med%d", rx);
    if (ImGui::InvisibleButton(id, ImVec2(M.d(w), M.d(hh)))) medidorModo_ = (medidorModo_ + 1) % 3;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("S-meter do %s%s\nClique: leitura em S / dB / PEAK", rx == 0 ? "MAIN" : "SUB",
                                                  rx == ativo_ ? " (receptor ouvido)" : " (lido no espectro, no filtro dele)");
}

// Caixinha ao lado do medidor (a "filter function display" do radio): o
// espectro em volta da sintonia do receptor com o desenho do filtro em vermelho
void Ui::caixaFiltro(float x, float y, float w, float h, int rx)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(M.p(x, y), M.p(x + w, y + h), IM_COL32(0x01, 0x02, 0x06, 255));
    dl->AddRect(M.p(x, y), M.p(x + w, y + h), IM_COL32(0x30, 0x38, 0x48, 255), 0, 0, M.d(0.7f));
    double f0, f1;
    faixaRx(rx, f0, f1);
    const double meio = (f0 + f1) * 0.5, span = std::max(1500.0, (f1 - f0) * 1.7);
    auto fx = [&](double f) { return (float)(x + 3 + (f - (meio - span / 2)) / span * (w - 6)); };
    dl->PushClipRect(M.p(x + 1, y + 1), M.p(x + w - 1, y + h - 1), true);
    const int nb = (int)suave_.size();
    if (r_->ligado() && nb > 16 && fft_.bins.size() == (size_t)nb) {
        const double sr = fft_.taxa ? fft_.taxa : r_->taxa();
        const float range = std::clamp(wfRange_, 10.f, 120.f);
        std::vector<ImVec2> pts;
        const int n = 48;
        for (int i = 0; i <= n; ++i) {
            const double f = meio - span / 2 + span * i / n;
            const int bi = (int)std::lround((f - (double)fft_.centro) / sr * nb + nb / 2.0);
            if (bi < 0 || bi >= nb) continue;
            const float db = suave_[bi];
            const float yy = y + h - 4 - std::clamp((db + range) / range, 0.f, 1.f) * (h - 10);
            pts.push_back(M.p(x + 3 + (w - 6) * i / (float)n, yy));
        }
        if (pts.size() > 1) dl->AddPolyline(pts.data(), (int)pts.size(), IM_COL32(0xd8, 0xe4, 0xff, 200), 0, M.d(0.7f));
    }
    // o filtro (trapezio vermelho)
    const float xa = fx(f0), xb = fx(f1), sl = std::min(4.f, (xb - xa) * 0.2f);
    const ImU32 vm = r_->ligado() ? IM_COL32(0xff, 0x3c, 0x30, 255) : IM_COL32(0x80, 0x30, 0x30, 255);
    dl->AddLine(M.p(xa, y + h - 4), M.p(xa + sl, y + 9), vm, M.d(1.2f));
    dl->AddLine(M.p(xa + sl, y + 9), M.p(xb - sl, y + 9), vm, M.d(1.2f));
    dl->AddLine(M.p(xb - sl, y + 9), M.p(xb, y + h - 4), vm, M.d(1.2f));
    const Receptor& R = rx_[rx];
    if (R.nch || R.bef) {      // NOTCH / CONT (so faz sentido em SSB)
        const std::string m = rx == ativo_ ? r_->modo() : R.modo;
        auto fNotch = [&](float t) {
            const double nh = 150.0 * std::pow(3500.0 / 150.0, std::clamp(t, 0.f, 1.f));
            return m == "LSB" ? (rx == ativo_ ? (double)r_->vfo() : (double)R.f) - nh : (rx == ativo_ ? (double)r_->vfo() : (double)R.f) + nh;
        };
        if (R.nch) { const float xn = fx(fNotch(R.notch)); dl->AddLine(M.p(xn, y + 9), M.p(xn, y + h - 4), P_CIANO, M.d(1.f)); }
        if (R.bef) { const float xn = fx(fNotch(R.cont)); dl->AddLine(M.p(xn, y + 9), M.p(xn, y + h - 4), P_AMBAR, M.d(2.f)); }
    }
    dl->PopClipRect();
    dl->AddLine(M.p(x + 3, y + h - 2.5f), M.p(x + w - 3, y + h - 2.5f), IM_COL32(0x2a, 0x6c, 0xff, 255), M.d(1.6f));
}

// Linha do VFO de um receptor: "VFO", o modo na etiqueta vermelha e a frequencia
void Ui::ladoVfo(float x, float y, float w, float h, int rx)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const Receptor& R = rx_[rx];
    const bool at = rx == ativo_;
    const bool lig = r_->ligado();
    const uint64_t f = at ? r_->vfo() : R.f;
    const std::string md = at ? r_->modo() : R.modo;
    char b[64];
    if (at) dl->AddRectFilledMultiColor(M.p(x, y), M.p(x + w, y + h), IM_COL32(0x10, 0x20, 0x48, 160), IM_COL32(0x10, 0x20, 0x48, 160),
                                        IM_COL32(0x10, 0x20, 0x48, 0), IM_COL32(0x10, 0x20, 0x48, 0));
    // VFO / M-xx
    if (R.memModo && !mem_.empty()) std::snprintf(b, sizeof b, "M-%02d", std::clamp(R.memCanal, 0, (int)mem_.size() - 1) + 1);
    else std::snprintf(b, sizeof b, "VFO");
    texto(f_.painel, 7.6f, x + 4, y + 1, at ? P_TFTTXT : IM_COL32(0x70, 0x90, 0xc0, 255), b);
    // etiqueta do modo
    dl->AddRectFilled(M.p(x + 4, y + 15), M.p(x + 46, y + 28), at ? P_MODO : IM_COL32(0x90, 0x3a, 0x2a, 255), M.d(1.5f));
    textoCabe(f_.painel, 7.2f, x + 25, y + 16.2f, P_BRANCO, nomeModo(md).c_str(), 0.5f, 40.f);
    // indicadores pequenos
    {
        float xi = x + 30;
        auto ind = [&](const char* t, ImU32 c) {
            texto(f_.painel, 5.2f, xi, y + 2.5f, c, t);
            xi += larguraTexto(f_.painel, 5.2f, t) + 4.f;
        };
        const bool busy = lig && r_->aoVivo() && ocupadoRx(rx);
        if (busy) ind("BUSY", P_VERDE);
        if (txVfo_ == rx && txVfo_ != ativo_) ind("TX", P_VERMELHO);
        if (R.lock) ind("LOCK", P_VERMELHO);
        if (R.fine) ind("FINE", P_CIANO);
        if (R.mute) ind("MUTE", P_VERMELHO);
        if (R.sql > -90.f) {
            const bool fechado = at && lig && !sqlAberto_;
            std::snprintf(b, sizeof b, "SQL%+d", (int)std::lround(R.sql));
            ind(b, fechado ? P_VERMELHO : P_AMBAR);
        }
    }
    {
        float xi = x + 50;
        if (R.ritOn) { std::snprintf(b, sizeof b, "CLAR %+.3f", R.rit / 1000.0); texto(f_.painel, 5.4f, xi, y + 19.5f, P_LARANJA, b); xi += larguraTexto(f_.painel, 5.4f, b) + 4; }
        if (at && fast_) texto(f_.painel, 5.4f, xi, y + 19.5f, P_AMBAR, "FAST");
    }
    // frequencia
    const float tamF = 25.f;
    const float xr = x + w - 4;
    if (entrando_ && at) {
        const std::string t = (entrada_.empty() ? std::string("_") : entrada_) + " kHz";
        texto(f_.lcd, 20.f, xr, y + 3, P_AMBAR, t.c_str(), 1.f);
    } else {
        const std::string fs = fmtFreqLcd(f);
        const ImU32 cf = !lig ? P_APAGADO : at ? P_BRANCO : IM_COL32(0xc0, 0xcc, 0xe0, 255);
        texto(f_.lcd, tamF, xr, y + 1, cf, fs.c_str(), 1.f);
        rodaDigitos(f_.lcd, tamF, xr, y + 1, fs, rx, 1);
    }
    // clique na frequencia: o receptor passa a ser o ouvido; no que ja e, digita a frequencia
    ImGui::SetCursorScreenPos(M.p(x + w - 150, y));
    char id[16]; std::snprintf(id, sizeof id, "##vfo%d", rx);
    if (ImGui::InvisibleButton(id, ImVec2(M.d(150), M.d(h)))) {
        if (!at) ativar(rx);
        else { entrando_ = !entrando_; entrada_.clear(); }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", at ? "Clique: digitar a frequência (kHz, Enter confirma, Esc cancela)\nRoda do mouse em cima de um dígito: muda aquele dígito"
                                   : "Clique: ouvir este receptor (passa a ser a banda de operação)\nRoda do mouse em cima de um dígito: muda aquele dígito");
}

// Celulas ANT / ATT / IPO / R.FIL / AGC (tocar muda, como na tela do radio)
void Ui::celulasInfo(float x, float y, float w, float h, int rx)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    auto& c = Config::instance();
    Receptor& R = rx_[rx];
    const float cw = w / 5.f;
    const char* cab[5] = {"ANT", "ATT", "IPO", "R.FIL", "AGC"};
    char val[5][24];
    std::snprintf(val[0], 24, "%s", !r_->temDispositivo() ? "--" : r_->tipo() == "rtltcp" ? "TCP" : r_->tipo() == "sdrplay" ? "RSP" : "RTL");
    if (att_) std::snprintf(val[1], 24, "%ddB", att_ * 6); else std::snprintf(val[1], 24, "OFF");
    std::snprintf(val[2], 24, "%s", pamp_ == 0 ? "IPO" : pamp_ == 1 ? "AMP1" : "AMP2");
    {
        const std::string m = rx == ativo_ ? r_->modo() : R.modo;
        const int g = grupoModo(m);
        const float bw = rx == ativo_ ? (float)r_->banda() : (g == 2 ? filHi_[g][R.fil] * 2.f : filHi_[g][R.fil]);
        if (bw >= 1000) std::snprintf(val[3], 24, bw >= 10000 ? "%.0fk" : "%.1fk", bw / 1000.f);
        else std::snprintf(val[3], 24, "%.0f", bw);
        std::snprintf(val[3] + std::strlen(val[3]), 24 - std::strlen(val[3]), " %s", kNomesFil[R.fil]);
    }
    { const char* ag[4] = {"OFF", "SLOW", "MID", "FAST"}; std::snprintf(val[4], 24, "%s", ag[R.agcOff ? 0 : std::clamp(R.agc, 1, 3)]); }
    const char* dicas[5] = {
        "ANT: o aparelho (RTL-SDR USB, RTL-TCP pela rede, SDRplay) - abre a CONFIGURAÇÃO",
        "ATT: atenuador 0 / 6 / 12 / 18 dB",
        "IPO / AMP1 / AMP2: pré-amplificador (clique troca)\nIPO = sem pré, AMP1 = +6 dB, AMP2 = +12 dB (ganho limpo, sem saturar o dongle)\nSDRplay: +10 / +20 dB de FI",
        "R.FIL: filtro do receptor (A / B / C) com a largura de cada um\nClique: troca o filtro",
        "AGC: SLOW / MID / FAST / OFF (clique troca)"};
    for (int i = 0; i < 5; ++i) {
        const float xc = x + i * cw;
        ImGui::SetCursorScreenPos(M.p(xc, y));
        char id[24]; std::snprintf(id, sizeof id, "##cel%d_%d", rx, i);
        const bool clicou = ImGui::InvisibleButton(id, ImVec2(M.d(cw - 1), M.d(h)));
        const bool sobre = ImGui::IsItemHovered();
        const ImU32 t1 = sobre ? IM_COL32(0x2c, 0x42, 0x70, 255) : IM_COL32(0x1c, 0x2c, 0x50, 255);
        dl->AddRectFilledMultiColor(M.p(xc + 0.5f, y), M.p(xc + cw - 0.5f, y + h), t1, t1, IM_COL32(0x0a, 0x12, 0x26, 255), IM_COL32(0x0a, 0x12, 0x26, 255));
        texto(f_.painel, 5.2f, xc + cw * 0.5f, y + 0.8f, P_TFTCAB, cab[i], 0.5f);
        textoCabe(f_.painel, 7.4f, xc + cw * 0.5f, y + 8.6f, P_BRANCO, val[i], 0.5f, cw - 3);
        if (sobre) ImGui::SetTooltip("%s", dicas[i]);
        if (!clicou) continue;
        switch (i) {
        case 0: abrirConfig(); break;
        case 1: att_ = (att_ + 1) % 4; break;
        case 2: pamp_ = (pamp_ + 1) % 3; break;
        case 3: R.fil = (R.fil + 1) % 3; if (rx == ativo_) aplicarFiltro(); break;
        case 4: if (R.agcOff) { R.agcOff = false; R.agc = 1; } else if (R.agc >= 3) R.agcOff = true; else R.agc++; break;
        }
    }
    (void)c;
}

// A tela TFT inteira
void Ui::telaTft()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // moldura (vidro preto brilhante)
    dl->AddRectFilled(M.p(80, 34), M.p(692, 530), IM_COL32(0x0b, 0x0b, 0x0d, 255), M.d(12));
    dl->AddRect(M.p(80, 34), M.p(692, 530), IM_COL32(0x34, 0x36, 0x3c, 255), M.d(12), 0, M.d(1.2f));
    dl->AddRectFilledMultiColor(M.p(88, 36), M.p(684, 44), IM_COL32(255, 255, 255, 16), IM_COL32(255, 255, 255, 16),
                                IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 0));
    const float x0 = 92, y0 = 44, x1 = 680, y1 = 444;
    dl->AddRectFilled(M.p(x0, y0), M.p(x1, y1), P_LCD, M.d(2));
    const bool lig = r_->ligado();
    float yy = y0 + 4;
    if (!expand_) {
        for (int i = 0; i < 2; ++i) {
            const float xs = x0 + 4 + i * 294.f;
            medidorFt(xs, yy, 204, 80, i);
            caixaFiltro(xs + 210, yy + 10, 76, 70, i);
        }
        yy += 84;
    }
    for (int i = 0; i < 2; ++i) ladoVfo(x0 + 4 + i * 294.f, yy, 286, 32, i);
    yy += 34;
    for (int i = 0; i < 2; ++i) celulasInfo(x0 + 4 + i * 294.f, yy, 286, 21, i);
    yy += 23;
    // faixa de cima do escopo: MAIN/SUB e CENTER | velocidade | SPAN
    {
        dl->AddRectFilledMultiColor(M.p(x0 + 2, yy), M.p(x1 - 2, yy + 11), IM_COL32(0x24, 0x48, 0xa8, 255), IM_COL32(0x24, 0x48, 0xa8, 60),
                                    IM_COL32(0x10, 0x24, 0x60, 60), IM_COL32(0x10, 0x24, 0x60, 255));
        texto(f_.painel, 7.f, x0 + 6, yy + 1, P_BRANCO, ativo_ == 0 ? "MAIN" : "SUB");
        const int v = (int)std::lround(wfSpeedEsc_);
        const char* vel = v <= 2 ? "SLOW1" : v <= 4 ? "SLOW2" : v <= 6 ? "FAST1" : v <= 8 ? "FAST2" : "FAST3";
        const double span = r_->taxa() / (1.0 + zoom_);
        char b[96];
        if (span >= 1e6) std::snprintf(b, sizeof b, "%s | %s | SPAN %.2fMHz", modoCentro_ ? "CENTER" : "FIX", vel, span / 1e6);
        else std::snprintf(b, sizeof b, "%s | %s | SPAN %.0fkHz", modoCentro_ ? "CENTER" : "FIX", vel, span / 1000.0);
        texto(f_.painel, 6.2f, x1 - 6, yy + 1.4f, P_TFTTXT, b, 1.f);
        if (pausa_ && std::fmod(agora(), 1.0) < 0.6) texto(f_.painel, 7.f, (x0 + x1) * 0.5f, yy + 1, P_AMBAR, "HOLD", 0.5f);
        else if (scopeAtt_) { std::snprintf(b, sizeof b, "ATT %ddB", scopeAtt_ * 10); texto(f_.painel, 6.2f, (x0 + x1) * 0.5f, yy + 1.4f, P_AMBAR, b, 0.5f); }
        if (modo3d_) texto(f_.painel, 6.2f, x0 + 40, yy + 1.4f, P_TFTTXT, "3DSS");
        yy += 12;
    }
    // escopo
    const float yEsc1 = 412;
    if (modo3d_) tresDss(x0 + 2, yy, x1 - x0 - 4, yEsc1 - yy);
    else escopo(x0 + 2, yy, x1 - x0 - 4, yEsc1 - yy);
    teclasSoft(x0 + 2, yEsc1 + 2, x1 - x0 - 4, 28);
    if (!lig) {
        dl->AddRectFilled(M.p(220, 290), M.p(552, 312), IM_COL32(0, 0, 0, 200), M.d(4));
        texto(f_.painel, 9.f, 386, 294, P_AMBAR, "Aperte ON/OFF (à esquerda da tela) para ligar", 0.5f);
    }
    if (dim_ > 0) dl->AddRectFilled(M.p(x0, y0), M.p(x1, y1), IM_COL32(0, 0, 0, dim_ * 45));
}

// As 8 teclas de toque embaixo do escopo
void Ui::teclasSoft(float x, float y, float w, float h)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float cw = w / 8.f;
    const char* rot[8] = {modoCentro_ ? "CENTER" : "FIX", "SPAN", "3DSS", mono_ ? "MONO" : "DUAL", "MULTI", "EXPAND", "HOLD", ""};
    const bool on[8] = {modoCentro_, zoom_ > 0, modo3d_, mono_, ifOn_, expand_, pausa_, false};
    const char* dicas[8] = {
        "CENTER / FIX: a faixa acompanha a sintonia (CENTER) ou fica parada (FIX)",
        "SPAN: largura do escopo",
        "3DSS: troca a cachoeira pelo espectro em 3 dimensões (3DSS) e volta",
        "DUAL / MONO: DUAL mostra a marca do SUB no escopo; MONO só a do MAIN",
        "MULTI: liga/desliga o IF DISPLAY (zoom de 20 kHz em volta da sintonia) dentro do escopo",
        "EXPAND: escopo maior (esconde os S-meters)",
        "HOLD: congela o escopo e a cachoeira",
        "MULTI: o que o botão MULTI faz agora (clique para escolher)"};
    for (int i = 0; i < 8; ++i) {
        const float xc = x + i * cw;
        ImGui::SetCursorScreenPos(M.p(xc, y));
        char id[16]; std::snprintf(id, sizeof id, "##soft%d", i);
        const bool clicou = ImGui::InvisibleButton(id, ImVec2(M.d(cw - 1), M.d(h)));
        const bool sobre = ImGui::IsItemHovered(), at = ImGui::IsItemActive();
        const ImU32 t1 = at ? IM_COL32(0x10, 0x1c, 0x38, 255) : sobre ? IM_COL32(0x34, 0x4c, 0x80, 255) : IM_COL32(0x24, 0x36, 0x60, 255);
        dl->AddRectFilledMultiColor(M.p(xc + 1, y), M.p(xc + cw - 1, y + h), t1, t1, IM_COL32(0x08, 0x10, 0x24, 255), IM_COL32(0x08, 0x10, 0x24, 255));
        if (i < 7) {
            texto(f_.painel, 8.f, xc + cw * 0.5f, y + h * 0.5f - 5.5f, on[i] ? IM_COL32(0x7c, 0xc4, 0xff, 255) : P_BRANCO, rot[i], 0.5f);
        } else {
            dl->AddRect(M.p(xc + 4, y + 2), M.p(xc + cw - 4, y + h - 2), P_LARANJA, M.d(2), 0, M.d(1.f));
            texto(f_.painel, 6.f, xc + cw * 0.5f, y + 4, P_LARANJA, "MULTI", 0.5f);
            textoCabe(f_.painel, 6.4f, xc + cw * 0.5f, y + 13.5f, P_LARANJA, nomeFuncaoMulti(multiFunc_), 0.5f, cw - 10);
        }
        if (sobre) ImGui::SetTooltip("%s", dicas[i]);
        if (!clicou) continue;
        switch (i) {
        case 0: modoCentro_ = !modoCentro_; if (modoCentro_) r_->centralizar(r_->centroSemDc(r_->vfo())); break;
        case 1: ImGui::OpenPopup("##spanFt"); break;
        case 2: modo3d_ = !modo3d_; break;
        case 3: mono_ = !mono_; break;
        case 4: ifOn_ = !ifOn_; break;
        case 5: expand_ = !expand_; break;
        case 6: pausa_ = !pausa_; if (pausa_) congelado_ = suave_; break;
        case 7: ImGui::OpenPopup("##multiFt"); break;
        }
    }
    if (ImGui::BeginPopup("##spanFt")) {
        ImGui::TextDisabled("SPAN (largura do escopo)");
        const float z[6] = {0, 1, 3, 7, 15, 31};
        for (float zz : z) {
            const double span = r_->taxa() / (1.0 + zz);
            char t[32];
            if (span >= 1e6) std::snprintf(t, sizeof t, "%.2f MHz", span / 1e6); else std::snprintf(t, sizeof t, "%.0f kHz", span / 1000.0);
            if (ImGui::Selectable(t, std::fabs(zoom_ - zz) < 0.3f)) { zoom_ = zz; centrarParaZoom(true); }
        }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##multiFt")) {
        ImGui::TextDisabled("Função do botão MULTI");
        for (int k = 0; k < 9; ++k) if (ImGui::Selectable(nomeFuncaoMulti(k), multiFunc_ == k)) multiFunc_ = k;
        ImGui::EndPopup();
    }
}

const char* Ui::nomeFuncaoMulti(int func) const
{
    static const char* n[9] = {"LEVEL", "SPEED", "CONTRAST", "STEP DIAL", "MEM CH", "DNR LEVEL", "NB LEVEL", "R.FIL", "AF GAIN"};
    return n[std::clamp(func, 0, 8)];
}

// MULTI (e o anel MPVD em C.S): gira a funcao escolhida
void Ui::funcaoMulti(int func, int passos)
{
    if (passos == 0) return;
    Receptor& R = rx_[ativo_];
    char b[64];
    switch (func) {
    case 0: wfBrilho_ = std::clamp(wfBrilho_ + passos * 2.f, 0.f, 200.f); autoWf_ = false;
            std::snprintf(b, sizeof b, "LEVEL %.0f", wfBrilho_ - 100.f); aviso(b, 1.0); break;
    case 1: wfSpeedEsc_ = std::clamp(wfSpeedEsc_ + (passos > 0 ? 1.f : -1.f), 1.f, 10.f);
            std::snprintf(b, sizeof b, "SPEED %.0f", wfSpeedEsc_); aviso(b, 1.0); break;
    case 2: wfRange_ = std::clamp(wfRange_ - passos * 2.f, 20.f, 120.f); autoWf_ = false;
            std::snprintf(b, sizeof b, "CONTRAST %.0f dB", wfRange_); aviso(b, 1.0); break;
    case 3: {
        const int64_t ps = std::max(1, passo_);
        const int64_t nf = ((int64_t)r_->vfo() / ps + passos) * ps;
        if (nf > 1000) { sintonizar((uint64_t)nf, false); R.f = (uint64_t)nf; }
        break; }
    case 4:
        if (mem_.empty()) { aviso("Nenhuma memória gravada (V>M grava)"); break; }
        R.memModo = true;
        R.memCanal = ((R.memCanal + passos) % (int)mem_.size() + (int)mem_.size()) % (int)mem_.size();
        sintonizarMemoria((size_t)R.memCanal);
        break;
    case 5: nrEspForca_ = std::clamp(nrEspForca_ + passos * 2.f, 0.f, 100.f);
            std::snprintf(b, sizeof b, "DNR LEVEL %.0f", nrEspForca_); aviso(b, 1.0); break;
    case 6: nbNivel_ = std::clamp(nbNivel_ + passos * 2.f, 0.f, 100.f);
            std::snprintf(b, sizeof b, "NB LEVEL %.0f", nbNivel_); aviso(b, 1.0); break;
    case 7: R.fil = ((R.fil + (passos > 0 ? 1 : -1)) % 3 + 3) % 3; aplicarFiltro();
            std::snprintf(b, sizeof b, "R.FIL %s", kNomesFil[R.fil]); aviso(b, 1.0); break;
    case 8: R.af = std::clamp(R.af + passos * 2.f, 0.f, 100.f);
            std::snprintf(b, sizeof b, "AF GAIN %.0f", R.af); aviso(b, 1.0); break;
    }
}

// 3DSS: o espectro em 3 dimensoes (frequencia, nivel e o tempo para o fundo)
void Ui::tresDss(float x, float y, float w, float h)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = M.p(x, y), b = M.p(x + w, y + h);
    const float sx = a.x, sy = a.y, sw = b.x - a.x, sh = b.y - a.y;
    dl->AddRectFilled(a, b, IM_COL32(0, 0, 0, 255));
    const float hEsc = std::max(14.f * s_, M.d(10));
    const float hz = sh - hEsc;
    constexpr int NP = 180, NL = 42;
    // guarda uma linha nova a cada 60 ms (o HOLD para tudo)
    const double t = agora();
    const int nb = (int)suave_.size();
    if (!pausa_ && r_->ligado() && nb > 16 && t - ult3d_ > 0.06) {
        ult3d_ = t;
        if (hist3d_.size() != (size_t)NL) { hist3d_.assign(NL, std::vector<float>(NP, 0.f)); hist3dPos_ = 0; }
        std::vector<float>& L = hist3d_[hist3dPos_];
        hist3dPos_ = (hist3dPos_ + 1) % NL;
        const double span = taxaMostrada_ / (1.0 + zoom_);
        const double binsPorHz = nb / (double)taxaMostrada_;
        const double bIni = ((centroVisual_ - span / 2) - (double)centroMostrado_) * binsPorHz + nb / 2.0;
        const double bPorCol = span * binsPorHz / NP;
        const float ref = (wfBrilho_ - 100.f) * 0.5f, range = std::max(10.f, wfRange_);
        for (int c = 0; c < NP; ++c) {
            const int b0 = (int)std::floor(bIni + c * bPorCol);
            const int b1 = std::max(b0 + 1, (int)std::floor(bIni + (c + 1) * bPorCol));
            float m = -200;
            for (int k = std::max(0, b0); k < b1 && k < nb; ++k) m = std::max(m, suave_[k]);
            L[c] = m <= -199 ? 0.f : std::clamp((m - (ref - range)) / range, 0.f, 1.f);
        }
        // o ruido fica rente ao chao (morrinhos baixos); so os sinais sobem
        std::vector<float> tmp(L);
        std::nth_element(tmp.begin(), tmp.begin() + NP / 4, tmp.end());
        const float piso = std::max(0.f, tmp[NP / 4] - 0.02f);
        for (int c = 0; c < NP; ++c) L[c] = std::clamp((L[c] - piso) / std::max(0.2f, 1.f - piso) * 0.95f, 0.f, 1.f);
    }
    // chao e paredes em perspectiva
    const float yF = sy + hz - 3, yB = sy + hz * 0.30f;
    const float cxm = sx + sw * 0.5f;
    auto px = [&](float u, float z) { const float s = 1.f - 0.36f * z; return cxm + (u - 0.5f) * sw * s; };
    auto pyBase = [&](float z) { return yF - (yF - yB) * z; };
    const ImU32 grade = IM_COL32(0x40, 0x48, 0x60, 150);
    for (int i = 0; i <= 10; ++i) {
        const float u = i / 10.f;
        dl->AddLine(ImVec2(px(u, 0), pyBase(0)), ImVec2(px(u, 1), pyBase(1)), grade, 1.f);
    }
    for (int k = 0; k <= 6; ++k) {
        const float z = k / 6.f;
        dl->AddLine(ImVec2(px(0, z), pyBase(z)), ImVec2(px(1, z), pyBase(z)), grade, 1.f);
        // paredes dos lados (sobem)
        const float hp = hz * 0.42f * (1.f - 0.45f * z);
        dl->AddLine(ImVec2(px(0, z), pyBase(z)), ImVec2(px(0, z), pyBase(z) - hp), grade, 1.f);
        dl->AddLine(ImVec2(px(1, z), pyBase(z)), ImVec2(px(1, z), pyBase(z) - hp), grade, 1.f);
    }
    for (int j = 1; j <= 4; ++j) {
        const float f = j / 4.f;
        dl->AddLine(ImVec2(px(0, 0), pyBase(0) - hz * 0.42f * f), ImVec2(px(0, 1), pyBase(1) - hz * 0.42f * 0.55f * f), grade, 1.f);
        dl->AddLine(ImVec2(px(1, 0), pyBase(0) - hz * 0.42f * f), ImVec2(px(1, 1), pyBase(1) - hz * 0.42f * 0.55f * f), grade, 1.f);
    }
    // as linhas, de tras para a frente (a da frente tampa as de tras)
    if (hist3d_.size() == (size_t)NL && r_->ligado()) {
        dl->PushClipRect(ImVec2(sx, sy), ImVec2(sx + sw, sy + hz), true);
        std::vector<ImVec2> pts(NP);
        for (int k = NL - 1; k >= 0; --k) {
            const std::vector<float>& L = hist3d_[(hist3dPos_ - 1 - k + 2 * NL) % NL];
            const float z = k / (float)(NL - 1);
            const float base = pyBase(z), amp = hz * 0.62f * (1.f - 0.45f * z);
            for (int c = 0; c < NP; ++c) pts[c] = ImVec2(px(c / (float)(NP - 1), z), base - L[c] * amp);
            for (int c = 0; c + 1 < NP; ++c)
                dl->AddQuadFilled(pts[c], pts[c + 1], ImVec2(pts[c + 1].x, base + 1), ImVec2(pts[c].x, base + 1), IM_COL32(0, 0, 0, 235));
            const int al = (int)(110 + 145 * (1.f - z));
            for (int c = 0; c + 1 < NP; ++c)
                dl->AddLine(pts[c], pts[c + 1], corNivel3d(std::max(L[c], L[c + 1]), al), k == 0 ? 1.6f : 1.1f);
        }
        dl->PopClipRect();
    }
    // marcas (VFO vermelho; o SUB em DUAL)
    {
        const float xv = (float)hzParaX((double)r_->vfo(), sx, sw);
        dl->AddLine(ImVec2(xv, sy + 6), ImVec2(xv, sy + hz), IM_COL32(0xff, 0x40, 0x30, 220), std::max(1.f, s_));
        dl->AddTriangleFilled(ImVec2(xv - 4 * s_, sy), ImVec2(xv + 4 * s_, sy), ImVec2(xv, sy + 6 * s_), IM_COL32(0xff, 0x40, 0x30, 255));
        if (!mono_) {
            const float xo = (float)hzParaX((double)rx_[1 - ativo_].f, sx, sw);
            dl->AddLine(ImVec2(xo, sy + 6), ImVec2(xo, sy + hz), IM_COL32(0x5c, 0xb0, 0xff, 150), std::max(1.f, s_));
            dl->AddTriangleFilled(ImVec2(xo - 4 * s_, sy), ImVec2(xo + 4 * s_, sy), ImVec2(xo, sy + 6 * s_), P_AZULSUB);
        }
    }
    escala(sx, sy + hz, sw, hEsc);
    // clicar / arrastar sintoniza, como no escopo normal
    if (interacaoEspectro(sx, sy, sw, hz, "##esp3d")) {
        const float mx = ImGui::GetIO().MousePos.x;
        dl->AddLine(ImVec2(mx, sy), ImVec2(mx, sy + hz), IM_COL32(255, 255, 255, 60));
        ImGui::SetTooltip("%s", fmtFreq((uint64_t)std::max(0.0, xParaHz(mx, sx, sw))).c_str());
    }
}

// Teclas embaixo da tela: TUNE  VOX  MOX  ZIN/SPOT  DISP  S.MENU  FUNC
void Ui::teclasTft()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float y = 452, w = 74, h = 19;
    auto xk = [](int i) { return 98.f + i * 82.4f; };
    {
        const int k = tecla("kTune", "TUNE", xk(0), y, w, h, autoWf_ ? 3 : 1, nullptr,
                            "TUNE: ajuste automático do escopo e da cachoeira (como o AT)\nSegurar: guarda a aparência atual como padrão");
        if (k == 1) autoCachoeira();
        if (k == 2) padrao();
    }
    {   // VOX = squelch automatico
        Receptor& Rv = rx_[ativo_];
        if (Rv.sql <= -90.f) sqlAuto_[ativo_] = false;
        const bool ligado = sqlAuto_[ativo_];
        if (tecla("kVox", "VOX", xk(1), y, w, h, sqlMedindo_ ? 5 : ligado ? 3 : 1, nullptr,
                  "VOX: squelch automático.\nClique: mede o ruído por 4 s e fecha o squelch logo acima.\n"
                  "Aceso: clique de novo para desligar (squelch aberto).\nAjuste fino à mão: anel RF/SQL (FM) ou FUNC > RF/SQL.")) {
            if (sqlMedindo_) { sqlMedindo_ = false; Rv.sql = -90.f; sql_ = -90.f; sqlAplicado_ = -90.f; sqlAuto_[ativo_] = false; }
            else if (ligado) { Rv.sql = -90.f; sql_ = -90.f; sqlAplicado_ = -90.f; sqlAuto_[ativo_] = false; aviso("Squelch aberto"); }
            else if (r_->aoVivo()) { sqlMedindo_ = true; sqlAmostras_.clear(); sqlIni_ = agora(); sqlUlt_ = 0; }
        }
    }
    if (tecla("kMox", "MOX", xk(2), y, w, h, sendMudo_ ? 4 : 1, nullptr, "MOX: silencia todo o áudio (MUTE geral)")) sendMudo_ = !sendMudo_;
    if (tecla("kZin", "ZIN/SPOT", xk(3), y, w, h, 0, nullptr, "ZIN/SPOT: sintonia automática no pico mais próximo (CW / AM / FM)")) cwTune();
    {
        const int k = tecla("kDisp", "DISP", xk(4), y, w, h, 0, nullptr,
                            "DISP: tamanho do espectro x cachoeira no escopo\nSegurar: brilho da tela (TFT DIMMER)");
        if (k == 1) { espFrac_ = espFrac_ < 0.25f ? 0.32f : espFrac_ < 0.42f ? 0.55f : 0.18f; }
        if (k == 2) { dim_ = (dim_ + 1) % 4; const char* nm[4] = {"100%", "75%", "50%", "30%"}; aviso(std::string("TFT DIMMER ") + nm[dim_]); }
    }
    if (tecla("kSmenu", "S.MENU", xk(5), y, w, h, 0, nullptr, "S.MENU: ajustes do escopo (velocidade, nível, contraste, cores, marcas)"))
        ImGui::OpenPopup("##smenuFt");
    if (tecla("kFunc", "FUNC", xk(6), y, w, h, 0, nullptr, "FUNC: funções e configuração (aparelho, decodificadores, memórias, gravador, GEN...)"))
        ImGui::OpenPopup("##funcFt");
    // abertura do cartao SD
    dl->AddRectFilled(M.p(228, 490), M.p(312, 498), IM_COL32(0x02, 0x02, 0x03, 255), M.d(1.5f));
    dl->AddRect(M.p(228, 490), M.p(312, 498), IM_COL32(0x40, 0x42, 0x48, 255), M.d(1.5f), 0, M.d(0.8f));

    if (ImGui::BeginPopup("##smenuFt")) {
        ImGui::TextDisabled("SCOPE MENU");
        ImGui::Separator();
        int v = (int)std::lround(wfSpeedEsc_);
        ImGui::SetNextItemWidth(220 * s_);
        if (ImGui::SliderInt("SPEED (velocidade)", &v, 1, 10)) wfSpeedEsc_ = (float)v;
        float lv = wfBrilho_ - 100.f;
        ImGui::SetNextItemWidth(220 * s_);
        if (ImGui::SliderFloat("LEVEL (nível)", &lv, -100.f, 100.f, "%.0f")) { wfBrilho_ = lv + 100.f; autoWf_ = false; }
        ImGui::SetNextItemWidth(220 * s_);
        if (ImGui::SliderFloat("CONTRAST (faixa dB)", &wfRange_, 20.f, 120.f, "%.0f dB")) autoWf_ = false;
        ImGui::SetNextItemWidth(220 * s_);
        float ef = espFrac_ * 100.f;
        if (ImGui::SliderFloat("Altura do espectro", &ef, 10.f, 85.f, "%.0f%%")) espFrac_ = ef / 100.f;
        if (ImGui::BeginMenu("COLOR (cores da cachoeira)")) {
            const char* nm[4] = {"Eclipse", "Oceano", "Clássica", "Cinza"};
            for (int i = 0; i < 4; ++i)
                if (ImGui::MenuItem(nm[i], nullptr, paleta_ == i)) { paleta_ = i; g_paletaCachoeira = i; lutRef_ = 1e9f; lutRange_ = 1e9f; }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("MARKER (nomes das memórias no escopo)", nullptr, memVisivel_)) memVisivel_ = !memVisivel_;
        if (ImGui::MenuItem("ATT do escopo (escurece 10 dB por vez)", nullptr, scopeAtt_ > 0)) {
            const int novo = (scopeAtt_ + 1) % 4;
            wfBrilho_ = std::clamp(wfBrilho_ + (scopeAtt_ - novo) * 20.f, 0.f, 200.f);
            scopeAtt_ = novo; autoWf_ = false;
        }
        if (ImGui::MenuItem("AUTO (ajuste automático)", nullptr, autoWf_)) autoCachoeira();
        ImGui::EndPopup();
    }
}

// Coluna MODE / SSB / CW / BK-IN / MONI e o botao MULTI
void Ui::colunaModos()
{
    const std::string m = r_->modo();
    const float xc = FX(98), xk = FX(78), wk = FW(40);
    auto rot = [&](float yFoto, const char* t) { rotulo(18.f, xc, FY(yFoto), P_ROTULO, t, 0.5f, 46.f); };
    g_borracha = 1;                       // MODE, SSB e CW: so pontinhos
    rot(251, "MODE");
    if (tecla("kMode", "", xk, FY(265), wk, FH(27), 0, nullptr, "MODE: escolher o modo (LSB, USB, CW, AM, FM, FM largo) e os decodificadores (DATA, RTTY, PSK)"))
        ImGui::OpenPopup("##modosFt");
    rot(310, "SSB");
    if (tecla("kSsb", "", xk, FY(323), wk, FH(29), (m == "LSB" || m == "USB") ? 3 : 0, nullptr, "SSB: LSB / USB (cada toque troca)")) {
        if (m == "LSB") mudarModoPainel("USB");
        else if (m == "USB") mudarModoPainel("LSB");
        else mudarModoPainel(r_->vfo() < 10000000 ? "LSB" : "USB");
    }
    rot(370, "CW");
    if (tecla("kCw", "", xk, FY(385), wk, FH(25), m == "CW" ? 3 : 0, nullptr, "CW: telegrafia (largura no SHIFT-WIDTH, tom no PROC/PITCH)"))
        mudarModoPainel("CW");
    g_borracha = 0;                       // BK-IN e MONI: pontinhos e o oval
    rot(465, "BK-IN");
    if (tecla("kBkin", "", xk, FY(480), wk, FH(27), r_->decoders().tipo() == Decoders::CW ? 3 : 0, nullptr, "BK-IN: abre o decodificador de CW (telegrafia em texto)")) {
        decAberta_ = true;
        if (r_->decoders().tipo() != Decoders::CW) escolherDecoder(Decoders::CW, true);
    }
    rot(525, "MONI");
    if (tecla("kMoni", "", xk, FY(542), wk, FH(25), 0, nullptr, "MONI: escolher a saída de som (placa / fone)")) {
        saidas_ = SomNativo::listarSaidas();
        ImGui::OpenPopup("##saidaFt");
    }
    // MULTI (no radio fica no canto de baixo da moldura da tela)
    rotulo(18.f, xc + 3, FY(600), P_ROTULO, "MULTI", 0.5f, 46.f);
    rotulo(11.5f, xc + 3, FY(615.5f), P_ROTULO, "PUSH ON", 0.5f, 46.f);
    {
        bool clique = false;
        char d[200];
        std::snprintf(d, sizeof d, "MULTI: %s (gire)\nClique: escolher a função (LEVEL, SPEED, STEP DIAL, MEM CH...)", nomeFuncaoMulti(multiFunc_));
        const int p = encoder("eMulti", 722, 468, 23, angMulti_, d, &clique);
        if (clique) ImGui::OpenPopup("##multiFt2");
        if (p) funcaoMulti(multiFunc_, p);
    }
    if (ImGui::BeginPopup("##multiFt2")) {
        ImGui::TextDisabled("Função do botão MULTI");
        for (int k = 0; k < 9; ++k) if (ImGui::Selectable(nomeFuncaoMulti(k), multiFunc_ == k)) multiFunc_ = k;
        ImGui::Separator();
        ImGui::TextDisabled("Passo do STEP DIAL e da roda do mouse no escopo");
        const int ps[] = {10, 50, 100, 500, 1000, 2500, 5000, 6250, 8330, 9000, 10000, 12500, 25000, 100000};
        for (int p : ps) if (ImGui::Selectable(fmtPasso(p).c_str(), passo_ == p)) passo_ = p;
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##modosFt")) {
        ImGui::TextDisabled("MODE");
        const char* ms[6] = {"LSB", "USB", "CW", "AM", "NFM", "WFM"};
        const char* nm[6] = {"LSB", "USB", "CW", "AM", "FM (estreito)", "FM largo (radiodifusão)"};
        for (int i = 0; i < 6; ++i) if (ImGui::Selectable(nm[i], m == ms[i])) mudarModoPainel(ms[i]);
        ImGui::Separator();
        if (ImGui::Selectable("DATA - decodificadores digitais...")) decAberta_ = true;
        if (ImGui::Selectable("RTTY")) { escolherDecoder(Decoders::RTTY, true); decAberta_ = true; }
        if (ImGui::Selectable("PSK (BPSK31/63/125...)")) { escolherDecoder(Decoders::BPSK, true); decAberta_ = true; }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##saidaFt")) {
        auto& c = Config::instance();
        ImGui::TextDisabled("Saída de som");
        if (ImGui::Selectable("Padrão do Windows", c.str("audio_saida").empty())) { ra_->trocarSaida(""); rb_->trocarSaida(""); }
        for (const auto& s : saidas_)
            if (ImGui::Selectable(s.c_str(), c.str("audio_saida") == s)) { ra_->trocarSaida(s); rb_->trocarSaida(s); }
        ImGui::EndPopup();
    }
}

// Lista do GEN (cobertura geral) dentro de um popup / menu
void Ui::itensGen(int qual)
{
    const ItemGen* it = qual == 0 ? kGen1 : kGen2;
    const size_t n = qual == 0 ? sizeof kGen1 / sizeof kGen1[0] : sizeof kGen2 / sizeof kGen2[0];
    for (size_t i = 0; i < n; ++i) {
        if (it[i].grupo) ImGui::SeparatorText(it[i].grupo);
        char r[160], mhz[24];
        std::snprintf(mhz, sizeof mhz, "%.4f", it[i].hz / 1e6);
        for (char* p = mhz; *p; ++p) if (*p == '.') *p = ',';
        std::snprintf(r, sizeof r, "%11s MHz   %-38s %s", mhz, it[i].nome, it[i].modo);
        ImGui::PushID((int)i + qual * 100);
        if (ImGui::Selectable(r)) {
            rx_[ativo_].memModo = false;
            if (std::string(it[i].modo) != "WFM" && it[i].hz < 30000000) Config::instance().set("qmode", std::string("auto"));
            mudarModoPainel(it[i].modo);
            sintonizar(it[i].hz, false);
        }
        ImGui::PopID();
    }
}

// A faixa das bandas (vidro preto, em cima da coluna MODE como no radio):
// janelinha de cima = marca da banda, LED azul = banda do SUB, LED branco
// embaixo = banda do MAIN
void Ui::faixaBandas()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float x0 = FX(78), x1 = FX(990), y0 = FY(108), y1 = FY(222);
    dl->AddRectFilled(M.p(x0 - 3, y0 - 3), M.p(x1 + 3, y1 + 3), IM_COL32(0x2a, 0x2e, 0x34, 255), M.d(9));
    dl->AddRect(M.p(x0 - 3, y0 - 3), M.p(x1 + 3, y1 + 3), IM_COL32(0x58, 0x60, 0x6a, 200), M.d(9), 0, M.d(0.8f));
    dl->AddRectFilled(M.p(x0, y0), M.p(x1, y1), IM_COL32(0x03, 0x04, 0x05, 255), M.d(7));
    dl->AddRectFilledMultiColor(M.p(x0 + 5, y0 + 1), M.p(x1 - 5, y0 + 9), IM_COL32(255, 255, 255, 20), IM_COL32(255, 255, 255, 20),
                                IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 0));
    auto tituloPontilhado = [&](float cx, float yc, float xa, float xb, const char* t, ImU32 cor) {
        const float tw = larguraTexto(f_.painel, 6.6f, t);
        texto(f_.painel, 6.6f, cx, yc - tamReal(6.6f) * 0.55f, cor, t, 0.5f);
        for (float x = cx - tw * 0.5f - 6; x > xa; x -= 3.3f) dl->AddRectFilled(M.p(x, yc - 0.45f), M.p(x + 1.2f, yc + 0.45f), alfa(cor, 150));
        for (float x = cx + tw * 0.5f + 5; x < xb; x += 3.3f) dl->AddRectFilled(M.p(x, yc - 0.45f), M.p(x + 1.2f, yc + 0.45f), alfa(cor, 150));
    };
    tituloPontilhado(FX(567), FY(137), FX(420), FX(715), "SUB", P_AZULSUB);
    tituloPontilhado(FX(572), FY(212), FX(425), FX(720), "MAIN", IM_COL32(0xc8, 0xcc, 0xd4, 255));
    {   // LEDzinho branco na ponta esquerda da faixa
        const bool lig = r_->ligado();
        if (lig) dl->AddCircleFilled(M.p(FX(93), FY(202)), M.d(3.0f), IM_COL32(255, 255, 255, 50), 12);
        dl->AddRectFilled(M.p(FX(88), FY(200)), M.p(FX(99), FY(205)), lig ? IM_COL32(0xf4, 0xf6, 0xff, 255) : IM_COL32(0x22, 0x24, 0x28, 255), M.d(1.2f));
    }
    auto banda = [&](uint64_t f) { for (int b = 0; b < kNB; ++b) if (f >= kBandasHam[b].lo && f <= kBandasHam[b].hi) return b; return -1; };
    const uint64_t fM = ativo_ == 0 ? r_->vfo() : rx_[0].f, fS = ativo_ == 1 ? r_->vfo() : rx_[1].f;
    const int bM = banda(fM), bS = banda(fS);
    for (int b = 0; b < kNB; ++b) {
        const float xc = FX(139 + b * 71.2f), w = FW(64), x = xc - w * 0.5f;
        const ImU32 apag = IM_COL32(0x1a, 0x1d, 0x21, 255);
        // janelinha de cima (marca da banda) - apagada
        dl->AddRectFilled(M.p(xc - FW(7), FY(122)), M.p(xc + FW(7), FY(128)), IM_COL32(0x14, 0x16, 0x19, 255), M.d(1.2f));
        dl->AddRect(M.p(xc - FW(7), FY(122)), M.p(xc + FW(7), FY(128)), IM_COL32(0x3a, 0x40, 0x48, 180), M.d(1.2f), 0, M.d(0.5f));
        // LED do SUB (azul) e do MAIN (branco)
        const bool lS = b == bS, lM = b == bM;
        const float lw = FW(25);
        if (lS) dl->AddRectFilled(M.p(xc - lw - 2, FY(144) - 2), M.p(xc + lw + 2, FY(150) + 2), IM_COL32(0x40, 0x9c, 0xff, 70), M.d(2.5f));
        dl->AddRectFilled(M.p(xc - lw, FY(144)), M.p(xc + lw, FY(150)), lS ? IM_COL32(0x58, 0xb8, 0xff, 255) : apag, M.d(1.4f));
        if (lM) dl->AddRectFilled(M.p(xc - lw - 2, FY(199) - 2), M.p(xc + lw + 2, FY(205) + 2), IM_COL32(0xd8, 0xd0, 0xff, 70), M.d(2.5f));
        dl->AddRectFilled(M.p(xc - lw, FY(199)), M.p(xc + lw, FY(205)), lM ? IM_COL32(0xf2, 0xf0, 0xff, 255) : apag, M.d(1.4f));
        char id[12], d[160];
        std::snprintf(id, sizeof id, "kB%d", b);
        if (b == kNB - 1) std::snprintf(d, sizeof d, "70/GEN: banda de 4 m (70 MHz) e a cobertura geral (ondas curtas, médias, VHF, UHF)");
        else std::snprintf(d, sizeof d, "Banda de %s MHz para o %s (volta onde estava nesta banda)", kBandasHam[b].rot, ativo_ == 0 ? "MAIN" : "SUB");
        const int k = tecla(id, kBandasHam[b].rot, x, FY(159), w, FH(30), 0, nullptr, d);
        if (k) {
            if (b == kNB - 1) ImGui::OpenPopup("##genFt");
            else irBanda(b);
        }
    }
    if (ImGui::BeginPopup("##genFt")) {
        if (ImGui::Selectable("70 MHz - banda de 4 m")) irBanda(kNB - 1);
        ImGui::Separator();
        if (ImGui::BeginMenu("GEN - HF (ondas curtas, médias, utilitárias)")) { itensGen(0); ImGui::EndMenu(); }
        if (ImGui::BeginMenu("GEN - VHF / UHF")) { itensGen(1); ImGui::EndMenu(); }
        ImGui::EndPopup();
    }
}

// O meio: RX/TX, MAIN-SUB, SYNC/SPLIT, CLAR, os AF-RF/SQL, FAST/LOCK, a
// moldura do dial com as teclas em volta, a QMB, VC TUNE / C.S e o dial
void Ui::blocoCentro()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    Receptor& A = rx_[ativo_];
    const float pcx = FX(580), pcy = FY(514);     // centro do dial (na foto: 580, 514)
    const float Fp = 98.f;                       // raio da face do dial (papel)
    const ImU32 corSub = IM_COL32(0xa4, 0xc8, 0xec, 255);

    // fileira RX TX MAIN<>SUB TX RX (o LED fica em cima da borda; teclas claras, como na foto)
    g_ledFora = 2;
    g_teclaTom = 1;
    if (tecla("kMrx", "RX", FX(320), FY(230), FW(95), FH(25), ativo_ == 0 ? 2 : 1, nullptr, "RX (MAIN): ouvir o receptor MAIN")) ativar(0);
    if (tecla("kMtx", "TX", FX(422), FY(230), FW(70), FH(25), txVfo_ == 0 ? 4 : 1, nullptr, "TX (MAIN): marca o MAIN como frequência de \"TX\" (SPLIT)")) txVfo_ = 0;
    if (tecla("kStx", "TX", FX(652), FY(230), FW(73), FH(25), txVfo_ == 1 ? 4 : 1, nullptr, "TX (SUB): marca o SUB como frequência de \"TX\" (SPLIT)", corSub)) txVfo_ = 1;
    if (tecla("kSrx", "RX", FX(732), FY(230), FW(88), FH(25), ativo_ == 1 ? 2 : 1, nullptr, "RX (SUB): ouvir o receptor SUB", corSub)) ativar(1);
    g_ledFora = 0;
    {
        const float x0 = FX(502), x1 = FX(640), yc = FY(242.5f);
        if (tecla("kMs", "", x0, FY(230), x1 - x0, FH(25), 0, nullptr,
                  "MAIN <> SUB: troca as frequências do MAIN e do SUB (frequência, modo e filtro)\n"
                  "Volume, squelch, NB/DNR e os outros botões de cada receptor ficam onde estão."))
            trocarMainSub();
        const float xm = (x0 + x1) * 0.5f;
        texto(f_.painel, 8.f, xm - 7, yc - 5.2f, P_BRANCO, "MAIN", 1.f);
        texto(f_.painel, 8.f, xm + 7, yc - 5.2f, corSub, "SUB", 0.f);
        dl->AddLine(M.p(xm - 5, yc - 1.4f), M.p(xm + 4, yc - 1.4f), P_ROTULO, M.d(0.9f));
        dl->AddTriangleFilled(M.p(xm + 4, yc - 3.2f), M.p(xm + 4, yc + 0.4f), M.p(xm + 6, yc - 1.4f), P_ROTULO);
        dl->AddLine(M.p(xm - 4, yc + 2.0f), M.p(xm + 5, yc + 2.0f), corSub, M.d(0.9f));
        dl->AddTriangleFilled(M.p(xm - 4, yc + 0.2f), M.p(xm - 4, yc + 3.8f), M.p(xm - 6, yc + 2.0f), corSub);
    }
    g_teclaTom = 0;
    // SYNC / SPLIT (LED na janelinha de cima)
    g_ledFora = 1;
    {
        const int k = tecla("kSync", "SYNC", FX(145), FY(267), FW(70), FH(25), tracking_ ? 3 : 1, nullptr,
                            "SYNC: o SUB anda junto com o MAIN quando se gira o dial\nSegurar: o SUB vai para a frequência do MAIN");
        if (k == 1) tracking_ = !tracking_;
        if (k == 2) { sincronizarAtivo(); rx_[1].f = rx_[0].f; rx_[1].modo = rx_[0].modo; rx_[1].fil = rx_[0].fil;
                      if (ativo_ == 1) { r_->setModo(rx_[1].modo); sintonizar(rx_[1].f, false); aplicarFiltro(); }
                      aviso("SUB = MAIN"); }
    }
    {
        const bool split = txVfo_ != ativo_;
        const int k = tecla("kSplit", "SPLIT", FX(227), FY(267), FW(65), FH(25), split ? 3 : 1, nullptr,
                            "SPLIT: escuta num receptor e marca o outro como \"TX\"\nSegurar: QUICK SPLIT (o outro receptor vai 5 kHz acima)");
        if (k == 1) txVfo_ = split ? ativo_ : 1 - ativo_;
        if (k == 2) { sincronizarAtivo(); rx_[1 - ativo_].f = r_->vfo() + 5000; rx_[1 - ativo_].modo = r_->modo(); txVfo_ = 1 - ativo_; aviso("QUICK SPLIT +5 kHz"); }
    }
    g_ledFora = 0;
    rotulo(18.f, FX(220), FY(310), P_ROTULO, "CLAR", 0.5f);
    {
        const int k = tecla("kClarRx", "RX", FX(148), FY(325), FW(67), FH(30), 0, nullptr,
                            "CLAR RX: liga o clarificador (desloca a escuta) do receptor ouvido\nO valor: anel MPVD com CLAR aceso. Segurar: zera");
        if (k == 1) rx_[ativo_].ritOn = !rx_[ativo_].ritOn;
        if (k == 2) { rx_[ativo_].rit = 0; aviso("CLAR zerado"); }
    }
    {
        Receptor& O = rx_[1 - ativo_];
        const int k = tecla("kClarTx", "TX", FX(228), FY(325), FW(64), FH(30), 0, nullptr,
                            "CLAR TX: (num receptor) liga o clarificador do OUTRO receptor (o de \"TX\")\nSegurar: zera");
        if (k == 1) O.ritOn = !O.ritOn;
        if (k == 2) { O.rit = 0; aviso("CLAR zerado"); }
    }
    // AF-RF/SQL do SUB (em cima, azul) e do MAIN (embaixo)
    auto afRf = [&](int rx, float cx, float cy, float yRot, const char* id) {
        Receptor& R = rx_[rx];
        const std::string md = rx == ativo_ ? r_->modo() : R.modo;
        const bool sql = rfSql_ == 2 || (rfSql_ == 0 && grupoModo(md) >= 3);
        char d[260], sq[24];
        if (R.sql <= -90) std::snprintf(sq, sizeof sq, "aberto"); else std::snprintf(sq, sizeof sq, "%+d dB", (int)std::lround(R.sql));
        std::snprintf(d, sizeof d, "AF: volume do %s (%d%%)\n%s\n(anel em %s - FUNC > Áudio > Anel RF/SQL escolhe)",
                      rx == 0 ? "MAIN" : "SUB", (int)std::lround(R.af),
                      sql ? (std::string("SQL: squelch (") + sq + ")").c_str() : (std::string("RF: ganho do dongle (") + std::to_string((int)std::lround(49.6f * R.rf / 100.f)) + " dB)").c_str(),
                      rfSql_ == 0 ? "automático: SQL no FM, RF nos outros" : rfSql_ == 1 ? "RF" : "SQL");
        g_corRot = rx == 1 ? P_AZULSUB : IM_COL32(0xf4, 0xf4, 0xf4, 255);
        float di, dO;
        const float tOut = sql ? (R.sql + 90.f) / 120.f : R.rf / 100.f;
        if (giro(id, cx, cy, 30, rx == 1 ? "SUB AF" : "MAIN AF", "RF/SQL", R.af / 100.f, tOut, di, dO, d, true, yRot)) {
            R.af = std::clamp(R.af + di * 100.f, 0.f, 100.f);
            if (dO != 0) {
                if (sql) { R.sql = std::clamp(R.sql + dO * 120.f, -90.f, 30.f); if (rx == ativo_) { sql_ = R.sql; sqlAplicado_ = R.sql; } if (R.sql <= -90.f) sqlAuto_[rx] = false; }
                else R.rf = std::clamp(R.rf + dO * 100.f, 0.f, 100.f);
            }
            if (rx != ativo_ && di != 0) aviso(rx == 1 ? "Este AF é do SUB. Ouvindo o MAIN: use o MAIN AF (embaixo)" : "Este AF é do MAIN. Ouvindo o SUB: use o SUB AF (em cima)", 2.5);
        }
        g_corRot = IM_COL32(0xf4, 0xf4, 0xf4, 255);
    };
    afRf(1, FX(225), FY(457), FY(370) - 4.6f, "gAfSub");
    g_ledFora = 1;
    if (tecla("kFast", "FAST", FX(150), FY(542), FW(65), FH(23), fast_ ? 3 : 1, nullptr, "FAST: o dial anda 10 vezes mais rápido")) fast_ = !fast_;
    if (tecla("kLock", "LOCK", FX(227), FY(542), FW(65), FH(23), A.lock ? 3 : 1, nullptr, "LOCK: trava o dial do receptor ouvido")) A.lock = !A.lock;
    g_ledFora = 0;
    afRf(0, FX(222), FY(668), FY(586) - 4.6f, "gAfMain");

    // ---- a moldura do dial (preta brilhante), como no radio ----
    {
        const ImVec2 c = M.p(pcx, pcy);
        const float F = M.d(Fp);
        const ImU32 preto = cinzaFrio(0.03f);
        // meia largura do queixo (em F) pela altura v abaixo do centro: estreita em curva
        const float vFundo = (kLH - 16.f - pcy) / Fp;            // o fundo do queixo (acima do pe do painel)
        const float vCurva = 1.62f, rxC = 0.85f, cxC = 1.24f - rxC, ryC = std::max(0.05f, vFundo - vCurva);
        auto meiaQueixo = [=](float v) {
            static const float tv[4] = {0.95f, 1.20f, 1.40f, 1.62f}, tw[4] = {1.62f, 1.52f, 1.38f, 1.24f};
            if (v <= tv[0]) return tw[0];
            for (int k = 1; k < 4; ++k) if (v <= tv[k]) return tw[k - 1] + (tw[k] - tw[k - 1]) * (v - tv[k - 1]) / (tv[k] - tv[k - 1]);
            if (v >= vFundo) return cxC;
            const float sn = (v - vCurva) / ryC;                    // canto arredondado (quarto de elipse)
            return cxC + rxC * std::sqrt(std::max(0.f, 1.f - sn * sn));
        };
        // meio contorno do queixo (lado esquerdo, de cima para baixo), em F: x negativo, v
        auto ladoQueixo = [=]() {
            std::vector<ImVec2> r;
            for (int k = 0; k <= 6; ++k) { const float v = 0.95f + (vCurva - 0.95f) * k / 6.f; r.push_back(ImVec2(-meiaQueixo(v), v)); }
            for (int k = 1; k <= 14; ++k) {
                const float t = (float)(kPi * 0.5) * k / 14.f;
                r.push_back(ImVec2(-(cxC + rxC * std::cos(t)), vCurva + ryC * std::sin(t)));
            }
            return r;
        };
        // empurra cada ponto de um contorno g pixels para fora (pela normal)
        auto crescer = [](std::vector<ImVec2> p, float g) {
            const int n = (int)p.size();
            if (g == 0.f || n < 3) return p;
            float area = 0.f;
            for (int k = 0; k < n; ++k) { const ImVec2 a = p[k], b = p[(k + 1) % n]; area += a.x * b.y - b.x * a.y; }
            const float sg = area > 0.f ? 1.f : -1.f;
            auto nor = [&](ImVec2 u, ImVec2 v) {
                const float dx = v.x - u.x, dy = v.y - u.y, l = std::sqrt(dx * dx + dy * dy);
                return l < 1e-4f ? ImVec2(0, 0) : ImVec2(dy / l * sg, -dx / l * sg);
            };
            std::vector<ImVec2> q(n);
            for (int k = 0; k < n; ++k) {
                const ImVec2 n1 = nor(p[(k + n - 1) % n], p[k]), n2 = nor(p[k], p[(k + 1) % n]);
                ImVec2 m(n1.x + n2.x, n1.y + n2.y);
                const float l = std::sqrt(m.x * m.x + m.y * m.y);
                if (l < 1e-4f) m = n1; else { m.x /= l; m.y /= l; }
                q[k] = ImVec2(p[k].x + m.x * g, p[k].y + m.y * g);
            }
            return q;
        };
        const float yChao = M.p(0, kLH - 0.5f).y;
        // a base da moldura (o aro, a parte de cima, os suportes da QMB e do VC TUNE / C.S, o
        // queixo e, se petalas, os sulcos das teclas grandes): g pixels maior, deslocada de o
        auto base = [&](float g, ImVec2 o, ImU32 cor, bool petalas) {
            dl->AddCircleFilled(ImVec2(c.x + o.x, c.y + o.y), F * 1.60f + g, cor, 160);
            for (int sx = -1; sx <= 1; sx += 2) {
                const float xa = sx < 0 ? pcx - 1.95f * Fp : pcx + 1.40f * Fp, xb = sx < 0 ? pcx - 1.40f * Fp : pcx + 1.95f * Fp;
                const ImVec2 a0 = M.p(xa, pcy - 0.86f * Fp), b0 = M.p(xb, pcy + 0.86f * Fp);
                dl->AddRectFilled(ImVec2(a0.x - g + o.x, a0.y - g + o.y), ImVec2(b0.x + g + o.x, b0.y + g + o.y), cor, M.d(12) + g);
            }
            {   // em cima, ate a fileira RX / TX (nao cresce para cima)
                const ImVec2 a0 = M.p(pcx - 1.86f * Fp, pcy - 1.93f * Fp), b0 = M.p(pcx + 1.71f * Fp, pcy - 0.95f * Fp);
                dl->AddRectFilled(ImVec2(a0.x - g + o.x, a0.y + std::min(0.f, o.y)), ImVec2(b0.x + g + o.x, b0.y + g + o.y), cor, M.d(22) + g);
            }
            {   // o queixo: desce ate o pe do painel, estreitando em curva
                std::vector<ImVec2> q;
                const std::vector<ImVec2> lado = ladoQueixo();
                for (const ImVec2& w : lado) q.push_back(M.p(pcx + w.x * Fp, pcy + w.y * Fp));
                for (int k = (int)lado.size() - 1; k >= 0; --k) q.push_back(M.p(pcx - lado[k].x * Fp, pcy + lado[k].y * Fp));
                q = crescer(q, g);
                for (ImVec2& v : q) { v.x += o.x; v.y = std::min(yChao, v.y + o.y); }
                leque(dl, q, M.p(pcx, pcy + (0.95f + vFundo) * 0.5f * Fp), [&](ImVec2) { return cor; });
            }
            if (petalas)
                for (int sy = -1; sy <= 1; sy += 2)
                    for (int sx = -1; sx <= 1; sx += 2) {
                        std::vector<ImVec2> f = crescer(contornoPetala(c, F, sx, sy, true), g);
                        float xs = 0, ys = 0;
                        for (ImVec2& q : f) { q.x += o.x; q.y += o.y; xs += q.x; ys += q.y; }
                        leque(dl, f, ImVec2(xs / f.size(), ys / f.size()), [&](ImVec2) { return cor; });
                    }
        };
        // a borda em alto-relevo em volta de tudo (como na foto): luz na quina de cima/esquerda,
        // sombra embaixo/direita, o alto da borda e o sulco por dentro; depois a moldura preta
        {
            const float G = F * 0.082f, d = std::max(1.f, M.d(1.0f));
            base(G + d * 0.5f, ImVec2(-d, -d), cinzaFrio(0.44f), true);
            base(G + d * 0.5f, ImVec2(d * 1.2f, d * 1.4f), cinzaFrio(0.004f), true);
            base(G, ImVec2(0, 0), cinzaFrio(0.17f), true);
            base(G - d * 1.4f, ImVec2(d * 0.6f, d * 0.7f), cinzaFrio(0.10f), true);
            base(G * 0.32f, ImVec2(0, 0), cinzaFrio(0.010f), true);
            base(0.f, ImVec2(0, 0), preto, false);
        }
        {   // o brilho largo do queixo (embaixo, a esquerda) e a quina que passa embaixo do dial
            const ImVec2 uv = dl->_Data->TexUvWhitePixel;
            const int nx = 16, ny = 5;
            const float yA = pcy + 1.18f * Fp, yB = pcy + vFundo * Fp - 1.f;
            auto ponto = [&](int ix, int iy) {
                const float t = (float)iy / ny, yy = yA + (yB - yA) * t;
                const float meiaL = meiaQueixo((yy - pcy) / Fp);
                const float xx = pcx + Fp * meiaL * (-1.f + 2.f * ix / nx) * 0.92f;
                return M.p(xx, yy);
            };
            auto corQ = [&](ImVec2 q) {
                const float u = (q.x - c.x) / F, v = (q.y - c.y) / F;
                const float l = 0.03f + 0.09f * std::exp(-0.5f * ((u + 0.85f) / 0.42f) * ((u + 0.85f) / 0.42f)) * std::clamp((v - 1.25f) / 0.30f, 0.f, 1.f);
                return cinzaFrio(l);
            };
            dl->PrimReserve(nx * ny * 6, (nx + 1) * (ny + 1));
            const ImDrawIdx i0 = (ImDrawIdx)dl->_VtxCurrentIdx;
            for (int iy = 0; iy <= ny; ++iy)
                for (int ix = 0; ix <= nx; ++ix) { const ImVec2 q = ponto(ix, iy); dl->PrimWriteVtx(q, uv, corQ(q)); }
            for (int iy = 0; iy < ny; ++iy)
                for (int ix = 0; ix < nx; ++ix) {
                    const ImDrawIdx a = (ImDrawIdx)(i0 + iy * (nx + 1) + ix), b = (ImDrawIdx)(a + nx + 1);
                    dl->PrimWriteIdx(a); dl->PrimWriteIdx((ImDrawIdx)(a + 1)); dl->PrimWriteIdx((ImDrawIdx)(b + 1));
                    dl->PrimWriteIdx(a); dl->PrimWriteIdx((ImDrawIdx)(b + 1)); dl->PrimWriteIdx(b);
                }
            dl->PathArcTo(c, F * 1.618f, rad(38), rad(142), 48);
            dl->PathStroke(cinzaFrio(0.20f, 190), 0, 1.0f);
        }
        // a viseira (a parte de cima, entre MAIN e SUB): superficie que pega a luz do
        // estudio - clara um pouco a esquerda do meio e escurecendo para os lados,
        // mais depressa para a direita (medida na foto)
        {
            const ImVec2 uv = dl->_Data->TexUvWhitePixel;
            const float yTop = M.p(0, pcy - 1.86f * Fp).y, yLado = c.y - F * 1.38f;
            const float xE = M.p(pcx - 1.80f * Fp, 0).x, xD = M.p(pcx + 1.62f * Fp, 0).x;
            auto corV = [&](ImVec2 q) {
                const float u = (q.x - c.x) / F + 0.30f, sg = u < 0.f ? 0.50f : 0.42f;
                float l = 0.20f + 0.55f * std::exp(-0.5f * (u / sg) * (u / sg));
                if (q.y > c.y - F * 1.52f) l *= 0.80f;      // abaixo das teclas MAIN / SUB: faixa mais escura
                return cinzaFrio(l);
            };
            std::vector<ImVec2> baixo;
            const float RX = 1.62f, RY = 1.69f;                               // o sulco da gola (elipse)
            const float sn = -1.38f / RY, cs = std::sqrt(std::max(0.f, 1.f - sn * sn));
            const float xc = RX * cs;                                         // onde o sulco cruza yLado
            for (int k = 0; k <= 8; ++k) baixo.push_back(ImVec2(xE + (c.x - F * xc - xE) * k / 8.f, yLado));
            const float aE = std::atan2(sn, -cs), aD = std::atan2(sn, cs);
            for (int k = 1; k < 48; ++k) {
                const float a = aE + (aD - aE) * k / 48.f;
                baixo.push_back(ImVec2(c.x + std::cos(a) * F * RX, c.y + std::sin(a) * F * RY));
            }
            for (int k = 0; k <= 8; ++k) baixo.push_back(ImVec2(c.x + F * xc + (xD - c.x - F * xc) * k / 8.f, yLado));
            const int nb = (int)baixo.size();
            dl->PrimReserve((nb - 1) * 6, nb * 2);
            const ImDrawIdx i0 = (ImDrawIdx)dl->_VtxCurrentIdx;
            for (const ImVec2& q : baixo) {
                dl->PrimWriteVtx(q, uv, corV(q));
                const ImVec2 t(q.x, yTop);
                dl->PrimWriteVtx(t, uv, corV(t));
            }
            for (int k = 0; k < nb - 1; ++k) {
                const ImDrawIdx a = (ImDrawIdx)(i0 + 2 * k);
                dl->PrimWriteIdx(a); dl->PrimWriteIdx((ImDrawIdx)(a + 1)); dl->PrimWriteIdx((ImDrawIdx)(a + 3));
                dl->PrimWriteIdx(a); dl->PrimWriteIdx((ImDrawIdx)(a + 3)); dl->PrimWriteIdx((ImDrawIdx)(a + 2));
            }
            // a quina de cima da viseira (pega luz)
            dl->AddLine(ImVec2(xE + M.d(6), yTop + 0.5f), ImVec2(xD - M.d(6), yTop + 0.5f), cinzaFrio(0.42f, 150), 1.0f);
        }
        // os sulcos das teclas grandes (CLAR, VC TUNE, FINE, MAIN/SUB), tambem em duas passadas
        for (int passada = 0; passada < 2; ++passada) {
            const float e = passada == 0 ? 1.2f : 0.f;
            const ImU32 cor = passada == 0 ? cinzaFrio(0.17f) : preto;
            for (int sy = -1; sy <= 1; sy += 2)
                for (int sx = -1; sx <= 1; sx += 2) {
                    std::vector<ImVec2> f = contornoPetala(c, F, sx, sy, true);
                    float xs = 0, ys = 0;
                    for (const ImVec2& q : f) { xs += q.x; ys += q.y; }
                    const ImVec2 m(xs / f.size(), ys / f.size());
                    if (e > 0) for (ImVec2& q : f) {                 // 1 px para fora (a partir do meio)
                        const float dx = q.x - m.x, dy = q.y - m.y, dd = std::max(1.f, std::sqrt(dx * dx + dy * dy));
                        q.x += dx / dd * e; q.y += dy / dd * e;
                    }
                    leque(dl, f, m, [&](ImVec2) { return cor; });
                    if (passada == 1) dl->AddPolyline(f.data(), (int)f.size(), preto, ImDrawFlags_Closed, 1.0f);
                }
        }
        // a gola em volta do anel: vao escuro, o labio que pega a luz e o sulco preto.
        // A gola e mais baixa que o anel: na perspectiva sai um pouco para cima, a esquerda.
        anelLuz(dl, c, F * 1.425f, F * 1.612f, 0.f, (float)(2 * kPi), 180, [](float, float) { return cinzaFrio(0.022f); });
        const ImVec2 cg(c.x - F * 0.025f, c.y - F * 0.035f);
        anelLuz(dl, cg, F * 1.470f, F * 1.500f, 0.f, (float)(2 * kPi), 180, [&](float a, float rr) {
            return cinzaFrio(rr < F * 1.48f ? 0.03f : tabela(T_LABIO, a));
        });
        anelLuz(dl, cg, F * 1.500f, F * 1.545f, 0.f, (float)(2 * kPi), 180, [&](float a, float rr) {
            const float l = tabela(T_LABIO, a);
            return cinzaFrio(rr < F * 1.52f ? l * 0.85f : l * 0.10f);
        });
    }
    // MAIN / SUB grandes (banda de operacao), nos rebaixos da parte de cima da moldura
    if (teclaCunha("kMain", "MAIN", FX(334), FX(452), FX(352), FX(432), FY(270), FY(312), ativo_ == 0,
                   "MAIN: o dial, o som e os botões passam a comandar o receptor MAIN")) ativar(0);
    if (teclaCunha("kSub", "SUB", FX(687), FX(805), FX(702), FX(792), FY(270), FY(312), ativo_ == 1,
                   "SUB: o dial, o som e os botões passam a comandar o receptor SUB")) ativar(1);
    // teclas grandes em volta do dial (CLAR, VC TUNE, FINE TUNING, MAIN/SUB)
    {
        char d[200];
        std::snprintf(d, sizeof d, "CLAR: o anel MPVD ajusta o clarificador do %s (%+.3f kHz)\nSegurar: zera", ativo_ == 0 ? "MAIN" : "SUB", A.rit / 1000.0);
        const int k = teclaPetala("kClar", "CLAR", pcx, pcy, Fp, -1, -1, mpvdFunc_ == 1 ? 3 : 1, d);
        if (k == 1) mpvdFunc_ = mpvdFunc_ == 1 ? 0 : 1;
        if (k == 2) { A.rit = 0; aviso("CLAR zerado"); }
    }
    if (teclaPetala("kVcBig", "VC\nTUNE", pcx, pcy, Fp, +1, -1, mpvdFunc_ == 2 ? 3 : 1,
                    "VC TUNE: o anel MPVD ajusta o ganho de RF do receptor ouvido (sintonia fina da sensibilidade)") == 1)
        mpvdFunc_ = mpvdFunc_ == 2 ? 0 : 2;
    if (teclaPetala("kFine", "FINE\nTUNING", pcx, pcy, Fp, -1, +1, A.fine ? 3 : 1, "FINE TUNING: passos finos no dial (1 Hz)") == 1) A.fine = !A.fine;
    if (teclaPetala("kMsAnel", "MAIN\n/SUB", pcx, pcy, Fp, +1, +1, mpvdFunc_ == 4 ? 3 : 1,
                    "MAIN/SUB: o anel MPVD sintoniza o OUTRO receptor (o que não está no dial)") == 1)
        mpvdFunc_ = mpvdFunc_ == 4 ? 0 : 4;
    {   // QMB (azul), na moldura a esquerda
        char d[200];
        std::snprintf(d, sizeof d, "QMB (memória rápida): clique chama as memórias, uma por vez (%d gravadas)\nSegurar: grava a frequência atual (até 10)", (int)qm_.size());
        rotulo(16.f, pcx - 1.795f * Fp, pcy - 0.47f * Fp, P_ROTULO, "QMB", 0.5f, 0.5f * Fp);
        const int k = tecla("kQmb", "", pcx - 1.91f * Fp, pcy - 0.34f * Fp, 0.23f * Fp, 0.52f * Fp, 0, nullptr, d, 0, true);
        if (k == 1) {
            if (qm_.empty()) aviso("QMB vazia (segure a tecla QMB para gravar)");
            else {
                qmIdx_ = (qmIdx_ + 1) % (int)qm_.size();
                mudarModoPainel(qm_[qmIdx_].modo);
                sintonizar(qm_[qmIdx_].f, false);
                char a[48]; std::snprintf(a, sizeof a, "QMB %d", qmIdx_ + 1); aviso(a);
            }
        }
        if (k == 2) {
            qm_.insert(qm_.begin(), Rapida{r_->vfo(), r_->modo()});
            if (qm_.size() > 10) qm_.resize(10);
            qmIdx_ = -1;
            aviso("Gravado na QMB");
        }
    }
    {   // VC TUNE pequena (com LED) e C.S, na moldura a direita
        const std::string q = Config::instance().qmode();
        const int led = q == "on" ? 4 : q == "auto" ? 3 : 1;
        if (tecla("kVcSmall", "VC\nTUNE", pcx + 1.645f * Fp, pcy - 0.58f * Fp, 0.25f * Fp, 0.43f * Fp, led, nullptr,
                  "VC TUNE (pré-seletor): entrada de HF do dongle (amostragem direta Q)\nAUTO liga abaixo de 24 MHz (LED laranja), ON (vermelho), OFF")) {
            auto& c = Config::instance();
            c.set("qmode", std::string(q == "auto" ? "on" : q == "on" ? "off" : "auto"));
            r_->aplicarConfig(); sintonizar(r_->vfo(), false);
            aviso(std::string("VC TUNE (entrada HF): ") + (q == "auto" ? "ON" : q == "on" ? "OFF" : "AUTO"));
        }
    }
    {
        char d[220];
        std::snprintf(d, sizeof d, "C.S (Custom Select): o anel MPVD faz %s\nSegurar: escolher a função do C.S", nomeFuncaoMulti(csFunc_));
        const int k = tecla("kCs", "C.S", pcx + 1.645f * Fp, pcy + 0.02f * Fp, 0.25f * Fp, 0.29f * Fp, 0, nullptr, d);
        if (k == 1) mpvdFunc_ = mpvdFunc_ == 3 ? 0 : 3;
        if (k == 2) ImGui::OpenPopup("##csFt");
        // tracinho embaixo do C.S (o LED do C.S)
        const float xm = pcx + 1.77f * Fp, ym = pcy + 0.25f * Fp;
        dl->AddRectFilled(M.p(xm - 3.2f, ym - 0.7f), M.p(xm + 3.2f, ym + 0.7f), mpvdFunc_ == 3 ? P_LARANJA : IM_COL32(0x10, 0x12, 0x14, 255), M.d(0.7f));
    }
    if (ImGui::BeginPopup("##csFt")) {
        ImGui::TextDisabled("Função do C.S (anel MPVD)");
        for (int k = 0; k < 9; ++k) if (ImGui::Selectable(nomeFuncaoMulti(k), csFunc_ == k)) { csFunc_ = k; mpvdFunc_ = 3; }
        ImGui::EndPopup();
    }
    // o dial e o anel MPVD
    {
        const char* nomeAnel[5] = {"sintonia rápida (10x)", "CLAR (clarificador)", "VC TUNE (ganho de RF)", "C.S", "MAIN/SUB (o outro receptor)"};
        char da[200];
        std::snprintf(da, sizeof da, "Anel MPVD: %s%s%s\nTeclas CLAR / VC TUNE / C.S / MAIN/SUB escolhem o que ele faz",
                      nomeAnel[mpvdFunc_], mpvdFunc_ == 3 ? " - " : "", mpvdFunc_ == 3 ? nomeFuncaoMulti(csFunc_) : "");
        int pa = 0;
        const int p = dialMpvd("dMain", pcx, pcy, 1.17f * Fp, 1.425f * Fp, angDialA_, angMpvd_, A.lock, pa,
                               "Dial: sintoniza o receptor ouvido (arraste girando ou use a roda do mouse)\nGirar rápido acelera. FAST = 10x, FINE TUNING = 1 Hz",
                               da);
        if (p) sintonizarRx(ativo_, p);
        if (pa) {
            switch (mpvdFunc_) {
            case 0: sintonizarRx(ativo_, (int64_t)pa * 10); break;
            case 1: { A.rit = std::clamp(A.rit + pa * 10, -9990, 9990); A.ritOn = true; break; }
            case 2: { A.rf = std::clamp(A.rf + pa * 1.f, 0.f, 100.f); char b[48]; std::snprintf(b, sizeof b, "RF %.1f dB", 49.6f * A.rf / 100.f); aviso(b, 1.0); break; }
            case 3: funcaoMulti(csFunc_, pa); break;
            case 4: sintonizarRx(1 - ativo_, pa); break;
            }
        }
    }
}

// A direita: MEMORY, MIC/SPEED-PROC/PITCH e as duas fileiras (SUB em cima, MAIN embaixo)
void Ui::blocoDireita()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // MEMORY (com os quadradinhos de pontos dos lados)
    {
        const float cx = FX(918), yc = FY(251);
        rotulo(16.f, cx, yc, P_ROTULO, "MEMORY", 0.5f);
        const float tw = larguraRotulo(16.f, "MEMORY");
        for (int s = -1; s <= 1; s += 2)
            for (int i = 0; i < 9; ++i)
                for (int j = 0; j < 3; ++j) {
                    const float x = s < 0 ? cx - tw * 0.5f - 4.f - i * 2.6f : cx + tw * 0.5f + 4.f + i * 2.6f;
                    const float y = yc - 2.6f + j * 2.6f + ((i & 1) ? 1.3f : 0.f);
                    if (j == 2 && (i & 1)) continue;
                    dl->AddCircleFilled(M.p(x, y), std::max(0.5f, M.d(0.7f)), alfa(P_ROTULO2, 170), 4);
                }
        Receptor& R = rx_[ativo_];
        if (tecla("kVm", "V/M", FX(847), FY(270), FW(40), FH(22), R.memModo ? 3 : 0, nullptr, "V/M: alterna entre VFO e canais de memória (o dial e o MULTI em MEM CH passam os canais)")) {
            if (mem_.empty()) aviso("Nenhuma memória gravada (V>M grava)");
            else {
                R.memModo = !R.memModo;
                if (R.memModo) {
                    int mais = 0; long long melhor = LLONG_MAX;
                    for (int i = 0; i < (int)mem_.size(); ++i) {
                        const long long dd = std::llabs((long long)mem_[i].f - (long long)r_->vfo());
                        if (dd < melhor) { melhor = dd; mais = i; }
                    }
                    R.memCanal = mais; sintonizarMemoria((size_t)mais);
                }
            }
        }
        if (tecla("kMv", "M>V", FX(902), FY(270), FW(40), FH(22), 0, nullptr, "M>V: o canal de memória vira o VFO (sai do modo memória)")) R.memModo = false;
        {
            const int k = tecla("kVm2", "V>M", FX(952), FY(270), FW(40), FH(22), memAberta_ ? 3 : 0, nullptr, "V>M: grava a frequência atual numa memória (com nome)\nSegurar: abre a lista de memórias");
            if (k == 1) { memAberta_ = true; std::snprintf(memNome_, sizeof memNome_, "%s %s", fmtFreq(r_->vfo()).c_str(), r_->modo().c_str()); }
            if (k == 2) memAberta_ = !memAberta_;
        }
    }
    // MIC/SPEED - PROC/PITCH
    {
        float di, dO; char d[260];
        const bool cw = grupoModo(r_->modo()) == 1;
        std::snprintf(d, sizeof d, "MIC/SPEED: tonalidade do áudio (%s %d%%)\n%s",
                      tom_ < 45 ? "grave" : tom_ > 55 ? "agudo" : "plano", (int)std::lround(std::fabs(tom_ - 50) * 2),
                      cw ? (std::string("PITCH: tom do CW e centro do APF (") + std::to_string((int)cwPitch_) + " Hz)").c_str()
                         : (std::string("PROC: força do compressor de áudio (") + std::to_string((int)std::lround(procNivel_ * 100)) + "%)" + (proc_ ? "" : " - girar liga")).c_str());
        if (giro("gMic", FX(1100), FY(240), 31, "MIC/SPEED", "PROC/PITCH", tom_ / 100.f, cw ? (cwPitch_ - 300.f) / 700.f : procNivel_, di, dO, d,
                 true, FY(135) - 4.6f)) {
            tom_ = std::clamp(tom_ + di * 100.f, 0.f, 100.f); if (di != 0) rxeq_ = 0;
            if (dO != 0) {
                if (cw) {
                    static float acc = 0; acc += dO * 700.f;
                    while (acc >= 10) { cwPitch_ = std::min(1000.f, cwPitch_ + 10); acc -= 10; }
                    while (acc <= -10) { cwPitch_ = std::max(300.f, cwPitch_ - 10); acc += 10; }
                } else {
                    procNivel_ = std::clamp(procNivel_ + dO, 0.f, 1.f);
                    if (!proc_) { proc_ = true; aviso("PROC ligado"); }
                }
            }
        }
        // PROC (liga/desliga o compressor) e KEYER (decodificador de CW): os textinhos sao clicaveis
        auto rotClic = [&](const char* id, const char* t, float x, bool on, const char* dica) {
            const float tw = larguraTexto(f_.painel, 5.8f, t);
            const float y = FY(150) - 3.6f;
            texto(f_.painel, 5.8f, x, y, on ? P_LARANJA : P_ROTULO, t, 0.5f);
            ImGui::SetCursorScreenPos(M.p(x - tw * 0.5f - 2, y - 2));
            const bool c = ImGui::InvisibleButton(id, ImVec2(M.d(tw + 4), M.d(11)));
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", dica);
            return c;
        };
        if (rotClic("##proc", "PROC", FX(1044), proc_, "PROC: liga/desliga o compressor de áudio (força no anel PROC/PITCH)")) proc_ = !proc_;
        if (rotClic("##keyer", "KEYER", FX(1152), r_->decoders().tipo() == Decoders::CW, "KEYER: abre o decodificador de CW")) {
            decAberta_ = true;
            if (r_->decoders().tipo() != Decoders::CW) escolherDecoder(Decoders::CW, true);
        }
    }
    // as duas fileiras: SUB (rotulos azuis, em cima) e MAIN (brancos, embaixo)
    // yRot / yTecla / yKnob / yRotKnob: posicoes na foto (px)
    auto fileira = [&](int rx, float yTeclaFoto, float yKnobFoto, float yRotKnobFoto) {
        Receptor& R = rx_[rx];
        const char* nome = rx == 0 ? "MAIN" : "SUB";
        g_corRot = rx == 1 ? P_AZULSUB : IM_COL32(0xf4, 0xf4, 0xf4, 255);
        g_borracha = 0;
        const float xs[6] = {FX(847), FX(902), FX(957), FX(1022), FX(1077), FX(1132)};
        const float ws[6] = {FW(35), FW(35), FW(35), FW(45), FW(45), FW(45)};
        const float yk = FY(yTeclaFoto), hk = FH(22.5f);
        char id[16], d[220];
        auto led = [](bool a, bool b) { return a && b ? 5 : a ? 3 : b ? 5 : 1; };
        std::snprintf(id, sizeof id, "kNb%d", rx);
        std::snprintf(d, sizeof d, "NB: apagador de pulsos do %s (no áudio)\nSegurar: NB2 (no IQ, antes do demodulador)\nNível: MULTI ou C.S em NB LEVEL", nome);
        { const int k = tecla(id, "", xs[0], yk, ws[0], hk, led(R.nb1, R.nb2), "NB", d); if (k == 1) R.nb1 = !R.nb1; if (k == 2) R.nb2 = !R.nb2; }
        std::snprintf(id, sizeof id, "kDnr%d", rx);
        std::snprintf(d, sizeof d, "DNR: redutor de ruído espectral do %s (o NR ESPECTRAL do RXSDR)\nSegurar: NR1 (redutor simples)\nForça: MULTI ou C.S em DNR LEVEL", nome);
        { const int k = tecla(id, "", xs[1], yk, ws[1], hk, led(R.nr2, R.nr1), "DNR", d); if (k == 1) R.nr2 = !R.nr2; if (k == 2) R.nr1 = !R.nr1; }
        std::snprintf(id, sizeof id, "kDnf%d", rx);
        std::snprintf(d, sizeof d, "DNF: notch automático do %s (tira a portadora/apito sozinho)\nSegurar: BC (cancelador de vários apitos)", nome);
        { const int k = tecla(id, "", xs[2], yk, ws[2], hk, led(R.anch, R.bc), "DNF", d); if (k == 1) R.anch = !R.anch; if (k == 2) R.bc = !R.bc; }
        std::snprintf(id, sizeof id, "kNch%d", rx);
        std::snprintf(d, sizeof d, "NOTCH: notch manual do %s (posição no botão NOTCH)\nSegurar: estreito / largo%s", nome, R.nchLargo ? " (agora largo)" : "");
        { const int k = tecla(id, "", xs[3], yk, ws[3], hk, R.nch ? (R.nchLargo ? 5 : 3) : 1, "NOTCH", d); if (k == 1) R.nch = !R.nch; if (k == 2) { R.nch = true; R.nchLargo = !R.nchLargo; } }
        std::snprintf(id, sizeof id, "kCont%d", rx);
        std::snprintf(d, sizeof d, "CONT: contorno - filtro que abaixa uma faixa do áudio do %s (posição no anel CONT/APF)", nome);
        if (tecla(id, "", xs[4], yk, ws[4], hk, R.bef ? 3 : 1, "CONT", d)) R.bef = !R.bef;
        std::snprintf(id, sizeof id, "kApf%d", rx);
        std::snprintf(d, sizeof d, "APF: filtro de pico de áudio para CW do %s (centro no PITCH)", nome);
        if (tecla(id, "", xs[5], yk, ws[5], hk, R.apf ? 3 : 1, "APF", d)) R.apf = !R.apf;
        const float yKnob = FY(yKnobFoto), yRotK = FY(yRotKnobFoto) - 4.6f;
        // SHIFT - WIDTH
        float di, dO;
        {
            const std::string md = rx == ativo_ ? r_->modo() : R.modo;
            const int g = grupoModo(md);
            float& hi = filHi_[g][R.fil];
            float& lo = filLo_[g][R.fil];
            float tIn = 0.5f, tOut = 0.5f;
            switch (g) {
            case 0: std::snprintf(d, sizeof d, "SHIFT: desloca o filtro do %s (%.2f a %.2f kHz)\nWIDTH: largura (%.2f kHz)  - R.FIL %s", nome, lo / 1000.f, hi / 1000.f, (hi - lo) / 1000.f, kNomesFil[R.fil]);
                    tIn = (lo + hi) * 0.5f / 3000.f; tOut = (hi - lo - 400.f) / 4600.f; break;
            case 1: std::snprintf(d, sizeof d, "SHIFT: desloca a faixa do CW (%+.0f Hz)\nWIDTH: largura do filtro de CW (%.0f Hz)  - R.FIL %s", lo, hi, kNomesFil[R.fil]);
                    tIn = (lo + 800) / 1600.f; tOut = (hi - 50) / 2450.f; break;
            case 2: std::snprintf(d, sizeof d, "SHIFT: corte baixo do AM (%.2f kHz)\nWIDTH: corte alto do AM (%.1f kHz)  - R.FIL %s", lo / 1000.f, hi / 1000.f, kNomesFil[R.fil]);
                    tIn = lo / 1000.f; tOut = (hi - 2000) / 6000.f; break;
            case 3: std::snprintf(d, sizeof d, "WIDTH: largura do FM (%.1f kHz)  - R.FIL %s", hi / 1000.f, kNomesFil[R.fil]);
                    tOut = (hi - 6000) / 24000.f; break;
            default: std::snprintf(d, sizeof d, "WIDTH: largura do FM largo (%.0f kHz)  - R.FIL %s", hi / 1000.f, kNomesFil[R.fil]);
                    tOut = (hi - 100000) / 150000.f; break;
            }
            std::snprintf(id, sizeof id, "gSw%d", rx);
            if (giro(id, FX(920), yKnob, 31.5f, "SHIFT", "WIDTH", tIn, tOut, di, dO, d, true, yRotK)) {
                switch (g) {
                case 0: {
                    float s = di * 3000.f;                       // SHIFT: as duas bordas juntas
                    s = std::clamp(s, -lo, 5000.f - hi);
                    lo += s; hi += s;
                    hi = std::clamp(hi + dO * 4600.f, lo + 400.f, 5000.f);   // WIDTH: a borda de cima
                    break; }
                case 1: lo = std::clamp(lo + di * 1600.f, -800.f, 800.f); hi = std::clamp(hi + dO * 2450.f, 50.f, 2500.f); break;
                case 2: lo = std::clamp(lo + di * 1000.f, 0.f, 1000.f); hi = std::clamp(hi + dO * 6000.f, 2000.f, 8000.f); break;
                case 3: hi = std::clamp(hi + (di + dO) * 24000.f, 6000.f, 30000.f); break;
                default: hi = std::clamp(hi + (di + dO) * 150000.f, 100000.f, 250000.f); break;
                }
                if (rx == ativo_) aplicarFiltro();
            }
        }
        // NOTCH - CONT/APF
        {
            const double nh = 150.0 * std::pow(3500.0 / 150.0, std::clamp(R.notch, 0.f, 1.f));
            const double ch = 150.0 * std::pow(3500.0 / 150.0, std::clamp(R.cont, 0.f, 1.f));
            std::snprintf(d, sizeof d, "NOTCH: frequência do notch do %s (%.0f Hz)\n%s", nome, nh,
                          R.apf ? (std::string("APF: centro do filtro de pico (") + std::to_string((int)cwPitch_) + " Hz)").c_str()
                                : (std::string("CONT: frequência do contorno (") + std::to_string((int)std::lround(ch)) + " Hz)").c_str());
            std::snprintf(id, sizeof id, "gNc%d", rx);
            if (giro(id, FX(1100), yKnob, 31.5f, "NOTCH", "CONT/APF", R.notch, R.apf ? (cwPitch_ - 300.f) / 700.f : R.cont, di, dO, d, true, yRotK)) {
                R.notch = std::clamp(R.notch + di, 0.f, 1.f);
                if (dO != 0) {
                    if (R.apf) cwPitch_ = std::clamp(cwPitch_ + dO * 700.f, 300.f, 1000.f);
                    else { R.cont = std::clamp(R.cont + dO, 0.f, 1.f); if (!R.bef) R.bef = true; }
                }
            }
        }
        g_corRot = IM_COL32(0xf4, 0xf4, 0xf4, 255);
    };
    fileira(1, 327.5f, 450.f, 372.f);
    fileira(0, 542.5f, 665.f, 585.f);
}

// FUNC (tecla FUNC ou botao direito no chassi)
void Ui::popupsPainel()
{
    auto& c = Config::instance();
    if (ImGui::BeginPopup("##funcFt")) {
        ImGui::TextDisabled("MB1SDR  v" RXSDR_VERSAO "  -  FUNC");
        ImGui::Separator();
        if (ImGui::MenuItem("Configuração (aparelho, som, S-meter)")) abrirConfig();
        if (ImGui::MenuItem("Decodificadores digitais (DATA)")) decAberta_ = true;
        if (ImGui::MenuItem("Memórias (lista)")) memAberta_ = true;
        if (ImGui::MenuItem("Digitar a frequência (kHz)")) { entrando_ = true; entrada_.clear(); }
        if (ImGui::BeginMenu("Gravador")) {
            if (ImGui::MenuItem(r_->som().gravando() ? "Parar a gravação (STOP)" : "Gravar o áudio (REC)")) alternarGravacao();
            if (ImGui::MenuItem("Tocar a última gravação (PLAY)")) tocarUltimaGravacao();
            if (ImGui::MenuItem("Parar de tocar")) PlaySoundA(nullptr, nullptr, 0);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("GEN - cobertura geral HF")) { itensGen(0); ImGui::EndMenu(); }
        if (ImGui::BeginMenu("GEN - VHF / UHF")) { itensGen(1); ImGui::EndMenu(); }
        ImGui::Separator();
        if (ImGui::BeginMenu("Aparelho")) {
            if (ImGui::MenuItem("T-AGC (AGC do dongle)", nullptr, c.agc())) { c.set("agc", !c.agc()); r_->aplicarConfig(); }
            if (ImGui::MenuItem("Bias-T (alimenta a antena ativa)", nullptr, c.biasT())) { c.set("bias_t", !c.biasT()); r_->aplicarConfig(); }
            if (ImGui::MenuItem("Tirar o risco do DC do centro", nullptr, c.dcRemove())) c.set("dc_remove", !c.dcRemove());
            if (ImGui::BeginMenu("Taxa de amostragem (largura do escopo)")) {
                const unsigned tx[] = {250000, 1024000, 1400000, 1800000, 1920000, 2048000, 2400000, 2560000, 2880000, 3200000};
                for (unsigned t : tx) {
                    char r[32]; std::snprintf(r, sizeof r, "%.3f Msps", t / 1e6);
                    if (ImGui::MenuItem(r, nullptr, c.sampleRate() == t)) { c.set("sample_rate", (long long)t); r_->aplicarConfig(); }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("XVTR (upconverter)")) {
                const struct { const char* n; uint64_t hz; } op[] = {{"Desligado", 0}, {"125 MHz (Ham It Up, SpyVerter)", 125000000},
                                                                     {"120 MHz", 120000000}, {"100 MHz", 100000000}, {"106,25 MHz", 106250000}};
                for (const auto& o : op)
                    if (ImGui::MenuItem(o.n, nullptr, r_->xvtr() == o.hz)) {
                        r_->setXvtr(o.hz);
                        if (o.hz) c.set("qmode", std::string("off"));
                        r_->aplicarConfig();
                    }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Áudio")) {
            if (ImGui::MenuItem("PROC (compressor)", nullptr, proc_)) proc_ = !proc_;
            if (ImGui::BeginMenu("RX EQ (equalização)")) {
                const char* nm[5] = {"OFF (plano)", "HB1 agudo", "HB2 mais agudo", "BB1 grave", "BB2 mais grave"};
                const float tons[5] = {50, 68, 85, 32, 15};
                for (int i = 0; i < 5; ++i) if (ImGui::MenuItem(nm[i], nullptr, rxeq_ == i)) { rxeq_ = i; tom_ = tons[i]; }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Anel RF/SQL dos botões AF")) {
                if (ImGui::MenuItem("Automático (SQL no FM, RF nos outros)", nullptr, rfSql_ == 0)) rfSql_ = 0;
                if (ImGui::MenuItem("RF (ganho)", nullptr, rfSql_ == 1)) rfSql_ = 1;
                if (ImGui::MenuItem("SQL (squelch)", nullptr, rfSql_ == 2)) rfSql_ = 2;
                ImGui::EndMenu();
            }
            ImGui::SetNextItemWidth(200 * s_);
            float hang = sqlHang_;
            if (ImGui::SliderFloat("Demora do squelch (s)", &hang, 0.f, 2.f, "%.1f")) sqlHang_ = hang;
            ImGui::SetNextItemWidth(200 * s_);
            if (ImGui::SliderFloat("Força do DNR", &nrEspForca_, 0.f, 100.f, "%.0f")) {}
            ImGui::SetNextItemWidth(200 * s_);
            if (ImGui::SliderFloat("Nível do NB", &nbNivel_, 0.f, 100.f, "%.0f")) {}
            ImGui::SetNextItemWidth(200 * s_);
            if (ImGui::SliderFloat("Força do NR1", &nrNivel_, 0.f, 100.f, "%.0f")) {}
            ImGui::SetNextItemWidth(200 * s_);
            float nb2 = nb2Nivel_ * 100.f;
            if (ImGui::SliderFloat("Nível do NB2", &nb2, 0.f, 100.f, "%.0f")) nb2Nivel_ = nb2 / 100.f;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tela")) {
            const char* nm[4] = {"100%", "75%", "50%", "30%"};
            if (ImGui::BeginMenu("TFT DIMMER (brilho)")) {
                for (int i = 0; i < 4; ++i) if (ImGui::MenuItem(nm[i], nullptr, dim_ == i)) dim_ = i;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("S-meter: leitura")) {
                const char* mm[3] = {"S", "dB", "PEAK"};
                for (int i = 0; i < 3; ++i) if (ImGui::MenuItem(mm[i], nullptr, medidorModo_ == i)) medidorModo_ = i;
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Escopo em tela cheia (ESC volta)")) scp_ = 2;
            if (ImGui::MenuItem("Nomes das memórias no escopo", nullptr, memVisivel_)) memVisivel_ = !memVisivel_;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("TIMER (desligar depois de)")) {
            const int mins[] = {15, 30, 60, 90, 120};
            for (int m : mins) {
                char r[32]; std::snprintf(r, sizeof r, "%d minutos", m);
                if (ImGui::MenuItem(r)) sleepAte_ = agora() + m * 60.0;
            }
            if (ImGui::MenuItem("Desligado (cancelar)", nullptr, sleepAte_ <= 0)) sleepAte_ = 0;
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("VOICE: falar a frequência")) falarFrequencia(ativo_);
        ImGui::Separator();
        if (ImGui::MenuItem("Salvar a configuração agora")) {
            salvarEstado(); Config::instance().salvar(true);
            aviso("Configuração salva");
        }
        if (ImGui::BeginMenu("Idioma / Language")) {
            if (ImGui::MenuItem("Português", nullptr, !emIngles())) { definirIngles(false); Config::instance().set("lang", std::string("pt")); }
            if (ImGui::MenuItem("English", nullptr, emIngles())) { definirIngles(true); Config::instance().set("lang", std::string("en")); }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Capturar a tela (PNG)", "F12")) { g_pedirCaptura = true; }
        if (ImGui::MenuItem("Sobre")) sobreAberta_ = true;
        ImGui::EndPopup();
    }
}

// ===========================================================================
//  MB1SDR: pecas do painel do MB1 (posicoes em px da foto)
// ===========================================================================

// Tecla. estilo 0 = do painel (pilula cinza afundada, LED redondo a esquerda),
// 1 = do vidro preto (bandas, SPLIT...), 2 = tecla de toque da tela.
// led: 0 sem, 1 apagado, 2 verde, 3 laranja, 4 vermelho, 5 branco, 6 ciano, 7 amarelo.
// Devolve 1 = clique, 2 = segurou (0,6 s).
int Ui::teclaIc(const char* id, const char* txt, float x0, float y0, float x1, float y1, int led,
                const char* rot, float yRot, const char* dica, int estilo, float alinTxt, unsigned corTxt)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = Q(x0, y0), b = Q(x1, y1);
    ImGui::SetCursorScreenPos(a);
    ImGui::PushID(id);
    ImGui::InvisibleButton("##ic", ImVec2(std::max(1.f, b.x - a.x), std::max(1.f, b.y - a.y)));
    const ImGuiID iid = ImGui::GetItemID();
    const bool sobre = ImGui::IsItemHovered(), ativo = ImGui::IsItemActive();
    int res = 0;
    if (ImGui::IsItemActivated()) longoFeito()[iid] = 0;
    if (ativo && ImGui::GetIO().MouseDownDuration[0] > 0.6f && longoFeito()[iid] == 0) { longoFeito()[iid] = 1; res = 2; }
    if (ImGui::IsItemDeactivated() && sobre && longoFeito()[iid] == 0) res = 1;
    ImGui::PopID();
    const float h = y1 - y0;
    const float um = std::max(1.f, QD(1.2f));
    if (estilo == 0) {
        const float r = QD(h * 0.40f), g = QD(3.2f);
        dl->AddRectFilled(ImVec2(a.x - g, a.y - g), ImVec2(b.x + g, b.y + g * 1.25f), IM_COL32(0x0b, 0x0c, 0x0d, 255), r + g);
        dl->AddRect(ImVec2(a.x - g, a.y - g), ImVec2(b.x + g, b.y + g * 1.25f), IM_COL32(0x5c, 0x5e, 0x62, 120), r + g, 0, 1.f);
        const ImU32 c0 = ativo ? IM_COL32(0x1e, 0x1f, 0x21, 255) : sobre ? IM_COL32(0x5e, 0x61, 0x66, 255) : IM_COL32(0x4c, 0x4e, 0x52, 255);
        const ImU32 c1 = ativo ? IM_COL32(0x12, 0x13, 0x14, 255) : IM_COL32(0x24, 0x25, 0x28, 255);
        retGrad(dl, a, b, r, c0, c1);
        dl->AddRect(a, b, IM_COL32(0x84, 0x88, 0x8e, 170), r, 0, 1.f);
        dl->AddLine(ImVec2(a.x + r, a.y + um), ImVec2(b.x - r, a.y + um), IM_COL32(255, 255, 255, 55), 1.f);
        if (led > 0) {
            const ImVec2 lc = Q(x0 + h * 0.56f, (y0 + y1) * 0.5f);
            const float lr = std::max(1.5f, QD(h * 0.19f));
            const ImU32 cl = corLedIc(led);
            if (led >= 2) dl->AddCircleFilled(lc, lr * 2.2f, alfa(cl, 60), 16);
            dl->AddCircleFilled(lc, lr * 1.25f, IM_COL32(0x08, 0x08, 0x09, 255), 16);
            dl->AddCircleFilled(lc, lr, cl, 16);
            if (led >= 2) dl->AddCircleFilled(ImVec2(lc.x - lr * 0.3f, lc.y - lr * 0.3f), lr * 0.4f, IM_COL32(255, 255, 255, 150), 10);
        }
    } else if (estilo == 1) {
        const float r = QD(6.f), g = QD(3.5f);
        dl->AddRectFilled(ImVec2(a.x - g, a.y - g), ImVec2(b.x + g, b.y + g * 1.2f), IM_COL32(0x1a, 0x1b, 0x1e, 255), r + g);
        dl->AddRect(ImVec2(a.x - g, a.y - g), ImVec2(b.x + g, b.y + g * 1.2f), IM_COL32(0x58, 0x5b, 0x60, 190), r + g, 0, 1.f);
        const ImU32 c0 = ativo ? IM_COL32(0x04, 0x04, 0x05, 255) : sobre ? IM_COL32(0x30, 0x32, 0x37, 255) : IM_COL32(0x22, 0x24, 0x28, 255);
        const ImU32 c1 = ativo ? IM_COL32(0x02, 0x02, 0x03, 255) : IM_COL32(0x04, 0x05, 0x06, 255);
        retGrad(dl, a, b, r, c0, c1);
        dl->AddRect(a, b, IM_COL32(0x70, 0x74, 0x7a, 210), r, 0, 1.f);
        dl->AddLine(ImVec2(a.x + r, a.y + um), ImVec2(b.x - r, a.y + um), IM_COL32(255, 255, 255, 70), 1.f);
        if (led > 0) {      // LED fininho embaixo do texto (banda / funcao em uso)
            const ImU32 cl = corLedIc(led);
            dl->AddRectFilled(Q(x0 + 10, y1 - 6), Q(x1 - 10, y1 - 3.5f), led >= 2 ? cl : IM_COL32(0x10, 0x11, 0x12, 255), QD(1.5f));
        }
    } else {
        const float r = QD(5.f);
        const ImU32 c0 = ativo ? IM_COL32(0x22, 0x24, 0x27, 255) : sobre ? IM_COL32(0x74, 0x78, 0x7e, 255) : IM_COL32(0x60, 0x63, 0x68, 255);
        const ImU32 c1 = ativo ? IM_COL32(0x16, 0x17, 0x19, 255) : IM_COL32(0x2a, 0x2c, 0x2f, 255);
        retGrad(dl, a, b, r, c0, c1);
        dl->AddRect(a, b, IM_COL32(0xa4, 0xa8, 0xae, 220), r, 0, 1.f);
    }
    if (txt && *txt) {
        const bool tela = estilo == 2;
        const float cap = tela ? 15.5f : estilo == 1 ? 13.5f : 11.5f;
        ImU32 ct = corTxt ? (ImU32)corTxt : IM_COL32(0xff, 0xff, 0xff, 255);
        if (tela && led >= 2 && !corTxt) ct = IM_COL32(0xff, 0xc0, 0x40, 255);
        const float xt = alinTxt <= 0.01f ? x0 + 11.f : alinTxt >= 0.99f ? x1 - 11.f : (x0 + x1) * 0.5f;
        tx(tela ? fL() : fP(), cap, xt, (y0 + y1) * 0.5f, ct, txt, alinTxt, tela ? 1.f : 0.92f, (x1 - x0) - 12.f);
    }
    if (rot && *rot) tx(fP(), 12.5f, (x0 + x1) * 0.5f, yRot, IM_COL32(0xf4, 0xf4, 0xf4, 255), rot, 0.5f, 0.94f);
    if (sobre && dica && *dica && !ativo) ImGui::SetTooltip("%s", dica);
    return res;
}

// Botao giratorio do MB1: anel prata serrilhado (o de fora) e o miolo preto.
// duplo: o miolo (tIn) e o anel (tOut) sao dois controles; arrastar para
// cima/direita aumenta, a roda tambem. pontos: os pontinhos brancos em volta.
bool Ui::giroIc(const char* id, float cx, float cy, float r, float tIn, float tOut, float& dIn, float& dOut,
                const char* dica, bool duplo, bool pontos)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    dIn = dOut = 0;
    const ImVec2 c = Q(cx, cy);
    const float R = QD(r);
    const float TWO_PI = (float)(2 * kPi);
    const ImVec2 cc(c.x + R * 0.03f, c.y + R * 0.08f);   // o miolo (a camera esta um pouco acima)
    const float Rc = R * 0.71f;
    if (pontos)
        for (int i = 0; i < 11; ++i) {
            const ImVec2 d = dirAng(rad(125.f + 29.f * i));
            dl->AddCircleFilled(ImVec2(c.x + d.x * R * 1.22f, c.y - R * 0.04f + d.y * R * 1.18f), std::max(1.f, QD(2.7f)), IM_COL32(0xee, 0xf0, 0xf2, 235), 10);
        }
    ImGui::SetCursorScreenPos(ImVec2(c.x - R * 1.1f, c.y - R * 1.1f));
    ImGui::PushID(id);
    ImGui::InvisibleButton("##gi", ImVec2(R * 2.2f, R * 2.2f));
    const ImGuiID iid = ImGui::GetItemID();
    const bool sobre = ImGui::IsItemHovered(), ativo = ImGui::IsItemActive();
    ImGui::PopID();
    const float dxm = io.MousePos.x - cc.x, dym = io.MousePos.y - cc.y;
    const bool noMiolo = !duplo || std::sqrt(dxm * dxm + dym * dym) <= Rc * 1.05f;
    if (ImGui::IsItemActivated()) parteGiro()[iid] = noMiolo ? 0 : 1;
    if (ativo) {
        const float dv = (-io.MouseDelta.y + io.MouseDelta.x) / M.d(150.f);
        if (parteGiro()[iid] == 0) dIn = dv; else dOut = dv;
    }
    if (sobre && io.MouseWheel != 0) {
        const float dv = io.MouseWheel * 0.02f;
        if (noMiolo) dIn = dv; else dOut = dv;
    }
    const float aFora = angKnob(duplo ? tOut : tIn), aIn = angKnob(tIn);
    // sombra
    dl->AddCircleFilled(ImVec2(c.x + R * 0.05f, c.y + R * 0.14f), R * 1.03f, IM_COL32(0, 0, 0, 150), 48);
    // anel de fora: aro cromado, serrilhado prata e o chanfro de dentro
    auto luzAro = [](float a) { return 0.55f + 0.42f * sino(a, rad(-128), rad(48)) + 0.25f * sino(a, rad(45), rad(35)) - 0.14f * sino(a, rad(100), rad(30)); };
    anelLuz(dl, c, R * 0.88f, R, 0.f, TWO_PI, 96, [&](float a, float rr) { return cinzaN(std::min(1.f, luzAro(a) * 1.12f) * (rr > R * 0.975f ? 0.75f : 1.f)); });
    anelLuz(dl, c, R * 0.72f, R * 0.88f, 0.f, TWO_PI, 96, [&](float a, float) { return cinzaN(luzAro(a) * 0.62f); });
    {
        const int nd = 64;
        for (int k = 0; k < nd; ++k) {
            const float an = aFora + k * TWO_PI / nd;
            const ImVec2 d = dirAng(an);
            const float l = luzAro(an);
            dl->AddLine(ImVec2(c.x + d.x * R * 0.745f, c.y + d.y * R * 0.745f), ImVec2(c.x + d.x * R * 0.87f, c.y + d.y * R * 0.87f),
                        cinzaN(l * 0.30f, 200), std::max(1.f, R * 0.03f));
        }
    }
    anelLuz(dl, c, R * 0.68f, R * 0.74f, 0.f, TWO_PI, 96, [&](float a, float) { return cinzaN(0.30f + 0.55f * sino(a, rad(60), rad(50)) + 0.25f * sino(a, rad(-130), rad(40))); });
    circuloSuave(dl, c, R, 96, 1.f, [&](float a) { return cinzaN(luzAro(a) * 0.5f, 220); });
    if (duplo) {     // marquinha branca do anel
        const ImVec2 d = dirAng(aFora);
        dl->AddLine(ImVec2(c.x + d.x * R * 0.76f, c.y + d.y * R * 0.76f), ImVec2(c.x + d.x * R * 0.92f, c.y + d.y * R * 0.92f),
                    (ativo && parteGiro()[iid] == 1) ? P_LARANJA : IM_COL32(0xf8, 0xf8, 0xf8, 255), std::max(1.2f, QD(3.f)));
    }
    // o miolo preto brilhante
    dl->AddCircleFilled(ImVec2(cc.x, cc.y + R * 0.03f), Rc * 1.02f, IM_COL32(0, 0, 0, 200), 48);
    anelLuz(dl, cc, Rc * 0.84f, Rc, 0.f, TWO_PI, 72, [&](float a, float rr) {
        const float l = 0.05f + 0.30f * sino(a, rad(-120), rad(38)) + 0.10f * sino(a, rad(55), rad(30));
        return cinzaN(rr > Rc * 0.95f ? l * 0.5f : l);
    });
    dl->AddCircleFilled(cc, Rc * 0.84f, cinzaN(0.035f), 64);
    dl->AddCircleFilled(ImVec2(cc.x - Rc * 0.18f, cc.y - Rc * 0.22f), Rc * 0.50f, IM_COL32(255, 255, 255, 10), 40);
    {   // risco branco do miolo
        const ImVec2 d = dirAng(aIn);
        dl->AddLine(ImVec2(cc.x + d.x * Rc * 0.42f, cc.y + d.y * Rc * 0.42f), ImVec2(cc.x + d.x * Rc * 0.92f, cc.y + d.y * Rc * 0.92f),
                    (ativo && parteGiro()[iid] == 0) ? P_LARANJA : IM_COL32(0xf8, 0xf8, 0xf8, 255), std::max(1.5f, QD(3.6f)));
    }
    if (ativo) dl->AddCircle(c, R * 1.04f, alfa(P_LARANJA, 130), 48, std::max(1.f, QD(2.f)));
    if ((sobre || ativo) && dica) ImGui::SetTooltip("%s", dica);
    return dIn != 0 || dOut != 0;
}

// MULTI: botao preto serrilhado, sem fim (cada 15 graus um passo). clique = sem girar
int Ui::encoderIc(const char* id, float cx, float cy, float r, float& ang, const char* dica, bool* clique)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 c = Q(cx, cy);
    const float R = QD(r);
    ImGui::SetCursorScreenPos(ImVec2(c.x - R, c.y - R));
    ImGui::PushID(id);
    ImGui::InvisibleButton("##en", ImVec2(R * 2, R * 2));
    const ImGuiID iid = ImGui::GetItemID();
    const bool sobre = ImGui::IsItemHovered(), ativo = ImGui::IsItemActive();
    ImGui::PopID();
    int passos = 0;
    static std::unordered_map<ImGuiID, float> resto, andou;
    const float aM = std::atan2(io.MousePos.y - c.y, io.MousePos.x - c.x);
    if (ImGui::IsItemActivated()) { angArrasto()[iid] = aM; andou[iid] = 0; }
    if (ativo) {
        float d = aM - angArrasto()[iid];
        while (d > kPi) d -= (float)(2 * kPi);
        while (d < -kPi) d += (float)(2 * kPi);
        angArrasto()[iid] = aM;
        ang += d; andou[iid] += std::fabs(d); resto[iid] += d;
        const float passo = (float)(15.0 * kPi / 180.0);
        while (resto[iid] >= passo) { ++passos; resto[iid] -= passo; }
        while (resto[iid] <= -passo) { --passos; resto[iid] += passo; }
    }
    if (ImGui::IsItemDeactivated() && clique && andou[iid] < 0.08f && sobre) *clique = true;
    if (sobre && io.MouseWheel != 0) {
        int n = (int)std::lround(io.MouseWheel);
        if (n == 0) n = io.MouseWheel > 0 ? 1 : -1;
        passos += n; ang += n * 0.26f;
    }
    const float TWO_PI = (float)(2 * kPi);
    dl->AddCircleFilled(ImVec2(c.x + R * 0.05f, c.y + R * 0.12f), R * 1.03f, IM_COL32(0, 0, 0, 170), 48);
    auto luz = [](float a) { return 0.30f + 0.48f * sino(a, rad(-125), rad(42)) + 0.16f * sino(a, rad(50), rad(30)); };
    anelLuz(dl, c, R * 0.80f, R, 0.f, TWO_PI, 96, [&](float a, float rr) { return cinzaN(luz(a) * (rr > R * 0.97f ? 0.6f : 1.f)); });
    for (int k = 0; k < 56; ++k) {
        const float an = ang + k * TWO_PI / 56;
        const ImVec2 d = dirAng(an);
        dl->AddLine(ImVec2(c.x + d.x * R * 0.82f, c.y + d.y * R * 0.82f), ImVec2(c.x + d.x * R * 0.985f, c.y + d.y * R * 0.985f),
                    cinzaN(luz(an) * 0.30f, 220), std::max(1.f, R * 0.04f));
    }
    circuloSuave(dl, c, R, 96, 1.f, [&](float a) { return cinzaN(luz(a) * 0.5f, 220); });
    const ImVec2 ct(c.x + R * 0.02f, c.y + R * 0.05f);
    anelLuz(dl, ct, R * 0.70f, R * 0.80f, 0.f, TWO_PI, 72, [&](float a, float) { return cinzaN(0.05f + 0.22f * sino(a, rad(-120), rad(40))); });
    dl->AddCircleFilled(ct, R * 0.70f, cinzaN(0.03f), 64);
    anelLuz(dl, ct, R * 0.30f, R * 0.70f, 0.f, TWO_PI, 72, [&](float a, float rr) { return cinzaN((0.02f + 0.06f * sino(a, rad(60), rad(45))) * (rr / (R * 0.70f))); });
    if (ativo) dl->AddCircle(c, R * 1.04f, alfa(P_LARANJA, 150), 48, std::max(1.f, QD(2.f)));
    if ((sobre || ativo) && dica) ImGui::SetTooltip("%s", dica);
    return passos;
}

// XFC / CHANGE: botao redondo de metal escovado num aro preto
int Ui::botaoRedondo(const char* id, float cx, float cy, float r, const char* dica, bool* segura)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 c = Q(cx, cy);
    const float R = QD(r);
    ImGui::SetCursorScreenPos(ImVec2(c.x - R, c.y - R));
    ImGui::PushID(id);
    ImGui::InvisibleButton("##br", ImVec2(R * 2, R * 2));
    const ImGuiID iid = ImGui::GetItemID();
    const bool sobre = ImGui::IsItemHovered(), ativo = ImGui::IsItemActive();
    int res = 0;
    if (ImGui::IsItemActivated()) longoFeito()[iid] = 0;
    if (ativo && ImGui::GetIO().MouseDownDuration[0] > 0.6f && longoFeito()[iid] == 0) { longoFeito()[iid] = 1; res = 2; }
    if (ImGui::IsItemDeactivated() && sobre && longoFeito()[iid] == 0) res = 1;
    ImGui::PopID();
    if (segura) *segura = ativo;
    const float TWO_PI = (float)(2 * kPi);
    dl->AddCircleFilled(c, R, cinzaN(0.02f), 64);
    anelLuz(dl, c, R * 0.90f, R, 0.f, TWO_PI, 72, [](float a, float) { return cinzaN(0.05f + 0.18f * sino(a, rad(-120), rad(40))); });
    const float rm = R * 0.80f;
    anelLuz(dl, c, rm, rm * 1.07f, 0.f, TWO_PI, 72, [](float a, float) { return cinzaN(0.35f + 0.50f * sino(a, rad(-130), rad(45)) + 0.25f * sino(a, rad(50), rad(35))); });
    discoCone(dl, c, rm, [](float a) { return 0.22f + 0.30f * sino(a, rad(-135), rad(30)) + 0.30f * sino(a, rad(45), rad(30))
                                              + 0.10f * sino(a, rad(135), rad(25)) + 0.10f * sino(a, rad(-45), rad(25)); }, ativo ? 0.6f : sobre ? 1.15f : 1.f);
    for (int i = 1; i <= 6; ++i) dl->AddCircle(c, rm * i / 7.f, IM_COL32(0, 0, 0, (i & 1) ? 22 : 10), 0, 1.f);
    circuloSuave(dl, c, rm * 1.07f, 72, 1.f, [](float a) { return cinzaN(0.25f + 0.3f * sino(a, rad(-130), rad(45)), 220); });
    if (sobre && dica && !ativo) ImGui::SetTooltip("%s", dica);
    return res;
}

// Dial grande do MB1: o serrilhado preto, o aro cromado, a face conica
// escovada e a covinha do dedo (gira com o dial). 100 passos por volta;
// girando rapido o passo multiplica. Roda do mouse: 5 passos por dente.
int Ui::dialIc(const char* id, float cx, float cy, float rFace, float& ang, bool travado, const char* dica)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 c = Q(cx, cy);
    const float F = QD(rFace), RO = F * 1.14f;
    ImGui::SetCursorScreenPos(ImVec2(c.x - RO, c.y - RO));
    ImGui::PushID(id);
    ImGui::InvisibleButton("##dl", ImVec2(2 * RO, 2 * RO));
    const ImGuiID iid = ImGui::GetItemID();
    const bool sobreB = ImGui::IsItemHovered(), ativo = ImGui::IsItemActive();
    ImGui::PopID();
    const float dist = std::sqrt((io.MousePos.x - c.x) * (io.MousePos.x - c.x) + (io.MousePos.y - c.y) * (io.MousePos.y - c.y));
    const bool dentro = dist <= RO;
    int passos = 0;
    static std::unordered_map<ImGuiID, float> resto;
    const float aM = std::atan2(io.MousePos.y - c.y, io.MousePos.x - c.x);
    if (ImGui::IsItemActivated()) {
        angArrasto()[iid] = aM;
        if (!dentro) ImGui::ClearActiveID();      // canto do quadrado: e das teclas em volta
    }
    if (ativo && dentro && !travado) {
        float d = aM - angArrasto()[iid];
        while (d > kPi) d -= (float)(2 * kPi);
        while (d < -kPi) d += (float)(2 * kPi);
        angArrasto()[iid] = aM;
        ang += d;
        const float vel = std::fabs(d) / std::max(0.001f, io.DeltaTime) / (float)(2 * kPi);
        const float mult = vel > 2.0f ? 10.f : vel > 0.8f ? 4.f : 1.f;
        resto[iid] += d * mult;
        const float passo = (float)(2 * kPi / 100.0);
        while (resto[iid] >= passo) { ++passos; resto[iid] -= passo; }
        while (resto[iid] <= -passo) { --passos; resto[iid] += passo; }
    } else if (ativo) angArrasto()[iid] = aM;
    if (io.MouseWheel != 0 && !ativo && sobreB && dentro && !travado) {
        int n = (int)std::lround(io.MouseWheel);
        if (n == 0) n = io.MouseWheel > 0 ? 1 : -1;
        passos += n * 5; ang += n * 5 * (float)(2 * kPi / 100.0);
    }
    const float TWO_PI = (float)(2 * kPi);
    // sombra no painel
    dl->AddCircleFilled(ImVec2(c.x + F * 0.02f, c.y + F * 0.05f), RO * 1.01f, IM_COL32(0, 0, 0, 90), 128);
    // o serrilhado (a saia do dial, com a luz de cima / esquerda)
    const ImVec2 cs(c.x, c.y - F * 0.055f);
    auto luzS = [](float a) { return 0.32f + 0.34f * sino(a, rad(-125), rad(50)) + 0.14f * sino(a, rad(-30), rad(35)) + 0.12f * sino(a, rad(170), rad(30)); };
    anelLuz(dl, cs, F * 1.05f, RO, 0.f, TWO_PI, 180, [&](float a, float rr) { return cinzaN(luzS(a) * (rr > RO * 0.985f ? 0.55f : 1.f)); });
    {
        const int nd = 170;
        for (int k = 0; k < nd; ++k) {
            const float an = ang + k * TWO_PI / nd;
            const ImVec2 d = dirAng(an);
            const float l = luzS(an);
            dl->AddLine(ImVec2(cs.x + d.x * F * 1.075f, cs.y + d.y * F * 1.075f), ImVec2(cs.x + d.x * RO * 0.985f, cs.y + d.y * RO * 0.985f),
                        cinzaN(l * 0.30f, 230), std::max(1.f, F * 0.022f));
            const ImVec2 d2 = dirAng(an + TWO_PI / nd * 0.5f);
            dl->AddLine(ImVec2(cs.x + d2.x * F * 1.08f, cs.y + d2.y * F * 1.08f), ImVec2(cs.x + d2.x * RO * 0.97f, cs.y + d2.y * RO * 0.97f),
                        cinzaN(std::min(1.f, l * 1.5f), 160), std::max(1.f, F * 0.012f));
        }
    }
    circuloSuave(dl, cs, RO, 180, 1.f, [&](float a) { return cinzaN(luzS(a) * 0.6f, 230); });
    // o aro cromado em volta da face
    auto luzC = [](float a) { return 0.78f + 0.22f * sino(a, rad(150), rad(55)) + 0.20f * sino(a, rad(-55), rad(40)) - 0.30f * sino(a, rad(-10), rad(18)) - 0.15f * sino(a, rad(-140), rad(30)); };
    anelLuz(dl, c, F * 0.985f, F * 1.06f, 0.f, TWO_PI, 180, [&](float a, float rr) {
        const float l = std::min(1.f, luzC(a) * 1.15f);
        return cinzaN(rr > F * 1.05f ? l * 0.75f : rr < F * 0.995f ? l * 0.7f : l);
    });
    circuloSuave(dl, c, F * 1.06f, 180, 1.f, [&](float a) { return cinzaN(luzC(a) * 0.45f, 220); });
    // a face conica escovada (a luz e fixa, nao gira) com os sulcos finos
    discoCone(dl, c, F, luzCone, 1.f);
    for (int i = 1; i <= 26; ++i) dl->AddCircle(c, F * i / 27.f, IM_COL32(0, 0, 0, (i & 1) ? 18 : 8), 0, 1.f);
    circuloSuave(dl, c, F, 180, 1.f, [](float a) { return cinzaN(luzCone(a) * 0.8f, 230); });
    // a covinha do dedo (gira com o dial)
    {
        const ImVec2 d = dirAng(ang - (float)(kPi * 0.5));
        const ImVec2 f(c.x + d.x * F * 0.56f, c.y + d.y * F * 0.56f);
        const float rf = F * 0.33f;
        dl->AddCircleFilled(ImVec2(f.x, f.y + rf * 0.05f), rf * 1.04f, IM_COL32(0, 0, 0, 200), 64);
        anelLuz(dl, f, rf * 0.78f, rf, 0.f, TWO_PI, 96, [](float a, float) { return cinzaN(0.03f + 0.10f * sino(a, rad(90), rad(50))); });
        anelLuz(dl, f, rf * 0.60f, rf * 0.78f, 0.f, TWO_PI, 96, [](float a, float rr) {
            const float l = 0.30f + 0.62f * sino(a, rad(-135), rad(40)) + 0.40f * sino(a, rad(50), rad(35));
            (void)rr; return cinzaN(l);
        });
        anelLuz(dl, f, 0.f, rf * 0.60f, 0.f, TWO_PI, 96, [&](float a, float rr) {
            const float t = rr / (rf * 0.60f);
            return cinzaN((0.03f + 0.10f * sino(a, rad(70), rad(40))) * (0.5f + 0.5f * t));
        });
        dl->AddCircleFilled(ImVec2(f.x - rf * 0.18f, f.y - rf * 0.20f), rf * 0.10f, IM_COL32(255, 255, 255, 70), 16);
        circuloSuave(dl, f, rf * 0.78f, 96, 1.f, [](float a) { return cinzaN(0.2f + 0.5f * sino(a, rad(-135), rad(40)), 200); });
    }
    if (ativo && dentro) dl->AddCircle(c, F * 1.06f, alfa(P_LARANJA, 120), 0, std::max(1.f, QD(2.5f)));
    if (travado) tx(fP(), 16.f, cx, cy + rFace * 0.45f, alfa(P_VERMELHO, 230), "LOCK", 0.5f);
    if (sobreB && dentro && !ativo && dica) ImGui::SetTooltip("%s", dica);
    return travado ? 0 : passos;
}

// ===========================================================================
//  MB1SDR: a caixa do radio (tampa, moldura prata, painel, vidros)
// ===========================================================================
void Ui::icChassi()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImDrawListFlags fl0 = dl->Flags;
    if (g_fundoChave) dl->Flags &= ~ImDrawListFlags_AntiAliasedFill;   // sem franja rosa na cor-chave
    // ---- a tampa de cima, vista em perspectiva (com as grades de ventilacao) ----
    quadCores(dl, Q(222, 104), Q(2187, 104), Q(2334, 160), Q(81, 160),
              IM_COL32(0x36, 0x37, 0x39, 255), IM_COL32(0x33, 0x34, 0x36, 255), IM_COL32(0x4a, 0x4b, 0x4e, 255), IM_COL32(0x4d, 0x4e, 0x51, 255));
    // ---- a frente: moldura prata e o painel preto texturizado ----
    {
        const ImVec2 a = Q(75, 158), b = Q(2342, 943);
        dl->AddRectFilled(a, ImVec2(b.x, (a.y + b.y) * 0.5f), IM_COL32(0x9a, 0x9c, 0xa0, 255), QD(14), ImDrawFlags_RoundCornersTop);
        dl->AddRectFilled(ImVec2(a.x, (a.y + b.y) * 0.5f - 2), b, IM_COL32(0x84, 0x86, 0x8a, 255), QD(46), ImDrawFlags_RoundCornersBottom);
    }
    dl->Flags = fl0;
    // brilho do aro prata (claro em cima, a esquerda) e a beirada de baixo
    dl->AddRectFilledMultiColor(Q(80, 160), Q(2338, 168), IM_COL32(0xd8, 0xda, 0xde, 255), IM_COL32(0xc0, 0xc2, 0xc6, 255),
                                IM_COL32(0x8c, 0x8e, 0x92, 255), IM_COL32(0x8c, 0x8e, 0x92, 255));
    dl->AddRectFilled(Q(90, 176), Q(2330, 931), IM_COL32(0x2c, 0x2e, 0x30, 255), QD(38), ImDrawFlags_RoundCornersBottom);
    dl->AddRectFilledMultiColor(Q(90, 176), Q(2330, 560), IM_COL32(0x35, 0x37, 0x39, 255), IM_COL32(0x32, 0x34, 0x36, 255),
                                IM_COL32(0x2c, 0x2e, 0x30, 0), IM_COL32(0x2c, 0x2e, 0x30, 0));
    dl->AddRect(Q(90, 176), Q(2330, 931), IM_COL32(0x16, 0x17, 0x18, 255), QD(38), ImDrawFlags_RoundCornersBottom, std::max(1.f, QD(2.f)));
    // textura do painel (pontinhos claros e escuros, sempre nos mesmos lugares)
    {
        const float x0 = 92, x1 = 2328, y0 = 180, y1 = 928;
        const float t = std::max(1.f, QD(1.6f));
        for (uint32_t i = 0; i < 5200; ++i) {
            const uint32_t h = hashIc(i * 2654435761u + 17u);
            const float x = x0 + (h & 0xffff) / 65535.f * (x1 - x0), y = y0 + (h >> 16) / 65535.f * (y1 - y0);
            if (x > 484 && x < 1596 && y < 816) continue;           // a tela
            if (x > 1594 && x < 2084 && y < 838) continue;          // o vidro preto da direita
            const ImVec2 q = Q(x, y);
            dl->AddRectFilled(q, ImVec2(q.x + t, q.y + t), (h & 0x300) ? IM_COL32(0, 0, 0, 70) : IM_COL32(255, 255, 255, 34));
        }
    }
    // a barra prata em cima da tela e do vidro
    dl->AddRectFilledMultiColor(Q(484, 166), Q(2084, 178), IM_COL32(0xb4, 0xb6, 0xba, 255), IM_COL32(0xa8, 0xaa, 0xae, 255),
                                IM_COL32(0x76, 0x78, 0x7c, 255), IM_COL32(0x76, 0x78, 0x7c, 255));
    // grades de ventilacao da tampa
    for (int ld = 0; ld < 2; ++ld) {
        const float xa = ld == 0 ? 330.f : 1650.f, xb = ld == 0 ? 760.f : 2120.f;
        for (int row = 0; row < 4; ++row) {
            const float y = 127.f + row * 7.5f, hh = 3.0f + row * 0.4f, pas = 16.f;
            for (float x = xa + ((row & 1) ? pas * 0.5f : 0.f); x < xb; x += pas)
                dl->AddRectFilled(Q(x, y), Q(x + 8.f, y + hh), IM_COL32(0x16, 0x17, 0x19, 255));
        }
    }
    for (float x = 260; x < 2200; x += 22.f) dl->AddRectFilled(Q(x, 152.5f), Q(x + 12.f, 155.5f), IM_COL32(0x22, 0x24, 0x26, 200));
    dl->AddLine(Q(222, 104.6f), Q(2187, 104.6f), IM_COL32(0x9a, 0x9c, 0xa0, 200), 1.f);
    // ---- a moldura da tela (preta) com o vidro ----
    dl->AddRectFilled(Q(484, 178), Q(1596, 815), IM_COL32(0x0d, 0x0e, 0x10, 255), QD(6));
    dl->AddRectFilledMultiColor(Q(484, 178), Q(492, 815), IM_COL32(0x9a, 0x9c, 0xa0, 255), IM_COL32(0x3a, 0x3c, 0x40, 255),
                                IM_COL32(0x3a, 0x3c, 0x40, 255), IM_COL32(0x9a, 0x9c, 0xa0, 255));
    dl->AddRectFilledMultiColor(Q(1588, 178), Q(1596, 815), IM_COL32(0x3a, 0x3c, 0x40, 255), IM_COL32(0x8a, 0x8c, 0x90, 255),
                                IM_COL32(0x8a, 0x8c, 0x90, 255), IM_COL32(0x3a, 0x3c, 0x40, 255));
    tx(fP(), 11.5f, 548, 192, IM_COL32(0xf0, 0xf0, 0xf2, 255), "HF/VHF/UHF  SDR RECEIVER", 0.f, 0.95f);
    tx(fL(), 16.f, 1522, 192, IM_COL32(0xd8, 0xda, 0xde, 255), "MB1SDR", 1.f, 1.12f);
    dl->AddRectFilled(Q(512, 206), Q(1560, 803), IM_COL32(0x02, 0x02, 0x03, 255), QD(16));
    dl->AddRect(Q(512, 206), Q(1560, 803), IM_COL32(0x6a, 0x6c, 0x70, 255), QD(16), 0, std::max(1.f, QD(2.f)));
    dl->AddLine(Q(526, 801), Q(1546, 801), IM_COL32(0xa8, 0xaa, 0xae, 200), std::max(1.f, QD(3.f)));
    // ---- o vidro preto da direita (MULTI, bandas, dial) ----
    dl->AddRectFilled(Q(1594, 180), Q(2084, 838), IM_COL32(0x2a, 0x2b, 0x2e, 255), QD(16));
    retGrad(dl, Q(1598, 183), Q(2080, 834), QD(14), IM_COL32(0x16, 0x17, 0x1a, 255), IM_COL32(0x07, 0x07, 0x08, 255));
    dl->AddRect(Q(1598, 183), Q(2080, 834), IM_COL32(0x4e, 0x50, 0x55, 230), QD(14), 0, 1.f);
    dl->AddLine(Q(1612, 186), Q(2066, 186), IM_COL32(255, 255, 255, 40), 1.f);
    // ---- a tampa e a "barra de titulo": versao, minimizar / maximizar / fechar, arrastar ----
    tx(fP(), 11.f, 262, 121, IM_COL32(0xd4, 0xd6, 0xda, 255), "MB1SDR  v" RXSDR_VERSAO, 0.f, 0.95f);
    {
        HWND hw = (HWND)ImGui::GetMainViewport()->PlatformHandleRaw;
        if (!hw) hw = GetActiveWindow();
        const bool max = hw && IsZoomed(hw);
        const float bw = 40.f, bh = 24.f, gap = 6.f, by = 113.f, bx0 = 2172.f - 3 * bw - 2 * gap;
        for (int i = 0; i < 3; ++i) {
            const float x = bx0 + i * (bw + gap);
            const ImVec2 a = Q(x, by), b = Q(x + bw, by + bh);
            ImGui::SetCursorScreenPos(a);
            ImGui::PushID(9100 + i);
            const bool clicou = ImGui::InvisibleButton("##jan", ImVec2(std::max(1.f, b.x - a.x), std::max(1.f, b.y - a.y)));
            const bool sob = ImGui::IsItemHovered(), at = ImGui::IsItemActive();
            ImGui::PopID();
            ImU32 fundo = at ? IM_COL32(0x10, 0x12, 0x14, 255) : sob ? IM_COL32(0x60, 0x64, 0x6a, 255) : IM_COL32(0x34, 0x36, 0x3a, 255);
            if (i == 2 && (sob || at)) fundo = at ? IM_COL32(0x90, 0x1c, 0x14, 255) : IM_COL32(0xc4, 0x2b, 0x1c, 255);
            dl->AddRectFilled(a, b, fundo, QD(4));
            dl->AddRect(a, b, IM_COL32(0x80, 0x84, 0x8a, 200), QD(4), 0, 1.f);
            const ImU32 cs = IM_COL32(0xee, 0xf0, 0xf4, 255);
            const float cx = x + bw * 0.5f, cy = by + bh * 0.5f, t = std::max(1.f, QD(1.8f));
            if (i == 0) dl->AddLine(Q(cx - 7, cy + 4), Q(cx + 7, cy + 4), cs, t);
            else if (i == 1) {
                if (max) {
                    dl->AddRect(Q(cx - 6, cy - 3), Q(cx + 4, cy + 7), cs, 0.f, 0, t);
                    dl->AddLine(Q(cx - 2, cy - 7), Q(cx + 8, cy - 7), cs, t);
                    dl->AddLine(Q(cx + 8, cy - 7), Q(cx + 8, cy + 3), cs, t);
                } else dl->AddRect(Q(cx - 7, cy - 6), Q(cx + 7, cy + 6), cs, 0.f, 0, t);
            } else {
                dl->AddLine(Q(cx - 6, cy - 6), Q(cx + 6, cy + 6), cs, t);
                dl->AddLine(Q(cx + 6, cy - 6), Q(cx - 6, cy + 6), cs, t);
            }
            if (sob && !at) ImGui::SetTooltip("%s", i == 0 ? "Minimizar" : i == 1 ? (max ? "Restaurar" : "Maximizar") : "Fechar");
            if (clicou && hw) {
                if (i == 0) ShowWindow(hw, SW_MINIMIZE);
                else if (i == 1) ShowWindow(hw, max ? SW_RESTORE : SW_MAXIMIZE);
                else PostMessageW(hw, WM_CLOSE, 0, 0);
            }
        }
        // arrastar pela tampa (fora dos botoes); duplo clique maximiza / volta
        const ImVec2 t0 = Q(75, 104), t1 = Q(2342, 160);
        const ImVec2 mp = ImGui::GetIO().MousePos;
        const bool naTampa = mp.x >= t0.x && mp.x < t1.x && mp.y >= t0.y && mp.y < t1.y && !ImGui::IsAnyItemHovered() &&
                             ImGui::IsWindowHovered(ImGuiHoveredFlags_None);
        if (naTampa && hw) {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) ShowWindow(hw, max ? SW_RESTORE : SW_MAXIMIZE);
            else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                ReleaseCapture();
                SendMessageW(hw, WM_NCLBUTTONDOWN, HTCAPTION, 0);
                ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, false);
            }
        }
    }
    // botao direito no painel = MENU (FUNC)
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##funcFt");
}

// ===========================================================================
//  MB1SDR: a coluna da esquerda (POWER, jaques, AF-RF/SQL, NB/NR, USB, MIC,
//  e a tecla CACHOEIRA no lugar do cartao SD)
// ===========================================================================
void Ui::icEsquerda()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool lig = r_->ligado();
    const ImU32 BR = IM_COL32(0xf4, 0xf4, 0xf4, 255);
    // o nome no lugar da marca: placa escura com "MB1SDR" prateado
    {
        tx(fP(), 26.f, 207, 234.5f, IM_COL32(0x14, 0x0c, 0x00, 230), "MB1SDR", 0.5f, 0.80f, 156.f);   // sombra
        tx(fP(), 26.f, 205, 232.5f, IM_COL32(0xe8, 0xb8, 0x4c, 255), "MB1SDR", 0.5f, 0.80f, 156.f);   // dourado
    }
    // POWER (LED ciano aceso com o radio ligado)
    if (teclaIc("iPwr", "", 310, 229, 445, 256, lig ? 2 : 1, "POWER", 208, lig ? "POWER: desligar o rádio (para o dongle)" : "POWER: ligar o rádio (abre o dongle)", 0) == 1)
        ligarDesligar();
    if (teclaIc("iTx", "", 121, 302, 215, 330, sendMudo_ ? 4 : 0, "TRANSMIT", 283, "TRANSMIT: (num receptor) silencia todo o áudio - MUTE geral", 0) == 1)
        sendMudo_ = !sendMudo_;
    if (teclaIc("iTimer", "", 245, 302, 336, 330, sleepAte_ > 0 ? 2 : 1, "TIMER", 283, "TIMER: desligar o rádio depois de um tempo", 0) == 1)
        ImGui::OpenPopup("##timerIc");
    {
        const int k = teclaIc("iTuner", "", 367, 302, 460, 330, autoWf_ ? 2 : 1, "TUNER", 283,
                              "TUNER: ajuste automático do escopo e da cachoeira\nSegurar: guarda a aparência atual como padrão", 0);
        if (k == 1) autoCachoeira();
        if (k == 2) padrao();
    }
    // jaques PHONES e ELEC-KEY
    auto jaque = [&](float cx, float cy, const char* rot, bool fone) {
        tx(fP(), 12.5f, cx, cy - 50, BR, rot, 0.5f, 0.94f);
        const ImVec2 c = Q(cx, cy);
        dl->AddCircleFilled(c, QD(36), IM_COL32(0x0e, 0x0f, 0x10, 255), 48);
        anelLuz(dl, c, QD(19), QD(28), 0.f, (float)(2 * kPi), 64, [](float a, float rr) {
            (void)rr; return cinzaN(0.35f + 0.45f * sino(a, rad(-130), rad(50)) + 0.25f * sino(a, rad(50), rad(40)));
        });
        circuloSuave(dl, c, QD(28), 64, 1.f, [](float a) { return cinzaN(0.2f + 0.3f * sino(a, rad(-130), rad(50)), 220); });
        dl->AddCircleFilled(c, QD(19), IM_COL32(0x05, 0x05, 0x06, 255), 40);
        if (fone) {
            dl->AddCircleFilled(c, QD(14), IM_COL32(0x2a, 0x2a, 0x2c, 255), 32);
            dl->AddCircleFilled(c, QD(8), IM_COL32(0x08, 0x08, 0x08, 255), 24);
            dl->AddRectFilled(Q(cx - 3, cy - 14), Q(cx + 3, cy - 6), IM_COL32(0x80, 0x78, 0x60, 255));
        } else {
            anelLuz(dl, c, QD(13), QD(17), 0.f, (float)(2 * kPi), 48, [](float a, float) { return cinzaN(0.25f + 0.55f * sino(a, rad(60), rad(45))); });
            dl->AddCircleFilled(c, QD(13), IM_COL32(0x02, 0x02, 0x02, 255), 32);
        }
    };
    jaque(163, 412, "PHONES", true);
    jaque(163, 530, "ELEC-KEY", false);
    {   // clicar nos jaques: PHONES = saida de som, ELEC-KEY = decodificador de CW
        ImGui::SetCursorScreenPos(Q(127, 376));
        if (ImGui::InvisibleButton("##phones", ImVec2(QD(72), QD(72)))) { saidas_ = SomNativo::listarSaidas(); ImGui::OpenPopup("##saidaIc"); }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("PHONES: escolher a saída de som (placa / fone)");
        ImGui::SetCursorScreenPos(Q(127, 494));
        if (ImGui::InvisibleButton("##elkey", ImVec2(QD(72), QD(72)))) {
            decAberta_ = true;
            if (r_->decoders().tipo() != Decoders::CW) escolherDecoder(Decoders::CW, true);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("ELEC-KEY: abre o decodificador de CW (telegrafia em texto)");
    }
    // os dois AF-RF/SQL (MAIN em cima, SUB embaixo)
    auto afRf = [&](int rx, float yRot, float cy) {
        Receptor& R = rx_[rx];
        const std::string md = rx == ativo_ ? r_->modo() : R.modo;
        const bool sql = rfSql_ == 2 || (rfSql_ == 0 && grupoModo(md) >= 3);
        rotuloDuploIc(331, yRot, 12.5f, BR, rx == 0 ? "MAIN:AF" : "SUB:AF", "RF/SQL");
        {   // MUTE (clicavel): fica vermelho com o receptor mudo
            const ImVec2 a = Q(427, yRot - 8), b = Q(477, yRot + 8);
            dl->AddRectFilled(a, b, R.mute ? IM_COL32(0xe0, 0x30, 0x28, 255) : IM_COL32(0xd8, 0xda, 0xde, 255), QD(4));
            tx(fP(), 9.f, 452, yRot, R.mute ? BR : IM_COL32(0x20, 0x22, 0x26, 255), "MUTE", 0.5f, 0.92f);
            ImGui::SetCursorScreenPos(a);
            char id[16]; std::snprintf(id, sizeof id, "##mute%d", rx);
            if (ImGui::InvisibleButton(id, ImVec2(std::max(1.f, b.x - a.x), std::max(1.f, b.y - a.y)))) R.mute = !R.mute;
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("MUTE: silencia o %s", rx == 0 ? "MAIN" : "SUB");
        }
        tx(fP(), 11.5f, 447, yRot + 23, BR, "TX/RX", 0.5f, 0.94f);
        {   // LED TX/RX: verde com sinal (squelch aberto), vermelho se este e o receptor de "TX" no SPLIT
            const bool at = rx == ativo_;
            const bool busy = lig && r_->aoVivo() && ocupadoRx(rx);
            const ImU32 cl = (txVfo_ == rx && txVfo_ != ativo_) ? IM_COL32(0xff, 0x40, 0x30, 255) : busy ? IM_COL32(0xd8, 0xff, 0x3c, 255) : IM_COL32(0x30, 0x32, 0x2c, 255);
            if (cl != IM_COL32(0x30, 0x32, 0x2c, 255)) dl->AddRectFilled(Q(447, yRot + 34), Q(477, yRot + 46), alfa(cl, 70), QD(5));
            dl->AddRectFilled(Q(451, yRot + 37), Q(473, yRot + 43), cl, QD(3));
        }
        char d[300], sq[24];
        if (R.sql <= -90) std::snprintf(sq, sizeof sq, "aberto"); else std::snprintf(sq, sizeof sq, "%+d dB", (int)std::lround(R.sql));
        std::snprintf(d, sizeof d, "AF (miolo): volume do %s (%d%%)\n%s (anel)\n(o anel faz %s - MENU > Áudio > Anel RF/SQL escolhe)",
                      rx == 0 ? "MAIN" : "SUB", (int)std::lround(R.af),
                      sql ? (std::string("SQL: squelch (") + sq + ")").c_str() : (std::string("RF: ganho do dongle (") + std::to_string((int)std::lround(49.6f * R.rf / 100.f)) + " dB)").c_str(),
                      rfSql_ == 0 ? "SQL no FM e RF nos outros" : rfSql_ == 1 ? "RF" : "SQL");
        float di, dO;
        const float tOut = sql ? (R.sql + 90.f) / 120.f : R.rf / 100.f;
        char id[16]; std::snprintf(id, sizeof id, "gAfIc%d", rx);
        if (giroIc(id, 356, cy, 71, R.af / 100.f, tOut, di, dO, d, true, true)) {
            R.af = std::clamp(R.af + di * 100.f, 0.f, 100.f);
            if (dO != 0) {
                if (sql) { R.sql = std::clamp(R.sql + dO * 120.f, -90.f, 30.f); if (rx == ativo_) { sql_ = R.sql; sqlAplicado_ = R.sql; } if (R.sql <= -90.f) sqlAuto_[rx] = false; }
                else R.rf = std::clamp(R.rf + dO * 100.f, 0.f, 100.f);
            }
            if (rx != ativo_ && di != 0) aviso(rx == 1 ? "Este AF é do SUB. Ouvindo o MAIN: use o MAIN AF (em cima)" : "Este AF é do MAIN. Ouvindo o SUB: use o SUB AF (embaixo)", 2.5);
        }
    };
    afRf(0, 369, 464);
    afRf(1, 667, 762);
    // NB / NR (do receptor em uso)
    {
        Receptor& R = rx_[ativo_];
        const int k1 = teclaIc("iNb", "", 252, 587, 348, 613, R.nb1 ? 2 : R.nb2 ? 3 : 1, "NB", 565,
                               "NB: apagador de pulsos (no áudio)\nSegurar: NB2 (no IQ, antes do demodulador)\nNível: MULTI em NB LEVEL", 0);
        if (k1 == 1) R.nb1 = !R.nb1;
        if (k1 == 2) { R.nb2 = !R.nb2; aviso(R.nb2 ? "NB2 ligado" : "NB2 desligado"); }
        const int k2 = teclaIc("iNr", "", 367, 587, 462, 613, R.nr1 ? 3 : R.nr2 ? 2 : 1, "NR", 565,
                               "NR: redutor de ruído espectral (o NR ESPECTRAL do RXSDR)\nSegurar: NR1 (redutor simples)\nForça: MULTI em DNR LEVEL", 0);
        if (k2 == 1) R.nr2 = !R.nr2;
        if (k2 == 2) { R.nr1 = !R.nr1; aviso(R.nr1 ? "NR1 ligado" : "NR1 desligado"); }
    }
    // USB (enfeite)
    {
        dl->AddRectFilled(Q(113, 595), Q(216, 733), IM_COL32(0x1c, 0x1d, 0x1f, 255), QD(5));
        dl->AddRect(Q(113, 595), Q(216, 733), IM_COL32(0x5c, 0x5e, 0x62, 200), QD(5), 0, 1.f);
        // simbolo USB
        const ImU32 b = BR;
        const float t = std::max(1.f, QD(2.f));
        dl->AddCircleFilled(Q(142, 615), QD(4), b, 12);
        dl->AddLine(Q(142, 615), Q(186, 615), b, t);
        dl->AddTriangleFilled(Q(186, 610), Q(186, 620), Q(194, 615), b);
        dl->AddLine(Q(155, 615), Q(162, 607), b, t); dl->AddLine(Q(162, 607), Q(172, 607), b, t);
        dl->AddCircleFilled(Q(174, 607), QD(3), b, 10);
        dl->AddLine(Q(160, 615), Q(166, 622), b, t); dl->AddLine(Q(166, 622), Q(176, 622), b, t);
        dl->AddRectFilled(Q(175, 619), Q(181, 625), b);
        for (int i = 0; i < 2; ++i) {
            const float y = 634 + i * 57.f;
            dl->AddRectFilled(Q(120, y), Q(210, y + 38), IM_COL32(0xb8, 0xba, 0xbe, 255), QD(2));
            dl->AddRectFilled(Q(124, y + 4), Q(206, y + 34), IM_COL32(0x08, 0x08, 0x09, 255), QD(1));
            dl->AddRectFilled(Q(128, y + 9), Q(202, y + 18), IM_COL32(0x3a, 0x3c, 0x40, 255));
            for (int k = 0; k < 4; ++k) dl->AddRectFilled(Q(140 + k * 16.f, y + 18), Q(148 + k * 16.f, y + 21), IM_COL32(0xc8, 0xb0, 0x60, 255));
        }
    }
    // MIC (conector de 8 pinos)
    {
        tx(fP(), 12.5f, 196, 760, BR, "MIC", 0.5f, 0.94f);
        const ImVec2 c = Q(195, 835);
        dl->AddCircleFilled(c, QD(68), IM_COL32(0x16, 0x17, 0x18, 255), 64);
        anelLuz(dl, c, QD(50), QD(62), 0.f, (float)(2 * kPi), 96, [](float a, float) {
            return cinzaN(0.45f + 0.45f * sino(a, rad(-140), rad(55)) + 0.30f * sino(a, rad(40), rad(40)));
        });
        circuloSuave(dl, c, QD(62), 96, 1.f, [](float a) { return cinzaN(0.3f + 0.3f * sino(a, rad(-140), rad(55)), 220); });
        for (int i = 0; i < 48; ++i) {
            const ImVec2 d = dirAng(i * (float)(2 * kPi / 48));
            dl->AddLine(ImVec2(c.x + d.x * QD(51), c.y + d.y * QD(51)), ImVec2(c.x + d.x * QD(60), c.y + d.y * QD(60)), IM_COL32(0x40, 0x42, 0x46, 160), 1.f);
        }
        dl->AddCircleFilled(c, QD(49), IM_COL32(0x0a, 0x0a, 0x0b, 255), 64);
        anelLuz(dl, c, QD(43), QD(49), 0.f, (float)(2 * kPi), 64, [](float a, float) { return cinzaN(0.06f + 0.20f * sino(a, rad(60), rad(50))); });
        dl->AddRectFilled(Q(189, 780), Q(201, 790), IM_COL32(0x60, 0x62, 0x66, 255), QD(2));
        auto pino = [&](float px, float py) {
            const ImVec2 p = Q(px, py);
            dl->AddCircleFilled(p, QD(7), IM_COL32(0x02, 0x02, 0x02, 255), 16);
            dl->AddCircleFilled(p, QD(5), IM_COL32(0xd8, 0xd4, 0xc8, 255), 16);
            dl->AddCircleFilled(ImVec2(p.x - QD(1.5f), p.y - QD(1.5f)), QD(2), IM_COL32(255, 255, 255, 200), 8);
        };
        for (int i = 0; i < 7; ++i) {
            const ImVec2 d = dirAng((float)(-kPi / 2 + (i + 0.5) * 2 * kPi / 7.5));
            pino(195 + d.x * 30, 835 + d.y * 30);
        }
        pino(195, 835);
    }
    // CACHOEIRA: no lugar do cartao SD (liga o espectro e a cachoeira grandes em cima do radio)
    {
        dl->AddRectFilled(Q(262, 857), Q(458, 898), IM_COL32(0x0c, 0x0d, 0x0e, 255), QD(5));
        if (teclaIc("iCach", "CACHOEIRA", 270, 864, 450, 892, cimaAtivo_ ? 3 : 1, nullptr, 0,
                    cimaAtivo_ ? "CACHOEIRA: desligar o espectro e a cachoeira grandes em cima do rádio"
                               : "CACHOEIRA: ligar o espectro e a cachoeira grandes em cima do rádio (ocupa a tela)", 0, 0.5f,
                    cimaAtivo_ ? IM_COL32(0xff, 0xb0, 0x40, 255) : 0) == 1)
            cimaAtivo_ = !cimaAtivo_;
    }
}

// ===========================================================================
//  MB1SDR: a tela (TFT) do MB1
// ===========================================================================
void Ui::icTela()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool lig = r_->ligado();
    const ImU32 W = lig ? IM_COL32(0xf4, 0xf4, 0xf6, 255) : IM_COL32(0x80, 0x82, 0x86, 255);
    const ImU32 CI = IM_COL32(0xb8, 0xbc, 0xc2, 255);
    dl->AddRectFilled(Q(521, 211), Q(1555, 791), IM_COL32(0, 0, 0, 255), QD(5));
    dl->PushClipRect(Q(521, 211), Q(1555, 791), true);
    icCelulas();
    const float yS = expand_ ? -110.f : 0.f;
    char b[96];
    // ---- a linha de cima de cada receptor: ANT, BW, SFT, o desenho do filtro e o MET ----
    for (int rx = 0; rx < 2; ++rx) {
        const float dx = 518.f * rx;
        Receptor& R = rx_[rx];
        const bool at = rx == ativo_;
        const std::string md = at ? r_->modo() : R.modo;
        const int g = grupoModo(md);
        const char* dev = !r_->temDispositivo() ? "--" : r_->tipo() == "rtltcp" ? "TCP" : r_->tipo() == "sdrplay" ? "RSP" : "RTL";
        std::snprintf(b, sizeof b, "ANT %s", dev);
        tx(fL(), 11.5f, 647 + dx, 227, W, b);
        auto caixa = [&](float x0, float x1, float xa, float y0, float y1, const char* aba, const char* val, const char* idc, const char* dica) {
            dl->AddRect(Q(x0, y0), Q(x1, y1), IM_COL32(0xa8, 0xaa, 0xae, 255), QD(3), 0, std::max(1.f, QD(1.6f)));
            dl->AddRectFilled(Q(x0 + 2, y0 + 2), Q(xa, y1 - 2), IM_COL32(0x66, 0x68, 0x6c, 255), QD(2));
            tx(fL(), 11.f, (x0 + 2 + xa) * 0.5f, (y0 + y1) * 0.5f, W, aba, 0.5f);
            tx(fL(), 11.5f, x1 - 6, (y0 + y1) * 0.5f, W, val, 1.f);
            ImGui::SetCursorScreenPos(Q(x0, y0));
            const ImVec2 sz(std::max(1.f, QD(x1 - x0)), std::max(1.f, QD(y1 - y0)));
            const bool c = ImGui::InvisibleButton(idc, sz);
            if (ImGui::IsItemHovered() && dica) ImGui::SetTooltip("%s", dica);
            return c;
        };
        const float bw = at ? (float)r_->banda() : (g == 2 ? filHi_[g][R.fil] * 2.f : filHi_[g][R.fil]);
        if (bw >= 10000) std::snprintf(b, sizeof b, "%.0fk", bw / 1000.f);
        else if (bw >= 1000) std::snprintf(b, sizeof b, "%.1fk", bw / 1000.f);
        else std::snprintf(b, sizeof b, "%.0f", bw);
        char idc[24];
        std::snprintf(idc, sizeof idc, "##bwc%d", rx);
        if (caixa(740 + dx, 841 + dx, 774 + dx, 218, 238, "BW", b, idc, "BW: largura do filtro - clique para escolher (o TWIN-PBT também ajusta)")) {
            if (!at) ativar(rx);
            ImGui::OpenPopup("##bwIc");
        }
        {
            const float lo = filLo_[g][R.fil], hi = filHi_[g][R.fil];
            const int sft = g == 0 ? (int)std::lround(((lo + hi) * 0.5f - 1450.f) / 10.f) * 10 : g == 1 ? (int)std::lround(lo) : 0;
            std::snprintf(b, sizeof b, "%d", sft);
        }
        std::snprintf(idc, sizeof idc, "##sfc%d", rx);
        if (caixa(855 + dx, 956 + dx, 900 + dx, 218, 238, "SFT", b, idc, "SFT: deslocamento do filtro (Hz)\nClique: volta ao centro")) {
            float& lo = filLo_[g][R.fil];
            float& hi = filHi_[g][R.fil];
            if (g == 0) { const float s = 1450.f - (lo + hi) * 0.5f; lo += s; hi += s; if (lo < 0) { hi -= lo; lo = 0; } }
            else if (g == 1) lo = 0;
            if (at) aplicarFiltro();
        }
        {   // o desenho do filtro (passa-faixa): mais largo com o filtro mais largo
            static const float mx[5] = {5000, 2500, 16000, 30000, 250000};
            const float f = std::clamp(bw / mx[std::clamp(g, 0, 4)], 0.05f, 1.f);
            const float xc = 1000 + dx, ht = 6.f + 14.f * f, hb = ht + 7.f;
            const ImU32 c = W;
            const float t = std::max(1.f, QD(1.6f));
            dl->AddLine(Q(968 + dx, 239), Q(xc - hb, 239), c, t);
            dl->AddLine(Q(xc - hb, 239), Q(xc - ht, 221), c, t);
            dl->AddLine(Q(xc - ht, 221), Q(xc + ht, 221), c, t);
            dl->AddLine(Q(xc + ht, 221), Q(xc + hb, 239), c, t);
            dl->AddLine(Q(xc + hb, 239), Q(1032 + dx, 239), c, t);
            dl->AddLine(Q(xc, 226), Q(xc, 239), c, t);
            ImGui::SetCursorScreenPos(Q(966 + dx, 216));
            std::snprintf(idc, sizeof idc, "##flc%d", rx);
            if (ImGui::InvisibleButton(idc, ImVec2(QD(68), QD(26)))) { R.fil = (R.fil + 1) % 3; if (at) aplicarFiltro(); }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Filtro FIL%d (clique: FIL1 / FIL2 / FIL3)", R.fil + 1);
        }
        {   // MET: a leitura do S-meter (clique troca S / dB / PEAK)
            const float pos = std::clamp(at ? smAtual_ : smOutro_, 0.f, 1.02f);
            if (!lig) std::snprintf(b, sizeof b, "--");
            else if (at && medidorModo_ == 1) std::snprintf(b, sizeof b, "%.0fdB", peakDb_);
            else if (at && medidorModo_ == 2) std::snprintf(b, sizeof b, "PK%.0f", peakDbSql_);
            else std::snprintf(b, sizeof b, "%s", leituraS(pos).c_str());
            std::snprintf(idc, sizeof idc, "##met%d", rx);
            if (caixa(645 + dx, 758 + dx, 692 + dx, 243, 263, "MET", b, idc, "MET: leitura do S-meter (clique: S / dB / PEAK)"))
                medidorModo_ = (medidorModo_ + 1) % 3;
        }
    }
    if (!expand_) { icMedidor(0, 0.f); icMedidor(1, 518.f); }
    // ---- o meio: relogio, TX, DUAL-W ----
    {
        SYSTEMTIME lt, ut;
        GetLocalTime(&lt); GetSystemTime(&ut);
        std::snprintf(b, sizeof b, "%d:%02d", lt.wHour, lt.wMinute);
        tx(fL(), 19.f, 1053, 228, W, b);
        const float w = txW(fL(), 19.f, b);
        std::snprintf(b, sizeof b, "%02d", lt.wSecond);
        tx(fL(), 11.f, 1053 + w + 6, 232, W, b);
        tx(fL(), 9.f, 1063, 246, CI, "UTC");
        std::snprintf(b, sizeof b, "%d:%02d", ut.wHour, ut.wMinute);
        tx(fL(), 9.f, 1112, 246, CI, b);
    }
    if (!expand_) {
        const float dx = 518.f * txVfo_;
        const ImU32 vm = lig ? IM_COL32(0xff, 0x26, 0x20, 255) : IM_COL32(0x80, 0x30, 0x2c, 255);
        dl->AddRect(Q(642 + dx, 362), Q(697 + dx, 392), vm, QD(3), 0, std::max(1.f, QD(2.2f)));
        tx(fL(), 18.f, 669.5f + dx, 377, vm, "TX", 0.5f);
        ImGui::SetCursorScreenPos(Q(642 + dx, 362));
        if (ImGui::InvisibleButton("##txic", ImVec2(QD(55), QD(30)))) txVfo_ = 1 - txVfo_;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("TX: o receptor marcado como \"TX\" (SPLIT). Clique: troca");
    }
    if (tracking_) {
        dl->AddRectFilled(Q(1052, 364 + (expand_ ? -150.f : 0.f)), Q(1143, 383 + (expand_ ? -150.f : 0.f)), IM_COL32(0x16, 0x8c, 0x34, 255), QD(3));
        dl->AddRect(Q(1052, 364 + (expand_ ? -150.f : 0.f)), Q(1143, 383 + (expand_ ? -150.f : 0.f)), IM_COL32(0x60, 0xd0, 0x80, 255), QD(3), 0, 1.f);
        tx(fL(), 12.f, 1097.5f, 373.5f + (expand_ ? -150.f : 0.f), IM_COL32(0xf4, 0xf8, 0xf4, 255), "DUAL-W", 0.5f);
    }
    icVfo(0, yS);
    icVfo(1, yS);
    if (rx_[ativo_].lock) {        // a chavinha amarela (dial travado)
        const ImU32 am = IM_COL32(0xd8, 0xb8, 0x20, 255);
        const float y = 515 + yS;
        dl->AddCircle(Q(1060, y), QD(5.5f), am, 16, std::max(1.f, QD(2.5f)));
        dl->AddLine(Q(1066, y), Q(1082, y), am, std::max(1.f, QD(3.f)));
        dl->AddLine(Q(1076, y), Q(1076, y + 5), am, std::max(1.f, QD(2.5f)));
        dl->AddLine(Q(1081, y), Q(1081, y + 5), am, std::max(1.f, QD(2.5f)));
    }
    icEscopos(528 + yS);
    icTeclasSoft();
    dl->PopClipRect();
    if (!lig) {
        dl->AddRectFilled(Q(800, 590), Q(1300, 640), IM_COL32(0, 0, 0, 210), QD(6));
        tx(fP(), 13.f, 1050, 615, P_AMBAR, "Aperte POWER (em cima, à esquerda) para ligar", 0.5f);
    }
    if (dim_ > 0) dl->AddRectFilled(Q(521, 211), Q(1555, 791), IM_COL32(0, 0, 0, dim_ * 45));
}

// As celulas da coluna da esquerda da tela (ANT, P.AMP, ATT, IP+, AGC, VOX, COMP)
void Ui::icCelulas()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    auto& c = Config::instance();
    const bool lig = r_->ligado();
    const ImU32 W = lig ? IM_COL32(0xf4, 0xf4, 0xf6, 255) : IM_COL32(0x80, 0x82, 0x86, 255);
    Receptor& R = rx_[ativo_];
    if (R.sql <= -90.f) sqlAuto_[ativo_] = false;
    const char* ag[4] = {"OFF", "SLOW", "MID", "FAST"};
    char v[7][24];
    std::snprintf(v[0], 24, "%s", !r_->temDispositivo() ? "--" : r_->tipo() == "rtltcp" ? "TCP" : r_->tipo() == "sdrplay" ? "RSP" : "RTL");
    std::snprintf(v[1], 24, "%s", pamp_ == 0 ? "OFF" : pamp_ == 1 ? "1" : "2");
    if (att_) std::snprintf(v[2], 24, "%ddB", att_ * 6); else std::snprintf(v[2], 24, "OFF");
    std::snprintf(v[3], 24, "%s", c.dcRemove() ? "ON" : "OFF");
    std::snprintf(v[4], 24, "%s", ag[R.agcOff ? 0 : std::clamp(R.agc, 1, 3)]);
    std::snprintf(v[5], 24, "%s", sqlMedindo_ ? "..." : sqlAuto_[ativo_] ? "ON" : "OFF");
    std::snprintf(v[6], 24, "%s", proc_ ? "ON" : "OFF");
    const char* cab[7] = {"ANT", "P.AMP", "ATT", "IP+", "AGC", "VOX", "COMP"};
    const bool on[7] = {false, pamp_ > 0, att_ > 0, c.dcRemove(), !R.agcOff, sqlAuto_[ativo_] || sqlMedindo_, proc_};
    const char* dicas[7] = {
        "ANT: o aparelho (RTL-SDR USB, RTL-TCP pela rede, SDRplay) - abre a configuração",
        "P.AMP: pré-amplificador OFF / 1 / 2 (clique troca)\nOFF = sem pré, 1 = +6 dB, 2 = +12 dB (ganho limpo, sem saturar o dongle)\nSDRplay: +10 / +20 dB de FI",
        "ATT: atenuador OFF / 6 / 12 / 18 dB",
        "IP+: tira o risco (DC) do centro do espectro",
        "AGC: FAST / MID / SLOW / OFF (clique troca)",
        "VOX: squelch automático.\nClique: mede o ruído por 4 s e fecha o squelch logo acima.\nLigado: clique de novo para abrir o squelch.",
        "COMP: compressor de áudio (PROC). A força: MENU > Áudio"};
    for (int i = 0; i < 7; ++i) {
        const float y0 = 216.f + i * 74.5f, y1 = y0 + 73.5f;
        ImGui::SetCursorScreenPos(Q(527, y0));
        char id[16]; std::snprintf(id, sizeof id, "##icc%d", i);
        const bool clicou = ImGui::InvisibleButton(id, ImVec2(QD(105), QD(72)));
        const bool sobre = ImGui::IsItemHovered();
        dl->AddRectFilled(Q(527, y0), Q(633, y1), sobre ? IM_COL32(0x1c, 0x22, 0x2c, 255) : IM_COL32(0x02, 0x02, 0x03, 255));
        dl->AddRect(Q(527, y0), Q(633, y1), IM_COL32(0x6c, 0x6e, 0x72, 255), 0.f, 0, std::max(1.f, QD(1.6f)));
        // a barrinha ciano da direita (acesa com a funcao ligada)
        if (on[i] && lig) dl->AddRectFilledMultiColor(Q(624, y0 + 4), Q(631, y1 - 4), IM_COL32(0x20, 0xd8, 0xe0, 255), IM_COL32(0x20, 0xd8, 0xe0, 255),
                                                       IM_COL32(0x08, 0x50, 0x60, 255), IM_COL32(0x08, 0x50, 0x60, 255));
        if (i == 6) {
            tx(fL(), 12.5f, 578, y0 + 16, W, cab[i], 0.5f);
            tx(fL(), 12.5f, 578, y0 + 37, W, v[i], 0.5f);
            tx(fL(), 12.5f, 578, y0 + 58, W, procNivel_ < 0.34f ? "NAR" : procNivel_ < 0.67f ? "MID" : "WIDE", 0.5f);
        } else {
            tx(fL(), 13.f, 578, y0 + 22, W, cab[i], 0.5f);
            tx(fL(), 13.f, 578, y0 + 51, W, v[i], 0.5f);
        }
        if (sobre) ImGui::SetTooltip("%s", dicas[i]);
        if (!clicou) continue;
        switch (i) {
        case 0: abrirConfig(); break;
        case 1: pamp_ = (pamp_ + 1) % 3; break;
        case 2: att_ = (att_ + 1) % 4; break;
        case 3: c.set("dc_remove", !c.dcRemove()); break;
        case 4: if (R.agcOff) { R.agcOff = false; R.agc = 1; } else if (R.agc >= 3) R.agcOff = true; else R.agc++; break;
        case 5:
            if (sqlMedindo_) { sqlMedindo_ = false; R.sql = -90.f; sql_ = -90.f; sqlAplicado_ = -90.f; sqlAuto_[ativo_] = false; }
            else if (sqlAuto_[ativo_]) { R.sql = -90.f; sql_ = -90.f; sqlAplicado_ = -90.f; sqlAuto_[ativo_] = false; aviso("Squelch aberto"); }
            else if (r_->aoVivo()) { sqlMedindo_ = true; sqlAmostras_.clear(); sqlIni_ = agora(); sqlUlt_ = 0; }
            break;
        case 6: proc_ = !proc_; break;
        }
    }
}

// O S-meter do MB1 (igual nos dois receptores): arcos S / Id / Po / SWR /
// COMP / ALC / Vd, como na foto. So o S anda (num receptor). dx = deslocamento
// do da direita (px da foto).
void Ui::icMedidor(int rx, float dx)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool lig = r_->ligado();
    const ImU32 Wc = lig ? IM_COL32(0xf6, 0xf6, 0xf6, 255) : IM_COL32(0x7a, 0x7c, 0x80, 255);
    const ImU32 Vc = lig ? IM_COL32(0xff, 0x1e, 0x18, 255) : IM_COL32(0x7c, 0x2c, 0x28, 255);
    const ImU32 Ac = lig ? IM_COL32(0x1e, 0x8c, 0xff, 255) : IM_COL32(0x2a, 0x4c, 0x7c, 255);
    const float cxA = 844.f;
    struct Arco { float cy, R; };
    const Arco aS{653, 381}, aI{719, 429}, aP{697, 390}, aW{664, 339}, aC{609, 257}, aL{609, 251}, aV{609, 247};
    auto yA = [&](const Arco& a, float x) { const float d = x - cxA; return a.cy - std::sqrt(std::max(0.f, a.R * a.R - d * d)); };
    auto arco = [&](const Arco& a, float xa, float xb, ImU32 cor, float esp) {
        std::vector<ImVec2> p;
        const int n = 40;
        for (int k = 0; k <= n; ++k) { const float x = xa + (xb - xa) * k / n; p.push_back(Q(x + dx, yA(a, x))); }
        dl->AddPolyline(p.data(), (int)p.size(), cor, 0, std::max(1.f, QD(esp)));
    };
    auto marca = [&](const Arco& a, float x, float l0, float l1, ImU32 cor, float esp) {   // l > 0: para fora (para cima)
        const float y = yA(a, x), nx = (x - cxA) / a.R, ny = (y - a.cy) / a.R;
        dl->AddLine(Q(x + dx + nx * l0, y + ny * l0), Q(x + dx + nx * l1, y + ny * l1), cor, std::max(1.f, QD(esp)));
    };
    auto rot = [&](const Arco& a, float x, float dist, float cap, ImU32 cor, const char* t) {   // dist > 0: acima do arco
        const float y = yA(a, x), nx = (x - cxA) / a.R, ny = (y - a.cy) / a.R;
        tx(fL(), cap, x + dx + nx * dist, y + ny * dist, cor, t, 0.5f);
    };
    // S (branco ate o 9, vermelho depois)
    arco(aS, 668, 845, Wc, 6.5f);
    arco(aS, 845, 1022, Vc, 6.5f);
    const float xS[10] = {690, 713, 730, 747, 763, 780, 796, 812, 829, 845};
    for (int i = 1; i < 10; ++i) marca(aS, xS[i], 2.f, (i % 2) ? 10.f : 6.5f, Wc, 2.4f);
    const float xD[6] = {869, 893, 915, 937, 961, 985};
    for (int i = 0; i < 6; ++i) marca(aS, xD[i], 2.f, (i % 2) ? 10.f : 6.5f, Vc, 2.4f);
    tx(fL(), 13.f, 657 + dx, 306, Wc, "S", 0.5f);
    rot(aS, 713, 15.f, 14.f, Wc, "1");
    rot(aS, 780, 15.f, 14.f, Wc, "5");
    rot(aS, 845, 15.f, 14.f, Wc, "9");
    rot(aS, 893, 14.f, 12.5f, Vc, "+20");
    rot(aS, 937, 14.f, 12.5f, Vc, "+40");
    tx(fL(), 12.5f, 995 + dx, 286, Vc, "+60dB", 0.5f);
    // Id (A)
    arco(aI, 680, 1012, Wc, 2.4f);
    const float xI[7] = {727, 768, 810, 855, 900, 945, 987};
    for (int i = 0; i < 7; ++i) marca(aI, xI[i], -1.f, (i % 2) ? -4.f : -6.5f, Wc, 2.f);
    rot(aI, 727, -10.f, 9.f, Wc, "0"); rot(aI, 810, -10.f, 9.f, Wc, "10"); rot(aI, 900, -10.f, 9.f, Wc, "20"); rot(aI, 983, -10.f, 9.f, Wc, "30");
    tx(fL(), 9.f, 1004 + dx, 325, Wc, "A", 0.5f);
    tx(fL(), 8.f, 669 + dx, 328, Wc, "Id", 0.5f);
    // Po (%)
    arco(aP, 695, 995, Wc, 2.4f);
    const float xP[8] = {740, 763, 785, 812, 863, 895, 927, 960};
    for (int i = 0; i < 8; ++i) marca(aP, xP[i], -1.f, -5.f, Wc, 2.f);
    rot(aP, 740, -10.f, 9.f, Wc, "0"); rot(aP, 785, -10.f, 9.f, Wc, "10"); rot(aP, 812, -10.f, 9.f, Wc, "20");
    rot(aP, 863, -10.f, 9.f, Wc, "50"); rot(aP, 927, -10.f, 9.f, Wc, "100");
    tx(fL(), 9.f, 981 + dx, 337, Wc, "%", 0.5f);
    tx(fL(), 8.f, 705 + dx, 339, Wc, "Po", 0.5f);
    // SWR
    arco(aW, 723, 975, Wc, 3.2f);
    const float xW[6] = {758, 797, 823, 848, 890, 938};
    for (int i = 0; i < 6; ++i) marca(aW, xW[i], 1.5f, 6.f, Wc, 2.f);
    rot(aW, 758, -11.f, 9.f, Wc, "1"); rot(aW, 797, -11.f, 9.f, Wc, "1.5"); rot(aW, 823, -11.f, 9.f, Wc, "2"); rot(aW, 848, -11.f, 9.f, Wc, "3");
    {   // infinito
        const float y = yA(aW, 938) + 11.f;
        dl->AddCircle(Q(934 + dx, y), QD(3.6f), Wc, 12, std::max(1.f, QD(1.6f)));
        dl->AddCircle(Q(941.5f + dx, y), QD(3.6f), Wc, 12, std::max(1.f, QD(1.6f)));
    }
    tx(fL(), 8.5f, 733 + dx, 355, Wc, "SWR", 0.5f);
    // COMP (azul), ALC (vermelho) e Vd
    arco(aC, 752, 935, Ac, 3.6f);
    const float xC[6] = {767, 790, 815, 845, 878, 905};
    for (int i = 0; i < 6; ++i) marca(aC, xC[i], 1.f, (i == 0 || i == 2 || i == 4) ? 8.f : 5.f, Ac, 2.f);
    rot(aC, 767, 12.f, 9.5f, Ac, "0"); rot(aC, 815, 12.f, 9.5f, Ac, "10"); rot(aC, 878, 12.f, 9.5f, Ac, "20");
    tx(fL(), 9.f, 939 + dx, 363, Ac, "dB", 0.5f);
    tx(fL(), 8.5f, 734 + dx, 368, Ac, "COMP", 0.5f);
    arco(aL, 776, 845, Vc, 4.5f);
    marca(aL, 845, -3.f, 3.f, Vc, 4.f);
    tx(fL(), 9.f, 818 + dx, 376, Vc, "ALC", 0.5f);
    arco(aV, 856, 892, Wc, 2.2f);
    marca(aV, 858, 0.f, -5.f, Wc, 1.8f); marca(aV, 876, 0.f, -4.f, Wc, 1.8f); marca(aV, 890, 0.f, -5.f, Wc, 1.8f);
    tx(fL(), 8.5f, 858 + dx, 377, Wc, "10", 0.5f);
    tx(fL(), 8.5f, 889 + dx, 378, Wc, "16V", 0.5f);
    tx(fL(), 8.5f, 915 + dx, 377, Wc, "Vd", 0.5f);
    // o ponteiro (branco), do pivo embaixo ate a escala S
    {
        const float p = std::clamp(rx == ativo_ ? smAtual_ : smOutro_, 0.f, 1.05f);
        static const float tp[18] = {0, 0.06f, 0.117f, 0.17f, 0.224f, 0.282f, 0.34f, 0.392f, 0.444f, 0.495f, 0.546f, 0.62f, 0.693f, 0.77f, 0.845f, 0.92f, 0.996f, 1.05f};
        static const float tx_[18] = {670, 690, 713, 730, 747, 763, 780, 796, 812, 829, 845, 869, 893, 915, 937, 961, 985, 1005};
        float xs = tx_[0];
        for (int i = 0; i < 17; ++i)
            if (p >= tp[i] && p <= tp[i + 1]) { xs = tx_[i] + (tx_[i + 1] - tx_[i]) * (p - tp[i]) / (tp[i + 1] - tp[i]); break; }
        if (p > tp[17]) xs = tx_[17];
        const float px = cxA, py = 425.f;
        const float ys = yA(aS, xs);
        float ux = xs - px, uy = ys - py;
        const float L = std::sqrt(ux * ux + uy * uy);
        ux /= L; uy /= L;
        dl->PushClipRect(Q(640 + dx, 250), Q(1040 + dx, 381), true);
        dl->AddLine(Q(px + dx + ux * 58.f, py + uy * 58.f), Q(px + dx + ux * (L + 7.f), py + uy * (L + 7.f)),
                    lig ? IM_COL32(0xff, 0xff, 0xff, 255) : IM_COL32(0x90, 0x90, 0x94, 255), std::max(1.f, QD(2.6f)));
        dl->PopClipRect();
    }
    ImGui::SetCursorScreenPos(Q(650 + dx, 266));
    char id[16]; std::snprintf(id, sizeof id, "##icmed%d", rx);
    if (ImGui::InvisibleButton(id, ImVec2(QD(380), QD(112)))) medidorModo_ = (medidorModo_ + 1) % 3;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("S-meter do %s%s\nClique: leitura em S / dB / PEAK (na caixa MET)", rx == 0 ? "MAIN" : "SUB",
                                                  rx == ativo_ ? " (receptor ouvido)" : " (lido no espectro, no filtro dele)");
}

// A linha do VFO de um receptor: VFO, o modo (azul), FIL, a frequencia e a linha da memoria
void Ui::icVfo(int rx, float yS)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    Receptor& R = rx_[rx];
    const bool at = rx == ativo_;
    const bool lig = r_->ligado();
    const ImU32 W = lig ? IM_COL32(0xf4, 0xf4, 0xf6, 255) : IM_COL32(0x80, 0x82, 0x86, 255);
    const uint64_t f = at ? r_->vfo() : R.f;
    const std::string md = at ? r_->modo() : R.modo;
    const float dv = 470.f * rx, df = 485.f * rx, ds = 483.f * rx;
    char b[96], id[24];
    // VFO / MEMO
    std::snprintf(b, sizeof b, "%s", R.memModo ? "MEMO" : "VFO");
    tx(fL(), 13.f, R.memModo ? 652 + dv : 663 + dv, 417 + yS, W, b);
    // o modo (etiqueta azul): clique escolhe
    {
        const ImVec2 a = Q(711 + dv, 397 + yS), c = Q(818 + dv, 432 + yS);
        ImGui::SetCursorScreenPos(a);
        std::snprintf(id, sizeof id, "##mdic%d", rx);
        const bool cl = ImGui::InvisibleButton(id, ImVec2(std::max(1.f, c.x - a.x), std::max(1.f, c.y - a.y)));
        const bool sob = ImGui::IsItemHovered();
        retGrad(dl, a, c, QD(5), sob ? IM_COL32(0x40, 0x9c, 0xe0, 255) : IM_COL32(0x2e, 0x86, 0xc8, 255), IM_COL32(0x16, 0x56, 0x92, 255));
        dl->AddRect(a, c, IM_COL32(0x6c, 0xb8, 0xec, 255), QD(5), 0, 1.f);
        std::string nm = md == "NFM" ? "FM" : md == "WFM" ? "WFM" : md;
        tx(fL(), 15.f, 764.5f + dv, 414.5f + yS, lig ? IM_COL32(0xf6, 0xfa, 0xff, 255) : IM_COL32(0x90, 0x98, 0xa4, 255), nm.c_str(), 0.5f);
        if (sob) ImGui::SetTooltip("Modo do %s: clique para escolher (LSB, USB, CW, AM, FM, WFM e os decodificadores)", rx == 0 ? "MAIN" : "SUB");
        if (cl) { if (!at) ativar(rx); ImGui::OpenPopup("##modosIc"); }
    }
    // FIL1/2/3
    std::snprintf(b, sizeof b, "FIL%d", R.fil + 1);
    tx(fL(), 13.f, 826 + dv, 417 + yS, W, b);
    ImGui::SetCursorScreenPos(Q(824 + dv, 404 + yS));
    std::snprintf(id, sizeof id, "##filic%d", rx);
    if (ImGui::InvisibleButton(id, ImVec2(QD(48), QD(26)))) { R.fil = (R.fil + 1) % 3; if (at) aplicarFiltro(); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Filtro: FIL1 / FIL2 / FIL3 (clique troca; a largura fica no TWIN-PBT e no BW)");
    // etiquetas pequenas do que esta ligado
    {
        float xi = 882 + dv;
        const float xMax = rx == 0 ? 1040.f : 1550.f;
        auto ind = [&](const char* t, ImU32 c) {
            const float w = txW(fL(), 9.f, t) + 8.f;
            if (xi + w > xMax) return;
            dl->AddRect(Q(xi, 410 + yS), Q(xi + w, 425 + yS), alfa(c, 200), QD(2), 0, 1.f);
            tx(fL(), 9.f, xi + w * 0.5f, 417.5f + yS, c, t, 0.5f);
            xi += w + 4.f;
        };
        const ImU32 cz = IM_COL32(0xc8, 0xcc, 0xd2, 255), lr = IM_COL32(0xff, 0xa0, 0x30, 255);
        const bool busy = lig && r_->aoVivo() && ocupadoRx(rx);
        if (busy) ind("BUSY", IM_COL32(0x60, 0xff, 0x70, 255));
        if (R.nb1 || R.nb2) ind(R.nb2 ? "NB2" : "NB", cz);
        if (R.nr2) ind("NR", cz);
        if (R.nr1) ind("NR1", IM_COL32(0xff, 0xa0, 0x30, 255));
        if (R.nch) ind("NOTCH", cz);
        if (R.anch) ind("AN", cz);
        if (R.bef) ind("CONT", cz);
        if (R.apf) ind("APF", cz);
        if (R.mute) ind("MUTE", IM_COL32(0xff, 0x50, 0x40, 255));
        if (R.sql > -90.f) { std::snprintf(b, sizeof b, "SQL%+d", (int)std::lround(R.sql)); ind(b, (at && lig && !sqlAberto_) ? IM_COL32(0xff, 0x50, 0x40, 255) : lr); }
        if (R.ritOn) { std::snprintf(b, sizeof b, "RIT%+.2f", R.rit / 1000.0); ind(b, lr); }
        if (at && fast_) ind("FAST", lr);
        if (R.fine) ind("FINE", lr);
    }
    // a frequencia
    const float xr = 1035 + df, yc = 472 + yS, cap = 50.f;
    if (entrando_ && at) {
        const std::string t = (entrada_.empty() ? std::string("_") : entrada_) + " kHz";
        tx(fL(), 40.f, xr, yc, P_AMBAR, t.c_str(), 1.f);
    } else {
        const std::string fs = fmtFreqLcd(f);
        const ImU32 cf = !lig ? IM_COL32(0x50, 0x52, 0x56, 255) : at ? IM_COL32(0xf6, 0xf6, 0xf6, 255) : IM_COL32(0xa8, 0xaa, 0xae, 255);
        for (float o : {2.4f, 1.2f, 0.f}) tx(fL(), cap, xr - 2.4f + o, yc, cf, fs.c_str(), 1.f, 1.06f);   // negrito: 3 passadas
        // roda do mouse em cima de um digito: muda aquele digito
        if (ImGui::IsWindowHovered()) {
            ImGuiIO& io = ImGui::GetIO();
            const float px = pxCap(fL(), cap);
            ImFont* fo = melhor(fL(), px);
            float xx = Q(xr, 0).x;
            uint64_t peso = 1;
            const float yt = Q(0, yc - cap * 0.62f).y, yb = Q(0, yc + cap * 0.62f).y;
            for (int j = (int)fs.size() - 1; j >= 0; --j) {
                const char ch[2] = {fs[(size_t)j], 0};
                const float wc = fo->CalcTextSizeA(px, FLT_MAX, 0, ch).x * 1.06f;
                if (ch[0] >= '0' && ch[0] <= '9') {
                    const ImVec2 a(xx - wc, yt), c(xx, yb);
                    if (ImGui::IsMouseHoveringRect(a, c, false)) {
                        dl->AddLine(ImVec2(a.x + 1, c.y + 1), ImVec2(c.x - 1, c.y + 1), IM_COL32(0x40, 0xc8, 0xff, 255), std::max(1.5f, QD(2.5f)));
                        if (io.MouseWheel != 0) {
                            int n = (int)std::lround(io.MouseWheel);
                            if (n == 0) n = io.MouseWheel > 0 ? 1 : -1;
                            mudarFreqRx(rx, (int64_t)n * (int64_t)peso);
                            io.MouseWheel = 0;
                        }
                    }
                    if (peso < 1000000000ULL) peso *= 10;
                }
                xx -= wc;
            }
        }
    }
    {   // clique na frequencia: este receptor passa a ser o ouvido; no que ja e, digita a frequencia
        ImGui::SetCursorScreenPos(Q(xr - 330, yc - 30));
        std::snprintf(id, sizeof id, "##fqic%d", rx);
        if (ImGui::InvisibleButton(id, ImVec2(QD(330), QD(60)))) {
            if (!at) ativar(rx);
            else { entrando_ = !entrando_; entrada_.clear(); }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", at ? "Clique: digitar a frequência (kHz, Enter confirma, Esc cancela)\nRoda do mouse em cima de um dígito: muda aquele dígito"
                                       : "Clique: ouvir este receptor\nRoda do mouse em cima de um dígito: muda aquele dígito");
    }
    // a linha de baixo: o canal de memoria (numero, frequencia e modo)
    if (!mem_.empty()) {
        int ch = std::clamp(R.memCanal, 0, (int)mem_.size() - 1);
        if (!R.memModo) {        // no VFO: a memoria mais perto da frequencia
            long long melhor = LLONG_MAX;
            for (int i = 0; i < (int)mem_.size(); ++i) {
                const long long dd = std::llabs((long long)mem_[i].f - (long long)f);
                if (dd < melhor) { melhor = dd; ch = i; }
            }
        }
        const Memoria& m = mem_[(size_t)ch];
        std::string fm = fmtFreqLcd(m.f);
        if (fm.size() > 1) fm.pop_back();          // como o radio: 7.035.00
        std::snprintf(b, sizeof b, "%d", ch + 1);
        tx(fL(), 11.5f, 735 + ds, 507 + yS, IM_COL32(0xd8, 0xda, 0xde, 255), b, 0.5f);
        std::snprintf(b, sizeof b, "%s  %s", fm.c_str(), m.modo.c_str());
        tx(fL(), 11.5f, 768 + ds, 507 + yS, IM_COL32(0xd8, 0xda, 0xde, 255), b);
        if (!m.nome.empty()) tx(fL(), 9.5f, 1035 + df, 507 + yS, IM_COL32(0x90, 0x94, 0x9a, 255), m.nome.c_str(), 1.f, 1.f, 120.f);
    }
}

// O cabecalho do escopo e os escopos (o principal e, com DUAL, o do outro receptor)
void Ui::icEscopos(float yT)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool lig = r_->ligado();
    const ImU32 W = lig ? IM_COL32(0xf4, 0xf4, 0xf6, 255) : IM_COL32(0x80, 0x82, 0x86, 255);
    const float yh = yT + 8.5f;
    char b[64];
    auto caixinha = [&](float x0, float x1, ImU32 fundo, ImU32 ct, const char* t, float cap = 10.5f) {
        dl->AddRectFilled(Q(x0, yh - 8), Q(x1, yh + 8), fundo, QD(3));
        tx(fL(), cap, (x0 + x1) * 0.5f, yh, ct, t, 0.5f);
    };
    caixinha(763, 790, mono_ ? IM_COL32(0x40, 0x40, 0x44, 255) : IM_COL32(0xe0, 0x30, 0x28, 255), W, "M");
    caixinha(793, 813, IM_COL32(0x3c, 0x3e, 0x42, 255), IM_COL32(0x8c, 0x8e, 0x92, 255), "S");
    caixinha(817, 840, IM_COL32(0x3c, 0x3e, 0x42, 255), IM_COL32(0x8c, 0x8e, 0x92, 255), "T");
    caixinha(853, 917, pausa_ ? IM_COL32(0xff, 0xa0, 0x20, 255) : IM_COL32(0x3c, 0x3e, 0x42, 255),
             pausa_ ? IM_COL32(0x10, 0x10, 0x10, 255) : IM_COL32(0x8c, 0x8e, 0x92, 255), "HOLD");
    {
        const int v = (int)std::lround(wfSpeedEsc_), n = v <= 3 ? 1 : v <= 6 ? 2 : 3;
        for (int i = 0; i < 3; ++i) {
            const float x = 921 + i * 11.f;
            dl->AddTriangleFilled(Q(x, yh - 6), Q(x, yh + 6), Q(x + 9, yh), i < n ? IM_COL32(0x2c, 0xa4, 0xff, 255) : IM_COL32(0x20, 0x34, 0x48, 255));
        }
    }
    tx(fL(), 12.5f, 1093, yh, W, "SPECTRUM SCOPE", 0.5f);
    caixinha(1243, 1316, IM_COL32(0x1c, 0x56, 0xb0, 255), W, modoCentro_ ? "CENT" : "FIX", 11.f);
    ImGui::SetCursorScreenPos(Q(1243, yh - 8));
    if (ImGui::InvisibleButton("##fixic", ImVec2(QD(73), QD(16)))) { modoCentro_ = !modoCentro_; if (modoCentro_) r_->centralizar(r_->centroSemDc(r_->vfo())); }
    caixinha(1330, 1388, IM_COL32(0xd8, 0x30, 0x2c, 255), W, ativo_ == 0 ? "MAIN" : "SUB", 11.f);
    const double span = (taxaMostrada_ ? taxaMostrada_ : r_->taxa()) / (1.0 + zoom_);
    tx(fL(), 11.f, 1400, yh, W, "Grid");
    {
        const double g = span / 10.0;
        if (g >= 1e6) std::snprintf(b, sizeof b, "%.1fM/10dB", g / 1e6);
        else if (g >= 1000) std::snprintf(b, sizeof b, g >= 10000 ? "%.0fk/10dB" : "%.1fk/10dB", g / 1000.0);
        else std::snprintf(b, sizeof b, "%.0f/10dB", g);
        tx(fL(), 11.f, 1532, yh, W, b, 1.f);
    }
    // ---- os escopos ----
    const float y0 = yT + 19.f, y1 = 742.f;
    auto fotoX = [&](float sx) { return (sx - M.ox) / (M.k * kK) + 75.f; };
    auto rotulosBaixo = [&](float xa, float xb, double lo, double hi) {
        const double sp = hi - lo;
        static const double passos[] = {500, 1000, 2000, 2500, 5000, 10000, 20000, 25000, 50000, 100000, 200000, 250000, 500000, 1e6, 2e6, 5e6};
        double st = passos[0];
        for (double p : passos) { st = p; if (sp / p <= 6.5) break; }
        for (double f = std::ceil(lo / st) * st; f <= hi; f += st) {
            const float x = xa + (float)((f - lo) / sp) * (xb - xa);
            if (x < xa + 18 || x > xb - 18) continue;
            dl->AddLine(Q(x, y1 - 20), Q(x, y1 - 16), W, 1.f);
            if (st >= 10000) std::snprintf(b, sizeof b, "%.2f", f / 1e6); else std::snprintf(b, sizeof b, "%.3f", f / 1e6);
            tx(fL(), 10.f, x, y1 - 9, W, b, 0.5f);
        }
    };
    auto bordas = [&](float xa, float xb, double lo, double hi) {
        std::snprintf(b, sizeof b, "%.3f", lo / 1e6); tx(fL(), 12.f, xa + 9, y0 + 10, W, b, 0.f);
        std::snprintf(b, sizeof b, "%.3f", hi / 1e6); tx(fL(), 12.f, xb - 9, y0 + 10, W, b, 1.f);
    };
    // DUAL: o escopo do MAIN fica sempre a esquerda e o do SUB a direita (como no radio);
    // a moldura laranja vai para o do receptor em uso
    const bool dir = dual_ && ativo_ == 1;
    const float xA0 = dir ? 1103.f : 650.f, xA1 = !dual_ ? 1542.f : dir ? 1542.f : 1090.f;
    {   // trocou o receptor em uso: a cachoeira do outro comeca limpa (a antiga era de outra frequencia)
        static int ultAtivo = -1;
        if (ultAtivo != ativo_) { if (subTex_) criarTexturaSub(); ultAtivo = ativo_; }
    }
    {   // o principal (receptor ouvido)
        dl->AddRect(Q(xA0, y0), Q(xA1, y1), dual_ ? IM_COL32(0xff, 0x98, 0x10, 255) : IM_COL32(0x70, 0x72, 0x76, 255), QD(3), 0, std::max(1.f, QD(dual_ ? 3.f : 1.6f)));
        const ImVec2 A = Q(xA0 + 4, y0 + 20), B = Q(xA1 - 4, y1 - 21);
        const float sx = A.x, sy = A.y, sw = B.x - A.x, sh = B.y - A.y;
        float xm = sx;
        if (ifOn_) {
            const float wIf = std::floor(sw * 0.26f);
            ifDisplay(sx, sy, wIf, sh);
            xm = sx + wIf + 2.f;
        }
        const float wm = sx + sw - xm;
        const float hEsp = std::floor(sh * 0.54f);
        wfVelDesenho_ = wfSpeedEsc_;
        const bool troca = pausa_ && congelado_.size() == suave_.size() && !suave_.empty();
        if (troca) suave_.swap(congelado_);
        semDb_ = true;
        dl->PushClipRect(ImVec2(xm, sy), ImVec2(xm + wm, sy + sh), true);
        espectro(xm, sy, wm, hEsp);
        dl->PopClipRect();
        semDb_ = false;
        if (troca) suave_.swap(congelado_);
        cachoeira(xm, sy + hEsp, wm, sh - hEsp);
        const double lo = xParaHz(xm, xm, wm), hi = xParaHz(xm + wm, xm, wm);
        bordas(fotoX(xm), xA1 - 4, lo, hi);
        rotulosBaixo(fotoX(xm), xA1 - 4, lo, hi);
        // as marcas: M (vermelho) no VFO e a linha tracejada verde do outro receptor
        {
            const float xv = (float)hzParaX((double)r_->vfo(), xm, wm);
            if (xv >= xm && xv <= xm + wm) {
                const float fx = fotoX(xv);
                dl->AddRectFilled(Q(fx - 8, y0 + 3), Q(fx + 8, y0 + 18), IM_COL32(0xe0, 0x30, 0x28, 255), QD(3));
                tx(fL(), 10.f, fx, y0 + 10.5f, IM_COL32(0xff, 0xff, 0xff, 255), ativo_ == 0 ? "M" : "S", 0.5f);
            }
            if (!mono_) {
                const float xo = (float)hzParaX((double)rx_[1 - ativo_].f, xm, wm);
                if (xo >= xm && xo <= xm + wm)
                    for (float y = sy; y < sy + hEsp; y += QD(9)) dl->AddLine(ImVec2(xo, y), ImVec2(xo, std::min(sy + hEsp, y + QD(5))), IM_COL32(0x30, 0xe0, 0x50, 230), std::max(1.f, QD(1.6f)));
            }
        }
        const Receptor& R = rx_[ativo_];
        if ((R.nch || R.bef) && (r_->modo() == "USB" || r_->modo() == "LSB")) {
            const double nh = 150.0 * std::pow(3500.0 / 150.0, std::clamp(R.notch, 0.f, 1.f));
            const double fn = r_->modo() == "USB" ? (double)r_->vfo() + nh : (double)r_->vfo() - nh;
            const float xn = (float)hzParaX(fn, xm, wm);
            dl->AddLine(ImVec2(xn, sy), ImVec2(xn, sy + hEsp), alfa(P_CIANO, 200), std::max(1.f, QD(2.f)));
        }
        if (pausa_ && std::fmod(agora(), 1.0) < 0.6) tx(fL(), 13.f, (xA0 + xA1) * 0.5f, y0 + 30, P_AMBAR, "HOLD", 0.5f);
    }
    if (dual_) {   // o do outro receptor
        const int o = 1 - ativo_;
        Receptor& O = rx_[o];
        const float xB0 = dir ? 650.f : 1103.f, xB1 = dir ? 1090.f : 1542.f;
        dl->AddRect(Q(xB0, y0), Q(xB1, y1), IM_COL32(0x70, 0x72, 0x76, 255), QD(3), 0, std::max(1.f, QD(1.6f)));
        subCentro_ = (double)O.f;
        subSpan_ = span;
        const ImVec2 A = Q(xB0 + 4, y0 + 20), B = Q(xB1 - 4, y1 - 21);
        escopoSub(A.x, A.y, B.x - A.x, B.y - A.y);
        const double lo = subCentro_ - span / 2, hi = subCentro_ + span / 2;
        bordas(xB0 + 4, xB1 - 4, lo, hi);
        rotulosBaixo(xB0 + 4, xB1 - 4, lo, hi);
        {   // a sintonia dele (tracejado verde) e a marca
            const float xc = (A.x + B.x) * 0.5f, hs = std::floor((B.y - A.y) * 0.54f);
            for (float y = A.y; y < A.y + hs; y += QD(9)) dl->AddLine(ImVec2(xc, y), ImVec2(xc, std::min(A.y + hs, y + QD(5))), IM_COL32(0x30, 0xe0, 0x50, 230), std::max(1.f, QD(1.6f)));
            const float fx = fotoX(xc);
            dl->AddRectFilled(Q(fx - 8, y0 + 3), Q(fx + 8, y0 + 18), IM_COL32(0x20, 0xa0, 0x40, 255), QD(3));
            tx(fL(), 10.f, fx, y0 + 10.5f, IM_COL32(0xff, 0xff, 0xff, 255), o == 0 ? "M" : "S", 0.5f);
        }
        // clique sintoniza o outro receptor; duplo clique passa a ouvi-lo; a roda anda um passo
        ImGui::SetCursorScreenPos(A);
        ImGui::InvisibleButton("##subesc", ImVec2(std::max(1.f, B.x - A.x), std::max(1.f, B.y - A.y)));
        if (ImGui::IsItemHovered()) {
            ImGuiIO& io = ImGui::GetIO();
            const double hz = lo + (io.MousePos.x - A.x) / std::max(1.f, B.x - A.x) * span;
            dl->AddLine(ImVec2(io.MousePos.x, A.y), ImVec2(io.MousePos.x, B.y), IM_COL32(255, 255, 255, 60));
            ImGui::SetTooltip("%s  (%s)\nClique: sintoniza o %s aqui   Duplo clique: passa a ouvir o %s\nRoda do mouse: um passo",
                              fmtFreq((uint64_t)std::max(0.0, hz)).c_str(), o == 0 ? "MAIN" : "SUB", o == 0 ? "MAIN" : "SUB", o == 0 ? "MAIN" : "SUB");
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) ativar(o);
            else if (ImGui::IsItemClicked() && !O.lock) {
                const int64_t p = std::max(1, passo_);
                O.f = (uint64_t)std::max<int64_t>(1000, (int64_t)std::llround(hz / p) * p);
                O.memModo = false;
            }
            if (io.MouseWheel != 0 && !O.lock) {
                O.f = (uint64_t)std::max<int64_t>(1000, (int64_t)O.f + (io.MouseWheel > 0 ? passo_ : -passo_));
                io.MouseWheel = 0;
            }
        }
    }
    (void)lig;
}

// As teclas de toque embaixo do escopo (duas paginas: <MENU1> e <MENU2>)
void Ui::icTeclasSoft()
{
    static const float xs[7][2] = {{528, 664}, {676, 812}, {823, 958}, {970, 1106}, {1118, 1253}, {1265, 1402}, {1413, 1550}};
    const float y0 = 748, y1 = 786;
    auto k = [&](int i, const char* id, const char* t, bool on, const char* dica) { return teclaIc(id, t, xs[i][0], y0, xs[i][1], y1, on ? 3 : 0, nullptr, 0, dica, 2); };
    Receptor& R = rx_[ativo_];
    if (paginaSoft_ == 0) {
        if (k(0, "sk0", "<MENU1>", false, "<MENU1>: troca para a página 2 (NOTCH, A-NOTCH, CONT, BC, DECODE, IF)") == 1) paginaSoft_ = 1;
        if (k(1, "sk1", "EDGE", zoom_ > 0, "EDGE: largura do escopo (SPAN)") == 1) ImGui::OpenPopup("##spanIc");
        if (k(2, "sk2", "HOLD", pausa_, "HOLD: congela o espectro e a cachoeira") == 1) { pausa_ = !pausa_; if (pausa_) congelado_ = suave_; }
        if (k(3, "sk3", "CENT/FIX", modoCentro_, "CENT/FIX: o escopo acompanha a sintonia (CENT) ou fica parado (FIX)") == 1) {
            modoCentro_ = !modoCentro_;
            if (modoCentro_) r_->centralizar(r_->centroSemDc(r_->vfo()));
        }
        if (k(4, "sk4", "MAIN/SUB", ativo_ == 1, "MAIN/SUB: o escopo principal (e o dial) passam para o outro receptor") == 1) ativar(1 - ativo_);
        if (k(5, "sk5", "DUAL", dual_, "DUAL: dois escopos lado a lado (o do MAIN e o do SUB)") == 1) dual_ = !dual_;
        const int e = k(6, "sk6", "EXPD/SET", expand_, "EXPD/SET: escopo maior (esconde os S-meters)\nSegurar: ajustes do escopo (SET)");
        if (e == 1) expand_ = !expand_;
        if (e == 2) ImGui::OpenPopup("##smenuIc");
    } else {
        if (k(0, "sk0", "<MENU2>", false, "<MENU2>: volta para a página 1 (EDGE, HOLD, CENT/FIX, MAIN/SUB, DUAL, EXPD/SET)") == 1) paginaSoft_ = 0;
        {
            const int n = k(1, "sk1", "NOTCH", R.nch, "NOTCH: notch manual (posição no anel PITCH)\nSegurar: estreito / largo");
            if (n == 1) R.nch = !R.nch;
            if (n == 2) { R.nch = true; R.nchLargo = !R.nchLargo; aviso(R.nchLargo ? "NOTCH largo" : "NOTCH estreito"); }
        }
        if (k(2, "sk2", "A-NOTCH", R.anch, "A-NOTCH: notch automático (tira a portadora/apito sozinho)") == 1) R.anch = !R.anch;
        if (k(3, "sk3", "CONT", R.bef, "CONT: contorno - abaixa uma faixa do áudio (posição no anel PITCH)") == 1) R.bef = !R.bef;
        if (k(4, "sk4", "BC", R.bc, "BC: cancelador de vários apitos") == 1) R.bc = !R.bc;
        if (k(5, "sk5", "DECODE", decAberta_, "DECODE: os decodificadores digitais (RTTY, PSK, FT8, CW, DMR, ADS-B...)") == 1) decAberta_ = !decAberta_;
        if (k(6, "sk6", "IF SCOPE", ifOn_, "IF SCOPE: zoom de 20 kHz em volta da sintonia, dentro do escopo") == 1) ifOn_ = !ifOn_;
    }
}

// As teclas embaixo da tela
void Ui::icTeclasBaixo()
{
    const float y0 = 878, y1 = 905, yr = 864;
    if (teclaIc("bMenu", "", 513, y0, 624, y1, 0, "MENU", yr, "MENU: funções e configuração (aparelho, decodificadores, memórias, gravador, GEN...)", 0) == 1)
        ImGui::OpenPopup("##funcFt");
    {
        const int k = teclaIc("bMscope", "", 657, y0, 766, y1, 0, "M.SCOPE", yr, "M.SCOPE: o escopo na janela inteira (ESC volta)\nSegurar: IF SCOPE (zoom de 20 kHz)", 0);
        if (k == 1) scp_ = 2;
        if (k == 2) ifOn_ = !ifOn_;
    }
    {
        char d[200];
        std::snprintf(d, sizeof d, "MP-W (memo pad): grava a frequência atual nas memórias rápidas (%d de 10)", (int)qm_.size());
        if (teclaIc("bMpw", "", 800, y0, 908, y1, 0, "MP-W", yr, d, 0) == 1) {
            qm_.insert(qm_.begin(), Rapida{r_->vfo(), r_->modo()});
            if (qm_.size() > 10) qm_.resize(10);
            qmIdx_ = -1;
            aviso("Gravado no MEMO PAD");
        }
    }
    if (teclaIc("bMpr", "", 943, y0, 1051, y1, 0, "MP-R", yr, "MP-R (memo pad): chama as memórias rápidas, uma por vez", 0) == 1) {
        if (qm_.empty()) aviso("MEMO PAD vazio (MP-W grava)");
        else {
            qmIdx_ = (qmIdx_ + 1) % (int)qm_.size();
            rx_[ativo_].memModo = false;
            mudarModoPainel(qm_[qmIdx_].modo);
            sintonizar(qm_[qmIdx_].f, false);
            char a[48]; std::snprintf(a, sizeof a, "MEMO PAD %d", qmIdx_ + 1); aviso(a);
        }
    }
    if (teclaIc("bAtune", "", 1085, y0, 1196, y1, 0, "AUTO TUNE", yr, "AUTO TUNE: sintonia automática no pico mais próximo (CW / AM / FM)", 0) == 1) cwTune();
    if (teclaIc("bQuick", "", 1227, y0, 1336, y1, 0, "QUICK", yr, "QUICK: menu rápido (memórias, passo, escopo, GEN)", 0) == 1) ImGui::OpenPopup("##quickIc");
    if (teclaIc("bExit", "", 1371, y0, 1478, y1, 0, "EXIT", yr, "EXIT: cancela a digitação e volta a tela ao normal", 0) == 1) {
        entrando_ = false; entrada_.clear(); expand_ = false; paginaSoft_ = 0;
    }
    const bool grav = r_->som().gravando();
    if (teclaIc("bRec", "", 1513, y0, 1583, y1, 0, "REC", yr, grav ? "REC: parar a gravação" : "REC: gravar o áudio (pasta Gravacoes)", 0) == 1) alternarGravacao();
    if (grav) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (std::fmod(agora(), 1.0) < 0.6) dl->AddCircleFilled(Q(1548, 891.5f), QD(5), IM_COL32(0xff, 0x30, 0x28, 255), 16);
    }
    {
        const int k = teclaIc("bPlay", "", 1603, y0, 1670, y1, 0, "PLAY", yr, "PLAY: toca a última gravação\nSegurar: para de tocar", 0);
        if (k == 1) tocarUltimaGravacao();
        if (k == 2) { PlaySoundA(nullptr, nullptr, 0); aviso("PLAY parado"); }
    }
}

// ===========================================================================
//  MB1SDR: o lado direito (MULTI, bandas, dial, TWIN-PBT, PITCH, RIT, SPEECH)
// ===========================================================================
void Ui::icDireita()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 BR = IM_COL32(0xf4, 0xf4, 0xf4, 255);
    Receptor& A = rx_[ativo_];
    Receptor& O = rx_[1 - ativo_];
    // o "delta" dos rotulos RIT/(delta)TX (o triangulo e desenhado: nem toda fonte tem a letra)
    auto rotDelta = [&](float cx, float yc, const char* antes, const char* depois, float cap) {
        const float wa = txW(fP(), cap, antes, 0.94f), wb = txW(fP(), cap, depois, 0.94f), wd = cap * 0.95f;
        float x = cx - (wa + wd + wb + 2.f) * 0.5f;
        tx(fP(), cap, x, yc, BR, antes, 0.f, 0.94f);
        x += wa + 1.f;
        const float t = std::max(1.f, QD(1.8f));
        dl->AddTriangle(Q(x + 1, yc + cap * 0.5f), Q(x + wd - 1, yc + cap * 0.5f), Q(x + wd * 0.62f, yc - cap * 0.5f), BR, t);
        x += wd + 1.f;
        tx(fP(), cap, x, yc, BR, depois, 0.f, 0.94f);
    };
    // ---- MULTI ----
    {
        dl->AddCircle(Q(1633, 207), QD(6), BR, 16, std::max(1.f, QD(2.f)));
        dl->AddCircleFilled(Q(1633, 207), QD(2.6f), BR, 10);
        tx(fP(), 12.5f, 1643, 207, BR, "MULTI", 0.f, 0.94f);
        bool clique = false;
        char d[220];
        std::snprintf(d, sizeof d, "MULTI: %s (gire)\nClique: escolher a função (LEVEL, SPEED, STEP DIAL, MEM CH, DNR, NB, AF...)", nomeFuncaoMulti(multiFunc_));
        const int p = encoderIc("eMultiIc", 1666, 280, 47, angMulti_, d, &clique);
        if (clique) ImGui::OpenPopup("##multiIc");
        if (p) funcaoMulti(multiFunc_, p);
    }
    // ---- teclas de banda (vidro preto) ----
    auto banda = [&](uint64_t f) { for (int k = 0; k < kNB; ++k) if (f >= kBandasHam[k].lo && f <= kBandasHam[k].hi) return k; return -1; };
    const int bAt = banda(r_->vfo());
    static const char* rotB[3][3] = {{"1.8", "3.5", "7"}, {"10", "14", "18"}, {"21", "24", "28"}};
    const float cx0[3] = {1747, 1858, 1970}, ry0[4] = {203, 278, 353, 428};
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) {
            const int bi = kBandaIc[r][c];
            char id[12], d[160];
            std::snprintf(id, sizeof id, "bB%d", bi);
            std::snprintf(d, sizeof d, "Banda de %s MHz para o %s (volta onde estava nesta banda)", kBandasHam[bi].rot, ativo_ == 0 ? "MAIN" : "SUB");
            if (teclaIc(id, rotB[r][c], cx0[c], ry0[r], cx0[c] + 96, ry0[r] + 40, bAt == bi ? 6 : 0, nullptr, 0, d, 1, 0.f) == 1) irBanda(bi);
        }
    if (teclaIc("bGene", "GENE", cx0[0], ry0[3], cx0[0] + 96, ry0[3] + 40, (bAt < 0 || bAt == 2 || bAt == 11) ? 6 : 0, nullptr, 0,
                "GENE: cobertura geral (5 MHz, 70 MHz, ondas curtas e médias, VHF, UHF)", 1, 0.f) == 1)
        ImGui::OpenPopup("##genIc");
    if (teclaIc("bB10", "50", cx0[1], ry0[3], cx0[1] + 96, ry0[3] + 40, bAt == 10 ? 6 : 0, nullptr, 0, "Banda de 50 MHz (6 m)", 1, 0.f) == 1) irBanda(10);
    {
        const int k = teclaIc("bMs", "MAIN/SUB", cx0[2], ry0[3], cx0[2] + 96, ry0[3] + 40, 0, nullptr, 0,
                              "MAIN/SUB: o dial, o som e os botões passam para o outro receptor\nSegurar: troca as frequências do MAIN e do SUB", 1);
        if (k == 1) ativar(1 - ativo_);
        if (k == 2) { trocarMainSub(); aviso("MAIN <> SUB"); }
    }
    {
        const bool split = txVfo_ != ativo_;
        const int k = teclaIc("bSplit", "SPLIT", 1625, ry0[2], 1712, ry0[2] + 40, split ? 3 : 0, nullptr, 0,
                              "SPLIT: escuta num receptor e marca o outro como \"TX\"\nSegurar: QUICK SPLIT (o outro receptor vai 5 kHz acima)", 1);
        if (k == 1) txVfo_ = split ? ativo_ : 1 - ativo_;
        if (k == 2) { sincronizarAtivo(); rx_[1 - ativo_].f = r_->vfo() + 5000; rx_[1 - ativo_].modo = r_->modo(); txVfo_ = 1 - ativo_; aviso("QUICK SPLIT +5 kHz"); }
    }
    {
        const int k = teclaIc("bDualw", "DUAL-W", 1625, ry0[3], 1712, ry0[3] + 40, tracking_ ? 2 : 0, nullptr, 0,
                              "DUAL-W: o SUB anda junto com o MAIN quando se gira o dial\nSegurar: o SUB vai para a frequência do MAIN", 1);
        if (k == 1) tracking_ = !tracking_;
        if (k == 2) {
            sincronizarAtivo(); rx_[1].f = rx_[0].f; rx_[1].modo = rx_[0].modo; rx_[1].fil = rx_[0].fil;
            if (ativo_ == 1) { r_->setModo(rx_[1].modo); sintonizar(rx_[1].f, false); aplicarFiltro(); }
            aviso("SUB = MAIN");
        }
    }
    // ---- XFC e CHANGE ----
    tx(fP(), 12.5f, 1663, 505, BR, "XFC", 0.5f, 0.94f);
    {
        bool seg = false;
        botaoRedondo("bXfc", 1663, 558, 37, "XFC: enquanto segura, ouve o outro receptor (o de \"TX\")", &seg);
        if (seg && xfcVolta_ < 0) { xfcVolta_ = ativo_; ativar(1 - ativo_); }
        if (!seg && xfcVolta_ >= 0) { ativar(xfcVolta_); xfcVolta_ = -1; }
    }
    tx(fP(), 12.5f, 2040, 505, BR, "CHANGE", 0.5f, 0.94f);
    if (botaoRedondo("bChange", 2040, 560, 37, "CHANGE: troca as frequências do MAIN e do SUB (frequência, modo e filtro)", nullptr) == 1) {
        trocarMainSub();
        aviso("MAIN <> SUB");
    }
    // ---- o rotulo MAIN/SUB com os dois LEDs e o dial ----
    {
        tx(fP(), 12.5f, 1850, 514, BR, "MAIN/SUB", 0.5f, 0.94f);
        for (int i = 0; i < 2; ++i) {
            const ImVec2 c = Q(i == 0 ? 1789.f : 1902.f, 514);
            const bool on = ativo_ == i;
            if (on) dl->AddCircleFilled(c, QD(9), IM_COL32(255, 255, 255, 50), 16);
            dl->AddCircleFilled(c, QD(5.5f), on ? IM_COL32(0xf8, 0xf8, 0xfc, 255) : IM_COL32(0x22, 0x24, 0x26, 255), 16);
            dl->AddCircle(c, QD(5.5f), IM_COL32(0x70, 0x72, 0x76, 200), 16, 1.f);
        }
        const int p = dialIc("dIc", 1853, 745, 166, angDialA_, A.lock,
                             "Dial: sintoniza o receptor ouvido (arraste girando ou use a roda do mouse)\nGirar rápido acelera. Passo no MULTI (STEP DIAL)");
        if (p) sintonizarRx(ativo_, p);
    }
    // ---- TWIN-PBT (SHIFT no miolo, WIDTH no anel) e o CLR ----
    {
        tx(fP(), 12.5f, 2194, 205, BR, "TWIN-PBT", 0.5f, 0.94f);
        const ImVec2 a = Q(2241, 197), b = Q(2276, 213);
        dl->AddRectFilled(a, b, IM_COL32(0xd0, 0xd2, 0xd6, 255), QD(3));
        tx(fP(), 8.5f, 2258.5f, 205, IM_COL32(0x20, 0x22, 0x26, 255), "CLR", 0.5f);
        const std::string md = r_->modo();
        const int g = grupoModo(md);
        float& hi = filHi_[g][A.fil];
        float& lo = filLo_[g][A.fil];
        ImGui::SetCursorScreenPos(a);
        if (ImGui::InvisibleButton("##clrpbt", ImVec2(std::max(1.f, b.x - a.x), std::max(1.f, b.y - a.y)))) {
            static const float hi0[5][3] = {{2700, 2400, 3600}, {500, 250, 1200}, {4000, 3000, 5000}, {12500, 10000, 25000}, {200000, 150000, 250000}};
            static const float lo0[5][3] = {{200, 400, 100}, {0, 0, 0}, {200, 250, 100}, {0, 0, 0}, {0, 0, 0}};
            hi = hi0[g][A.fil]; lo = lo0[g][A.fil];
            aplicarFiltro();
            aviso("TWIN-PBT: filtro de volta ao padrão");
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("CLR: o filtro volta ao padrão");
        char d[240];
        float tIn = 0.5f, tOut = 0.5f;
        switch (g) {
        case 0: std::snprintf(d, sizeof d, "TWIN-PBT: miolo = SHIFT (desloca o filtro: %.2f a %.2f kHz)\nanel = WIDTH (largura %.2f kHz)  - FIL%d", lo / 1000.f, hi / 1000.f, (hi - lo) / 1000.f, A.fil + 1);
                tIn = (lo + hi) * 0.5f / 3000.f; tOut = (hi - lo - 400.f) / 4600.f; break;
        case 1: std::snprintf(d, sizeof d, "TWIN-PBT: miolo = SHIFT (%+.0f Hz)\nanel = WIDTH do CW (%.0f Hz)  - FIL%d", lo, hi, A.fil + 1);
                tIn = (lo + 800) / 1600.f; tOut = (hi - 50) / 2450.f; break;
        case 2: std::snprintf(d, sizeof d, "TWIN-PBT: miolo = corte baixo do AM (%.2f kHz)\nanel = corte alto (%.1f kHz)  - FIL%d", lo / 1000.f, hi / 1000.f, A.fil + 1);
                tIn = lo / 1000.f; tOut = (hi - 2000) / 6000.f; break;
        case 3: std::snprintf(d, sizeof d, "TWIN-PBT: largura do FM (%.1f kHz)  - FIL%d", hi / 1000.f, A.fil + 1);
                tOut = (hi - 6000) / 24000.f; tIn = tOut; break;
        default: std::snprintf(d, sizeof d, "TWIN-PBT: largura do FM largo (%.0f kHz)  - FIL%d", hi / 1000.f, A.fil + 1);
                tOut = (hi - 100000) / 150000.f; tIn = tOut; break;
        }
        float di, dO;
        if (giroIc("gPbt", 2213, 290, 62, std::clamp(tIn, 0.f, 1.f), std::clamp(tOut, 0.f, 1.f), di, dO, d, true, false)) {
            switch (g) {
            case 0: {
                float s = di * 3000.f;
                s = std::clamp(s, -lo, 5000.f - hi);
                lo += s; hi += s;
                hi = std::clamp(hi + dO * 4600.f, lo + 400.f, 5000.f);
                break; }
            case 1: lo = std::clamp(lo + di * 1600.f, -800.f, 800.f); hi = std::clamp(hi + dO * 2450.f, 50.f, 2500.f); break;
            case 2: lo = std::clamp(lo + di * 1000.f, 0.f, 1000.f); hi = std::clamp(hi + dO * 6000.f, 2000.f, 8000.f); break;
            case 3: hi = std::clamp(hi + (di + dO) * 24000.f, 6000.f, 30000.f); break;
            default: hi = std::clamp(hi + (di + dO) * 150000.f, 100000.f, 250000.f); break;
            }
            aplicarFiltro();
        }
    }
    // ---- FILTER e APF/TPF ----
    {
        char d[160];
        std::snprintf(d, sizeof d, "FILTER: troca o filtro FIL1 / FIL2 / FIL3 (agora FIL%d)\nLED: verde = FIL1, laranja = FIL2, vermelho = FIL3\nSegurar: escolher a largura (BW)", A.fil + 1);
        const int k = teclaIc("bFilter", "", 2103, 403, 2197, 426, A.fil == 0 ? 2 : A.fil == 1 ? 3 : 4, "FILTER", 383, d, 0);
        if (k == 1) { A.fil = (A.fil + 1) % 3; aplicarFiltro(); char a[24]; std::snprintf(a, sizeof a, "FIL%d", A.fil + 1); aviso(a, 1.0); }
        if (k == 2) ImGui::OpenPopup("##bwIc");
    }
    {
        const int k = teclaIc("bApf", "", 2222, 403, 2312, 426, A.apf ? 3 : 1, "APF/TPF", 383,
                              "APF: filtro de pico de áudio para CW (centro no PITCH)\nSegurar: NOTCH manual", 0);
        if (k == 1) A.apf = !A.apf;
        if (k == 2) { A.nch = !A.nch; aviso(A.nch ? "NOTCH ligado" : "NOTCH desligado"); }
    }
    // ---- KEY SPEED (tom do audio) - PITCH (CW / NOTCH / CONT) ----
    {
        rotuloDuploIc(2207, 466, 12.5f, BR, "KEY SPEED", "PITCH");
        const bool cw = grupoModo(r_->modo()) == 1;
        float tOut;
        char d[300];
        if (cw) tOut = (cwPitch_ - 300.f) / 700.f;
        else tOut = A.nch ? A.notch : A.cont;
        const double nh = 150.0 * std::pow(3500.0 / 150.0, std::clamp(A.notch, 0.f, 1.f));
        const double ch = 150.0 * std::pow(3500.0 / 150.0, std::clamp(A.cont, 0.f, 1.f));
        std::snprintf(d, sizeof d, "KEY SPEED (miolo): tonalidade do áudio (%s %d%%)\nPITCH (anel): %s",
                      tom_ < 45 ? "grave" : tom_ > 55 ? "agudo" : "plano", (int)std::lround(std::fabs(tom_ - 50) * 2),
                      cw ? (std::string("tom do CW e centro do APF (") + std::to_string((int)cwPitch_) + " Hz)").c_str()
                         : A.nch ? (std::string("posição do NOTCH (") + std::to_string((int)std::lround(nh)) + " Hz)").c_str()
                                 : (std::string("posição do CONT (") + std::to_string((int)std::lround(ch)) + " Hz) - girar liga o CONT").c_str());
        float di, dO;
        if (giroIc("gPitch", 2215, 553, 60, tom_ / 100.f, std::clamp(tOut, 0.f, 1.f), di, dO, d, true, true)) {
            tom_ = std::clamp(tom_ + di * 100.f, 0.f, 100.f);
            if (di != 0) rxeq_ = 0;
            if (dO != 0) {
                if (cw) cwPitch_ = std::clamp(cwPitch_ + dO * 700.f, 300.f, 1000.f);
                else if (A.nch) A.notch = std::clamp(A.notch + dO, 0.f, 1.f);
                else { A.cont = std::clamp(A.cont + dO, 0.f, 1.f); if (!A.bef) { A.bef = true; aviso("CONT ligado"); } }
            }
        }
    }
    // ---- RIT, (delta)TX, CLEAR e o botao RIT/(delta)TX ----
    {
        tx(fP(), 12.5f, 2133, 654, BR, "RIT", 0.5f, 0.94f);
        rotDelta(2208, 654, "", "TX", 12.5f);
        tx(fP(), 12.5f, 2283, 654, BR, "CLEAR", 0.5f, 0.94f);
        if (teclaIc("bRit", "", 2103, 674, 2163, 697, 0, nullptr, 0, "RIT: liga o deslocamento da escuta do receptor ouvido (valor no botão RIT/TX)", 0) == 1) A.ritOn = !A.ritOn;
        if (teclaIc("bDtx", "", 2178, 674, 2238, 697, 0, nullptr, 0, "TX: (num receptor) liga o deslocamento do OUTRO receptor", 0) == 1) O.ritOn = !O.ritOn;
        const int k = teclaIc("bClear", "", 2253, 674, 2313, 697, 0, nullptr, 0, "CLEAR: zera o RIT\nSegurar: zera os dois receptores", 0);
        if (k == 1) { A.rit = 0; aviso("RIT zerado", 1.0); }
        if (k == 2) { A.rit = 0; O.rit = 0; aviso("RIT dos dois zerado"); }
        if (A.ritOn) dl->AddRectFilled(Q(2122, 668), Q(2144, 671), IM_COL32(0xff, 0x9a, 0x30, 255));
        if (O.ritOn) dl->AddRectFilled(Q(2197, 668), Q(2219, 671), IM_COL32(0xff, 0x9a, 0x30, 255));
        rotDelta(2210, 745, "RIT/", "TX", 12.5f);
        char d[160];
        std::snprintf(d, sizeof d, "RIT/TX: deslocamento da escuta do %s (%+.3f kHz)\nGirar liga o RIT; CLEAR zera", ativo_ == 0 ? "MAIN" : "SUB", A.rit / 1000.0);
        float di, dO;
        if (giroIc("gRitIc", 2217, 833, 58, (A.rit + 9990.f) / 19980.f, 0.f, di, dO, d, false, false)) {
            static float acc = 0;
            acc += (di + dO) * 2000.f;
            const int pas = (int)(acc / 10.f);
            if (pas != 0) { A.rit = std::clamp(A.rit + pas * 10, -9990, 9990); acc -= pas * 10.f; A.ritOn = true; }
        }
    }
    // ---- SPEECH (com a chave: segurar trava o dial) ----
    {
        const float ws = txW(fP(), 12.5f, "SPEECH", 0.94f), xs = 2041.f - (ws + 36.f) * 0.5f, xk = xs + ws + 5.f;
        tx(fP(), 12.5f, xs, 865, BR, "SPEECH", 0.f, 0.94f);
        dl->AddRectFilled(Q(xk, 857), Q(xk + 31, 874), IM_COL32(0xe0, 0xe2, 0xe6, 255), QD(3));
        const ImU32 k = IM_COL32(0x22, 0x24, 0x28, 255);
        dl->AddCircle(Q(xk + 9, 865.5f), QD(3.5f), k, 12, std::max(1.f, QD(1.8f)));
        dl->AddLine(Q(xk + 13, 865.5f), Q(xk + 25, 865.5f), k, std::max(1.f, QD(2.f)));
        dl->AddLine(Q(xk + 22, 865.5f), Q(xk + 22, 869.5f), k, std::max(1.f, QD(1.8f)));
        const int s = teclaIc("bSpeech", "", 1975, 880, 2108, 905, 0, nullptr, 0,
                              A.lock ? "SPEECH: fala a frequência (voz do Windows)\nSegurar: DESTRAVA o dial (LOCK)"
                                     : "SPEECH: fala a frequência (voz do Windows)\nSegurar: trava o dial (LOCK)", 0);
        if (s == 1) falarFrequencia(ativo_);
        if (s == 2) { A.lock = !A.lock; aviso(A.lock ? "LOCK: dial travado" : "LOCK: dial livre"); }
    }
}

// Os menus que as teclas do MB1 abrem
void Ui::icPopups()
{
    auto& c = Config::instance();
    Receptor& R = rx_[ativo_];
    const std::string m = r_->modo();
    auto itensScope = [&]() {
        int v = (int)std::lround(wfSpeedEsc_);
        ImGui::SetNextItemWidth(220 * s_);
        if (ImGui::SliderInt("SPEED (velocidade)", &v, 1, 10)) wfSpeedEsc_ = (float)v;
        float lv = wfBrilho_ - 100.f;
        ImGui::SetNextItemWidth(220 * s_);
        if (ImGui::SliderFloat("LEVEL (nível)", &lv, -100.f, 100.f, "%.0f")) { wfBrilho_ = lv + 100.f; autoWf_ = false; }
        ImGui::SetNextItemWidth(220 * s_);
        if (ImGui::SliderFloat("CONTRAST (faixa dB)", &wfRange_, 20.f, 120.f, "%.0f dB")) autoWf_ = false;
        if (ImGui::BeginMenu("COLOR (cores da cachoeira)")) {
            const char* nm[6] = {"Eclipse", "Oceano", "Clássica", "Cinza", "Azul escuro", "MB1 (azul)"};
            for (int i = 0; i < 6; ++i)
                if (ImGui::MenuItem(nm[i], nullptr, paleta_ == i)) { paleta_ = i; g_paletaCachoeira = i; lutRef_ = 1e9f; lutRange_ = 1e9f; }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Espectro preenchido (desligado = só a linha dos picos)", nullptr, espCheio_)) espCheio_ = !espCheio_;
        if (ImGui::MenuItem("MARKER (nomes das memórias no escopo)", nullptr, memVisivel_)) memVisivel_ = !memVisivel_;
        if (ImGui::MenuItem("DUAL (dois escopos)", nullptr, dual_)) dual_ = !dual_;
        if (ImGui::MenuItem("Marca do outro receptor no escopo", nullptr, !mono_)) mono_ = !mono_;
        if (ImGui::MenuItem("AUTO (ajuste automático)", nullptr, autoWf_)) autoCachoeira();
        if (ImGui::MenuItem("Guardar a aparência como padrão")) padrao();
    };
    if (ImGui::BeginPopup("##smenuIc")) {
        ImGui::TextDisabled("SCOPE SET");
        ImGui::Separator();
        itensScope();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##quickIc")) {
        ImGui::TextDisabled("QUICK MENU");
        ImGui::Separator();
        if (ImGui::MenuItem("Memórias: lista e gravar (V>M)")) { memAberta_ = true; std::snprintf(memNome_, sizeof memNome_, "%s %s", fmtFreq(r_->vfo()).c_str(), r_->modo().c_str()); }
        if (ImGui::MenuItem("MEMO: o dial passa os canais de memória (V/M)", nullptr, R.memModo)) {
            if (mem_.empty()) aviso("Nenhuma memória gravada");
            else {
                R.memModo = !R.memModo;
                if (R.memModo) {
                    int mais = 0; long long melhor = LLONG_MAX;
                    for (int i = 0; i < (int)mem_.size(); ++i) {
                        const long long dd = std::llabs((long long)mem_[i].f - (long long)r_->vfo());
                        if (dd < melhor) { melhor = dd; mais = i; }
                    }
                    R.memCanal = mais; sintonizarMemoria((size_t)mais);
                }
            }
        }
        if (ImGui::BeginMenu("Passo (STEP DIAL e roda do mouse no escopo)")) {
            const int ps[] = {10, 50, 100, 500, 1000, 2500, 5000, 6250, 8330, 9000, 10000, 12500, 25000, 100000};
            for (int p : ps) if (ImGui::MenuItem(fmtPasso(p).c_str(), nullptr, passo_ == p)) passo_ = p;
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("FAST: dial 10x mais rápido", nullptr, fast_)) fast_ = !fast_;
        if (ImGui::MenuItem("FINE: dial em passos de 1 Hz", nullptr, R.fine)) R.fine = !R.fine;
        if (ImGui::BeginMenu("SCOPE")) { itensScope(); ImGui::EndMenu(); }
        if (ImGui::BeginMenu("GENE - cobertura geral HF")) { itensGen(0); ImGui::EndMenu(); }
        if (ImGui::BeginMenu("GENE - VHF / UHF")) { itensGen(1); ImGui::EndMenu(); }
        if (ImGui::MenuItem("Decodificadores digitais (DECODE)")) decAberta_ = true;
        if (ImGui::MenuItem("Configuração (aparelho, som, S-meter)")) abrirConfig();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##timerIc")) {
        ImGui::TextDisabled("TIMER: desligar o rádio depois de");
        const int mins[] = {15, 30, 60, 90, 120};
        for (int mm : mins) {
            char r[32]; std::snprintf(r, sizeof r, "%d minutos", mm);
            if (ImGui::Selectable(r)) { sleepAte_ = agora() + mm * 60.0; aviso(std::string("TIMER: ") + r); }
        }
        if (ImGui::Selectable("Desligado (cancelar)", sleepAte_ <= 0)) sleepAte_ = 0;
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##saidaIc")) {
        ImGui::TextDisabled("Saída de som (PHONES)");
        if (ImGui::Selectable("Padrão do Windows", c.str("audio_saida").empty())) { ra_->trocarSaida(""); rb_->trocarSaida(""); }
        for (const auto& s : saidas_)
            if (ImGui::Selectable(s.c_str(), c.str("audio_saida") == s)) { ra_->trocarSaida(s); rb_->trocarSaida(s); }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##modosIc")) {
        ImGui::TextDisabled("MODE");
        const char* ms[6] = {"LSB", "USB", "CW", "AM", "NFM", "WFM"};
        const char* nm[6] = {"LSB", "USB", "CW", "AM", "FM (estreito)", "WFM (FM largo, radiodifusão)"};
        for (int i = 0; i < 6; ++i) if (ImGui::Selectable(nm[i], m == ms[i])) mudarModoPainel(ms[i]);
        ImGui::Separator();
        if (ImGui::Selectable("DATA - decodificadores digitais...")) decAberta_ = true;
        if (ImGui::Selectable("RTTY")) { escolherDecoder(Decoders::RTTY, true); decAberta_ = true; }
        if (ImGui::Selectable("PSK (BPSK31/63/125...)")) { escolherDecoder(Decoders::BPSK, true); decAberta_ = true; }
        if (ImGui::Selectable("CW (decodificador)")) { escolherDecoder(Decoders::CW, true); decAberta_ = true; }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##bwIc")) {
        const int g = grupoModo(m);
        float& hi = filHi_[g][R.fil];
        const float lo = filLo_[g][R.fil];
        const float bwAtual = g == 2 ? hi * 2.f : hi;
        static const float kSsb[] = {1800, 2100, 2400, 2700, 3000, 3600, 4000, 5000};
        static const float kCw[] = {50, 100, 200, 300, 500, 800, 1000, 2000};
        static const float kAm[] = {4000, 6000, 8000, 10000, 12000, 16000};
        static const float kFm[] = {6000, 8000, 10000, 12500, 15000, 20000, 25000};
        static const float kWfm[] = {100000, 150000, 180000, 200000, 250000};
        const float* l = g == 0 ? kSsb : g == 1 ? kCw : g == 2 ? kAm : g == 3 ? kFm : kWfm;
        const int n = g == 0 ? 8 : g == 1 ? 8 : g == 2 ? 6 : g == 3 ? 7 : 5;
        ImGui::TextDisabled("BW - largura (%s, FIL%d)", m.c_str(), R.fil + 1);
        for (int i = 0; i < n; ++i) {
            char t[32];
            if (l[i] >= 1000) std::snprintf(t, sizeof t, "%.1f kHz", l[i] / 1000.f); else std::snprintf(t, sizeof t, "%.0f Hz", l[i]);
            if (ImGui::Selectable(t, std::fabs(bwAtual - l[i]) < 1.f)) {
                hi = g == 2 ? l[i] / 2.f : l[i];
                if (g == 0) hi = std::max(hi, lo + 400.f);
                aplicarFiltro();
            }
        }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##spanIc")) {
        ImGui::TextDisabled("EDGE / SPAN (largura do escopo)");
        const float z[6] = {0, 1, 3, 7, 15, 31};
        for (float zz : z) {
            const double span = r_->taxa() / (1.0 + zz);
            char t[32];
            if (span >= 1e6) std::snprintf(t, sizeof t, "%.2f MHz", span / 1e6); else std::snprintf(t, sizeof t, "%.0f kHz", span / 1000.0);
            if (ImGui::Selectable(t, std::fabs(zoom_ - zz) < 0.3f)) { zoom_ = zz; centrarParaZoom(true); }
        }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##multiIc")) {
        ImGui::TextDisabled("Função do botão MULTI");
        for (int k = 0; k < 9; ++k) if (ImGui::Selectable(nomeFuncaoMulti(k), multiFunc_ == k)) multiFunc_ = k;
        ImGui::Separator();
        ImGui::TextDisabled("Passo do STEP DIAL e da roda do mouse no escopo");
        const int ps[] = {10, 50, 100, 500, 1000, 2500, 5000, 6250, 8330, 9000, 10000, 12500, 25000, 100000};
        for (int p : ps) if (ImGui::Selectable(fmtPasso(p).c_str(), passo_ == p)) passo_ = p;
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##genIc")) {
        if (ImGui::Selectable("5 MHz - banda de 60 m")) irBanda(2);
        if (ImGui::Selectable("70 MHz - banda de 4 m")) irBanda(11);
        ImGui::Separator();
        if (ImGui::BeginMenu("GENE - HF (ondas curtas, médias, utilitárias)")) { itensGen(0); ImGui::EndMenu(); }
        if (ImGui::BeginMenu("GENE - VHF / UHF")) { itensGen(1); ImGui::EndMenu(); }
        ImGui::EndPopup();
    }
}

// ===========================================================================
//  MB1SDR: o painel do MB1. Tudo medido na foto do radio
//  (mb1.jpg, 1900 x 721 px): x 6..1894 e y 3..673 da foto viram o
//  papel 1500 x 532. As funcoes ft9*() recebem as posicoes da FOTO.
// ===========================================================================
namespace {

// t (0..1) de um botao a partir do angulo (graus: 0 = para cima, + = horario, -135..135)
inline float tAng(float graus) { return std::clamp((graus + 135.f) / 270.f, 0.f, 1.f); }

// mostrador de 7 segmentos (o VFD): bits a=1 b=2 c=4 d=8 e=16 f=32 g=64; 10 = '-'
const unsigned char kSeg7[11] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x40};

// um digito: canto de cima x,y (foto), largura w, altura h, espessura t, inclinacao incl
void seg7(ImDrawList* dl, float x, float y, float w, float h, int dig, ImU32 on, ImU32 off, float t, float incl)
{
    const unsigned m = (dig >= 0 && dig <= 10) ? kSeg7[dig] : 0u;
    const bool temOff = ((off >> IM_COL32_A_SHIFT) & 0xff) > 0;
    const float g = t * 0.22f, s = t * 0.5f, yb = y + h;
    auto P = [&](float px, float py) { return Q(px + (yb - py) * incl, py); };
    auto hor = [&](float x1, float x2, float yy, bool aceso) {
        if (!aceso && !temOff) return;
        ImVec2 p[6] = {P(x1, yy), P(x1 + s, yy - s), P(x2 - s, yy - s), P(x2, yy), P(x2 - s, yy + s), P(x1 + s, yy + s)};
        dl->AddConvexPolyFilled(p, 6, aceso ? on : off);
    };
    auto ver = [&](float xx, float y1, float y2, bool aceso) {
        if (!aceso && !temOff) return;
        ImVec2 p[6] = {P(xx, y1), P(xx + s, y1 + s), P(xx + s, y2 - s), P(xx, y2), P(xx - s, y2 - s), P(xx - s, y1 + s)};
        dl->AddConvexPolyFilled(p, 6, aceso ? on : off);
    };
    const float xl = x + s, xr = x + w - s, yt = y + s, ym = y + h * 0.5f, ybb = y + h - s;
    hor(xl + g, xr - g, yt, (m & 1) != 0);
    ver(xr, yt + g, ym - g, (m & 2) != 0);
    ver(xr, ym + g, ybb - g, (m & 4) != 0);
    hor(xl + g, xr - g, ybb, (m & 8) != 0);
    ver(xl, ym + g, ybb - g, (m & 16) != 0);
    ver(xl, yt + g, ym - g, (m & 32) != 0);
    hor(xl + g, xr - g, ym, (m & 64) != 0);
}

// a frequencia em 7 segmentos alinhada a direita em xDir: MHz (sem zeros a esquerda) . kHz . Hz
struct Dig7 { float x0, x1; uint64_t peso; };
void freq7(ImDrawList* dl, uint64_t hz, float xDir, float y, float w, float h, float passo, float gap, ImU32 on, ImU32 off,
           float t, float incl, std::vector<Dig7>* caixas, int nMax = 9)
{
    const int nMhz = hz >= 100000000ULL ? 3 : hz >= 10000000ULL ? 2 : 1;
    const int n = std::min(nMax, 6 + nMhz);
    float x = xDir;
    uint64_t peso = 1;
    for (int i = 0; i < n; ++i) {
        if (i == 3 || i == 6) {            // o ponto (entre os grupos)
            x -= gap;
            const float px = x + gap * 0.20f, py = y + h - t * 0.95f, d = t * 0.95f;
            const float sh = t * incl;
            ImVec2 p[4] = {Q(px + sh, py), Q(px + d + sh, py), Q(px + d, py + d), Q(px, py + d)};
            dl->AddConvexPolyFilled(p, 4, on);
        }
        x -= passo;
        seg7(dl, x, y, w, h, (int)((hz / peso) % 10), on, off, t, incl);
        if (caixas) caixas->push_back({x, x + w, peso});
        peso *= 10;
    }
}

// texto com a seta do radio no meio (A>B, B>A, A<>B, M>A, A>M): tipo 0 = seta, 1 = dupla
void txSeta(float cx, float yc, float cap, ImU32 cor, const char* a, const char* b, int tipo)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float wa = txW(fP(), cap, a, 0.95f), wb = txW(fP(), cap, b, 0.95f), ws = cap * (tipo ? 1.25f : 0.9f);
    float x = cx - (wa + ws + wb + 2.f) * 0.5f;
    tx(fP(), cap, x, yc, cor, a, 0.f, 0.95f);
    x += wa + 1.f;
    const float hh = cap * 0.42f;
    if (tipo == 0) dl->AddTriangleFilled(Q(x, yc - hh), Q(x + ws, yc), Q(x, yc + hh), cor);
    else {
        dl->AddTriangleFilled(Q(x + ws * 0.48f, yc - hh), Q(x, yc), Q(x + ws * 0.48f, yc + hh), cor);
        dl->AddTriangleFilled(Q(x + ws * 0.52f, yc - hh), Q(x + ws, yc), Q(x + ws * 0.52f, yc + hh), cor);
    }
    x += ws + 1.f;
    tx(fP(), cap, x, yc, cor, b, 0.f, 0.95f);
}

// rotulo dos botoes duplos do Yaesu: "AGC" -(o)- "ATT", centrado em cx (cap em px da foto)
void rotDuplo9(float cx, float yc, float cap, ImU32 cor, const char* a, const char* b)
{
    const float wa = txW(fP(), cap, a, 0.95f), wb = txW(fP(), cap, b, 0.95f), ws = 2.f * cap;
    float x = cx - (wa + wb + ws + 3.f) * 0.5f;
    tx(fP(), cap, x, yc, cor, a, 0.f, 0.95f);
    x += wa + 1.5f;
    simboloDuplo(x, yc, cor, cap / 11.5f);
    x += ws + 1.5f;
    tx(fP(), cap, x, yc, cor, b, 0.f, 0.95f);
}

// parafuso do painel
void parafuso9(float cx, float cy, float r)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 c = Q(cx, cy);
    dl->AddCircleFilled(c, QD(r * 1.25f), IM_COL32(0x10, 0x11, 0x12, 255), 20);
    anelLuz(dl, c, 0.f, QD(r), 0.f, (float)(2 * kPi), 32, [](float a, float rr) { (void)rr; return cinzaN(0.18f + 0.40f * sino(a, rad(-130), rad(60))); });
    dl->AddCircleFilled(c, QD(r * 0.45f), IM_COL32(0x0c, 0x0c, 0x0d, 255), 12);
}

// caixa rebaixada do painel (as divisoes que o radio tem em volta dos grupos de botoes)
void caixa9(float x0, float y0, float x1, float y1, float r, ImU32 fundo)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(Q(x0, y0), Q(x1, y1), fundo, QD(r));
    dl->AddRect(Q(x0, y0), Q(x1, y1), IM_COL32(0x12, 0x13, 0x14, 255), QD(r), 0, std::max(1.f, QD(1.6f)));
    dl->AddLine(Q(x0 + r, y1 + 1.2f), Q(x1 - r, y1 + 1.2f), IM_COL32(255, 255, 255, 28), 1.f);
    dl->AddLine(Q(x1 + 1.2f, y0 + r), Q(x1 + 1.2f, y1 - r), IM_COL32(255, 255, 255, 18), 1.f);
}

} // namespace

// Tecla do MB1. estilo: 0 = preta com moldura prata, 1 = clara (IPO, B),
// 2 = colorida (corFace: A, TUNE, STO, RCL), 3 = chavinha com janela de LED (PROC, NB...),
// 4 = prata (teclas da tela TFT e F1..F7), 5 = tecla quadradinha com LED (15k / 6k / 3k / AUTO).
// led: 0 sem, 1 apagado, 2 verde, 3 laranja, 4 vermelho, 5 branco, 6 ciano, 7 amarelo.
// Devolve 1 = clique, 2 = segurou (0,6 s). segura: fica true enquanto aperta.
int Ui::tecla9(const char* id, const char* txt, float x0, float y0, float x1, float y1, int estilo, int led,
               const char* dica, unsigned corFace, unsigned corTxt, float cap, bool* segura)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = Q(x0, y0), b = Q(x1, y1);
    ImGui::SetCursorScreenPos(a);
    ImGui::PushID(id);
    ImGui::InvisibleButton("##t9", ImVec2(std::max(1.f, b.x - a.x), std::max(1.f, b.y - a.y)));
    const ImGuiID iid = ImGui::GetItemID();
    const bool sobre = ImGui::IsItemHovered(), ativo = ImGui::IsItemActive();
    int res = 0;
    if (ImGui::IsItemActivated()) longoFeito()[iid] = 0;
    if (ativo && ImGui::GetIO().MouseDownDuration[0] > 0.6f && longoFeito()[iid] == 0) { longoFeito()[iid] = 1; res = 2; }
    if (ImGui::IsItemDeactivated() && sobre && longoFeito()[iid] == 0) res = 1;
    ImGui::PopID();
    if (segura) *segura = ativo;
    const float h = y1 - y0, w = x1 - x0;
    const ImU32 cl = corLedIc(led);
    const float g = QD(estilo == 3 ? 2.6f : 2.2f);
    switch (estilo) {
    case 0: case 1: case 2: case 4: {
        const float r = QD(std::min(h, w) * (estilo == 4 ? 0.30f : 0.26f));
        // a moldura prata (clara em cima, escura embaixo)
        retGrad(dl, ImVec2(a.x - g, a.y - g), ImVec2(b.x + g, b.y + g * 1.3f), r + g,
                IM_COL32(0xd6, 0xd8, 0xdc, 255), IM_COL32(0x6a, 0x6c, 0x70, 255));
        dl->AddRect(ImVec2(a.x - g, a.y - g), ImVec2(b.x + g, b.y + g * 1.3f), IM_COL32(0x10, 0x11, 0x12, 200), r + g, 0, 1.f);
        ImU32 c0, c1;
        if (estilo == 0) {
            c0 = ativo ? IM_COL32(0x08, 0x08, 0x09, 255) : sobre ? IM_COL32(0x4a, 0x4c, 0x50, 255) : IM_COL32(0x34, 0x36, 0x39, 255);
            c1 = ativo ? IM_COL32(0x02, 0x02, 0x02, 255) : IM_COL32(0x0a, 0x0b, 0x0c, 255);
        } else if (estilo == 1) {
            c0 = ativo ? IM_COL32(0x90, 0x92, 0x96, 255) : sobre ? IM_COL32(0xff, 0xff, 0xff, 255) : IM_COL32(0xee, 0xef, 0xf1, 255);
            c1 = ativo ? IM_COL32(0x70, 0x72, 0x76, 255) : IM_COL32(0xb4, 0xb6, 0xba, 255);
        } else if (estilo == 2) {
            const ImU32 f = corFace ? (ImU32)corFace : IM_COL32(0xf0, 0x90, 0x90, 255);
            c0 = ativo ? mistura(f, IM_COL32(0, 0, 0, 255), 0.45f) : sobre ? mistura(f, IM_COL32(255, 255, 255, 255), 0.25f) : f;
            c1 = mistura(f, IM_COL32(0, 0, 0, 255), ativo ? 0.6f : 0.30f);
        } else {
            c0 = ativo ? IM_COL32(0x80, 0x82, 0x86, 255) : sobre ? IM_COL32(0xff, 0xff, 0xff, 255) : IM_COL32(0xf2, 0xf3, 0xf5, 255);
            c1 = ativo ? IM_COL32(0x50, 0x52, 0x56, 255) : IM_COL32(0x88, 0x8a, 0x8e, 255);
        }
        retGrad(dl, a, b, r, c0, c1);
        dl->AddLine(ImVec2(a.x + r, a.y + std::max(1.f, QD(1.f))), ImVec2(b.x - r, a.y + std::max(1.f, QD(1.f))), IM_COL32(255, 255, 255, estilo == 0 ? 60 : 140), 1.f);
        if (estilo == 0 && led > 0) {      // LED quadradinho a esquerda (ANTENNA SELECT)
            const ImVec2 la = Q(x0 + 3.f, y0 + h * 0.30f), lb = Q(x0 + 7.5f, y1 - h * 0.30f);
            if (led >= 2) dl->AddRectFilled(ImVec2(la.x - 2, la.y - 2), ImVec2(lb.x + 2, lb.y + 2), alfa(cl, 80), 2.f);
            dl->AddRectFilled(la, lb, led >= 2 ? cl : IM_COL32(0x3a, 0x3c, 0x40, 255), 1.f);
        }
        break; }
    case 3: {      // chavinha: moldura preta arredondada e a janela do LED
        const float r = QD(std::min(w, h) * 0.45f);
        dl->AddRectFilled(ImVec2(a.x - g, a.y - g), ImVec2(b.x + g, b.y + g), IM_COL32(0x0c, 0x0c, 0x0d, 255), r + g);
        dl->AddRect(ImVec2(a.x - g, a.y - g), ImVec2(b.x + g, b.y + g), IM_COL32(0x88, 0x8a, 0x8e, 160), r + g, 0, 1.f);
        retGrad(dl, a, b, r, ativo ? IM_COL32(0x10, 0x10, 0x11, 255) : sobre ? IM_COL32(0x58, 0x5a, 0x5e, 255) : IM_COL32(0x36, 0x38, 0x3b, 255),
                IM_COL32(0x0e, 0x0f, 0x10, 255));
        dl->AddRect(a, b, IM_COL32(0xb0, 0xb2, 0xb6, 150), r, 0, 1.f);
        const bool vert = h > w;
        const ImVec2 la = vert ? Q(x0 + w * 0.32f, y0 + h * 0.22f) : Q(x0 + w * 0.22f, y0 + h * 0.34f);
        const ImVec2 lb = vert ? Q(x1 - w * 0.32f, y1 - h * 0.22f) : Q(x1 - w * 0.22f, y1 - h * 0.34f);
        if (led >= 2) dl->AddRectFilled(ImVec2(la.x - 3, la.y - 3), ImVec2(lb.x + 3, lb.y + 3), alfa(cl, 70), QD(3));
        if (led >= 2) dl->AddRectFilled(la, lb, cl, QD(1.5f));
        else retGrad(dl, la, lb, QD(1.5f), IM_COL32(0xe8, 0xea, 0xee, 255), IM_COL32(0x70, 0x72, 0x76, 255));
        if (led >= 2) dl->AddRectFilled(la, ImVec2(lb.x, (la.y + lb.y) * 0.5f), IM_COL32(255, 255, 255, 60), QD(1.5f));
        break; }
    default: {     // 5: quadradinho com LED
        dl->AddRectFilled(ImVec2(a.x - g, a.y - g), ImVec2(b.x + g, b.y + g), IM_COL32(0x0c, 0x0c, 0x0d, 255), QD(2.5f));
        retGrad(dl, a, b, QD(2), sobre ? IM_COL32(0x58, 0x5a, 0x5e, 255) : IM_COL32(0x40, 0x42, 0x46, 255), IM_COL32(0x16, 0x17, 0x18, 255));
        const ImVec2 la = Q(x0 + w * 0.26f, y0 + h * 0.24f), lb = Q(x1 - w * 0.26f, y1 - h * 0.24f);
        if (led >= 2) dl->AddRectFilled(ImVec2(la.x - 3, la.y - 3), ImVec2(lb.x + 3, lb.y + 3), alfa(cl, 80), QD(3));
        dl->AddRectFilled(la, lb, led >= 2 ? cl : IM_COL32(0x22, 0x24, 0x28, 255), QD(1));
        break; }
    }
    if (txt && *txt) {
        const float c = cap > 0.f ? cap : std::clamp(h * 0.44f, 5.f, 10.5f);
        ImU32 ct = corTxt ? (ImU32)corTxt : (estilo == 1 || estilo == 4) ? IM_COL32(0x16, 0x17, 0x19, 255)
                                         : estilo == 2 ? IM_COL32(0x20, 0x18, 0x18, 255) : IM_COL32(0xf6, 0xf6, 0xf6, 255);
        tx(fP(), c, (x0 + x1) * 0.5f + (estilo == 0 && led > 0 ? 2.5f : 0.f), (y0 + y1) * 0.5f, ct, txt, 0.5f, 0.92f, w - 4.f);
    }
    if (sobre && dica && *dica && !ativo) ImGui::SetTooltip("%s", dica);
    return res;
}

// Botao do Yaesu: saia preta serrilhada (com a marca vermelha) e o miolo preto com o aro
// cromado. duplo: o miolo (tIn) e a saia (tOut) sao dois controles; arrastar para cima /
// direita aumenta, a roda tambem. marcas: tracinhos brancos em volta (rMarca, px da foto).
bool Ui::giro9(const char* id, float cx, float cy, float rO, float rI, float tIn, float tOut, float& dIn, float& dOut,
               const char* dica, bool duplo, int marcas, float rMarca)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    dIn = dOut = 0;
    const ImVec2 c = Q(cx, cy);
    const float R = QD(rO), Ri = QD(rI);
    const float TWO_PI = (float)(2 * kPi);
    for (int i = 0; i < marcas; ++i) {          // os tracinhos da escala
        const ImVec2 d = dirAng(rad(-225.f + 270.f * i / std::max(1, marcas - 1)));
        dl->AddLine(ImVec2(c.x + d.x * QD(rMarca), c.y + d.y * QD(rMarca)), ImVec2(c.x + d.x * QD(rMarca + 5.5f), c.y + d.y * QD(rMarca + 5.5f)),
                    IM_COL32(0xee, 0xf0, 0xf2, 230), std::max(1.f, QD(1.7f)));
    }
    ImGui::SetCursorScreenPos(ImVec2(c.x - R * 1.05f, c.y - R * 1.05f));
    ImGui::PushID(id);
    ImGui::InvisibleButton("##g9", ImVec2(R * 2.1f, R * 2.1f));
    const ImGuiID iid = ImGui::GetItemID();
    const bool sobre = ImGui::IsItemHovered(), ativo = ImGui::IsItemActive();
    ImGui::PopID();
    const float dxm = io.MousePos.x - c.x, dym = io.MousePos.y - c.y;
    const bool noMiolo = !duplo || std::sqrt(dxm * dxm + dym * dym) <= Ri * 1.04f;
    if (ImGui::IsItemActivated()) parteGiro()[iid] = noMiolo ? 0 : 1;
    if (ativo) {
        const float dv = (-io.MouseDelta.y + io.MouseDelta.x) / M.d(150.f);
        if (parteGiro()[iid] == 0) dIn = dv; else dOut = dv;
    }
    if (sobre && io.MouseWheel != 0) {
        const float dv = io.MouseWheel * 0.02f;
        if (noMiolo) dIn = dv; else dOut = dv;
    }
    const float aFora = angKnob(duplo ? tOut : tIn), aIn = angKnob(tIn);
    // sombra
    dl->AddCircleFilled(ImVec2(c.x + R * 0.04f, c.y + R * 0.10f), R * 1.03f, IM_COL32(0, 0, 0, 150), 48);
    // a saia serrilhada preta
    auto luzS = [](float a) { return 0.07f + 0.20f * sino(a, rad(-120), rad(45)) + 0.08f * sino(a, rad(55), rad(35)); };
    anelLuz(dl, c, Ri, R, 0.f, TWO_PI, 96, [&](float a, float rr) { return cinzaN(luzS(a) * (rr > R * 0.97f ? 0.7f : 1.f)); });
    {
        const int nd = std::max(36, (int)(rO * 1.6f));
        for (int k = 0; k < nd; ++k) {
            const float an = aFora + k * TWO_PI / nd;
            const ImVec2 d = dirAng(an);
            dl->AddLine(ImVec2(c.x + d.x * (Ri + (R - Ri) * 0.12f), c.y + d.y * (Ri + (R - Ri) * 0.12f)), ImVec2(c.x + d.x * R * 0.985f, c.y + d.y * R * 0.985f),
                        cinzaN(std::min(1.f, luzS(an) * 2.4f), 200), std::max(1.f, R * 0.028f));
        }
    }
    if (duplo && rO > rI * 1.25f) circuloSuave(dl, c, Ri + (R - Ri) * 0.45f, 96, std::max(1.f, QD(1.2f)), [](float) { return IM_COL32(0, 0, 0, 200); });
    circuloSuave(dl, c, R, 96, 1.f, [&](float a) { return cinzaN(luzS(a) * 1.6f, 230); });
    {   // a marca vermelha da saia
        const ImVec2 d = dirAng(aFora);
        dl->AddLine(ImVec2(c.x + d.x * (Ri + (R - Ri) * 0.25f), c.y + d.y * (Ri + (R - Ri) * 0.25f)), ImVec2(c.x + d.x * R * 0.96f, c.y + d.y * R * 0.96f),
                    (ativo && parteGiro()[iid] == 1) ? P_LARANJA : IM_COL32(0xf0, 0x40, 0x34, 255), std::max(1.2f, QD(2.4f)));
    }
    // o aro cromado do miolo (bem claro em cima, como na foto) e o miolo preto
    auto luzC = [](float a) { return 0.84f + 0.30f * sino(a, rad(-115), rad(55)) + 0.18f * sino(a, rad(60), rad(40)) - 0.22f * sino(a, rad(160), rad(30)); };
    anelLuz(dl, c, Ri * 0.84f, Ri, 0.f, TWO_PI, 96, [&](float a, float rr) {
        const float l = std::min(1.f, luzC(a) * 1.12f);
        return cinzaN(rr > Ri * 0.975f ? l * 0.55f : rr < Ri * 0.895f ? 0.10f + 0.12f * l : l);
    });
    dl->AddCircleFilled(c, Ri * 0.84f, cinzaN(0.035f), 64);
    anelLuz(dl, c, Ri * 0.30f, Ri * 0.84f, 0.f, TWO_PI, 72, [&](float a, float rr) {
        return cinzaN((0.02f + 0.07f * sino(a, rad(-120), rad(45))) * (rr / (Ri * 0.84f)));
    });
    {   // a marca do miolo
        const ImVec2 d = dirAng(aIn);
        dl->AddLine(ImVec2(c.x + d.x * Ri * 0.50f, c.y + d.y * Ri * 0.50f), ImVec2(c.x + d.x * Ri * 0.80f, c.y + d.y * Ri * 0.80f),
                    (ativo && parteGiro()[iid] == 0) ? P_LARANJA : duplo ? IM_COL32(0xf0, 0x40, 0x34, 255) : IM_COL32(0xf4, 0xf4, 0xf4, 255),
                    std::max(1.2f, QD(2.4f)));
    }
    if (ativo) dl->AddCircle(c, R * 1.04f, alfa(P_LARANJA, 130), 48, std::max(1.f, QD(1.6f)));
    if ((sobre || ativo) && dica) ImGui::SetTooltip("%s", dica);
    return dIn != 0 || dOut != 0;
}

// Dial principal do MB1: o anel de borracha com os ressaltos, o aro branco, a face
// preta e a covinha do dedo (gira com o dial). 100 passos por volta; rapido multiplica.
int Ui::dial9(const char* id, float cx, float cy, float rFace, float& ang, bool travado, const char* dica)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 c = Q(cx, cy);
    const float F = QD(rFace), RO = F * 1.20f;
    ImGui::SetCursorScreenPos(ImVec2(c.x - RO, c.y - RO));
    ImGui::PushID(id);
    ImGui::InvisibleButton("##d9", ImVec2(2 * RO, 2 * RO));
    const ImGuiID iid = ImGui::GetItemID();
    const bool sobreB = ImGui::IsItemHovered(), ativo = ImGui::IsItemActive();
    ImGui::PopID();
    const float dist = std::sqrt((io.MousePos.x - c.x) * (io.MousePos.x - c.x) + (io.MousePos.y - c.y) * (io.MousePos.y - c.y));
    const bool dentro = dist <= RO;
    int passos = 0;
    static std::unordered_map<ImGuiID, float> resto;
    const float aM = std::atan2(io.MousePos.y - c.y, io.MousePos.x - c.x);
    if (ImGui::IsItemActivated()) {
        angArrasto()[iid] = aM;
        if (!dentro) ImGui::ClearActiveID();
    }
    if (ativo && dentro && !travado) {
        float d = aM - angArrasto()[iid];
        while (d > kPi) d -= (float)(2 * kPi);
        while (d < -kPi) d += (float)(2 * kPi);
        angArrasto()[iid] = aM;
        ang += d;
        const float vel = std::fabs(d) / std::max(0.001f, io.DeltaTime) / (float)(2 * kPi);
        const float mult = vel > 2.0f ? 10.f : vel > 0.8f ? 4.f : 1.f;
        resto[iid] += d * mult;
        const float passo = (float)(2 * kPi / 100.0);
        while (resto[iid] >= passo) { ++passos; resto[iid] -= passo; }
        while (resto[iid] <= -passo) { --passos; resto[iid] += passo; }
    } else if (ativo) angArrasto()[iid] = aM;
    if (io.MouseWheel != 0 && !ativo && sobreB && dentro && !travado) {
        int n = (int)std::lround(io.MouseWheel);
        if (n == 0) n = io.MouseWheel > 0 ? 1 : -1;
        passos += n * 5; ang += n * 5 * (float)(2 * kPi / 100.0);
    }
    const float TWO_PI = (float)(2 * kPi);
    dl->AddCircleFilled(ImVec2(c.x + F * 0.02f, c.y + F * 0.05f), RO * 1.02f, IM_COL32(0, 0, 0, 120), 128);
    // o anel de borracha (cinza) com os ressaltos quadrados que giram com o dial
    auto luzA = [](float a) { return 0.36f + 0.24f * sino(a, rad(-125), rad(55)) + 0.10f * sino(a, rad(50), rad(40)); };
    anelLuz(dl, c, F * 1.06f, RO, 0.f, TWO_PI, 180, [&](float a, float rr) { return cinzaN(luzA(a) * (rr > RO * 0.985f ? 0.55f : 1.f)); });
    for (int k = 0; k < 120; ++k) {     // o serrilhado fino da borda
        const ImVec2 d = dirAng(ang + k * TWO_PI / 120);
        dl->AddLine(ImVec2(c.x + d.x * RO * 0.955f, c.y + d.y * RO * 0.955f), ImVec2(c.x + d.x * RO * 0.995f, c.y + d.y * RO * 0.995f), cinzaN(0.06f, 220), std::max(1.f, F * 0.012f));
    }
    for (int k = 0; k < 48; ++k) {      // os ressaltos
        const float an = ang + k * TWO_PI / 48;
        const ImVec2 d = dirAng(an), n(-d.y, d.x);
        const float r0 = F * 1.085f, r1 = F * 1.135f, l = F * 0.024f;
        const ImVec2 p0(c.x + d.x * r0 + n.x * l, c.y + d.y * r0 + n.y * l), p1(c.x + d.x * r1 + n.x * l, c.y + d.y * r1 + n.y * l);
        const ImVec2 p2(c.x + d.x * r1 - n.x * l, c.y + d.y * r1 - n.y * l), p3(c.x + d.x * r0 - n.x * l, c.y + d.y * r0 - n.y * l);
        dl->AddQuadFilled(p0, p1, p2, p3, cinzaN(luzA(an) * 0.28f));
        dl->AddLine(p0, p1, cinzaN(std::min(1.f, luzA(an) * 1.6f), 200), 1.f);
    }
    circuloSuave(dl, c, RO, 180, 1.f, [&](float a) { return cinzaN(luzA(a) * 0.5f, 230); });
    // o aro branco e a face preta
    dl->AddCircleFilled(c, F * 1.06f, IM_COL32(0x0a, 0x0a, 0x0b, 255), 180);
    anelLuz(dl, c, F * 0.995f, F * 1.035f, 0.f, TWO_PI, 180, [](float a, float) { return cinzaN(0.86f + 0.14f * sino(a, rad(-120), rad(60))); });
    dl->AddCircleFilled(c, F * 0.995f, cinzaN(0.045f), 180);
    for (int i = 1; i <= 22; ++i) dl->AddCircle(c, F * i / 23.f, IM_COL32(255, 255, 255, (i & 1) ? 7 : 3), 0, 1.f);
    anelLuz(dl, c, 0.f, F * 0.99f, 0.f, TWO_PI, 96, [&](float a, float rr) { return IM_COL32(255, 255, 255, (int)(14 * sino(a, rad(-125), rad(40)) * (rr / F))); });
    {   // a covinha do dedo
        const ImVec2 d = dirAng(ang + (float)(kPi * 0.5));
        const ImVec2 f(c.x + d.x * F * 0.46f, c.y + d.y * F * 0.46f);
        const float rf = F * 0.34f;
        dl->AddCircleFilled(ImVec2(f.x, f.y + rf * 0.05f), rf * 1.04f, IM_COL32(0, 0, 0, 220), 64);
        anelLuz(dl, f, rf * 0.80f, rf, 0.f, TWO_PI, 96, [](float a, float) { return cinzaN(0.05f + 0.22f * sino(a, rad(60), rad(50)) + 0.08f * sino(a, rad(-120), rad(40))); });
        anelLuz(dl, f, 0.f, rf * 0.80f, 0.f, TWO_PI, 96, [&](float a, float rr) {
            const float t = rr / (rf * 0.80f);
            return cinzaN((0.025f + 0.08f * sino(a, rad(-110), rad(40))) * (0.4f + 0.6f * t));
        });
        dl->AddCircleFilled(ImVec2(f.x - rf * 0.22f, f.y - rf * 0.25f), rf * 0.09f, IM_COL32(255, 255, 255, 60), 16);
    }
    if (ativo && dentro) dl->AddCircle(c, F * 1.04f, alfa(P_LARANJA, 120), 0, std::max(1.f, QD(2.f)));
    if (travado) tx(fP(), 14.f, cx, cy - rFace * 0.45f, alfa(P_VERMELHO, 230), "LOCK", 0.5f);
    if (sobreB && dentro && !ativo && dica) ImGui::SetTooltip("%s", dica);
    return travado ? 0 : passos;
}

// ===========================================================================
//  MB1SDR: a caixa do radio (tampa, frente cinza-escuro, divisoes, placa)
// ===========================================================================
void Ui::ft9Chassi()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImDrawListFlags fl0 = dl->Flags;
    if (g_fundoChave) dl->Flags &= ~ImDrawListFlags_AntiAliasedFill;   // sem franja rosa na cor-chave
    // ---- a tampa de cima, vista em perspectiva ----
    quadCores(dl, Q(205, 4), Q(1695, 4), Q(1892, 82), Q(8, 82),
              IM_COL32(0x2c, 0x2d, 0x2f, 255), IM_COL32(0x2a, 0x2b, 0x2d, 255), IM_COL32(0x3c, 0x3d, 0x40, 255), IM_COL32(0x3e, 0x3f, 0x42, 255));
    // ---- a frente ----
    dl->AddRectFilled(Q(6, 80), Q(1894, 673), IM_COL32(0x2e, 0x30, 0x33, 255), QD(16), ImDrawFlags_RoundCornersBottom);
    dl->Flags = fl0;
    dl->AddRectFilledMultiColor(Q(8, 84), Q(1892, 300), IM_COL32(0x36, 0x38, 0x3b, 255), IM_COL32(0x34, 0x36, 0x39, 255),
                                IM_COL32(0x2e, 0x30, 0x33, 0), IM_COL32(0x2e, 0x30, 0x33, 0));
    // os dois frisos prateados de cima
    dl->AddRectFilledMultiColor(Q(10, 80), Q(1890, 84), IM_COL32(0xc8, 0xca, 0xce, 255), IM_COL32(0xb8, 0xba, 0xbe, 255),
                                IM_COL32(0x60, 0x62, 0x66, 255), IM_COL32(0x60, 0x62, 0x66, 255));
    dl->AddRectFilledMultiColor(Q(14, 105), Q(1886, 108), IM_COL32(0x9c, 0x9e, 0xa2, 255), IM_COL32(0x90, 0x92, 0x96, 255),
                                IM_COL32(0x44, 0x46, 0x4a, 255), IM_COL32(0x44, 0x46, 0x4a, 255));
    for (float x = 230; x < 1680; x += 31.f) dl->AddLine(Q(x, 8), Q(x - (x - 950) * 0.115f, 78), IM_COL32(255, 255, 255, 6), 1.f);   // reflexo da tampa
    dl->AddLine(Q(205, 4.6f), Q(1695, 4.6f), IM_COL32(0x90, 0x92, 0x96, 200), 1.f);
    // textura fininha da frente (pontinhos sempre nos mesmos lugares)
    {
        const float t = std::max(1.f, QD(1.2f));
        for (uint32_t i = 0; i < 6400; ++i) {
            const uint32_t h = hashIc(i * 2654435761u + 91u);
            const float x = 10 + (h & 0xffff) / 65535.f * 1880.f, y = 110 + (h >> 16) / 65535.f * 558.f;
            if (x > 755 && x < 1235 && y < 265) continue;      // VFD
            if (x > 1240 && x < 1888 && y < 468) continue;     // TFT
            if (x > 128 && x < 432 && y > 130 && y < 266) continue;
            if (x > 450 && x < 750 && y > 130 && y < 266) continue;
            const ImVec2 q = Q(x, y);
            dl->AddRectFilled(q, ImVec2(q.x + t, q.y + t), (h & 0x300) ? IM_COL32(0, 0, 0, 55) : IM_COL32(255, 255, 255, 22));
        }
    }
    const ImU32 cx = IM_COL32(0x29, 0x2b, 0x2e, 255);
    // ---- as divisoes rebaixadas (como no radio) ----
    caixa9(20, 114, 754, 272, 8, IM_COL32(0x26, 0x28, 0x2b, 255));       // medidores + MOX/VOX/DIM
    caixa9(26, 306, 113, 434, 6, cx);                                        // PHONES
    caixa9(26, 446, 113, 522, 6, cx);                                        // KEY
    caixa9(26, 532, 126, 668, 6, cx);                                        // MIC
    caixa9(116, 306, 541, 574, 6, cx);                                       // os botoes do MAIN
    caixa9(544, 306, 698, 574, 6, cx);                                       // CONT / VRF / filtros
    caixa9(133, 578, 698, 662, 6, cx);                                       // fileira do SUB (B)
    caixa9(700, 578, 946, 662, 6, cx);                                       // AF-RF do SUB, RF PWR
    caixa9(1238, 470, 1421, 660, 6, cx);                                     // AF GAIN - RF GAIN
    caixa9(1424, 470, 1610, 660, 6, cx);                                     // SHIFT - WIDTH
    caixa9(1613, 470, 1888, 660, 6, cx);                                     // CLAR / VFO-B
    // o miolo do dial (o rebaixo escuro em volta das teclas e do dial)
    dl->AddRectFilled(Q(700, 298), Q(1236, 576), IM_COL32(0x22, 0x24, 0x26, 255), QD(10));
    dl->AddRect(Q(700, 298), Q(1236, 576), IM_COL32(0x10, 0x11, 0x12, 255), QD(10), 0, std::max(1.f, QD(1.6f)));
    dl->AddCircleFilled(Q(1000, 460), QD(140), IM_COL32(0x1a, 0x1b, 0x1d, 255), 128);
    dl->AddCircle(Q(1000, 460), QD(140), IM_COL32(0x44, 0x46, 0x4a, 200), 128, std::max(1.f, QD(1.5f)));
    // a moldura da coluna de modos e das teclas da direita do dial (em arco)
    retGrad(dl, Q(712, 368), Q(790, 572), QD(18), IM_COL32(0x2c, 0x2e, 0x31, 255), IM_COL32(0x1c, 0x1d, 0x1f, 255));
    retGrad(dl, Q(1148, 368), Q(1228, 572), QD(18), IM_COL32(0x2c, 0x2e, 0x31, 255), IM_COL32(0x1c, 0x1d, 0x1f, 255));
    // ---- a placa do nome (no lugar de YAESU HF TRANSCEIVER MB1) ----
    retGrad(dl, Q(132, 276), Q(744, 304), QD(3), IM_COL32(0x40, 0x42, 0x45, 255), IM_COL32(0x16, 0x17, 0x18, 255));
    dl->AddRectFilled(Q(136, 279), Q(740, 301), IM_COL32(0x1c, 0x1d, 0x1f, 255), QD(2));
    dl->AddLine(Q(138, 279.5f), Q(738, 279.5f), IM_COL32(255, 255, 255, 40), 1.f);
    tx(fP(), 13.f, 276.5f, 291.8f, IM_COL32(0x14, 0x0c, 0x00, 230), "SDR", 0.5f, 1.25f);       // sombra
    tx(fP(), 13.f, 275, 290.5f, IM_COL32(0xe8, 0xb8, 0x4c, 255), "SDR", 0.5f, 1.25f);           // dourado
    tx(fP(), 7.f, 455, 291, IM_COL32(0xe6, 0xe8, 0xec, 255), "HF/VHF/UHF  SDR  RECEIVER", 0.5f, 1.05f);
    {
        const float wf = txW(fP(), 12.f, "FT", 1.f), wd = txW(fP(), 8.f, "DX", 1.f), wn = txW(fP(), 12.f, "9000D", 1.f);
        float x = 596.f - (wf + wd + wn + 2.f) * 0.5f;
        const ImU32 cb = IM_COL32(0xee, 0xf0, 0xf2, 255);
        tx(fP(), 12.f, x, 290.5f, cb, "FT"); x += wf + 1.f;
        tx(fP(), 8.f, x, 292.5f, cb, "DX"); x += wd + 1.f;
        tx(fP(), 12.f, x, 290.5f, cb, "9000D");
    }
    // ---- parafusos ----
    parafuso9(40, 283, 6); parafuso9(45, 422, 5); parafuso9(39, 570, 5); parafuso9(114, 645, 5);
    parafuso9(788, 283, 6); parafuso9(1213, 283, 6); parafuso9(1855, 641, 7);
    // ---- a tampa como "barra de titulo": versao, minimizar / maximizar / fechar, arrastar ----
    tx(fP(), 8.5f, 250, 40, IM_COL32(0xc8, 0xca, 0xce, 255), "MB1SDR  v" RXSDR_VERSAO, 0.f, 0.95f);
    {
        HWND hw = (HWND)ImGui::GetMainViewport()->PlatformHandleRaw;
        if (!hw) hw = GetActiveWindow();
        const bool max = hw && IsZoomed(hw);
        const float bw = 32.f, bh = 19.f, gap = 5.f, by = 30.f, bx0 = 1700.f - 3 * bw - 2 * gap;
        for (int i = 0; i < 3; ++i) {
            const float x = bx0 + i * (bw + gap);
            const ImVec2 a = Q(x, by), b = Q(x + bw, by + bh);
            ImGui::SetCursorScreenPos(a);
            ImGui::PushID(9100 + i);
            const bool clicou = ImGui::InvisibleButton("##jan", ImVec2(std::max(1.f, b.x - a.x), std::max(1.f, b.y - a.y)));
            const bool sob = ImGui::IsItemHovered(), at = ImGui::IsItemActive();
            ImGui::PopID();
            ImU32 fundo = at ? IM_COL32(0x10, 0x12, 0x14, 255) : sob ? IM_COL32(0x60, 0x64, 0x6a, 255) : IM_COL32(0x34, 0x36, 0x3a, 255);
            if (i == 2 && (sob || at)) fundo = at ? IM_COL32(0x90, 0x1c, 0x14, 255) : IM_COL32(0xc4, 0x2b, 0x1c, 255);
            dl->AddRectFilled(a, b, fundo, QD(3));
            dl->AddRect(a, b, IM_COL32(0x88, 0x8c, 0x92, 200), QD(3), 0, 1.f);
            const ImU32 cs = IM_COL32(0xee, 0xf0, 0xf4, 255);
            const float ccx = x + bw * 0.5f, cy = by + bh * 0.5f, t = std::max(1.f, QD(1.5f));
            if (i == 0) dl->AddLine(Q(ccx - 6, cy + 3.5f), Q(ccx + 6, cy + 3.5f), cs, t);
            else if (i == 1) {
                if (max) {
                    dl->AddRect(Q(ccx - 5, cy - 2), Q(ccx + 3, cy + 6), cs, 0.f, 0, t);
                    dl->AddLine(Q(ccx - 2, cy - 5.5f), Q(ccx + 6, cy - 5.5f), cs, t);
                    dl->AddLine(Q(ccx + 6, cy - 5.5f), Q(ccx + 6, cy + 2.5f), cs, t);
                } else dl->AddRect(Q(ccx - 6, cy - 5), Q(ccx + 6, cy + 5), cs, 0.f, 0, t);
            } else {
                dl->AddLine(Q(ccx - 5, cy - 5), Q(ccx + 5, cy + 5), cs, t);
                dl->AddLine(Q(ccx + 5, cy - 5), Q(ccx - 5, cy + 5), cs, t);
            }
            if (sob && !at) ImGui::SetTooltip("%s", i == 0 ? "Minimizar" : i == 1 ? (max ? "Restaurar" : "Maximizar") : "Fechar");
            if (clicou && hw) {
                if (i == 0) ShowWindow(hw, SW_MINIMIZE);
                else if (i == 1) ShowWindow(hw, max ? SW_RESTORE : SW_MAXIMIZE);
                else PostMessageW(hw, WM_CLOSE, 0, 0);
            }
        }
        // arrastar pela tampa (fora dos botoes); duplo clique maximiza / volta
        const ImVec2 t0 = Q(6, 3), t1 = Q(1894, 80);
        const ImVec2 mp = ImGui::GetIO().MousePos;
        const bool naTampa = mp.x >= t0.x && mp.x < t1.x && mp.y >= t0.y && mp.y < t1.y && !ImGui::IsAnyItemHovered() &&
                             ImGui::IsWindowHovered(ImGuiHoveredFlags_None);
        if (naTampa && hw) {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) ShowWindow(hw, max ? SW_RESTORE : SW_MAXIMIZE);
            else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                ReleaseCapture();
                SendMessageW(hw, WM_NCLBUTTONDOWN, HTCAPTION, 0);
                ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, false);
            }
        }
    }
    // botao direito no painel = MENU (FUNC)
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##funcFt");
}

// ===========================================================================
//  MB1SDR: os dois S-meters analogicos (iluminados em ambar, como o radio
//  ligado). O da esquerda e o do MAIN (S / PO / COMP / ID / SWR), o da direita
//  e o do SUB (S / ALC / VDD).
// ===========================================================================
void Ui::ft9Medidor(int rx)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool lig = r_->ligado();
    const float dx = rx == 0 ? 0.f : 317.f;
    const float fx0 = 137 + dx, fx1 = 424 + dx, fy0 = 139, fy1 = 259;
    // a moldura prata e o vidro
    retGrad(dl, Q(fx0 - 8, fy0 - 7), Q(fx1 + 8, fy1 + 6), QD(10), IM_COL32(0xc4, 0xc6, 0xca, 255), IM_COL32(0x62, 0x64, 0x68, 255));
    dl->AddRect(Q(fx0 - 8, fy0 - 7), Q(fx1 + 8, fy1 + 6), IM_COL32(0x10, 0x11, 0x12, 255), QD(10), 0, 1.f);
    dl->AddRectFilled(Q(fx0 - 2, fy0 - 2), Q(fx1 + 2, fy1 + 2), IM_COL32(0x08, 0x08, 0x08, 255), QD(7));
    const ImU32 fundo = lig ? IM_COL32(0x1c, 0x10, 0x09, 255) : IM_COL32(0x04, 0x04, 0x05, 255);
    dl->AddRectFilled(Q(fx0, fy0), Q(fx1, fy1), fundo, QD(6));
    if (lig) {      // o brilho da lampada atras da escala
        const ImVec2 c = Q(283 + dx, 215);
        for (int i = 6; i >= 1; --i)
            dl->AddEllipseFilled(c, ImVec2(QD(22.f * i), QD(10.f * i)), IM_COL32(0x70, 0x38, 0x10, 10), 0.f, 48);
    }
    const ImU32 AM = lig ? IM_COL32(0xff, 0xbe, 0x4a, 255) : IM_COL32(0x88, 0x8a, 0x90, 255);
    const ImU32 RD = lig ? IM_COL32(0xff, 0x4c, 0x2c, 255) : IM_COL32(0x78, 0x7a, 0x92, 255);
    dl->PushClipRect(Q(fx0, fy0), Q(fx1, fy1), true);
    struct Arco { float xc, yv, a; };
    auto yA = [](const Arco& k, float x) { return k.yv + k.a * (x - k.xc) * (x - k.xc); };
    float ox = dx;      // deslocamento das escalas (as do SUB, abaixo, ja estao em px absolutos)
    auto arco = [&](const Arco& k, float xa, float xb, ImU32 cor, float esp) {
        std::vector<ImVec2> p;
        for (int i = 0; i <= 40; ++i) { const float x = xa + (xb - xa) * i / 40.f; p.push_back(Q(x + ox, yA(k, x))); }
        dl->AddPolyline(p.data(), (int)p.size(), cor, 0, std::max(1.f, QD(esp)));
    };
    auto normal = [&](const Arco& k, float x, float& nx, float& ny) {     // para cima (fora)
        const float s = 2.f * k.a * (x - k.xc);
        const float L = std::sqrt(1.f + s * s);
        nx = s / L; ny = -1.f / L;
    };
    auto marca = [&](const Arco& k, float x, float l0, float l1, ImU32 cor, float esp) {   // l > 0: para cima
        float nx, ny; normal(k, x, nx, ny);
        const float y = yA(k, x);
        dl->AddLine(Q(x + ox + nx * l0, y + ny * l0), Q(x + ox + nx * l1, y + ny * l1), cor, std::max(1.f, QD(esp)));
    };
    auto num = [&](const Arco& k, float x, float dist, float cap, ImU32 cor, const char* t) {
        float nx, ny; normal(k, x, nx, ny);
        const float y = yA(k, x);
        tx(fP(), cap, x + ox + nx * dist, y + ny * dist, cor, t, 0.5f, 0.95f);
    };
    // ---- a escala S (igual nos dois) ----
    const Arco aS{283, 176.5f + (rx ? 1.5f : 0.f), 0.00148f};
    arco(aS, 170, 284, AM, 1.6f);
    arco(aS, 283, 396, RD, 4.2f);
    const float xs[9] = {182, 197, 210, 224, 238, 250, 262, 273, 283};
    for (int i = 0; i < 9; ++i) marca(aS, xs[i], 0.f, (i % 2 == 0) ? 8.f : 5.f, AM, 1.5f);
    const float xd[6] = {301, 318, 335, 352, 370, 391};
    for (int i = 0; i < 6; ++i) marca(aS, xd[i], 1.5f, (i % 2) ? 8.f : 5.f, RD, 1.5f);
    tx(fP(), 10.f, 159 + dx, 168, AM, "S", 0.5f, 1.15f);
    num(aS, 182, 14.f, 8.5f, AM, "1"); num(aS, 210, 13.f, 8.5f, AM, "3"); num(aS, 238, 13.f, 8.5f, AM, "5");
    num(aS, 262, 13.f, 8.5f, AM, "7"); num(aS, 283, 13.f, 8.5f, AM, "9");
    num(aS, 318, 12.f, 7.f, RD, "+20"); num(aS, 352, 12.f, 7.f, RD, "+40"); num(aS, 391, 11.f, 7.f, RD, "+60");
    tx(fP(), 7.f, 398 + dx, 167, RD, "dB", 0.5f);
    if (rx == 0) {
        // PO (W)
        const Arco aP{282, 183.5f, 0.00150f};
        arco(aP, 175, 392, AM, 1.4f);
        const float xp[11] = {190, 212, 244, 266, 287, 310, 332, 354, 375, 384, 362};
        for (int i = 0; i < 9; ++i) marca(aP, xp[i], 0.f, -4.f, AM, 1.3f);
        num(aP, 190, -9.f, 7.f, AM, "0"); num(aP, 244, -9.f, 7.f, AM, "100"); num(aP, 287, -9.f, 7.f, AM, "150");
        num(aP, 332, -9.f, 7.f, AM, "200"); num(aP, 375, -9.f, 7.f, AM, "250");
        tx(fP(), 8.f, 399 + dx, 208, AM, "W", 0.5f);
        tx(fP(), 7.f, 160 + dx, 208, AM, "PO", 0.5f);
        // COMP (dB, em cima) / ID (A, embaixo)
        const Arco aC{290, 209.5f, 0.00120f};
        arco(aC, 205, 372, AM, 1.4f);
        const float xc[5] = {207, 250, 288, 315, 343};
        for (int i = 0; i < 5; ++i) marca(aC, xc[i], 0.f, 4.f, AM, 1.3f);
        const float xi[4] = {215, 272, 318, 350};
        for (int i = 0; i < 4; ++i) marca(aC, xi[i], 0.f, -4.f, AM, 1.3f);
        num(aC, 200, 6.f, 6.5f, AM, "0"); num(aC, 250, 9.f, 6.5f, AM, "5"); num(aC, 288, 9.f, 6.5f, AM, "10");
        num(aC, 315, 9.f, 6.5f, AM, "15"); num(aC, 343, 9.f, 6.5f, AM, "20");
        tx(fP(), 7.f, 378 + dx, 218, AM, "dB", 0.5f);
        num(aC, 215, -9.f, 6.5f, AM, "0"); num(aC, 272, -9.f, 6.5f, AM, "5"); num(aC, 318, -9.f, 6.5f, AM, "10"); num(aC, 350, -9.f, 6.5f, AM, "15");
        tx(fP(), 7.f, 371 + dx, 236, AM, "A", 0.5f);
        tx(fP(), 6.5f, 172 + dx, 229, AM, "COMP", 0.5f);
        tx(fP(), 6.5f, 196 + dx, 240, AM, "ID", 0.5f);
        // SWR
        const Arco aW{290, 229.5f, 0.00095f};
        arco(aW, 225, 350, AM, 1.4f);
        const float xw[6] = {233, 257, 272, 283, 296, 330};
        for (int i = 0; i < 6; ++i) marca(aW, xw[i], 0.f, 3.5f, AM, 1.3f);
        num(aW, 233, -9.f, 6.5f, AM, "1"); num(aW, 257, -9.f, 6.f, AM, "1.5"); num(aW, 272, -9.f, 6.f, AM, "2");
        num(aW, 283, -9.f, 6.f, AM, "3"); num(aW, 296, -9.f, 6.f, AM, "5");
        {
            const float y = yA(aW, 330) + 9.f;
            dl->AddCircle(Q(327 + dx, y), QD(2.6f), AM, 12, std::max(1.f, QD(1.2f)));
            dl->AddCircle(Q(332.5f + dx, y), QD(2.6f), AM, 12, std::max(1.f, QD(1.2f)));
        }
        tx(fP(), 6.5f, 209 + dx, 252, AM, "SWR", 0.5f);
    } else {
        // a 2a linha, ALC e VDD
        ox = 0.f;
        const Arco a2{600, 184, 0.00140f};
        arco(a2, 490, 710, AM, 1.4f);
        const Arco aL{615, 204, 0.00120f};
        arco(aL, 505, 612, RD, 4.2f);
        arco(aL, 615, 700, AM, 1.4f);
        tx(fP(), 7.f, 468, 227, RD, "ALC", 0.5f);
        const Arco aV{603, 222.5f, 0.00195f};
        arco(aV, 512, 695, AM, 1.4f);
        const float xv[13] = {521, 532, 542, 552, 562, 572, 581, 590, 598, 617, 637, 658, 672};
        for (int i = 0; i < 13; ++i) marca(aV, xv[i], 0.f, (i % 2 == 0) ? 4.f : 2.5f, AM, 1.2f);
        num(aV, 542, -10.f, 6.5f, AM, "0"); num(aV, 562, -10.f, 6.5f, AM, "10"); num(aV, 581, -10.f, 6.5f, AM, "20");
        num(aV, 598, -10.f, 6.5f, AM, "30"); num(aV, 617, -10.f, 6.5f, AM, "40"); num(aV, 637, -10.f, 6.5f, AM, "50"); num(aV, 658, -10.f, 6.5f, AM, "60");
        tx(fP(), 7.f, 680, 246, AM, "V", 0.5f);
        tx(fP(), 6.5f, 497, 246, AM, "VDD", 0.5f);
        ox = dx;
    }
    // o ponteiro: pivo escondido embaixo da escala, a ponta no arco S
    {
        const float p = std::clamp(rx == ativo_ ? smAtual_ : smOutro_, 0.f, 1.05f);
        static const float tp[18] = {0, 0.06f, 0.117f, 0.17f, 0.224f, 0.282f, 0.34f, 0.392f, 0.444f, 0.495f, 0.546f, 0.62f, 0.693f, 0.77f, 0.845f, 0.92f, 0.996f, 1.05f};
        static const float tx_[18] = {168, 182, 197, 210, 224, 238, 250, 262, 273, 283, 301, 318, 335, 352, 370, 391, 396, 402};
        float xs2 = tx_[0];
        for (int i = 0; i < 17; ++i)
            if (p >= tp[i] && p <= tp[i + 1]) { xs2 = tx_[i] + (tx_[i + 1] - tx_[i]) * (p - tp[i]) / (tp[i + 1] - tp[i]); break; }
        if (p > tp[17]) xs2 = tx_[17];
        const float px = 283.f, py = 300.f, ys = yA(aS, xs2);
        float ux = xs2 - px, uy = ys - py;
        const float L = std::sqrt(ux * ux + uy * uy);
        ux /= L; uy /= L;
        const ImU32 cn = lig ? IM_COL32(0xff, 0x5a, 0x6a, 255) : IM_COL32(0xb0, 0x30, 0x48, 255);
        dl->AddLine(Q(px + dx + ux * 40.f + 1.f, py + uy * 40.f + 1.5f), Q(px + dx + ux * (L + 10.f) + 1.f, py + uy * (L + 10.f) + 1.5f), IM_COL32(0, 0, 0, 90), std::max(1.f, QD(2.2f)));
        dl->AddLine(Q(px + dx + ux * 40.f, py + uy * 40.f), Q(px + dx + ux * (L + 10.f), py + uy * (L + 10.f)), cn, std::max(1.f, QD(1.8f)));
    }
    // a tampinha do pivo
    dl->AddEllipseFilled(Q(283 + dx, 262), ImVec2(QD(15), QD(9)), IM_COL32(0x18, 0x18, 0x1a, 255), 0.f, 32);
    dl->AddEllipse(Q(283 + dx, 262), ImVec2(QD(15), QD(9)), IM_COL32(0x60, 0x62, 0x66, 180), 0.f, 32, 1.f);
    dl->PopClipRect();
    if (dim_ > 0) dl->AddRectFilled(Q(fx0, fy0), Q(fx1, fy1), IM_COL32(0, 0, 0, dim_ * 50), QD(6));
    // o vidro (reflexo)
    dl->AddRectFilledMultiColor(Q(fx0 + 3, fy0 + 2), Q(fx1 - 3, fy0 + 26), IM_COL32(255, 255, 255, 14), IM_COL32(255, 255, 255, 8),
                                IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 0));
    ImGui::SetCursorScreenPos(Q(fx0, fy0));
    char id[16]; std::snprintf(id, sizeof id, "##med9%d", rx);
    if (ImGui::InvisibleButton(id, ImVec2(QD(fx1 - fx0), QD(fy1 - fy0)))) medidorModo_ = (medidorModo_ + 1) % 3;
    if (ImGui::IsItemHovered()) {
        char b[64];
        const float pos = std::clamp(rx == ativo_ ? smAtual_ : smOutro_, 0.f, 1.02f);
        if (!lig) std::snprintf(b, sizeof b, "--");
        else if (rx == ativo_ && medidorModo_ == 1) std::snprintf(b, sizeof b, "%.0f dB", peakDb_);
        else if (rx == ativo_ && medidorModo_ == 2) std::snprintf(b, sizeof b, "PEAK %.0f dB", peakDbSql_);
        else std::snprintf(b, sizeof b, "%s", leituraS(pos).c_str());
        ImGui::SetTooltip("S-meter do %s: %s%s\nClique: leitura em S / dB / PEAK (aparece na tela TFT)", rx == 0 ? "MAIN (VFO-A)" : "SUB (VFO-B)", b,
                          rx == ativo_ ? "  (receptor ouvido)" : "  (lido no espectro, no filtro dele)");
    }
}

// ===========================================================================
//  MB1SDR: o VFD (as duas frequencias em 7 segmentos, RX / TX, a caixa do
//  CLAR / canal de memoria) e a fileira de LEDs A-BUSY ... HI SWR
// ===========================================================================
void Ui::ft9Visor()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool lig = r_->ligado();
    // a moldura cinza e o vidro preto
    retGrad(dl, Q(758, 100), Q(1232, 264), QD(10), IM_COL32(0x5a, 0x5c, 0x60, 255), IM_COL32(0x26, 0x28, 0x2b, 255));
    dl->AddRect(Q(758, 100), Q(1232, 264), IM_COL32(0x0e, 0x0f, 0x10, 255), QD(10), 0, std::max(1.f, QD(1.5f)));
    dl->AddRectFilled(Q(778, 113), Q(1214, 254), IM_COL32(0x0c, 0x0d, 0x0e, 255), QD(8));
    dl->AddRectFilled(Q(786, 119), Q(1206, 249), IM_COL32(0x03, 0x02, 0x02, 255), QD(5));
    const ImU32 LAR = lig ? IM_COL32(0xff, 0x6c, 0x38, 255) : IM_COL32(0x5a, 0x2a, 0x1c, 255);
    const ImU32 FAN = lig ? IM_COL32(0x2a, 0x10, 0x0a, 255) : IM_COL32(0x10, 0x07, 0x05, 255);
    const ImU32 GLO = IM_COL32(0xff, 0x50, 0x20, 38);
    // ---- RX / TX de cada VFO ----
    auto etiqueta = [&](float y0, const char* t, bool aceso, ImU32 cor) {
        const ImVec2 a = Q(828, y0), b = Q(862, y0 + 13);
        if (aceso && lig) {
            dl->AddRectFilled(ImVec2(a.x - 2, a.y - 2), ImVec2(b.x + 2, b.y + 2), alfa(cor, 60), QD(2));
            dl->AddRectFilled(a, b, cor, QD(1.5f));
            tx(fP(), 6.5f, 845, y0 + 6.5f, IM_COL32(0x18, 0x10, 0x0c, 255), t, 0.5f, 1.3f);
        } else {
            dl->AddRectFilled(a, b, IM_COL32(0x12, 0x0c, 0x0a, 255), QD(1.5f));
            tx(fP(), 6.5f, 845, y0 + 6.5f, IM_COL32(0x30, 0x22, 0x1c, 255), t, 0.5f, 1.3f);
        }
    };
    const ImU32 VD = IM_COL32(0x58, 0xe0, 0x6a, 255), VM = IM_COL32(0xf0, 0x38, 0x50, 255);
    etiqueta(170, "TX", txVfo_ == 0, VM);
    etiqueta(186, "RX", ativo_ == 0, VD);
    etiqueta(217, "TX", txVfo_ == 1, VM);
    etiqueta(233, "RX", ativo_ == 1, VD);
    // ---- as frequencias ----
    std::vector<Dig7> cxA, cxB;
    const uint64_t fA = ativo_ == 0 ? r_->vfo() : rx_[0].f, fB = ativo_ == 1 ? r_->vfo() : rx_[1].f;
    if (entrando_) {
        const std::string t = (entrada_.empty() ? std::string("") : entrada_) + ((std::fmod(agora(), 1.0) < 0.5) ? "_" : " ");
        tx(fL(), 26.f, 1158, 185, LAR, t.c_str(), 1.f, 1.1f);
        tx(fL(), 9.f, 1160, 205, LAR, "kHz  (Enter / ENT)", 1.f);
    } else {
        if (lig) freq7(dl, fA, 1160, 164, 22, 41, 28, 8, GLO, 0, 6.5f, 0.08f, nullptr);
        freq7(dl, fA, 1160, 164, 22, 41, 28, 8, LAR, FAN, 3.9f, 0.08f, &cxA);
    }
    if (!bDispOff_) {
        if (lig) freq7(dl, fB, 1056, 219, 12.5f, 25, 17, 5, GLO, 0, 4.6f, 0.10f, nullptr);
        freq7(dl, fB, 1056, 219, 12.5f, 25, 17, 5, LAR, FAN, 2.9f, 0.10f, &cxB);
    }
    // a caixa da direita: o CLAR (RIT) do receptor ouvido ou o canal de memoria
    {
        dl->AddRect(Q(1066, 211), Q(1159, 246), LAR, QD(2), 0, std::max(1.f, QD(1.4f)));
        const Receptor& R = rx_[ativo_];
        if (R.memModo && !mem_.empty()) {
            const int ch = std::clamp(R.memCanal, 0, 99) + 1;
            seg7(dl, 1118, 219, 12.5f, 22, (ch / 10) % 10, LAR, FAN, 2.9f, 0.10f);
            seg7(dl, 1136, 219, 12.5f, 22, ch % 10, LAR, FAN, 2.9f, 0.10f);
            tx(fP(), 6.f, 1074, 228, LAR, "MEM");
        } else {
            const int r = std::abs(R.rit);
            const uint64_t v = (uint64_t)r;
            // 0.000 (kHz) com o sinal na frente
            float x = 1155;
            uint64_t peso = 1;
            for (int i = 0; i < 4; ++i) {
                if (i == 3) {
                    x -= 4.f;
                    dl->AddRectFilled(Q(x + 0.6f, 240), Q(x + 3.2f, 242.6f), LAR);
                }
                x -= 14.f;
                seg7(dl, x, 220, 10.5f, 22, (int)((v / peso) % 10), LAR, FAN, 2.6f, 0.10f);
                peso *= 10;
            }
            if (R.rit < 0) seg7(dl, x - 14.f, 220, 10.5f, 22, 10, LAR, 0, 2.6f, 0.10f);
            if (R.ritOn) tx(fP(), 6.f, 1072, 216.5f, LAR, "CLAR");
        }
    }
    // roda do mouse em cima de um digito: muda aquele digito; clique escolhe / digita
    auto rodaDig = [&](const std::vector<Dig7>& cx, int rx, float y0, float y1) {
        if (!ImGui::IsWindowHovered()) return;
        ImGuiIO& io = ImGui::GetIO();
        for (const Dig7& d : cx) {
            const ImVec2 a = Q(d.x0 - 2, y0), c = Q(d.x1 + 4, y1);
            if (!ImGui::IsMouseHoveringRect(a, c, false)) continue;
            dl->AddLine(ImVec2(a.x + 2, c.y + 2), ImVec2(c.x - 2, c.y + 2), IM_COL32(0x40, 0xc8, 0xff, 255), std::max(1.5f, QD(1.8f)));
            if (io.MouseWheel != 0) {
                int n = (int)std::lround(io.MouseWheel);
                if (n == 0) n = io.MouseWheel > 0 ? 1 : -1;
                mudarFreqRx(rx, (int64_t)n * (int64_t)d.peso);
                io.MouseWheel = 0;
            }
        }
    };
    rodaDig(cxA, 0, 164, 205);
    rodaDig(cxB, 1, 219, 244);
    for (int rx = 0; rx < 2; ++rx) {
        if (rx == 1 && bDispOff_) continue;
        ImGui::SetCursorScreenPos(rx == 0 ? Q(870, 160) : Q(870, 214));
        char id[16]; std::snprintf(id, sizeof id, "##vfd9%d", rx);
        if (ImGui::InvisibleButton(id, rx == 0 ? ImVec2(QD(292), QD(50)) : ImVec2(QD(190), QD(34)))) {
            if (rx != ativo_) ativar(rx);
            else { entrando_ = !entrando_; entrada_.clear(); }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s\n%s\nRoda do mouse em cima de um dígito: muda aquele dígito", rx == 0 ? "VFO-A (MAIN)" : "VFO-B (SUB)",
                              rx == ativo_ ? "Clique: digitar a frequência (kHz, Enter confirma, Esc cancela)" : "Clique: ouvir este receptor");
    }
    if (dim_ > 0) dl->AddRectFilled(Q(786, 119), Q(1206, 249), IM_COL32(0, 0, 0, dim_ * 50), QD(5));
    dl->AddRectFilledMultiColor(Q(790, 121), Q(1202, 150), IM_COL32(255, 255, 255, 10), IM_COL32(255, 255, 255, 6),
                                IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 0));
    // ---- a fileira de LEDs ----
    retGrad(dl, Q(812, 271), Q(1180, 286), QD(3), IM_COL32(0x4a, 0x4c, 0x50, 255), IM_COL32(0x1a, 0x1b, 0x1c, 255));
    dl->AddRectFilled(Q(815, 273), Q(1177, 284), IM_COL32(0x0c, 0x0c, 0x0d, 255), QD(2));
    const Receptor& R0 = rx_[0];
    const Receptor& R1 = rx_[1];
    auto ocupado = [&](int rx) {
        const Receptor& R = rx_[rx];
        return lig && r_->aoVivo() && ocupadoRx(rx);
    };
    (void)R0; (void)R1;
    struct L9 { float x; const char* t; int cor; bool on; const char* dica; };
    const L9 leds[7] = {
        {830, "A-BUSY", 2, ocupado(0), "A-BUSY: sinal no MAIN (VFO-A)"},
        {860, "TX", 4, sendMudo_, "TX: aceso = áudio todo em MUTE (MOX)"},
        {890, "B-BUSY", 2, ocupado(1), "B-BUSY: sinal no SUB (VFO-B)"},
        {997, "", 3, r_->som().gravando(), "Gravando o áudio (REC)"},
        {1102, "SPLIT", 3, txVfo_ != ativo_, "SPLIT: o outro receptor está marcado como TX"},
        {1132, "DUAL", 2, dual_, "DUAL: os dois escopos na tela"},
        {1162, "HI SWR", 4, lig && r_->aoVivo() && smAtual_ > 0.93f, "HI SWR: sinal muito forte (passou de S9+40)"}};
    for (const L9& l : leds) {
        const ImU32 c = corLedIc(l.cor);
        if (l.on) dl->AddRectFilled(Q(l.x - 11, 274), Q(l.x + 11, 283.5f), alfa(c, 70), QD(2));
        dl->AddRectFilled(Q(l.x - 8, 276), Q(l.x + 8, 281.5f), l.on ? c : IM_COL32(0x2a, 0x2c, 0x2e, 255), QD(1));
        if (*l.t) tx(fP(), 5.6f, l.x + (l.t[0] == 'H' ? 4.f : 0.f), 292, IM_COL32(0xe8, 0xea, 0xee, 255), l.t, 0.5f, 0.95f);
        if (ImGui::IsMouseHoveringRect(Q(l.x - 12, 271), Q(l.x + 12, 297)) && ImGui::IsWindowHovered()) ImGui::SetTooltip("%s", l.dica);
    }
}

// ===========================================================================
//  MB1SDR: a tela TFT - as informacoes do VFO-A e do VFO-B em cima, o
//  escopo (espectro + cachoeira) no lugar do mapa-mundi e as teclas da tela
// ===========================================================================
void Ui::ft9Tft()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool lig = r_->ligado();
    // a moldura (cinza) com os parafusos e o vidro
    retGrad(dl, Q(1238, 95), Q(1890, 470), QD(12), IM_COL32(0x3a, 0x3c, 0x40, 255), IM_COL32(0x22, 0x24, 0x27, 255));
    dl->AddRect(Q(1238, 95), Q(1890, 470), IM_COL32(0x0e, 0x0f, 0x10, 255), QD(12), 0, std::max(1.f, QD(1.5f)));
    dl->AddRectFilled(Q(1284, 113), Q(1824, 428), IM_COL32(0x0a, 0x0a, 0x0b, 255), QD(5));
    dl->AddRect(Q(1284, 113), Q(1824, 428), IM_COL32(0x6a, 0x6c, 0x70, 200), QD(5), 0, 1.f);
    parafuso9(1253, 112, 6); parafuso9(1872, 112, 6); parafuso9(1253, 457, 6); parafuso9(1872, 457, 6);
    const float X0 = 1289, X1 = 1819, Y0 = 117, Y1 = 424;
    dl->AddRectFilled(Q(X0, Y0), Q(X1, Y1), IM_COL32(0, 0, 0, 255));
    const ImU32 W = lig ? IM_COL32(0xf2, 0xf4, 0xf6, 255) : IM_COL32(0x70, 0x72, 0x76, 255);
    const ImU32 CI = lig ? IM_COL32(0xa8, 0xac, 0xb4, 255) : IM_COL32(0x50, 0x52, 0x56, 255);
    const ImU32 AMB = lig ? IM_COL32(0xff, 0xb8, 0x48, 255) : IM_COL32(0x70, 0x58, 0x30, 255);
    const ImU32 AZ = IM_COL32(0x9c, 0xc8, 0xff, 255);
    dl->PushClipRect(Q(X0, Y0), Q(X1, Y1), true);
    char b[96];
    float yEsc = 197.f;
    if (!expand_) {
        // ---- a faixa de cima: VFO-A e VFO-B ----
        for (int rx = 0; rx < 2; ++rx) {
            const float x0 = rx == 0 ? 1293.f : 1490.f;
            Receptor& R = rx_[rx];
            const bool at = rx == ativo_;
            const std::string md = at ? r_->modo() : R.modo;
            const int g = grupoModo(md);
            const uint64_t f = at ? r_->vfo() : R.f;
            tx(fP(), 7.f, x0, 126, W, rx == 0 ? "VFO-A" : "VFO-B", 0.f, 0.95f);
            float xx = x0 + txW(fP(), 7.f, rx == 0 ? "VFO-A" : "VFO-B", 0.95f) + 2.f;
            tx(fP(), 5.f, xx, 124, at ? IM_COL32(0x60, 0xe8, 0x70, 255) : IM_COL32(0x40, 0x44, 0x48, 255), "RX");
            tx(fP(), 5.f, xx + 9, 124, txVfo_ == rx ? IM_COL32(0xff, 0x40, 0x50, 255) : IM_COL32(0x40, 0x44, 0x48, 255), "TX");
            tx(fP(), 5.f, x0, 134, CI, nomeModo(md).c_str());
            const char* dev = !r_->temDispositivo() ? "--" : r_->tipo() == "rtltcp" ? "TCP" : r_->tipo() == "sdrplay" ? "RSP" : "RTL";
            std::snprintf(b, sizeof b, "ANT %s", dev);
            tx(fP(), 6.f, x0 + 52, 131, W, b);
            // CLAR
            dl->AddRect(Q(x0 + 93, 126), Q(x0 + 132, 135), CI, QD(1), 0, 1.f);
            if (R.ritOn) std::snprintf(b, sizeof b, "CLAR%+.2f", R.rit / 1000.0); else std::snprintf(b, sizeof b, "CLAR OFF");
            tx(fP(), 4.6f, x0 + 112.5f, 130.5f, R.ritOn ? AMB : W, b, 0.5f, 0.9f, 37.f);
            // a frequencia em ambar
            std::snprintf(b, sizeof b, "%s", fmtFreqLcd(f).c_str());
            tx(fL(), 11.f, x0 + 2, 145, AMB, b, 0.f, 1.15f);
            // BW e o desenho do filtro
            const float bw = at ? (float)r_->banda() : (g == 2 ? filHi_[g][R.fil] * 2.f : filHi_[g][R.fil]);
            tx(fP(), 4.6f, x0 + 157, 125, W, "BW", 0.5f);
            if (bw >= 1000) std::snprintf(b, sizeof b, "%.2fkHz", bw / 1000.f); else std::snprintf(b, sizeof b, "%.0fHz", bw);
            tx(fP(), 5.5f, x0 + 157, 132, W, b, 0.5f);
            {
                static const float mx[5] = {5000, 2500, 16000, 30000, 250000};
                const float fr = std::clamp(bw / mx[std::clamp(g, 0, 4)], 0.08f, 1.f);
                const float cxx = x0 + 157, ht = 4.f + 11.f * fr, hb = ht + 4.f;
                dl->AddRectFilled(Q(cxx - 20, 137), Q(cxx + 20, 168), IM_COL32(0x80, 0x9c, 0xb8, 255));
                ImVec2 p[4] = {Q(cxx - hb, 166), Q(cxx - ht, 141), Q(cxx + ht, 141), Q(cxx + hb, 166)};
                dl->AddConvexPolyFilled(p, 4, IM_COL32(0xf4, 0xf8, 0xff, 255));
                ImGui::SetCursorScreenPos(Q(cxx - 20, 122));
                char idc[16]; std::snprintf(idc, sizeof idc, "##bw9%d", rx);
                if (ImGui::InvisibleButton(idc, ImVec2(QD(40), QD(46)))) { if (!at) ativar(rx); ImGui::OpenPopup("##bwIc"); }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("BW: largura do filtro do %s - clique para escolher\n(o SHIFT - WIDTH também ajusta)", rx == 0 ? "MAIN" : "SUB");
            }
            // VRF (barra = ganho de RF), FIL (no lugar do ROOFING) e SHIFT
            tx(fP(), 5.f, x0, 179, W, "RF");
            dl->AddRectFilled(Q(x0 + 13, 175), Q(x0 + 88, 184), IM_COL32(0x40, 0x42, 0x46, 255));
            dl->AddRectFilled(Q(x0 + 13, 175), Q(x0 + 13 + 75.f * std::clamp(R.rf / 100.f, 0.f, 1.f), 184), IM_COL32(0x70, 0x74, 0x7c, 255));
            dl->AddRectFilled(Q(x0 + 11 + 75.f * std::clamp(R.rf / 100.f, 0.f, 1.f), 174), Q(x0 + 15 + 75.f * std::clamp(R.rf / 100.f, 0.f, 1.f), 185), IM_COL32(0xff, 0x40, 0x30, 255));
            std::snprintf(b, sizeof b, "FIL%d", R.fil + 1);
            tx(fP(), 4.5f, x0 + 92, 174, W, "ROOFING");
            tx(fP(), 5.f, x0 + 133, 174, W, b);
            {
                const float lo = filLo_[g][R.fil], hi = filHi_[g][R.fil];
                const int sft = g == 0 ? (int)std::lround(((lo + hi) * 0.5f - 1450.f) / 10.f) * 10 : g == 1 ? (int)std::lround(lo) : 0;
                std::snprintf(b, sizeof b, "SHIFT %+.2f", sft / 1000.0);
            }
            tx(fP(), 5.f, x0 + 118, 186, W, b);
            // leitura do S-meter (METER)
            {
                const float pos = std::clamp(at ? smAtual_ : smOutro_, 0.f, 1.02f);
                if (!lig) std::snprintf(b, sizeof b, "--");
                else if (at && medidorModo_ == 1) std::snprintf(b, sizeof b, "%.0fdB", peakDb_);
                else if (at && medidorModo_ == 2) std::snprintf(b, sizeof b, "PK%.0f", peakDbSql_);
                else std::snprintf(b, sizeof b, "%s", leituraS(pos).c_str());
                tx(fP(), 5.5f, x0 + 60, 186, AZ, b);
            }
        }
        // a data e a hora (local e UTC)
        {
            SYSTEMTIME lt, ut;
            GetLocalTime(&lt); GetSystemTime(&ut);
            std::snprintf(b, sizeof b, "%04d/%02d/%02d", lt.wYear, lt.wMonth, lt.wDay);
            tx(fP(), 6.5f, 1690, 126, W, b);
            std::snprintf(b, sizeof b, "%02d:%02d:%02d", lt.wHour, lt.wMinute, lt.wSecond);
            tx(fP(), 7.f, 1690, 138, W, b);
            std::snprintf(b, sizeof b, "UTC %02d:%02d", ut.wHour, ut.wMinute);
            tx(fP(), 6.f, 1690, 150, CI, b);
            if (rx_[ativo_].lock) tx(fP(), 6.f, 1770, 150, IM_COL32(0xff, 0xd0, 0x40, 255), "LOCK");
            if (fast_) tx(fP(), 6.f, 1770, 138, IM_COL32(0xff, 0xa0, 0x30, 255), "FAST");
        }
        // o que esta ligado (etiquetas pequenas)
        {
            float xi = 1690;
            auto ind = [&](const char* t, ImU32 c) {
                const float w = txW(fP(), 4.8f, t) + 5.f;
                if (xi + w > 1816) return;
                dl->AddRect(Q(xi, 158), Q(xi + w, 167), alfa(c, 200), QD(1.5f), 0, 1.f);
                tx(fP(), 4.8f, xi + w * 0.5f, 162.5f, c, t, 0.5f);
                xi += w + 3.f;
            };
            const Receptor& R = rx_[ativo_];
            const ImU32 cz = IM_COL32(0xc8, 0xcc, 0xd2, 255), lr = IM_COL32(0xff, 0xa0, 0x30, 255);
            if (R.nb1 || R.nb2) ind(R.nb2 ? "NB2" : "NB", cz);
            if (R.nr2) ind("DNR", cz);
            if (R.nr1) ind("NR1", lr);
            if (R.nch) ind("NTCH", cz);
            if (R.anch) ind("DNF", cz);
            if (R.bef) ind("CONT", cz);
            if (R.apf) ind("APF", cz);
            if (R.bc) ind("BC", cz);
            if (R.mute) ind("MUTE", IM_COL32(0xff, 0x50, 0x40, 255));
            if (R.sql > -90.f) { std::snprintf(b, sizeof b, "SQL%+d", (int)std::lround(R.sql)); ind(b, (lig && !sqlAberto_) ? IM_COL32(0xff, 0x50, 0x40, 255) : lr); }
            if (att_) { std::snprintf(b, sizeof b, "ATT%d", att_ * 6); ind(b, lr); }
            if (pamp_) ind(pamp_ == 1 ? "AMP1" : "AMP2", lr);
            if (proc_) ind("PROC", lr);
        }
        tx(fP(), 6.5f, 1745, 186, W, mem_.empty() ? "SCOPE" : "SCOPE / MEM", 0.5f);
        dl->AddLine(Q(X0, 193), Q(X1, 193), IM_COL32(0x3a, 0x3c, 0x40, 255), 1.f);
    } else yEsc = 121.f;
    ft9Escopos(X0 + 2, yEsc, X1 - 2, 401.f);
    // ---- os rotulos das teclas de toque (F1 ... DISP) ----
    {
        static const char* p1[8] = {"SPAN", "HOLD", "CENT/FIX", "DUAL", "EXPAND", "LEVEL", "SET", "PAGE 2"};
        static const char* p2[8] = {"IF SCOPE", "DECODE", "MEMÓRIA", "RADAR", "REC", "PLAY", "TIMER", "PAGE 1"};
        const char** pg = paginaSoft_ == 0 ? p1 : p2;
        for (int i = 0; i < 8; ++i) {
            const float xa = X0 + 2 + i * 66.1f, xb = xa + 64.f;
            bool on = false;
            if (paginaSoft_ == 0) on = (i == 0 && zoom_ > 0) || (i == 1 && pausa_) || (i == 2 && modoCentro_) || (i == 3 && dual_) || (i == 4 && expand_);
            else on = (i == 0 && ifOn_) || (i == 1 && decAberta_) || (i == 2 && memAberta_) || (i == 4 && r_->som().gravando()) || (i == 6 && sleepAte_ > 0);
            retGrad(dl, Q(xa, 404), Q(xb, 421), QD(1.5f), on ? IM_COL32(0x60, 0x48, 0x20, 255) : IM_COL32(0x44, 0x46, 0x4a, 255), IM_COL32(0x22, 0x23, 0x25, 255));
            dl->AddRect(Q(xa, 404), Q(xb, 421), IM_COL32(0x6c, 0x6e, 0x72, 255), QD(1.5f), 0, 1.f);
            tx(fP(), 6.5f, (xa + xb) * 0.5f, 412.5f, on ? IM_COL32(0xff, 0xc0, 0x50, 255) : W, pg[i], 0.5f, 0.95f, 60.f);
            char kv[8]; std::snprintf(kv, sizeof kv, "KEY%d", i + 1);
            tx(fP(), 3.4f, xb - 2, 406.5f, CI, kv, 1.f);
        }
    }
    dl->PopClipRect();
    if (!lig) {
        dl->AddRectFilled(Q(1380, 280), Q(1730, 315), IM_COL32(0, 0, 0, 215), QD(5));
        tx(fP(), 9.f, 1555, 297.5f, P_AMBAR, "Aperte POWER (à esquerda) para ligar", 0.5f);
    }
    if (dim_ > 0) dl->AddRectFilled(Q(X0, Y0), Q(X1, Y1), IM_COL32(0, 0, 0, dim_ * 50));
    // ---- as teclas prateadas dos lados (bandas, teclado numerico com ENT) ----
    {
        static const char* esq[8] = {"1.8", "3.5", "5", "7", "10", "14", "18", "21"};
        static const char* dir[8] = {"24.5", "28", "50", "TRV", "GEN", "ENT", "V-B", "MNU"};
        static const int bEsq[8] = {0, 1, 2, 3, 4, 5, 6, 7};
        static const char* numE[8] = {"1", "2", "3", "4", "5", "6", "7", "8"};
        const float ys[8] = {162, 197, 231, 266, 300, 334, 368, 402};
        auto banda = [&](uint64_t f) { for (int k = 0; k < kNB; ++k) if (f >= kBandasHam[k].lo && f <= kBandasHam[k].hi) return k; return -1; };
        const int bAt = banda(r_->vfo());
        const ImU32 vermelho = IM_COL32(0xf0, 0x40, 0x34, 255);
        for (int i = 0; i < 8; ++i) {
            char id[12], d[200];
            // esquerda: 1.8 ... 21 (e os numeros 1 ... 8 no teclado)
            tx(fP(), 6.f, 1264, ys[i] - 15.5f, vermelho, numE[i], 0.5f);
            std::snprintf(id, sizeof id, "tE%d", i);
            std::snprintf(d, sizeof d, "Banda de %s MHz para o %s (volta onde estava nesta banda)\nCom o ENT aceso: o número %s", kBandasHam[bEsq[i]].rot,
                          ativo_ == 0 ? "MAIN" : "SUB", numE[i]);
            const int k = tecla9(id, esq[i], 1247, ys[i] - 10, 1281, ys[i] + 10, 0, 0, d, 0, bAt == bEsq[i] ? IM_COL32(0xff, 0xa8, 0x40, 255) : 0u, 7.f);
            if (k == 1) { if (entrando_) digito(i + 1); else irBanda(bEsq[i]); }
            // direita
            std::snprintf(id, sizeof id, "tD%d", i);
            const char* dd = "";
            switch (i) {
            case 0: dd = "Banda de 24,5 MHz (12 m)\nCom o ENT aceso: o número 9"; break;
            case 1: dd = "Banda de 28 MHz (10 m)\nCom o ENT aceso: o número 0"; break;
            case 2: dd = "Banda de 50 MHz (6 m)\nCom o ENT aceso: o ponto decimal"; break;
            case 3: dd = "TRV: cobertura geral VHF / UHF (aviação, marítimo, FM, satélites...)"; break;
            case 4: dd = "GEN: cobertura geral HF (ondas curtas, médias, utilitárias) e 70 MHz"; break;
            case 5: dd = entrando_ ? "ENT: confirma a frequência digitada" : "ENT: digitar a frequência pelas teclas de banda (os números vermelhos), em kHz"; break;
            case 6: dd = "V-B: o dial, o som e os botões passam para o outro receptor (VFO-A / VFO-B)"; break;
            default: dd = "MNU: menu (aparelho, decodificadores, memórias, gravador, configuração...)"; break;
            }
            int led = 0;
            if (i == 0 && bAt == 8) led = 1; else if (i == 1 && bAt == 9) led = 1; else if (i == 2 && bAt == 10) led = 1;
            else if (i == 5 && entrando_) led = 1; else if (i == 6 && ativo_ == 1) led = 1;
            const int k2 = tecla9(id, dir[i], 1836, ys[i] - 10, 1870, ys[i] + 10, 0, 0, dd, 0, led ? IM_COL32(0xff, 0xa8, 0x40, 255) : 0u, 7.f);
            if (k2 == 1) {
                switch (i) {
                case 0: if (entrando_) digito(9); else irBanda(8); break;
                case 1: if (entrando_) digito(0); else irBanda(9); break;
                case 2: if (entrando_) { if (entrada_.find('.') == std::string::npos) entrada_ += '.'; } else irBanda(10); break;
                case 3: ImGui::OpenPopup("##trv9"); break;
                case 4: ImGui::OpenPopup("##genIc"); break;
                case 5: if (entrando_) confirmarEntrada(); else { entrando_ = true; entrada_.clear(); } break;
                case 6: ativar(1 - ativo_); break;
                default: ImGui::OpenPopup("##funcFt"); break;
                }
            }
        }
        tx(fP(), 6.f, 1853, 146.5f, vermelho, "9", 0.5f);
        tx(fP(), 6.f, 1853, 181.5f, vermelho, "0", 0.5f);
        tx(fP(), 6.f, 1853, 216.f, vermelho, ".", 0.5f);
        dl->AddTriangleFilled(Q(1849, 287), Q(1857, 287), Q(1853, 282), vermelho);
        dl->AddTriangleFilled(Q(1849, 313), Q(1857, 313), Q(1853, 318), vermelho);
        if (ImGui::BeginPopup("##trv9")) {
            ImGui::TextDisabled("TRV - VHF / UHF");
            itensGen(1);
            ImGui::EndPopup();
        }
    }
    // ---- F1 ... F7 e DISP (embaixo da tela) ----
    {
        const float xs[8] = {1340, 1402, 1465, 1526, 1588, 1650, 1712, 1775};
        static const char* nm[8] = {"F 1", "F 2", "F 3", "F 4", "F 5", "F 6", "F 7", "DISP"};
        static const char* d1[8] = {"F1 - SPAN: largura do escopo", "F2 - HOLD: congela o espectro e a cachoeira",
                                     "F3 - CENT/FIX: o escopo acompanha a sintonia (CENT) ou fica parado (FIX)",
                                     "F4 - DUAL: dois escopos (VFO-A à esquerda, VFO-B à direita)",
                                     "F5 - EXPAND: o escopo ocupa a tela inteira (esconde as informações de cima)",
                                     "F6 - LEVEL: ajuste automático do escopo (AUTO)\nSegurar: guarda a aparência como padrão",
                                     "F7 - SET: ajustes do escopo (velocidade, nível, contraste, cores)",
                                     "DISP: troca a página das teclas F (PAGE 1 / PAGE 2)\nSegurar: brilho da tela (DIM)"};
        static const char* d2[8] = {"F1 - IF SCOPE: zoom de 20 kHz em volta da sintonia", "F2 - DECODE: os decodificadores digitais (RTTY, PSK, FT8, CW, DMR, ADS-B...)",
                                     "F3 - MEMÓRIA: a lista de memórias (gravar, chamar, apagar)", "F4 - RADAR: o radar aéreo (ADS-B / ACARS / VDL2 / HFDL)",
                                     "F5 - REC: grava o áudio (pasta Gravacoes)", "F6 - PLAY: toca a última gravação\nSegurar: para de tocar",
                                     "F7 - TIMER: desligar o rádio depois de um tempo", "DISP: troca a página das teclas F (PAGE 1 / PAGE 2)\nSegurar: brilho da tela (DIM)"};
        for (int i = 0; i < 8; ++i) {
            char id[12]; std::snprintf(id, sizeof id, "tF%d", i);
            const int k = tecla9(id, nm[i], xs[i] - 22, 441, xs[i] + 22, 458, 4, 0, paginaSoft_ == 0 ? d1[i] : d2[i], 0, 0, 5.5f);
            if (!k) continue;
            if (i == 7) {
                if (k == 1) paginaSoft_ = 1 - paginaSoft_;
                else { dim_ = (dim_ + 1) % 4; const char* n4[4] = {"100%", "75%", "50%", "30%"}; aviso(std::string("DIMMER ") + n4[dim_]); }
                continue;
            }
            if (paginaSoft_ == 0) {
                switch (i) {
                case 0: if (k == 1) ImGui::OpenPopup("##spanIc"); break;
                case 1: if (k == 1) { pausa_ = !pausa_; if (pausa_) congelado_ = suave_; } break;
                case 2: if (k == 1) { modoCentro_ = !modoCentro_; if (modoCentro_) r_->centralizar(r_->centroSemDc(r_->vfo())); } break;
                case 3: if (k == 1) dual_ = !dual_; break;
                case 4: if (k == 1) expand_ = !expand_; break;
                case 5: if (k == 1) autoCachoeira(); else padrao(); break;
                default: if (k == 1) ImGui::OpenPopup("##smenuIc"); break;
                }
            } else {
                switch (i) {
                case 0: if (k == 1) ifOn_ = !ifOn_; break;
                case 1: if (k == 1) decAberta_ = !decAberta_; break;
                case 2: if (k == 1) { memAberta_ = !memAberta_; std::snprintf(memNome_, sizeof memNome_, "%s %s", fmtFreq(r_->vfo()).c_str(), r_->modo().c_str()); } break;
                case 3: if (k == 1) abrirRadar(); break;
                case 4: if (k == 1) alternarGravacao(); break;
                case 5: if (k == 1) tocarUltimaGravacao(); else { PlaySoundA(nullptr, nullptr, 0); aviso("PLAY parado"); } break;
                default: if (k == 1) ImGui::OpenPopup("##timerIc"); break;
                }
            }
        }
    }
}

// O escopo da tela TFT (o principal e, com DUAL, o do outro receptor), em px da foto
void Ui::ft9Escopos(float X0, float Y0, float X1, float Y1)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool lig = r_->ligado();
    const ImU32 W = lig ? IM_COL32(0xf4, 0xf4, 0xf6, 255) : IM_COL32(0x80, 0x82, 0x86, 255);
    char b[64];
    auto fotoX = [&](float sx) { return (sx - M.ox) / (M.k * kK) + 6.f; };
    const float yLab = Y1 - 7.f;
    auto rotulosBaixo = [&](float xa, float xb, double lo, double hi) {
        const double sp = hi - lo;
        static const double passos[] = {500, 1000, 2000, 2500, 5000, 10000, 20000, 25000, 50000, 100000, 200000, 250000, 500000, 1e6, 2e6, 5e6};
        double st = passos[0];
        const double nMax = (xb - xa) > 300 ? 6.5 : 3.5;
        for (double p : passos) { st = p; if (sp / p <= nMax) break; }
        for (double f = std::ceil(lo / st) * st; f <= hi; f += st) {
            const float x = xa + (float)((f - lo) / sp) * (xb - xa);
            if (x < xa + 16 || x > xb - 16) continue;
            dl->AddLine(Q(x, Y1 - 14), Q(x, Y1 - 11), W, 1.f);
            if (st >= 10000) std::snprintf(b, sizeof b, "%.2f", f / 1e6); else std::snprintf(b, sizeof b, "%.3f", f / 1e6);
            tx(fP(), 5.f, x, yLab, W, b, 0.5f);
        }
    };
    auto bordas = [&](float xa, float xb, double lo, double hi, float y) {
        std::snprintf(b, sizeof b, "%.3f", lo / 1e6); tx(fP(), 5.5f, xa + 4, y, W, b, 0.f);
        std::snprintf(b, sizeof b, "%.3f", hi / 1e6); tx(fP(), 5.5f, xb - 4, y, W, b, 1.f);
    };
    const float yTop = Y0 + 12.f;           // a linha de cima (bordas e o M)
    const float xm0 = (X0 + X1) * 0.5f;
    const bool dir = dual_ && ativo_ == 1;
    const float xA0 = !dual_ ? X0 : dir ? xm0 + 2 : X0, xA1 = !dual_ ? X1 : dir ? X1 : xm0 - 2;
    {   // trocou o receptor em uso: a cachoeira do outro comeca limpa
        static int ultAtivo = -1;
        if (ultAtivo != ativo_) { if (subTex_) criarTexturaSub(); ultAtivo = ativo_; }
    }
    const double span = (taxaMostrada_ ? taxaMostrada_ : r_->taxa()) / (1.0 + zoom_);
    {   // o principal (receptor ouvido)
        dl->AddRect(Q(xA0, Y0), Q(xA1, Y1), dual_ ? IM_COL32(0xff, 0x98, 0x10, 255) : IM_COL32(0x50, 0x52, 0x56, 255), QD(2), 0, std::max(1.f, QD(dual_ ? 1.8f : 1.f)));
        const ImVec2 A = Q(xA0 + 2, yTop), B = Q(xA1 - 2, Y1 - 16);
        const float sx = A.x, sy = A.y, sw = B.x - A.x, sh = B.y - A.y;
        float xm = sx;
        if (ifOn_) {
            const float wIf = std::floor(sw * 0.26f);
            ifDisplay(sx, sy, wIf, sh);
            xm = sx + wIf + 2.f;
        }
        const float wm = sx + sw - xm;
        const float hEsp = std::floor(sh * 0.50f);
        wfVelDesenho_ = wfSpeedEsc_;
        const bool troca = pausa_ && congelado_.size() == suave_.size() && !suave_.empty();
        if (troca) suave_.swap(congelado_);
        semDb_ = true;
        dl->PushClipRect(ImVec2(xm, sy), ImVec2(xm + wm, sy + sh), true);
        espectro(xm, sy, wm, hEsp);
        dl->PopClipRect();
        semDb_ = false;
        if (troca) suave_.swap(congelado_);
        cachoeira(xm, sy + hEsp, wm, sh - hEsp);
        const double lo = xParaHz(xm, xm, wm), hi = xParaHz(xm + wm, xm, wm);
        bordas(fotoX(xm), xA1 - 2, lo, hi, Y0 + 6);
        rotulosBaixo(fotoX(xm), xA1 - 2, lo, hi);
        {
            const float xv = (float)hzParaX((double)r_->vfo(), xm, wm);
            if (xv >= xm && xv <= xm + wm) {
                const float fx = fotoX(xv);
                dl->AddRectFilled(Q(fx - 5, Y0 + 1.5f), Q(fx + 5, Y0 + 10.5f), IM_COL32(0xe0, 0x30, 0x28, 255), QD(2));
                tx(fP(), 5.5f, fx, Y0 + 6, IM_COL32(0xff, 0xff, 0xff, 255), ativo_ == 0 ? "A" : "B", 0.5f);
            }
            if (!mono_) {
                const float xo = (float)hzParaX((double)rx_[1 - ativo_].f, xm, wm);
                if (xo >= xm && xo <= xm + wm)
                    for (float y = sy; y < sy + hEsp; y += QD(7)) dl->AddLine(ImVec2(xo, y), ImVec2(xo, std::min(sy + hEsp, y + QD(4))), IM_COL32(0x30, 0xe0, 0x50, 230), std::max(1.f, QD(1.2f)));
            }
        }
        const Receptor& R = rx_[ativo_];
        if ((R.nch || R.bef) && (r_->modo() == "USB" || r_->modo() == "LSB")) {
            const double nh = 150.0 * std::pow(3500.0 / 150.0, std::clamp(R.nch ? R.notch : R.cont, 0.f, 1.f));
            const double fn = r_->modo() == "USB" ? (double)r_->vfo() + nh : (double)r_->vfo() - nh;
            const float xn = (float)hzParaX(fn, xm, wm);
            dl->AddLine(ImVec2(xn, sy), ImVec2(xn, sy + hEsp), alfa(P_CIANO, 200), std::max(1.f, QD(1.5f)));
        }
        if (pausa_ && std::fmod(agora(), 1.0) < 0.6) tx(fP(), 9.f, (xA0 + xA1) * 0.5f, yTop + 14, P_AMBAR, "HOLD", 0.5f);
    }
    if (dual_) {   // o do outro receptor
        const int o = 1 - ativo_;
        Receptor& O = rx_[o];
        const float xB0 = dir ? X0 : xm0 + 2, xB1 = dir ? xm0 - 2 : X1;
        dl->AddRect(Q(xB0, Y0), Q(xB1, Y1), IM_COL32(0x50, 0x52, 0x56, 255), QD(2), 0, 1.f);
        subCentro_ = (double)O.f;
        subSpan_ = span;
        const ImVec2 A = Q(xB0 + 2, yTop), B = Q(xB1 - 2, Y1 - 16);
        escopoSub(A.x, A.y, B.x - A.x, B.y - A.y);
        const double lo = subCentro_ - span / 2, hi = subCentro_ + span / 2;
        bordas(xB0 + 2, xB1 - 2, lo, hi, Y0 + 6);
        rotulosBaixo(xB0 + 2, xB1 - 2, lo, hi);
        {
            const float xc = (A.x + B.x) * 0.5f, hs = std::floor((B.y - A.y) * 0.50f);
            for (float y = A.y; y < A.y + hs; y += QD(7)) dl->AddLine(ImVec2(xc, y), ImVec2(xc, std::min(A.y + hs, y + QD(4))), IM_COL32(0x30, 0xe0, 0x50, 230), std::max(1.f, QD(1.2f)));
            const float fx = fotoX(xc);
            dl->AddRectFilled(Q(fx - 5, Y0 + 1.5f), Q(fx + 5, Y0 + 10.5f), IM_COL32(0x20, 0xa0, 0x40, 255), QD(2));
            tx(fP(), 5.5f, fx, Y0 + 6, IM_COL32(0xff, 0xff, 0xff, 255), o == 0 ? "A" : "B", 0.5f);
        }
        ImGui::SetCursorScreenPos(A);
        ImGui::InvisibleButton("##subesc9", ImVec2(std::max(1.f, B.x - A.x), std::max(1.f, B.y - A.y)));
        if (ImGui::IsItemHovered()) {
            ImGuiIO& io = ImGui::GetIO();
            const double hz = lo + (io.MousePos.x - A.x) / std::max(1.f, B.x - A.x) * span;
            dl->AddLine(ImVec2(io.MousePos.x, A.y), ImVec2(io.MousePos.x, B.y), IM_COL32(255, 255, 255, 60));
            ImGui::SetTooltip("%s  (%s)\nClique: sintoniza o %s aqui   Duplo clique: passa a ouvir o %s\nRoda do mouse: um passo",
                              fmtFreq((uint64_t)std::max(0.0, hz)).c_str(), o == 0 ? "VFO-A" : "VFO-B", o == 0 ? "VFO-A" : "VFO-B", o == 0 ? "VFO-A" : "VFO-B");
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) ativar(o);
            else if (ImGui::IsItemClicked() && !O.lock) {
                const int64_t p = std::max(1, passo_);
                O.f = (uint64_t)std::max<int64_t>(1000, (int64_t)std::llround(hz / p) * p);
                O.memModo = false;
            }
            if (io.MouseWheel != 0 && !O.lock) {
                O.f = (uint64_t)std::max<int64_t>(1000, (int64_t)O.f + (io.MouseWheel > 0 ? passo_ : -passo_));
                io.MouseWheel = 0;
            }
        }
    } else {
        // largura e grade (como no radio, no canto de cima)
        const double g = span / 10.0;
        if (g >= 1e6) std::snprintf(b, sizeof b, "SPAN %.1fM", span / 1e6);
        else std::snprintf(b, sizeof b, "SPAN %.0fk", span / 1000.0);
        tx(fP(), 5.5f, X0 + (X1 - X0) * 0.64f, Y0 + 6, IM_COL32(0x9c, 0xc8, 0xff, 255), b, 0.5f);
    }
}

// ===========================================================================
//  MB1SDR: o lado esquerdo - MOX / VOX / DIM, PHONES, KEY, MIC, IPO,
//  ANTENNA SELECT, TUNE, POWER e os botoes do MAIN (METER ... NB-SQL)
// ===========================================================================
void Ui::ft9Esquerda()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool lig = r_->ligado();
    const ImU32 BR = IM_COL32(0xf4, 0xf4, 0xf4, 255);
    Receptor& M0 = rx_[0];
    // ---- MOX / VOX / DIM ----
    tx(fP(), 7.5f, 95, 140, BR, "MOX", 0.5f);
    if (tecla9("kMox", "", 77, 150, 113, 167, 0, 0, "MOX: (num receptor) silencia todo o áudio - MUTE geral\nO LED TX do VFD acende", 0, 0, 0) == 1) sendMudo_ = !sendMudo_;
    if (sendMudo_) dl->AddRectFilled(Q(84, 157), Q(106, 160), IM_COL32(0xff, 0x40, 0x30, 255), QD(1));
    tx(fP(), 7.5f, 95, 180, BR, "VOX", 0.5f);
    {
        Receptor& Rv = rx_[ativo_];
        if (Rv.sql <= -90.f) sqlAuto_[ativo_] = false;
        const bool ligado = sqlAuto_[ativo_];
        if (tecla9("kVox", "", 77, 190, 113, 207, 0, 0,
                   "VOX: squelch automático do receptor ouvido.\nClique: mede o ruído por 4 s e fecha o squelch logo acima.\n"
                   "Aceso: clique de novo para abrir o squelch.\nMargem e demora: botão VOX - DELAY", 0, 0, 0) == 1) {
            if (sqlMedindo_) { sqlMedindo_ = false; Rv.sql = -90.f; sql_ = -90.f; sqlAplicado_ = -90.f; sqlAuto_[ativo_] = false; }
            else if (ligado) { Rv.sql = -90.f; sql_ = -90.f; sqlAplicado_ = -90.f; sqlAuto_[ativo_] = false; aviso("Squelch aberto"); }
            else if (r_->aoVivo()) { sqlMedindo_ = true; sqlAmostras_.clear(); sqlIni_ = agora(); sqlUlt_ = 0; }
        }
        if (ligado || sqlMedindo_) dl->AddRectFilled(Q(84, 197), Q(106, 200), sqlMedindo_ ? IM_COL32(0xf4, 0xf6, 0xff, 255) : IM_COL32(0xff, 0x9a, 0x30, 255), QD(1));
    }
    tx(fP(), 7.5f, 95, 220, BR, "DIM", 0.5f);
    if (tecla9("kDim", "", 77, 230, 113, 254, 0, 0, "DIM: brilho do VFD, dos medidores e da tela (100 / 75 / 50 / 30%)", 0, 0, 0) == 1) {
        dim_ = (dim_ + 1) % 4;
        const char* nm[4] = {"100%", "75%", "50%", "30%"};
        aviso(std::string("DIMMER ") + nm[dim_]);
    }
    // ---- PHONES / KEY / MIC ----
    auto jaque = [&](float cx, float cy) {
        const ImVec2 c = Q(cx, cy);
        dl->AddCircleFilled(c, QD(23), IM_COL32(0x0e, 0x0f, 0x10, 255), 48);
        anelLuz(dl, c, QD(13), QD(19), 0.f, (float)(2 * kPi), 64, [](float a, float) {
            return cinzaN(0.45f + 0.45f * sino(a, rad(-130), rad(50)) + 0.25f * sino(a, rad(50), rad(40)));
        });
        circuloSuave(dl, c, QD(19), 64, 1.f, [](float a) { return cinzaN(0.2f + 0.3f * sino(a, rad(-130), rad(50)), 220); });
        anelLuz(dl, c, QD(9.5f), QD(13), 0.f, (float)(2 * kPi), 48, [](float a, float) { return IM_COL32(0xc8, 0xb8, 0x90, (int)(120 + 120 * sino(a, rad(60), rad(60)))); });
        dl->AddCircleFilled(c, QD(9.5f), IM_COL32(0x03, 0x03, 0x03, 255), 32);
    };
    tx(fP(), 8.5f, 77, 343, BR, "PHONES", 0.5f, 0.95f);
    {   // o simbolo do fone
        const float t = std::max(1.f, QD(1.4f));
        dl->PathArcTo(Q(42, 345), QD(5.5f), (float)kPi, (float)(2 * kPi), 12);
        dl->PathStroke(BR, 0, t);
        dl->AddRectFilled(Q(35.5f, 344), Q(38.5f, 350), BR, QD(1));
        dl->AddRectFilled(Q(45.5f, 344), Q(48.5f, 350), BR, QD(1));
    }
    jaque(70, 383);
    tx(fP(), 8.5f, 77, 462, BR, "KEY", 0.5f, 0.95f);
    dl->AddLine(Q(46, 466), Q(56, 466), BR, std::max(1.f, QD(1.4f)));
    dl->AddRectFilled(Q(48, 459), Q(54, 463), BR);
    jaque(70, 500);
    ImGui::SetCursorScreenPos(Q(46, 360));
    if (ImGui::InvisibleButton("##phones9", ImVec2(QD(48), QD(48)))) { saidas_ = SomNativo::listarSaidas(); ImGui::OpenPopup("##saidaIc"); }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("PHONES: escolher a saída de som (placa / fone)");
    ImGui::SetCursorScreenPos(Q(46, 476));
    if (ImGui::InvisibleButton("##key9", ImVec2(QD(48), QD(48)))) {
        decAberta_ = true;
        if (r_->decoders().tipo() != Decoders::CW) escolherDecoder(Decoders::CW, true);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("KEY: abre o decodificador de CW (telegrafia em texto)");
    {   // MIC (conector XLR): clique grava o audio (o pontinho vermelho acende)
        const bool grav = r_->som().gravando();
        tx(fP(), 8.5f, 110, 546, BR, "MIC", 0.5f, 0.95f);
        const ImVec2 lc = Q(117, 562);
        if (grav && std::fmod(agora(), 1.0) < 0.6) dl->AddCircleFilled(lc, QD(6), IM_COL32(0xff, 0x30, 0x28, 80), 16);
        dl->AddCircleFilled(lc, QD(3.5f), grav ? IM_COL32(0xff, 0x36, 0x2c, 255) : IM_COL32(0x80, 0x18, 0x14, 255), 16);
        const ImVec2 c = Q(75, 602);
        dl->AddCircleFilled(c, QD(41), IM_COL32(0x16, 0x17, 0x18, 255), 64);
        anelLuz(dl, c, QD(30), QD(38), 0.f, (float)(2 * kPi), 96, [](float a, float) {
            return cinzaN(0.50f + 0.42f * sino(a, rad(-140), rad(55)) + 0.30f * sino(a, rad(40), rad(40)));
        });
        circuloSuave(dl, c, QD(38), 96, 1.f, [](float a) { return cinzaN(0.3f + 0.3f * sino(a, rad(-140), rad(55)), 220); });
        dl->AddCircleFilled(c, QD(30), IM_COL32(0x08, 0x08, 0x09, 255), 64);
        dl->AddRectFilled(Q(62, 566), Q(88, 578), IM_COL32(0xb8, 0xba, 0xbe, 255), QD(3));
        tx(fP(), 4.5f, 75, 572, IM_COL32(0x30, 0x30, 0x34, 255), "PUSH", 0.5f);
        auto furo = [&](float px, float py) {
            const ImVec2 p = Q(px, py);
            dl->AddCircleFilled(p, QD(5), IM_COL32(0x00, 0x00, 0x00, 255), 16);
            dl->AddCircle(p, QD(5), IM_COL32(0x50, 0x52, 0x56, 200), 16, 1.f);
        };
        furo(63, 598); furo(87, 598); furo(75, 617);
        ImGui::SetCursorScreenPos(Q(34, 561));
        if (ImGui::InvisibleButton("##mic9", ImVec2(QD(82), QD(82)))) alternarGravacao();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", grav ? "MIC: parar a gravação do áudio" : "MIC: gravar o áudio (pasta Gravacoes) - o LED vermelho acende");
    }
    // ---- IPO A / IPO B ----
    {
        auto aba = [&](float x0, float x1, const char* t) {
            dl->AddRectFilled(Q(x0, 313), Q(x1, 323), IM_COL32(0x9a, 0x9c, 0xa0, 255), QD(1.5f));
            tx(fP(), 5.5f, (x0 + x1) * 0.5f, 318, IM_COL32(0x14, 0x14, 0x16, 255), t, 0.5f);
        };
        aba(127, 151, "A"); aba(185, 209, "B");
        const char* tp = pamp_ == 0 ? "IPO" : pamp_ == 1 ? "AMP1" : "AMP2";
        if (tecla9("kIpoA", tp, 120, 330, 160, 350, 1, 0,
                   "IPO: pré-amplificador (vale para os dois receptores)\nIPO = sem pré, AMP1 = +6 dB, AMP2 = +12 dB (ganho limpo, sem saturar o dongle)\nSDRplay: +10 / +20 dB de FI\nClique troca",
                   0, pamp_ ? IM_COL32(0xc0, 0x30, 0x10, 255) : 0u, 7.f) == 1)
            pamp_ = (pamp_ + 1) % 3;
        auto& c = Config::instance();
        if (tecla9("kIpoB", "IPO", 177, 330, 217, 350, 1, 0,
                   "IPO (B): tira o risco (DC) do centro do espectro (IP+)\nVermelho = ligado", 0,
                   c.dcRemove() ? IM_COL32(0xc0, 0x30, 0x10, 255) : 0u, 7.f) == 1)
            c.set("dc_remove", !c.dcRemove());
    }
    // ---- ANTENNA SELECT ----
    {
        const ImU32 cz = IM_COL32(0xe0, 0xe2, 0xe6, 255);
        tx(fP(), 6.5f, 340, 321, cz, "ANTENNA SELECT", 0.5f, 0.95f);
        const float w = txW(fP(), 6.5f, "ANTENNA SELECT", 0.95f);
        dl->AddLine(Q(234, 321), Q(340 - w * 0.5f - 3, 321), cz, std::max(1.f, QD(1.f)));
        dl->AddLine(Q(340 + w * 0.5f + 3, 321), Q(440, 321), cz, std::max(1.f, QD(1.f)));
        const std::string tp = r_->tipo();
        const int dev = !r_->temDispositivo() ? -1 : tp == "rtltcp" ? 1 : tp == "sdrplay" ? 2 : 0;
        const float xs[5][2] = {{231, 262}, {272, 305}, {317, 350}, {362, 395}, {407, 440}};
        static const char* nm[5] = {"1", "2", "3", "4", "RX"};
        static const char* dc[5] = {"ANT 1: RTL-SDR pelo USB (LED aceso = em uso)\nClique: configuração do aparelho",
                                    "ANT 2: RTL-TCP pela rede (LED aceso = em uso)\nClique: configuração do aparelho",
                                    "ANT 3: SDRplay (LED aceso = em uso)\nClique: configuração do aparelho",
                                    "ANT 4: SATÉLITES / RADAR AÉREO\nClique: abre o radar aéreo (ADS-B, ACARS, VDL2, HFDL)",
                                    "RX: os decodificadores digitais (RTTY, PSK, FT8, CW, DMR, SSTV, satélites...)"};
        for (int i = 0; i < 5; ++i) {
            char id[12]; std::snprintf(id, sizeof id, "kAnt%d", i);
            const int led = i < 3 ? (dev == i ? 3 : 1) : i == 3 ? (radarAberto_ ? 3 : 1) : (decAberta_ ? 3 : 1);
            if (tecla9(id, nm[i], xs[i][0], 330, xs[i][1], 350, 0, led, dc[i], 0, 0, 7.f) == 1) {
                if (i < 3) abrirConfig();
                else if (i == 3) abrirRadar();
                else decAberta_ = !decAberta_;
            }
        }
        const int k = tecla9("kTune", "TUNE", 460, 330, 500, 350, 2, 0,
                             "TUNE: ajuste automático do escopo e da cachoeira (AUTO)\nSegurar: guarda a aparência atual como padrão",
                             autoWf_ ? IM_COL32(0xff, 0x8a, 0x86, 255) : IM_COL32(0xd8, 0x8c, 0x88, 255), IM_COL32(0x22, 0x10, 0x10, 255), 7.f);
        if (k == 1) autoCachoeira();
        if (k == 2) padrao();
    }
    // ---- POWER ----
    {
        tx(fP(), 8.5f, 152, 377, IM_COL32(0xff, 0x4a, 0x3c, 255), "POWER", 0.5f, 0.95f);
        // o suporte cromado em volta
        retGrad(dl, Q(118, 392), Q(184, 434), QD(16), IM_COL32(0x9a, 0x9c, 0xa0, 255), IM_COL32(0x3a, 0x3c, 0x40, 255));
        dl->AddRectFilled(Q(124, 396), Q(178, 430), IM_COL32(0x1c, 0x1d, 0x1f, 255), QD(14));
        const ImVec2 c = Q(151, 413);
        ImGui::SetCursorScreenPos(Q(126, 388));
        const bool cl = ImGui::InvisibleButton("##pwr9", ImVec2(QD(50), QD(50)));
        const bool sob = ImGui::IsItemHovered(), at = ImGui::IsItemActive();
        dl->AddCircleFilled(ImVec2(c.x, c.y + QD(1.5f)), QD(22), IM_COL32(0, 0, 0, 180), 48);
        anelLuz(dl, c, 0.f, QD(20), 0.f, (float)(2 * kPi), 64, [&](float a, float rr) {
            const float l = (0.05f + 0.20f * sino(a, rad(-120), rad(50))) * (0.5f + 0.5f * rr / QD(20));
            return cinzaN(at ? l * 0.5f : sob ? l * 1.5f : l);
        });
        circuloSuave(dl, c, QD(20), 64, 1.f, [](float a) { return cinzaN(0.15f + 0.35f * sino(a, rad(-120), rad(50)), 230); });
        const ImU32 cp = lig ? IM_COL32(0xff, 0x3c, 0x30, 255) : IM_COL32(0x70, 0x1c, 0x18, 255);
        if (lig) dl->AddCircleFilled(c, QD(11), IM_COL32(0xff, 0x30, 0x20, 40), 32);
        dl->PathArcTo(c, QD(7), rad(-50), rad(230), 24);
        dl->PathStroke(cp, 0, std::max(1.2f, QD(2.2f)));
        dl->AddLine(Q(151, 403), Q(151, 412), cp, std::max(1.2f, QD(2.2f)));
        if (sob && !at) ImGui::SetTooltip("%s", lig ? "POWER: desligar o rádio (para o dongle)" : "POWER: ligar o rádio (abre o dongle)");
        if (cl) ligarDesligar();
    }
    // ---- METER (escolhe a leitura do S-meter) ----
    {
        tx(fP(), 7.5f, 237, 362, BR, "METER", 0.5f, 0.95f);
        tx(fP(), 6.f, 207, 376, BR, "COMP", 0.5f); tx(fP(), 6.f, 235, 371, BR, "PO", 0.5f); tx(fP(), 6.f, 265, 376, BR, "SWR", 0.5f);
        tx(fP(), 6.f, 279, 391, BR, "IDD", 0.f); tx(fP(), 6.f, 279, 405, BR, "MIC", 0.f); tx(fP(), 6.f, 279, 414, BR, "LVL", 0.f);
        static const float angM[3] = {-55.f, -10.f, 30.f};
        float di, dO;
        static float acc = 0;
        const char* nm[3] = {"S (unidades S)", "dB (nível do sinal)", "PEAK (pico, o do squelch)"};
        char d[200];
        std::snprintf(d, sizeof d, "METER: o que a tela mostra do S-meter - agora %s\nGire: S / dB / PEAK (clicar no S-meter também troca)", nm[std::clamp(medidorModo_, 0, 2)]);
        if (giro9("gMeter", 237, 412, 30, 26, tAng(angM[std::clamp(medidorModo_, 0, 2)]), 0.f, di, dO, d, false, 0, 0)) {
            acc += di + dO;
            if (acc > 0.07f) { medidorModo_ = std::min(2, medidorModo_ + 1); acc = 0; }
            if (acc < -0.07f) { medidorModo_ = std::max(0, medidorModo_ - 1); acc = 0; }
        }
    }
    // ---- MONI (miolo: tom do audio) - PITCH (saia: CW / NOTCH / CONT) ----
    {
        Receptor& A = rx_[ativo_];
        rotDuplo9(357, 365, 7.2f, BR, "MONI", "PITCH");
        const bool cw = grupoModo(r_->modo()) == 1;
        float tOut = cw ? (cwPitch_ - 300.f) / 700.f : A.nch ? A.notch : A.cont;
        char d[300];
        const double nh = 150.0 * std::pow(3500.0 / 150.0, std::clamp(A.notch, 0.f, 1.f));
        const double ch = 150.0 * std::pow(3500.0 / 150.0, std::clamp(A.cont, 0.f, 1.f));
        std::snprintf(d, sizeof d, "MONI (miolo): tonalidade do áudio (%s %d%%)\nPITCH (saia): %s",
                      tom_ < 45 ? "grave" : tom_ > 55 ? "agudo" : "plano", (int)std::lround(std::fabs(tom_ - 50) * 2),
                      cw ? (std::string("tom do CW e centro do APF (") + std::to_string((int)cwPitch_) + " Hz)").c_str()
                         : A.nch ? (std::string("posição do NOTCH (") + std::to_string((int)std::lround(nh)) + " Hz)").c_str()
                                 : (std::string("posição do CONT (") + std::to_string((int)std::lround(ch)) + " Hz) - girar liga o CONT").c_str());
        float di, dO;
        if (giro9("gMoni", 357, 411, 29, 20, tom_ / 100.f, std::clamp(tOut, 0.f, 1.f), di, dO, d, true, 9, 33)) {
            tom_ = std::clamp(tom_ + di * 100.f, 0.f, 100.f);
            if (di != 0) rxeq_ = 0;
            if (dO != 0) {
                if (cw) cwPitch_ = std::clamp(cwPitch_ + dO * 700.f, 300.f, 1000.f);
                else if (A.nch) A.notch = std::clamp(A.notch + dO, 0.f, 1.f);
                else { A.cont = std::clamp(A.cont + dO, 0.f, 1.f); if (!A.bef) { A.bef = true; aviso("CONT ligado"); } }
            }
        }
        tx(fP(), 6.f, 274, 447, BR, "MONI", 0.5f);
        if (tecla9("kMoni", "", 296, 422, 312, 446, 3, 1, "MONI: fala a frequência do receptor ouvido (voz do Windows)", 0, 0, 0) == 1) falarFrequencia(ativo_);
    }
    // ---- AGC (miolo do MAIN) - ATT (saia) ----
    auto agcAtt = [&](int rx, const char* id, float cx, float cy, float rO, float rI, int marcas, float rM) {
        Receptor& R = rx_[rx];
        // posicoes: AGC SLOW -90 / MID -65 / FAST -38 / OFF +42; ATT 0 +5 / 6 +68 / 12 +92 / 18 +122
        static const float aAgc[4] = {-90.f, -65.f, -38.f, 42.f};
        static const float aAtt[4] = {5.f, 68.f, 92.f, 122.f};
        const int pa = R.agcOff ? 3 : std::clamp(R.agc, 1, 3) - 1;
        static float accA[2] = {0, 0}, accT = 0;
        char d[260];
        const char* nm[4] = {"SLOW", "MID", "FAST", "OFF"};
        std::snprintf(d, sizeof d, "AGC (miolo) do %s: SLOW / MID / FAST / OFF - agora %s\nATT (saia, vale para os dois): 0 / 6 / 12 / 18 dB - agora %d dB",
                      rx == 0 ? "MAIN" : "SUB", nm[pa], att_ * 6);
        float di, dO;
        if (giro9(id, cx, cy, rO, rI, tAng(aAgc[pa]), tAng(aAtt[std::clamp(att_, 0, 3)]), di, dO, d, true, marcas, rM)) {
            accA[rx] += di; accT += dO;
            int np = pa;
            if (accA[rx] > 0.07f) { np = std::min(3, pa + 1); accA[rx] = 0; }
            if (accA[rx] < -0.07f) { np = std::max(0, pa - 1); accA[rx] = 0; }
            if (np != pa) { if (np == 3) R.agcOff = true; else { R.agcOff = false; R.agc = np + 1; } aviso(std::string("AGC ") + nm[np], 1.0); }
            if (accT > 0.07f) { att_ = std::min(3, att_ + 1); accT = 0; aviso(att_ ? "ATT " + std::to_string(att_ * 6) + " dB" : std::string("ATT OFF"), 1.0); }
            if (accT < -0.07f) { att_ = std::max(0, att_ - 1); accT = 0; aviso(att_ ? "ATT " + std::to_string(att_ * 6) + " dB" : std::string("ATT OFF"), 1.0); }
        }
    };
    {
        rotDuplo9(465, 365, 7.2f, BR, "AGC", "ATT");
        tx(fP(), 6.f, 434, 376, BR, "FAST", 0.5f); tx(fP(), 6.f, 477, 372, BR, "AUTO/0", 0.5f); tx(fP(), 6.f, 515, 376, BR, "OFF/3", 0.5f);
        tx(fP(), 6.f, 432, 392, BR, "MID", 0.5f); tx(fP(), 6.f, 518, 392, BR, "6", 0.5f);
        tx(fP(), 6.f, 425, 412, BR, "SLOW", 0.5f); tx(fP(), 6.f, 523, 412, BR, "-12", 0.5f);
        tx(fP(), 6.f, 518, 431, BR, "18", 0.5f); tx(fP(), 5.5f, 518, 441, BR, "(dB)", 0.5f);
        agcAtt(0, "gAgc0", 470, 412, 29, 20, 0, 0);
    }
    // ---- MIC (miolo: forca do compressor) - PROC (saia: nivel do NB2) ----
    {
        rotDuplo9(165, 462, 7.2f, BR, "MIC", "PROC");
        char d[240];
        std::snprintf(d, sizeof d, "MIC (miolo): força do compressor de áudio (PROC) - %.0f%%\nPROC (saia): nível do NB2 - %.0f%%\nA tecla PROC liga o compressor",
                      procNivel_ * 100.f, nb2Nivel_ * 100.f);
        float di, dO;
        if (giro9("gMic", 160, 510, 29, 20, procNivel_, nb2Nivel_, di, dO, d, true, 9, 33)) {
            procNivel_ = std::clamp(procNivel_ + di, 0.f, 1.f);
            nb2Nivel_ = std::clamp(nb2Nivel_ + dO, 0.f, 1.f);
        }
        tx(fP(), 7.f, 182, 562, BR, "PROC", 0.5f);
        if (tecla9("kProc", "", 202, 534, 218, 557, 3, proc_ ? 4 : 1, "PROC: compressor de áudio (a força fica no MIC)", 0, 0, 0) == 1) proc_ = !proc_;
    }
    // ---- VOX (miolo: margem do squelch automatico) - DELAY (saia: demora do squelch) ----
    {
        rotDuplo9(267, 462, 7.2f, BR, "VOX", "DELAY");
        char d[240];
        std::snprintf(d, sizeof d, "VOX (miolo): margem do squelch automático (%.0f dB acima do ruído)\nDELAY (saia): demora para o squelch fechar (%.1f s)",
                      sqlMargem_, sqlHang_);
        float di, dO;
        if (giro9("gVox", 260, 510, 29, 20, sqlMargem_ / 12.f, sqlHang_ / 2.f, di, dO, d, true, 9, 33)) {
            sqlMargem_ = std::clamp(sqlMargem_ + di * 12.f, 0.f, 12.f);
            sqlHang_ = std::clamp(sqlHang_ + dO * 2.f, 0.f, 2.f);
        }
        tx(fP(), 7.f, 282, 563, BR, "KEYER", 0.5f);
        Receptor& A = rx_[ativo_];
        if (tecla9("kKeyer", "", 304, 534, 320, 557, 3, A.apf ? 3 : 1, "KEYER: APF - filtro de pico de áudio para CW (centro no PITCH)", 0, 0, 0) == 1) A.apf = !A.apf;
    }
    // ---- SPEED (miolo: velocidade da cachoeira) - CW DELAY (saia: volta do ponteiro) ----
    {
        rotDuplo9(372, 462, 7.2f, BR, "SPEED", "CW DELAY");
        char d[240];
        std::snprintf(d, sizeof d, "SPEED (miolo): velocidade da cachoeira (%.0f)\nCW DELAY (saia): tempo de volta do ponteiro do S-meter (%.1f s)",
                      wfSpeedEsc_, smRetorno_);
        float di, dO;
        if (giro9("gSpeed", 357, 510, 29, 20, (wfSpeedEsc_ - 1.f) / 9.f, (smRetorno_ - 0.1f) / 2.9f, di, dO, d, true, 9, 33)) {
            static float accS = 0;
            accS += di * 9.f;
            if (std::fabs(accS) >= 1.f) { wfSpeedEsc_ = std::clamp(wfSpeedEsc_ + (accS > 0 ? 1.f : -1.f), 1.f, 10.f); accS = 0; }
            smRetorno_ = std::clamp(smRetorno_ + dO * 2.9f, 0.1f, 3.f);
        }
        tx(fP(), 6.5f, 392, 557, BR, "BK-IN", 0.5f); tx(fP(), 6.5f, 392, 566, BR, "SPOT", 0.5f);
        if (tecla9("kSpot", "", 402, 534, 418, 557, 3, 1, "BK-IN / SPOT: sintonia automática no pico mais próximo (CW / AM / FM)", 0, 0, 0) == 1) cwTune();
    }
    // ---- NB (miolo: nivel do NB) - SQL (saia: squelch do MAIN) ----
    {
        rotDuplo9(467, 462, 7.2f, BR, "NB", "SQL");
        char d[240], sq[24];
        if (M0.sql <= -90) std::snprintf(sq, sizeof sq, "aberto"); else std::snprintf(sq, sizeof sq, "%+d dB", (int)std::lround(M0.sql));
        std::snprintf(d, sizeof d, "NB (miolo): nível do NB (%.0f)\nSQL (saia): squelch do MAIN (%s)", nbNivel_, sq);
        float di, dO;
        if (giro9("gNbSql0", 460, 510, 29, 20, nbNivel_ / 100.f, (M0.sql + 90.f) / 120.f, di, dO, d, true, 9, 33)) {
            nbNivel_ = std::clamp(nbNivel_ + di * 100.f, 0.f, 100.f);
            if (dO != 0) {
                M0.sql = std::clamp(M0.sql + dO * 120.f, -90.f, 30.f);
                if (ativo_ == 0) { sql_ = M0.sql; sqlAplicado_ = M0.sql; }
                if (M0.sql <= -90.f) sqlAuto_[0] = false;
            }
        }
        tx(fP(), 7.f, 492, 563, BR, "NB", 0.5f);
        const int k = tecla9("kNb0", "", 502, 534, 518, 557, 3, M0.nb1 ? 2 : M0.nb2 ? 3 : 1, "NB do MAIN: apagador de pulsos\nSegurar: NB2 (no IQ, antes do demodulador)", 0, 0, 0);
        if (k == 1) M0.nb1 = !M0.nb1;
        if (k == 2) { M0.nb2 = !M0.nb2; aviso(M0.nb2 ? "NB2 ligado" : "NB2 desligado"); }
    }
}

// ===========================================================================
//  MB1SDR: a coluna do meio (CONT-DNR, filtros, VRF-NTCH), as teclas de
//  MODE / QMB em volta do dial, o dial e as teclas A>B ... A>M
// ===========================================================================
void Ui::ft9Centro()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 BR = IM_COL32(0xf4, 0xf4, 0xf4, 255);
    Receptor& M0 = rx_[0];
    Receptor& A = rx_[ativo_];
    // ---- CONT (miolo) - DNR (saia) do MAIN ----
    {
        rotDuplo9(600, 322, 7.2f, BR, "CONT", "DNR");
        const double ch = 150.0 * std::pow(3500.0 / 150.0, std::clamp(M0.cont, 0.f, 1.f));
        char d[240];
        std::snprintf(d, sizeof d, "CONT (miolo): posição do contorno do MAIN (%d Hz) - girar liga o CONT\nDNR (saia): força do redutor de ruído (%.0f)",
                      (int)std::lround(ch), nrEspForca_);
        float di, dO;
        if (giro9("gCont0", 602, 372, 29, 20, M0.cont, nrEspForca_ / 100.f, di, dO, d, true, 0, 0)) {
            if (di != 0) { M0.cont = std::clamp(M0.cont + di, 0.f, 1.f); if (!M0.bef) { M0.bef = true; aviso("CONT do MAIN ligado"); } }
            nrEspForca_ = std::clamp(nrEspForca_ + dO * 100.f, 0.f, 100.f);
        }
        tx(fP(), 6.5f, 562, 402, BR, "CONT", 0.5f);
        if (tecla9("kCont0", "", 558, 414, 577, 424, 3, M0.bef ? 4 : 1, "CONT do MAIN: contorno - abaixa uma faixa do áudio (posição no CONT)", 0, 0, 0) == 1) M0.bef = !M0.bef;
        tx(fP(), 6.5f, 642, 402, BR, "DNR", 0.5f);
        const int k = tecla9("kDnr0", "", 628, 414, 647, 424, 3, M0.nr1 ? 3 : M0.nr2 ? 2 : 1,
                             "DNR do MAIN: redutor de ruído espectral\nSegurar: NR1 (redutor simples)\nForça: saia do CONT - DNR", 0, 0, 0);
        if (k == 1) M0.nr2 = !M0.nr2;
        if (k == 2) { M0.nr1 = !M0.nr1; aviso(M0.nr1 ? "NR1 do MAIN ligado" : "NR1 do MAIN desligado"); }
    }
    // ---- a coluna dos filtros: 15k / 6k / 3k / AUTO, R.FLT, DNF ----
    {
        static const char* nm[3] = {"15k", "6k", "3k"};
        const float ys[3] = {335, 362, 391};
        const float yl[3] = {322, 350, 377};
        for (int i = 0; i < 3; ++i) {
            tx(fP(), 6.5f, 676, yl[i], BR, nm[i], 0.5f);
            char id[12], d[160];
            std::snprintf(id, sizeof id, "kRf%d", i);
            std::snprintf(d, sizeof d, "Filtro FIL%d do receptor ouvido (LED aceso = em uso)\nA largura de cada um: SHIFT - WIDTH ou o BW da tela", i + 1);
            if (tecla9(id, "", 668, ys[i] - 5, 685, ys[i] + 5, 5, A.fil == i ? 4 : 1, d, 0, 0, 0) == 1) { A.fil = i; aplicarFiltro(); }
        }
        tx(fP(), 6.f, 676, 405, BR, "AUTO", 0.5f);
        if (tecla9("kRfAuto", "", 668, 414, 685, 424, 5, 1, "AUTO: o filtro do receptor ouvido volta à largura padrão", 0, 0, 0) == 1) {
            static const float hi0[5][3] = {{2700, 2400, 3600}, {500, 250, 1200}, {4000, 3000, 5000}, {12500, 10000, 25000}, {200000, 150000, 250000}};
            static const float lo0[5][3] = {{200, 400, 100}, {0, 0, 0}, {200, 250, 100}, {0, 0, 0}, {0, 0, 0}};
            const int g = grupoModo(r_->modo());
            filHi_[g][A.fil] = hi0[g][A.fil]; filLo_[g][A.fil] = lo0[g][A.fil];
            aplicarFiltro();
            aviso("Filtro de volta ao padrão");
        }
        tx(fP(), 7.f, 675, 435, BR, "R.FLT", 0.5f);
        const int k = tecla9("kRflt", "", 666, 444, 684, 474, 3, 0, "R.FLT: troca o filtro FIL1 / FIL2 / FIL3\nSegurar: escolher a largura (BW)", 0, 0, 0);
        if (k == 1) { A.fil = (A.fil + 1) % 3; aplicarFiltro(); char a[24]; std::snprintf(a, sizeof a, "FIL%d", A.fil + 1); aviso(a, 1.0); }
        if (k == 2) ImGui::OpenPopup("##bwIc");
        if (tecla9("kDnfL", "", 668, 500, 685, 510, 5, M0.anch ? 2 : 1, "DNF do MAIN: notch automático (aceso = ligado)", 0, 0, 0) == 1) M0.anch = !M0.anch;
        tx(fP(), 7.f, 675, 520, BR, "DNF", 0.5f);
        if (tecla9("kDnf", "", 666, 532, 684, 562, 3, 0, "DNF do MAIN: notch automático (tira a portadora / o apito sozinho)", 0, 0, 0) == 1) M0.anch = !M0.anch;
    }
    // ---- VRF/u-T (miolo: ganho de RF) - NTCH (saia: posicao do NOTCH) do MAIN ----
    {
        rotDuplo9(600, 454, 7.f, BR, "VRF/\xc2\xb5-T", "NTCH");
        tx(fP(), 6.f, 557, 466, BR, "DN", 0.5f); tx(fP(), 6.f, 647, 466, BR, "UP", 0.5f);
        const double nh = 150.0 * std::pow(3500.0 / 150.0, std::clamp(M0.notch, 0.f, 1.f));
        char d[240];
        std::snprintf(d, sizeof d, "VRF (miolo): ganho de RF do MAIN (%d dB)\nNTCH (saia): posição do NOTCH do MAIN (%d Hz)",
                      (int)std::lround(49.6f * M0.rf / 100.f), (int)std::lround(nh));
        float di, dO;
        if (giro9("gVrf0", 602, 507, 29, 20, M0.rf / 100.f, M0.notch, di, dO, d, true, 0, 0)) {
            M0.rf = std::clamp(M0.rf + di * 100.f, 0.f, 100.f);
            M0.notch = std::clamp(M0.notch + dO, 0.f, 1.f);
        }
        tx(fP(), 6.f, 557, 528, BR, "VRF", 0.5f); tx(fP(), 6.f, 557, 536, BR, "\xc2\xb5-T", 0.5f);
        if (tecla9("kVrf0", "", 558, 545, 577, 556, 3, M0.bc ? 4 : 1, "VRF / \xc2\xb5-T do MAIN: cancelador de vários apitos (BC)", 0, 0, 0) == 1) M0.bc = !M0.bc;
        tx(fP(), 6.5f, 642, 536, BR, "NTCH", 0.5f);
        const int k = tecla9("kNtch0", "", 628, 545, 647, 556, 3, M0.nch ? 3 : 1, "NTCH do MAIN: notch manual (posição na saia do VRF - NTCH)\nSegurar: estreito / largo", 0, 0, 0);
        if (k == 1) M0.nch = !M0.nch;
        if (k == 2) { M0.nch = true; M0.nchLargo = !M0.nchLargo; aviso(M0.nchLargo ? "NOTCH largo" : "NOTCH estreito"); }
    }
    // ---- RX / TX (as barrinhas em cima do dial) ----
    {
        const bool busyA = r_->ligado() && r_->aoVivo() && sqlAberto_;
        dl->AddRectFilled(Q(815, 314), Q(867, 329), IM_COL32(0x0c, 0x0c, 0x0d, 255), QD(3));
        dl->AddRectFilled(Q(817, 316), Q(865, 327), busyA ? IM_COL32(0x5c, 0xe0, 0x6c, 255) : IM_COL32(0x2a, 0x40, 0x2e, 255), QD(2));
        tx(fP(), 8.f, 885, 321, BR, "RX", 0.5f);
        dl->AddRectFilled(Q(1128, 314), Q(1179, 329), IM_COL32(0x0c, 0x0c, 0x0d, 255), QD(3));
        dl->AddRectFilled(Q(1130, 316), Q(1177, 327), sendMudo_ ? IM_COL32(0xff, 0x40, 0x48, 255) : IM_COL32(0xd0, 0xd8, 0xe8, 255), QD(2));
        tx(fP(), 8.f, 1100, 321, BR, "TX", 0.5f);
        if (ImGui::IsMouseHoveringRect(Q(815, 312), Q(905, 330)) && ImGui::IsWindowHovered()) ImGui::SetTooltip("RX: verde = squelch aberto no receptor ouvido");
        if (ImGui::IsMouseHoveringRect(Q(1090, 312), Q(1180, 330)) && ImGui::IsWindowHovered()) ImGui::SetTooltip("TX: vermelho = áudio em MUTE (MOX)");
    }
    // ---- MODE: A / B e os modos ----
    {
        tx(fP(), 8.5f, 767, 330, BR, "MODE", 0.5f);
        if (tecla9("kVa", "A", 705, 340, 752, 362, 2, 0, "A: ouvir o VFO-A (MAIN) - o dial e os botões passam para ele",
                   ativo_ == 0 ? IM_COL32(0xff, 0x90, 0x88, 255) : IM_COL32(0x9a, 0x70, 0x6c, 255), IM_COL32(0x30, 0x10, 0x10, 255), 9.f) == 1) ativar(0);
        if (tecla9("kVb", "B", 770, 340, 820, 362, 1, 0, "B: ouvir o VFO-B (SUB) - o dial e os botões passam para ele",
                   0, ativo_ == 1 ? IM_COL32(0xc0, 0x20, 0x10, 255) : 0u, 9.f) == 1) ativar(1);
        const std::string m = r_->modo();
        struct K { const char* t; float x0, y0, x1, y1; };
        const K ks[7] = {{"LSB", 725, 371, 757, 389}, {"USB", 723, 401, 755, 419}, {"CW", 721, 431, 753, 449}, {"AM", 723, 461, 755, 479},
                         {"FM", 726, 491, 758, 509}, {"RTTY", 734, 520, 770, 538}, {"PKT", 746, 549, 782, 567}};
        const bool rtty = decAberta_ && r_->decoders().tipo() == Decoders::RTTY;
        const bool pkt = decAberta_ && r_->decoders().tipo() != Decoders::RTTY;
        const bool on[7] = {m == "LSB", m == "USB", m == "CW", m == "AM", m == "NFM" || m == "WFM", rtty, pkt};
        const char* dc[7] = {"LSB", "USB", "CW: telegrafia (largura no SHIFT - WIDTH, tom no PITCH)", "AM",
                             "FM: FM estreito\nSegurar: FM largo (radiodifusão, WFM)", "RTTY: o decodificador de RTTY", "PKT: os decodificadores digitais (DATA)"};
        for (int i = 0; i < 7; ++i) {
            const ImVec2 lc = Q(ks[i].x0 - 7.5f, (ks[i].y0 + ks[i].y1) * 0.5f);
            if (on[i]) dl->AddCircleFilled(lc, QD(4.5f), IM_COL32(0xff, 0x40, 0x30, 70), 12);
            dl->AddCircleFilled(lc, QD(2.6f), on[i] ? IM_COL32(0xff, 0x40, 0x30, 255) : IM_COL32(0x40, 0x14, 0x12, 255), 12);
            char id[12]; std::snprintf(id, sizeof id, "kMd%d", i);
            const int k = tecla9(id, ks[i].t, ks[i].x0, ks[i].y0, ks[i].x1, ks[i].y1, 0, 0, dc[i], 0, 0, 7.5f);
            if (!k) continue;
            switch (i) {
            case 0: mudarModoPainel("LSB"); break;
            case 1: mudarModoPainel("USB"); break;
            case 2: mudarModoPainel("CW"); break;
            case 3: mudarModoPainel("AM"); break;
            case 4: mudarModoPainel(k == 2 ? "WFM" : "NFM"); break;
            case 5: escolherDecoder(Decoders::RTTY, true); decAberta_ = true; break;
            default: decAberta_ = !decAberta_; break;
            }
        }
    }
    // ---- QMB (STO / RCL), NAR, SPLIT, TXW, C.S ----
    {
        tx(fP(), 8.5f, 835, 363, BR, "QMB", 0.5f);
        char d[200];
        std::snprintf(d, sizeof d, "STO (QMB): grava a frequência atual nas memórias rápidas (%d de 10)", (int)qm_.size());
        if (tecla9("kSto", "STO", 790, 376, 837, 399, 2, 0, d, IM_COL32(0x9c, 0xb4, 0xf0, 255), IM_COL32(0xf4, 0xf8, 0xff, 255), 8.f) == 1) {
            qm_.insert(qm_.begin(), Rapida{r_->vfo(), r_->modo()});
            if (qm_.size() > 10) qm_.resize(10);
            qmIdx_ = -1;
            aviso("Gravado no QMB");
        }
        if (tecla9("kRcl", "RCL", 785, 407, 830, 430, 2, 0, "RCL (QMB): chama as memórias rápidas, uma por vez", IM_COL32(0x9c, 0xb4, 0xf0, 255), IM_COL32(0xf4, 0xf8, 0xff, 255), 8.f) == 1) {
            if (qm_.empty()) aviso("QMB vazio (STO grava)");
            else {
                qmIdx_ = (qmIdx_ + 1) % (int)qm_.size();
                rx_[ativo_].memModo = false;
                mudarModoPainel(qm_[qmIdx_].modo);
                sintonizar(qm_[qmIdx_].f, false);
                char a[48]; std::snprintf(a, sizeof a, "QMB %d", qmIdx_ + 1); aviso(a);
            }
        }
        if (tecla9("kNar", "NAR", 780, 445, 830, 465, 0, 0, "NAR: filtro estreito (FIL2) / volta ao FIL1", 0, A.fil == 1 ? IM_COL32(0xff, 0xb0, 0x40, 255) : 0u, 8.f) == 1) {
            A.fil = A.fil == 1 ? 0 : 1; aplicarFiltro();
        }
        const bool split = txVfo_ != ativo_;
        const int ks = tecla9("kSplit", "SPLIT", 790, 477, 840, 499, 0, 0,
                              "SPLIT: escuta num receptor e marca o outro como \"TX\"\nSegurar: QUICK SPLIT (o outro receptor vai 5 kHz acima)", 0,
                              split ? IM_COL32(0xff, 0xb0, 0x40, 255) : 0u, 8.f);
        if (ks == 1) txVfo_ = split ? ativo_ : 1 - ativo_;
        if (ks == 2) { sincronizarAtivo(); rx_[1 - ativo_].f = r_->vfo() + 5000; rx_[1 - ativo_].modo = r_->modo(); txVfo_ = 1 - ativo_; aviso("QUICK SPLIT +5 kHz"); }
        bool seg = false;
        tecla9("kTxw", "TXW", 800, 510, 850, 532, 0, 0, "TXW: enquanto segura, ouve o receptor marcado como \"TX\" (o outro)", 0, 0, 8.f, &seg);
        if (seg && xfcVolta_ < 0) { xfcVolta_ = ativo_; ativar(1 - ativo_); }
        if (!seg && xfcVolta_ >= 0) { ativar(xfcVolta_); xfcVolta_ = -1; }
        if (tecla9("kCs", "C.S", 830, 552, 875, 566, 0, 0, "C.S: menu rápido (memórias, passo, escopo, GEN, decodificadores)", 0, 0, 6.5f) == 1) ImGui::OpenPopup("##quickIc");
    }
    // ---- FAST / LOCK ----
    if (tecla9("kFast", "FAST", 1140, 340, 1175, 353, 0, 0, "FAST: o dial anda 10x mais rápido", 0, fast_ ? IM_COL32(0xff, 0xb0, 0x40, 255) : 0u, 6.f) == 1) fast_ = !fast_;
    if (tecla9("kLock", "LOCK", 1188, 340, 1222, 353, 0, 0, A.lock ? "LOCK: destrava o dial" : "LOCK: trava o dial", 0, A.lock ? IM_COL32(0xff, 0xb0, 0x40, 255) : 0u, 6.f) == 1) {
        A.lock = !A.lock; aviso(A.lock ? "LOCK: dial travado" : "LOCK: dial livre");
    }
    // ---- A>B, B>A, A<>B, V/M, M>A, A>M ----
    {
        auto copia = [&](int de, int para) {
            sincronizarAtivo();
            rx_[para].f = rx_[de].f; rx_[para].modo = rx_[de].modo; rx_[para].fil = rx_[de].fil;
            if (para == ativo_) { r_->setModo(rx_[para].modo); sintonizar(rx_[para].f, false); aplicarFiltro(); }
        };
        struct K { float x0, y0, x1, y1; };
        const K k6[6] = {{1170, 377, 1210, 397}, {1170, 410, 1210, 430}, {1170, 444, 1210, 464}, {1169, 477, 1209, 497}, {1160, 510, 1200, 530}, {1155, 542, 1195, 562}};
        const char* dc[6] = {"A>B: copia o VFO-A (MAIN) no VFO-B (SUB)", "B>A: copia o VFO-B (SUB) no VFO-A (MAIN)",
                             "A<>B: troca as frequências do VFO-A e do VFO-B (frequência, modo e filtro)",
                             "V/M: o dial passa os canais de memória (MEM) / volta ao VFO",
                             "M>A: a memória mais perto vai para o VFO (sai do modo memória)", "A>M: grava a frequência atual numa memória (abre a lista)"};
        for (int i = 0; i < 6; ++i) {
            char id[12]; std::snprintf(id, sizeof id, "kAb%d", i);
            const int k = tecla9(id, "", k6[i].x0, k6[i].y0, k6[i].x1, k6[i].y1, 0, 0, dc[i], 0, 0, 0);
            const float cx = (k6[i].x0 + k6[i].x1) * 0.5f, cy = (k6[i].y0 + k6[i].y1) * 0.5f;
            const bool vm = i == 3 && A.memModo;
            switch (i) {
            case 0: txSeta(cx, cy, 7.5f, BR, "A", "B", 0); break;
            case 1: txSeta(cx, cy, 7.5f, BR, "B", "A", 0); break;
            case 2: txSeta(cx, cy, 7.5f, BR, "A", "B", 1); break;
            case 3: tx(fP(), 7.5f, cx, cy, vm ? IM_COL32(0xff, 0xb0, 0x40, 255) : BR, "V/M", 0.5f, 0.95f); break;
            case 4: txSeta(cx, cy, 7.5f, BR, "M", "A", 0); break;
            default: txSeta(cx, cy, 7.5f, BR, "A", "M", 0); break;
            }
            if (k != 1) continue;
            switch (i) {
            case 0: copia(0, 1); aviso("A > B"); break;
            case 1: copia(1, 0); aviso("B > A"); break;
            case 2: trocarMainSub(); aviso("A <> B"); break;
            case 3:
                if (mem_.empty()) aviso("Nenhuma memória gravada (A>M grava)");
                else {
                    A.memModo = !A.memModo;
                    if (A.memModo) {
                        int mais = 0; long long melhor = LLONG_MAX;
                        for (int j = 0; j < (int)mem_.size(); ++j) {
                            const long long dd = std::llabs((long long)mem_[j].f - (long long)r_->vfo());
                            if (dd < melhor) { melhor = dd; mais = j; }
                        }
                        A.memCanal = mais; sintonizarMemoria((size_t)mais);
                    }
                }
                break;
            case 4: A.memModo = false; aviso("M > A: VFO"); break;
            default:
                memAberta_ = true;
                std::snprintf(memNome_, sizeof memNome_, "%s %s", fmtFreq(r_->vfo()).c_str(), r_->modo().c_str());
                break;
            }
        }
    }
    // ---- o dial ----
    {
        const int p = dial9("dial9", 1000, 460, 110, angDialA_, A.lock,
                            "Dial: sintoniza o receptor ouvido (arraste girando ou use a roda do mouse)\nGirar rápido acelera. Passo no menu C.S / MNU (STEP)");
        if (p) sintonizarRx(ativo_, p);
    }
}

// ===========================================================================
//  MB1SDR: embaixo a direita - AF GAIN-RF GAIN e SHIFT-WIDTH do MAIN,
//  CLAR / VFO-B (o botao do SUB) e as teclas em volta
// ===========================================================================
void Ui::ft9Direita()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 BR = IM_COL32(0xf4, 0xf4, 0xf4, 255);
    Receptor& M0 = rx_[0];
    Receptor& B1 = rx_[1];
    Receptor& A = rx_[ativo_];
    // ---- AF GAIN (miolo) - RF GAIN (saia) do MAIN ----
    {
        rotDuplo9(1322, 481, 8.5f, BR, "AF GAIN", "RF GAIN");
        char d[240];
        std::snprintf(d, sizeof d, "AF GAIN (miolo): volume do MAIN (VFO-A) - %d%%\nRF GAIN (saia): ganho de RF do MAIN - %d dB",
                      (int)std::lround(M0.af), (int)std::lround(49.6f * M0.rf / 100.f));
        float di, dO;
        if (giro9("gAf0", 1330, 577, 66, 50, M0.af / 100.f, M0.rf / 100.f, di, dO, d, true, 11, 74)) {
            M0.af = std::clamp(M0.af + di * 100.f, 0.f, 100.f);
            M0.rf = std::clamp(M0.rf + dO * 100.f, 0.f, 100.f);
            if (ativo_ != 0 && di != 0) aviso("Este é o volume do MAIN. Ouvindo o SUB: use o AF GAIN do SUB (embaixo, B)", 2.5);
        }
        tx(fP(), 7.f, 1397, 619, BR, "P.BACK", 0.5f);
        const int k = tecla9("kPback", "", 1377, 630, 1408, 644, 3, 1, "P.BACK: toca a última gravação\nSegurar: para de tocar", 0, 0, 0);
        if (k == 1) tocarUltimaGravacao();
        if (k == 2) { PlaySoundA(nullptr, nullptr, 0); aviso("P.BACK parado"); }
    }
    // ---- SHIFT (miolo) - WIDTH (saia) do MAIN ----
    auto shiftWidth = [&](int rx, const char* id, float cx, float cy, float rO, float rI, int marcas, float rM) {
        Receptor& R = rx_[rx];
        const std::string md = rx == ativo_ ? r_->modo() : R.modo;
        const int g = grupoModo(md);
        float& hi = filHi_[g][R.fil];
        float& lo = filLo_[g][R.fil];
        char d[260];
        float tIn = 0.5f, tOut = 0.5f;
        const char* qual = rx == 0 ? "MAIN" : "SUB";
        switch (g) {
        case 0: std::snprintf(d, sizeof d, "SHIFT (miolo) do %s: desloca o filtro (%.2f a %.2f kHz)\nWIDTH (saia): largura %.2f kHz  - FIL%d", qual, lo / 1000.f, hi / 1000.f, (hi - lo) / 1000.f, R.fil + 1);
                tIn = (lo + hi) * 0.5f / 3000.f; tOut = (hi - lo - 400.f) / 4600.f; break;
        case 1: std::snprintf(d, sizeof d, "SHIFT (miolo) do %s: %+.0f Hz\nWIDTH (saia): largura do CW %.0f Hz  - FIL%d", qual, lo, hi, R.fil + 1);
                tIn = (lo + 800) / 1600.f; tOut = (hi - 50) / 2450.f; break;
        case 2: std::snprintf(d, sizeof d, "SHIFT (miolo) do %s: corte baixo do AM (%.2f kHz)\nWIDTH (saia): corte alto (%.1f kHz)  - FIL%d", qual, lo / 1000.f, hi / 1000.f, R.fil + 1);
                tIn = lo / 1000.f; tOut = (hi - 2000) / 6000.f; break;
        case 3: std::snprintf(d, sizeof d, "WIDTH do %s: largura do FM (%.1f kHz)  - FIL%d", qual, hi / 1000.f, R.fil + 1);
                tOut = (hi - 6000) / 24000.f; tIn = tOut; break;
        default: std::snprintf(d, sizeof d, "WIDTH do %s: largura do FM largo (%.0f kHz)  - FIL%d", qual, hi / 1000.f, R.fil + 1);
                tOut = (hi - 100000) / 150000.f; tIn = tOut; break;
        }
        float di, dO;
        if (giro9(id, cx, cy, rO, rI, std::clamp(tIn, 0.f, 1.f), std::clamp(tOut, 0.f, 1.f), di, dO, d, true, marcas, rM)) {
            switch (g) {
            case 0: {
                float s = di * 3000.f;
                s = std::clamp(s, -lo, 5000.f - hi);
                lo += s; hi += s;
                hi = std::clamp(hi + dO * 4600.f, lo + 400.f, 5000.f);
                break; }
            case 1: lo = std::clamp(lo + di * 1600.f, -800.f, 800.f); hi = std::clamp(hi + dO * 2450.f, 50.f, 2500.f); break;
            case 2: lo = std::clamp(lo + di * 1000.f, 0.f, 1000.f); hi = std::clamp(hi + dO * 6000.f, 2000.f, 8000.f); break;
            case 3: hi = std::clamp(hi + (di + dO) * 24000.f, 6000.f, 30000.f); break;
            default: hi = std::clamp(hi + (di + dO) * 150000.f, 100000.f, 250000.f); break;
            }
            if (rx == ativo_) aplicarFiltro();
        }
    };
    {
        rotDuplo9(1517, 481, 8.5f, BR, "SHIFT", "WIDTH");
        shiftWidth(0, "gShift0", 1520, 577, 66, 50, 11, 74);
        tx(fP(), 7.f, 1442, 619, BR, "AFL", 0.5f);
        if (tecla9("kAfl0", "", 1428, 630, 1457, 644, 3, M0.mute ? 4 : 1, "AFL do MAIN: silencia o MAIN (MUTE)", 0, 0, 0) == 1) M0.mute = !M0.mute;
        tx(fP(), 7.f, 1585, 620, BR, "ACM", 0.5f);
        if (tecla9("kAcm", "", 1570, 630, 1601, 644, 3, ifOn_ ? 3 : 1, "ACM: IF SCOPE - zoom de 20 kHz em volta da sintonia, dentro do escopo", 0, 0, 0) == 1) ifOn_ = !ifOn_;
    }
    // ---- CLAR / VFO-B ----
    {
        // RX / TX do VFO-B
        dl->AddRectFilled(Q(1620, 477), Q(1654, 488), IM_COL32(0x0c, 0x0c, 0x0d, 255), QD(2.5f));
        dl->AddRectFilled(Q(1622, 479), Q(1652, 486), ativo_ == 1 ? IM_COL32(0x5c, 0xe0, 0x6c, 255) : IM_COL32(0x2a, 0x40, 0x2e, 255), QD(2));
        tx(fP(), 7.f, 1663, 482, BR, "RX", 0.5f);
        dl->AddRectFilled(Q(1830, 477), Q(1867, 488), IM_COL32(0x0c, 0x0c, 0x0d, 255), QD(2.5f));
        dl->AddRectFilled(Q(1832, 479), Q(1865, 486), txVfo_ == 1 ? IM_COL32(0xff, 0x40, 0x60, 255) : IM_COL32(0x50, 0x20, 0x28, 255), QD(2));
        tx(fP(), 7.f, 1820, 482, BR, "TX", 0.5f);
        // o rotulo CLAR / VFO-B (o que estiver valendo fica aceso)
        const ImU32 LAR = IM_COL32(0xff, 0x9a, 0x40, 255);
        const float wc = txW(fP(), 8.5f, "CLAR/", 0.95f), wv = txW(fP(), 8.5f, "VFO-B", 0.95f);
        const float x0 = 1742.f - (wc + wv) * 0.5f;
        tx(fP(), 8.5f, x0, 480, clarB_ == 1 ? LAR : BR, "CLAR/", 0.f, 0.95f);
        tx(fP(), 8.5f, x0 + wc, 480, clarB_ == 0 ? LAR : IM_COL32(0xb0, 0x80, 0x50, 255), "VFO-B", 0.f, 0.95f);
        tx(fP(), 9.f, 1690, 488, BR, "-", 0.5f);
        tx(fP(), 9.f, 1773, 488, BR, "+", 0.5f);
        // o arco laranja (A/B: aceso = o botao gira o VFO-B)
        {
            const ImVec2 c = Q(1740, 582);
            dl->PathArcTo(c, QD(82), rad(-62), rad(48), 32);
            dl->PathStroke(clarB_ == 0 ? IM_COL32(0xf0, 0x88, 0x30, 255) : IM_COL32(0x60, 0x40, 0x24, 255), 0, std::max(1.5f, QD(3.5f)));
        }
        char d[260];
        if (clarB_ == 0) std::snprintf(d, sizeof d, "VFO-B: sintoniza o SUB (%s)%s\nA/B (embaixo, à direita): passa a girar o CLAR",
                                       fmtFreq(B1.f).c_str(), mhzB_ ? " - de 1 MHz em 1 MHz (BAND/MHz)" : fastB_ ? " - rápido (FAST)" : "");
        else std::snprintf(d, sizeof d, "CLAR: deslocamento da escuta do receptor ouvido (%+.3f kHz)\nA/B: volta a girar o VFO-B", A.rit / 1000.0);
        float di, dO;
        if (giro9("gClarB", 1740, 582, 70, 55, (A.rit + 9990.f) / 19980.f, 0.5f, di, dO, d, false, 0, 0)) {
            static float acc = 0;
            acc += (di + dO) * 100.f;
            const int pas = (int)acc;
            if (pas != 0) {
                acc -= pas;
                if (clarB_ == 1) { A.rit = std::clamp(A.rit + pas * 10, -9990, 9990); A.ritOn = true; }
                else if (!B1.lock) {
                    const int64_t st = mhzB_ ? 1000000 : (int64_t)std::max(1, passo_) * (fastB_ ? 10 : 1);
                    B1.memModo = false;
                    mudarFreqRx(1, (int64_t)pas * st);
                }
            }
        }
        // as teclas da esquerda
        tx(fP(), 6.5f, 1652, 497, BR, "CLAR", 0.5f);
        {
            const int k = tecla9("kClar", "", 1630, 505, 1661, 519, 3, A.ritOn ? 3 : 1, "CLAR: liga o deslocamento da escuta (RIT) do receptor ouvido\nSegurar: zera o CLAR", 0, 0, 0);
            if (k == 1) A.ritOn = !A.ritOn;
            if (k == 2) { A.rit = 0; aviso("CLAR zerado"); }
        }
        tx(fP(), 6.f, 1636, 537, BR, "BAND/MHz", 0.5f);
        if (tecla9("kMhzB", "", 1620, 548, 1651, 562, 3, mhzB_ ? 3 : 1, "BAND/MHz: o botão VFO-B anda de 1 MHz em 1 MHz", 0, 0, 0) == 1) mhzB_ = !mhzB_;
        tx(fP(), 6.f, 1635, 577, IM_COL32(0xf0, 0x90, 0x48, 255), "B-DISP OFF", 0.5f);
        if (tecla9("kBdisp", "", 1622, 585, 1653, 599, 3, bDispOff_ ? 3 : 1, "B-DISP OFF: apaga / mostra a frequência do VFO-B no VFD", 0, 0, 0) == 1) bDispOff_ = !bDispOff_;
        tx(fP(), 6.5f, 1645, 616, BR, "MCH/GRP", 0.5f);
        if (tecla9("kMch", "", 1635, 628, 1666, 642, 3, memAberta_ ? 3 : 1, "MCH/GRP: a lista de memórias (grupos e canais)", 0, 0, 0) == 1) {
            memAberta_ = !memAberta_;
            std::snprintf(memNome_, sizeof memNome_, "%s %s", fmtFreq(r_->vfo()).c_str(), r_->modo().c_str());
        }
        // as teclas da direita
        tx(fP(), 6.f, 1813, 497, IM_COL32(0xf0, 0x90, 0x48, 255), "FAST", 0.5f); tx(fP(), 6.f, 1836, 497, BR, "RX", 0.5f);
        if (tecla9("kFastB", "", 1820, 505, 1851, 519, 3, fastB_ ? 3 : 1, "FAST: o botão VFO-B anda 10x mais rápido", 0, 0, 0) == 1) fastB_ = !fastB_;
        tx(fP(), 5.5f, 1838, 537, BR, "BAND/MHz TX", 0.5f);
        if (tecla9("kTxB", "", 1827, 548, 1858, 562, 3, txVfo_ == 1 ? 4 : 1, "TX: marca o VFO-B como frequência de \"TX\" (SPLIT)\nDe novo: volta o TX para o VFO-A", 0, 0, 0) == 1)
            txVfo_ = txVfo_ == 1 ? 0 : 1;
        tx(fP(), 5.5f, 1838, 577, BR, "B-MODE CLEAR", 0.5f);
        {
            const int k = tecla9("kBmode", "", 1827, 585, 1858, 599, 3, 1, "B-MODE: troca o modo do VFO-B (LSB / USB / CW / AM / FM)\nSegurar: CLEAR - zera o CLAR dos dois receptores", 0, 0, 0);
            if (k == 1) {
                static const char* ms[5] = {"LSB", "USB", "CW", "AM", "NFM"};
                int i = 0;
                const std::string mb = ativo_ == 1 ? r_->modo() : B1.modo;
                for (int j = 0; j < 5; ++j) if (mb == ms[j]) i = j;
                const std::string novo = ms[(i + 1) % 5];
                if (ativo_ == 1) mudarModoPainel(novo); else B1.modo = novo;
                aviso("VFO-B: " + nomeModo(novo), 1.2);
            }
            if (k == 2) { rx_[0].rit = 0; rx_[1].rit = 0; aviso("CLEAR: CLAR zerado"); }
        }
        tx(fP(), 7.f, 1820, 616, IM_COL32(0xf0, 0x90, 0x48, 255), "A/B", 0.5f);
        if (tecla9("kAB", "", 1785, 628, 1846, 642, 3, clarB_ == 0 ? 3 : 1,
                   "A/B: o botão grande gira o VFO-B (laranja aceso) ou o CLAR do receptor ouvido", 0, 0, 0) == 1)
            clarB_ = 1 - clarB_;
    }
}

// ===========================================================================
//  MB1SDR: a fileira de baixo (B = o SUB) e a tecla CACHOEIRA no lugar do
//  cartao de memoria
// ===========================================================================
void Ui::ft9Baixo()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 BR = IM_COL32(0xf4, 0xf4, 0xf4, 255);
    Receptor& S = rx_[1];
    const float yk0 = 628, yk1 = 642, yl = 620, yr = 588;
    // ---- AGC / ATT do SUB ----
    {
        tx(fP(), 6.5f, 147, 587, BR, "AGC", 0.5f);
        dl->AddCircle(Q(147, 597), QD(3), BR, 12, std::max(1.f, QD(1.f)));
        tx(fP(), 6.5f, 147, 609, BR, "ATT", 0.5f);
        tx(fP(), 5.5f, 147, 620, BR, "FLT", 0.5f); tx(fP(), 4.4f, 166, 620, BR, "SLW", 0.5f);
        tx(fP(), 4.4f, 177, 592, BR, "FAST", 0.5f); tx(fP(), 4.4f, 200, 586, BR, "AUTO/0", 0.5f); tx(fP(), 4.4f, 231, 592, BR, "OFF/3", 0.5f);
        tx(fP(), 4.4f, 166, 604, BR, "MID", 0.5f); tx(fP(), 4.4f, 236, 604, BR, "6", 0.5f); tx(fP(), 4.4f, 242, 620, BR, "12", 0.5f);
        tx(fP(), 4.4f, 241, 635, BR, "18", 0.5f); tx(fP(), 4.5f, 241, 644, BR, "(dB)", 0.5f);
        static const float aAgc[4] = {-90.f, -65.f, -38.f, 42.f};
        static const float aAtt[4] = {5.f, 68.f, 92.f, 122.f};
        const int pa = S.agcOff ? 3 : std::clamp(S.agc, 1, 3) - 1;
        static float accA = 0, accT = 0;
        const char* nm[4] = {"SLOW", "MID", "FAST", "OFF"};
        char d[240];
        std::snprintf(d, sizeof d, "AGC (miolo) do SUB: SLOW / MID / FAST / OFF - agora %s\nATT (saia, vale para os dois): 0 / 6 / 12 / 18 dB - agora %d dB", nm[pa], att_ * 6);
        float di, dO;
        if (giro9("gAgc1", 195, 632, 23, 17, tAng(aAgc[pa]), tAng(aAtt[std::clamp(att_, 0, 3)]), di, dO, d, true, 0, 0)) {
            accA += di; accT += dO;
            int np = pa;
            if (accA > 0.07f) { np = std::min(3, pa + 1); accA = 0; }
            if (accA < -0.07f) { np = std::max(0, pa - 1); accA = 0; }
            if (np != pa) { if (np == 3) S.agcOff = true; else { S.agcOff = false; S.agc = np + 1; } aviso(std::string("AGC do SUB ") + nm[np], 1.0); }
            if (accT > 0.07f) { att_ = std::min(3, att_ + 1); accT = 0; }
            if (accT < -0.07f) { att_ = std::max(0, att_ - 1); accT = 0; }
        }
        const int k = tecla9("kFlt1", "", 145, yk0, 160, yk1, 3, 1, "FLT do SUB: troca o filtro FIL1 / FIL2 / FIL3\nSegurar: escolher a largura (BW)", 0, 0, 0);
        if (k == 1) { S.fil = (S.fil + 1) % 3; if (ativo_ == 1) aplicarFiltro(); char a[32]; std::snprintf(a, sizeof a, "SUB FIL%d", S.fil + 1); aviso(a, 1.0); }
        if (k == 2) { if (ativo_ != 1) ativar(1); ImGui::OpenPopup("##bwIc"); }
    }
    // ---- VRF (tecla: BC) / VRF - NTCH / NTCH ----
    tx(fP(), 6.f, 259, yl, BR, "VRF", 0.5f);
    if (tecla9("kVrf1", "", 250, yk0, 265, yk1, 3, S.bc ? 3 : 1, "VRF do SUB: cancelador de vários apitos (BC)", 0, 0, 0) == 1) S.bc = !S.bc;
    {
        rotDuplo9(307, yr, 6.6f, BR, "VRF", "NTCH");
        tx(fP(), 4.5f, 272, 597, BR, "DN", 0.5f); tx(fP(), 4.5f, 345, 597, BR, "UP", 0.5f);
        const double nh = 150.0 * std::pow(3500.0 / 150.0, std::clamp(S.notch, 0.f, 1.f));
        char d[200];
        std::snprintf(d, sizeof d, "VRF (miolo): ganho de RF do SUB (%d dB)\nNTCH (saia): posição do NOTCH do SUB (%d Hz)", (int)std::lround(49.6f * S.rf / 100.f), (int)std::lround(nh));
        float di, dO;
        if (giro9("gVrf1", 297, 632, 23, 17, S.rf / 100.f, S.notch, di, dO, d, true, 0, 0)) {
            S.rf = std::clamp(S.rf + di * 100.f, 0.f, 100.f);
            S.notch = std::clamp(S.notch + dO, 0.f, 1.f);
        }
    }
    tx(fP(), 6.5f, 359, yl, BR, "NTCH", 0.5f);
    {
        const int k = tecla9("kNtch1", "", 350, yk0, 365, yk1, 3, S.nch ? 3 : 1, "NTCH do SUB: notch manual\nSegurar: estreito / largo", 0, 0, 0);
        if (k == 1) S.nch = !S.nch;
        if (k == 2) { S.nch = true; S.nchLargo = !S.nchLargo; aviso(S.nchLargo ? "NOTCH do SUB largo" : "NOTCH do SUB estreito"); }
    }
    // ---- NB - SQL do SUB ----
    {
        rotDuplo9(407, yr, 6.6f, BR, "NB", "SQL");
        char d[200], sq[24];
        if (S.sql <= -90) std::snprintf(sq, sizeof sq, "aberto"); else std::snprintf(sq, sizeof sq, "%+d dB", (int)std::lround(S.sql));
        std::snprintf(d, sizeof d, "NB (miolo): nível do NB (%.0f)\nSQL (saia): squelch do SUB (%s)", nbNivel_, sq);
        float di, dO;
        if (giro9("gNbSql1", 407, 632, 23, 17, nbNivel_ / 100.f, (S.sql + 90.f) / 120.f, di, dO, d, true, 9, 27)) {
            nbNivel_ = std::clamp(nbNivel_ + di * 100.f, 0.f, 100.f);
            if (dO != 0) {
                S.sql = std::clamp(S.sql + dO * 120.f, -90.f, 30.f);
                if (ativo_ == 1) { sql_ = S.sql; sqlAplicado_ = S.sql; }
                if (S.sql <= -90.f) sqlAuto_[1] = false;
            }
        }
    }
    tx(fP(), 6.5f, 463, yl, BR, "NB", 0.5f);
    {
        const int k = tecla9("kNb1", "", 455, yk0, 470, yk1, 3, S.nb1 ? 2 : S.nb2 ? 3 : 1, "NB do SUB: apagador de pulsos\nSegurar: NB2", 0, 0, 0);
        if (k == 1) S.nb1 = !S.nb1;
        if (k == 2) { S.nb2 = !S.nb2; aviso(S.nb2 ? "NB2 do SUB ligado" : "NB2 do SUB desligado"); }
    }
    // ---- SHIFT - WIDTH do SUB ----
    {
        rotDuplo9(517, yr, 6.6f, BR, "SHIFT", "WIDTH");
        // a mesma conta do MAIN (no filtro do SUB)
        const std::string md = ativo_ == 1 ? r_->modo() : S.modo;
        const int g = grupoModo(md);
        float& hi = filHi_[g][S.fil];
        float& lo = filLo_[g][S.fil];
        float tIn = 0.5f, tOut = 0.5f;
        char d[220];
        switch (g) {
        case 0: tIn = (lo + hi) * 0.5f / 3000.f; tOut = (hi - lo - 400.f) / 4600.f;
                std::snprintf(d, sizeof d, "SHIFT (miolo) do SUB: desloca o filtro (%.2f a %.2f kHz)\nWIDTH (saia): largura %.2f kHz - FIL%d", lo / 1000.f, hi / 1000.f, (hi - lo) / 1000.f, S.fil + 1); break;
        case 1: tIn = (lo + 800) / 1600.f; tOut = (hi - 50) / 2450.f;
                std::snprintf(d, sizeof d, "SHIFT (miolo) do SUB: %+.0f Hz\nWIDTH (saia): %.0f Hz - FIL%d", lo, hi, S.fil + 1); break;
        case 2: tIn = lo / 1000.f; tOut = (hi - 2000) / 6000.f;
                std::snprintf(d, sizeof d, "SHIFT do SUB: corte baixo do AM (%.2f kHz)\nWIDTH: corte alto (%.1f kHz) - FIL%d", lo / 1000.f, hi / 1000.f, S.fil + 1); break;
        case 3: tOut = (hi - 6000) / 24000.f; tIn = tOut; std::snprintf(d, sizeof d, "WIDTH do SUB: largura do FM (%.1f kHz)", hi / 1000.f); break;
        default: tOut = (hi - 100000) / 150000.f; tIn = tOut; std::snprintf(d, sizeof d, "WIDTH do SUB: largura do FM largo (%.0f kHz)", hi / 1000.f); break;
        }
        float di, dO;
        if (giro9("gShift1", 510, 632, 23, 17, std::clamp(tIn, 0.f, 1.f), std::clamp(tOut, 0.f, 1.f), di, dO, d, true, 9, 27)) {
            switch (g) {
            case 0: { float s = std::clamp(di * 3000.f, -lo, 5000.f - hi); lo += s; hi += s; hi = std::clamp(hi + dO * 4600.f, lo + 400.f, 5000.f); break; }
            case 1: lo = std::clamp(lo + di * 1600.f, -800.f, 800.f); hi = std::clamp(hi + dO * 2450.f, 50.f, 2500.f); break;
            case 2: lo = std::clamp(lo + di * 1000.f, 0.f, 1000.f); hi = std::clamp(hi + dO * 6000.f, 2000.f, 8000.f); break;
            case 3: hi = std::clamp(hi + (di + dO) * 24000.f, 6000.f, 30000.f); break;
            default: hi = std::clamp(hi + (di + dO) * 150000.f, 100000.f, 250000.f); break;
            }
            if (ativo_ == 1) aplicarFiltro();
        }
    }
    tx(fP(), 6.5f, 569, yl, BR, "CONT", 0.5f);
    if (tecla9("kCont1", "", 558, yk0, 573, yk1, 3, S.bef ? 3 : 1, "CONT do SUB: contorno (posição no CONT - DNR do SUB)", 0, 0, 0) == 1) S.bef = !S.bef;
    // ---- CONT - DNR do SUB ----
    {
        rotDuplo9(610, yr, 6.6f, BR, "CONT", "DNR");
        char d[200];
        std::snprintf(d, sizeof d, "CONT (miolo): posição do contorno do SUB - girar liga o CONT\nDNR (saia): força do redutor de ruído (%.0f)", nrEspForca_);
        float di, dO;
        if (giro9("gCont1", 610, 632, 23, 17, S.cont, nrEspForca_ / 100.f, di, dO, d, true, 0, 0)) {
            if (di != 0) { S.cont = std::clamp(S.cont + di, 0.f, 1.f); if (!S.bef) { S.bef = true; aviso("CONT do SUB ligado"); } }
            nrEspForca_ = std::clamp(nrEspForca_ + dO * 100.f, 0.f, 100.f);
        }
    }
    {   // a plaquinha B
        dl->AddRectFilled(Q(666, 585), Q(684, 601), IM_COL32(0xd8, 0xda, 0xde, 255), QD(2));
        tx(fP(), 8.f, 675, 593, IM_COL32(0x20, 0x22, 0x26, 255), "B", 0.5f);
    }
    tx(fP(), 6.5f, 667, yl, BR, "DNR", 0.5f);
    {
        const int k = tecla9("kDnr1", "", 660, yk0, 675, yk1, 3, S.nr1 ? 3 : S.nr2 ? 2 : 1, "DNR do SUB: redutor de ruído espectral\nSegurar: NR1", 0, 0, 0);
        if (k == 1) S.nr2 = !S.nr2;
        if (k == 2) { S.nr1 = !S.nr1; aviso(S.nr1 ? "NR1 do SUB ligado" : "NR1 do SUB desligado"); }
    }
    // ---- AF GAIN - RF GAIN do SUB ----
    {
        rotDuplo9(755, 589, 6.6f, BR, "AF GAIN", "RF GAIN");
        char d[200];
        std::snprintf(d, sizeof d, "AF GAIN (miolo): volume do SUB (VFO-B) - %d%%\nRF GAIN (saia): ganho de RF do SUB - %d dB", (int)std::lround(S.af), (int)std::lround(49.6f * S.rf / 100.f));
        float di, dO;
        if (giro9("gAf1", 755, 632, 23, 17, S.af / 100.f, S.rf / 100.f, di, dO, d, true, 9, 27)) {
            S.af = std::clamp(S.af + di * 100.f, 0.f, 100.f);
            S.rf = std::clamp(S.rf + dO * 100.f, 0.f, 100.f);
            if (ativo_ != 1 && di != 0) aviso("Este é o volume do SUB. Ouvindo o MAIN: use o AF GAIN grande (à direita)", 2.5);
        }
    }
    tx(fP(), 6.5f, 707, 621, BR, "DNF", 0.5f);
    if (tecla9("kDnf1", "", 699, 630, 714, 644, 3, S.anch ? 3 : 1, "DNF do SUB: notch automático", 0, 0, 0) == 1) S.anch = !S.anch;
    tx(fP(), 6.5f, 806, yl, BR, "AFL", 0.5f);
    if (tecla9("kAfl1", "", 800, yk0, 815, yk1, 3, S.mute ? 4 : 1, "AFL do SUB: silencia o SUB (MUTE)", 0, 0, 0) == 1) S.mute = !S.mute;
    // ---- RF PWR (miolo: LEVEL do escopo) - BIAS (saia: CONTRAST) ----
    {
        rotDuplo9(865, 589, 6.2f, BR, "RF PWR", "BIAS");
        char d[200];
        std::snprintf(d, sizeof d, "RF PWR (miolo): LEVEL do escopo e da cachoeira (%.0f)\nBIAS (saia): CONTRAST - faixa em dB (%.0f dB)", wfBrilho_ - 100.f, wfRange_);
        float di, dO;
        if (giro9("gPwr", 865, 632, 23, 17, wfBrilho_ / 200.f, (wfRange_ - 20.f) / 100.f, di, dO, d, true, 9, 27)) {
            wfBrilho_ = std::clamp(wfBrilho_ + di * 200.f, 0.f, 200.f);
            wfRange_ = std::clamp(wfRange_ + dO * 100.f, 20.f, 120.f);
            autoWf_ = false;
        }
        tx(fP(), 6.5f, 930, 619, BR, "CLASS-A", 0.5f);
        if (tecla9("kClassA", "", 919, 627, 936, 653, 3, espCheio_ ? 3 : 1, "CLASS-A: espectro preenchido (apagado = só a linha dos picos)", 0, 0, 0) == 1) espCheio_ = !espCheio_;
    }
    // ---- CACHOEIRA: no lugar do cartao de memoria ----
    {
        // a moldura dourada do buraco do cartao
        retGrad(dl, Q(1006, 618), Q(1142, 656), QD(4), IM_COL32(0xf6, 0xd8, 0x80, 255), IM_COL32(0x9a, 0x6c, 0x1c, 255));
        dl->AddRect(Q(1006, 618), Q(1142, 656), IM_COL32(0x5a, 0x3c, 0x08, 255), QD(4), 0, std::max(1.f, QD(1.f)));
        dl->AddLine(Q(1010, 619.2f), Q(1138, 619.2f), IM_COL32(255, 248, 220, 160), 1.f);
        dl->AddRectFilled(Q(1010, 622), Q(1138, 652), IM_COL32(0x0a, 0x0a, 0x0b, 255), QD(2.5f));
        dl->AddRect(Q(1010, 622), Q(1138, 652), IM_COL32(0x6a, 0x48, 0x10, 255), QD(2.5f), 0, 1.f);
        if (tecla9("kCach", "CACHOEIRA", 1019, 627, 1129, 647, 0, 0,
                   cimaAtivo_ ? "CACHOEIRA: desligar o espectro e a cachoeira grandes em cima do rádio"
                              : "CACHOEIRA: ligar o espectro e a cachoeira grandes em cima do rádio (ocupa a tela)",
                   0, cimaAtivo_ ? IM_COL32(0xff, 0xa8, 0x40, 255) : 0u, 8.f) == 1)
            cimaAtivo_ = !cimaAtivo_;
        if (cimaAtivo_) {
            dl->AddRectFilled(Q(1027, 643), Q(1121, 645), IM_COL32(0xff, 0x9a, 0x30, 255), QD(1));
        }
    }
}

// MB1SDR: o painel do MB1 (medido na foto endireitada mb1_vista_frente)
#include "PainelMB1.inc"

} // namespace masdr
