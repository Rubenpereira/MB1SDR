// ---------------------------------------------------------------------------
//  MB1SDR: o painel do transceptor de HF/VHF MB1 (sem a marca do fabricante)
//
//  Tudo foi medido na foto da frente do radio (mb1_vista_frente.webp) depois
//  de endireitada (tirada a perspectiva) para 1500 x 613 px: as funcoes mb1*()
//  recebem as posicoes NESSA foto. O papel = 1500 x (24 da tampa + 613 da
//  frente) + 30 dos pes; Q() / QD() / tx() convertem para a tela.
//
//  A tela do MB1 ja tem o espectro e a cachoeira: nao ha cachoeira separada.
//  A largura do escopo vai de 0,25 a 3,2 MSPS (tecla de toque SPAN / F6).
//  Este arquivo e incluido no fim de Painel.cpp (usa as pecas de la).
// ---------------------------------------------------------------------------

namespace {

// cores da tela do MB1
const ImU32 M_BRANCO  = IM_COL32(0xf2, 0xf4, 0xf8, 255);
const ImU32 M_CIANO   = IM_COL32(0x5c, 0xbe, 0xf6, 255);
const ImU32 M_APAG    = IM_COL32(0x3c, 0x43, 0x4e, 255);
const ImU32 M_VERDE   = IM_COL32(0x54, 0xe8, 0x6e, 255);
const ImU32 M_AMAR    = IM_COL32(0xf4, 0xec, 0x6c, 255);
const ImU32 M_ROSA    = IM_COL32(0xff, 0x68, 0x76, 255);
const ImU32 M_LARANJA = IM_COL32(0xee, 0x9c, 0x62, 255);
const ImU32 M_ROT     = IM_COL32(0xf6, 0xf6, 0xf6, 255);     // rotulos do painel (brancos)

// angulo (graus, 0 = direita, -90 = para cima) de um ponto do medidor
inline ImVec2 polar(float cx, float cy, float ang, float r) { return ImVec2(cx + std::cos(rad(ang)) * r, cy + std::sin(rad(ang)) * r); }

// Tecla do MB1: face preta brilhante dentro da moldura cinza (como no radio).
// Devolve 1 = clique, 2 = segurou 0,6 s. segura: true enquanto aperta.
int teclaMb(const char* id, float x0, float y0, float x1, float y1, const char* dica, bool* segura = nullptr, bool aceso = false)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = Q(x0, y0), b = Q(x1, y1);
    ImGui::SetCursorScreenPos(a);
    ImGui::PushID(id);
    ImGui::InvisibleButton("##mb", ImVec2(std::max(1.f, b.x - a.x), std::max(1.f, b.y - a.y)));
    const ImGuiID iid = ImGui::GetItemID();
    const bool sobre = ImGui::IsItemHovered(), ativo = ImGui::IsItemActive();
    int res = 0;
    if (ImGui::IsItemActivated()) longoFeito()[iid] = 0;
    if (ativo && ImGui::GetIO().MouseDownDuration[0] > 0.6f && longoFeito()[iid] == 0) { longoFeito()[iid] = 1; res = 2; }
    if (ImGui::IsItemDeactivated() && sobre && longoFeito()[iid] == 0) res = 1;
    ImGui::PopID();
    if (segura) *segura = ativo;
    const float r = QD(5.f), g = QD(2.4f), gg = QD(3.6f);
    // rebaixo escuro e a moldura cinza (mais clara em cima, como a luz da foto)
    dl->AddRectFilled(ImVec2(a.x - gg, a.y - gg), ImVec2(b.x + gg, b.y + gg * 1.15f), IM_COL32(0x10, 0x10, 0x11, 255), r + gg);
    retGrad(dl, ImVec2(a.x - g, a.y - g), ImVec2(b.x + g, b.y + g), r + g, IM_COL32(0x56, 0x57, 0x5b, 255), IM_COL32(0x28, 0x29, 0x2c, 255));
    // a face
    const ImU32 c0 = ativo ? IM_COL32(0x0a, 0x0a, 0x0b, 255) : sobre ? IM_COL32(0x36, 0x37, 0x3b, 255) : IM_COL32(0x26, 0x27, 0x2a, 255);
    const ImU32 c1 = ativo ? IM_COL32(0x05, 0x05, 0x06, 255) : IM_COL32(0x0e, 0x0e, 0x10, 255);
    retGrad(dl, a, b, r, c0, c1);
    if (aceso) dl->AddRect(a, b, IM_COL32(0xff, 0xa0, 0x40, 170), r, 0, std::max(1.f, QD(1.2f)));
    dl->AddLine(ImVec2(a.x + r, a.y + std::max(1.f, QD(1.f))), ImVec2(b.x - r, a.y + std::max(1.f, QD(1.f))), IM_COL32(255, 255, 255, ativo ? 10 : 40), 1.f);
    if (sobre && dica && *dica && !ativo) ImGui::SetTooltip("%s", dica);
    return res;
}

// tecla com o texto no meio (cap = altura das maiusculas, px da foto)
int teclaTxt(const char* id, const char* t, float x0, float y0, float x1, float y1, float cap, ImU32 cor, const char* dica,
             bool* segura = nullptr, bool aceso = false)
{
    const int k = teclaMb(id, x0, y0, x1, y1, dica, segura, aceso);
    tx(fP(), cap, (x0 + x1) * 0.5f, (y0 + y1) * 0.5f, cor, t, 0.5f, 0.93f, (x1 - x0) - 4.f);
    return k;
}

// LED redondo do painel (cinza-azulado apagado)
void ledMb(float cx, float cy, float r, ImU32 cor, bool aceso)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 c = Q(cx, cy);
    dl->AddCircleFilled(c, QD(r * 1.35f), IM_COL32(0x14, 0x14, 0x16, 255), 20);
    if (aceso) {
        dl->AddCircleFilled(c, QD(r * 2.4f), alfa(cor, 50), 24);
        dl->AddCircleFilled(c, QD(r), cor, 20);
        dl->AddCircleFilled(ImVec2(c.x - QD(r * 0.3f), c.y - QD(r * 0.3f)), QD(r * 0.4f), IM_COL32(255, 255, 255, 160), 10);
    } else {
        dl->AddCircleFilled(c, QD(r), IM_COL32(0x5a, 0x64, 0x78, 255), 20);
        dl->AddCircleFilled(ImVec2(c.x - QD(r * 0.25f), c.y - QD(r * 0.3f)), QD(r * 0.55f), IM_COL32(0x84, 0x90, 0xa6, 160), 12);
    }
}

// Botao do MB1: saia preta serrilhada e o miolo preto brilhante (duplo: o miolo e a
// saia sao dois controles). Arrastar para cima/direita aumenta; a roda tambem.
bool knobMb(const char* id, float cx, float cy, float R0, float tIn, float tOut, float& dIn, float& dOut,
            const char* dica, bool duplo)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    dIn = dOut = 0;
    const ImVec2 c = Q(cx, cy);
    const float R = QD(R0);
    const float TWO_PI = (float)(2 * kPi);
    const float Rc = R * 0.64f;
    const ImVec2 cc(c.x + R * 0.10f, c.y + R * 0.0f);
    ImGui::SetCursorScreenPos(ImVec2(c.x - R, c.y - R));
    ImGui::PushID(id);
    ImGui::InvisibleButton("##kn", ImVec2(R * 2.f, R * 2.f));
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
    // sombra no painel
    dl->AddCircleFilled(ImVec2(c.x + R * 0.06f, c.y + R * 0.12f), R * 1.04f, IM_COL32(0, 0, 0, 150), 48);
    // a saia: borracha preta com o serrilhado e o brilho em cima, a esquerda
    auto luz = [](float a) { return 0.08f + 0.38f * sino(a, rad(-125), rad(48)) + 0.14f * sino(a, rad(45), rad(35)); };
    anelLuz(dl, c, R * 0.70f, R, 0.f, TWO_PI, 96, [&](float a, float rr) { return cinzaN(luz(a) * (rr > R * 0.97f ? 0.55f : 1.f)); });
    for (int k = 0; k < 48; ++k) {
        const float an = aFora + k * TWO_PI / 48;
        const ImVec2 d = dirAng(an);
        dl->AddLine(ImVec2(c.x + d.x * R * 0.74f, c.y + d.y * R * 0.74f), ImVec2(c.x + d.x * R * 0.97f, c.y + d.y * R * 0.97f),
                    cinzaN(luz(an) * 0.35f, 220), std::max(1.f, R * 0.035f));
    }
    circuloSuave(dl, c, R, 96, 1.f, [&](float a) { return cinzaN(0.10f + 0.40f * sino(a, rad(-125), rad(45)), 230); });
    if (duplo) {    // marquinha da saia
        const ImVec2 d = dirAng(aFora);
        dl->AddLine(ImVec2(c.x + d.x * R * 0.74f, c.y + d.y * R * 0.74f), ImVec2(c.x + d.x * R * 0.93f, c.y + d.y * R * 0.93f),
                    (ativo && parteGiro()[iid] == 1) ? P_LARANJA : IM_COL32(0x9c, 0xa0, 0xa6, 220), std::max(1.2f, QD(2.f)));
    }
    // o miolo: degrau escuro, lateral e a tampa com o reflexo
    dl->AddCircleFilled(ImVec2(cc.x + Rc * 0.05f, cc.y + Rc * 0.10f), Rc * 1.06f, IM_COL32(0, 0, 0, 190), 48);
    anelLuz(dl, cc, Rc * 0.82f, Rc, 0.f, TWO_PI, 72, [&](float a, float rr) {
        const float l = 0.05f + 0.38f * sino(a, rad(-120), rad(40)) + 0.12f * sino(a, rad(55), rad(30));
        return cinzaN(rr > Rc * 0.96f ? l * 0.5f : l);
    });
    discoCone(dl, cc, Rc * 0.82f, [](float a) { return 0.07f + 0.30f * sino(a, rad(-130), rad(30)) + 0.12f * sino(a, rad(50), rad(28)); }, 1.f);
    dl->AddCircleFilled(ImVec2(cc.x - Rc * 0.22f, cc.y - Rc * 0.26f), Rc * 0.28f, IM_COL32(255, 255, 255, 18), 32);
    {   // risco do miolo
        const ImVec2 d = dirAng(aIn);
        dl->AddLine(ImVec2(cc.x + d.x * Rc * 0.45f, cc.y + d.y * Rc * 0.45f), ImVec2(cc.x + d.x * Rc * 0.78f, cc.y + d.y * Rc * 0.78f),
                    (ativo && parteGiro()[iid] == 0) ? P_LARANJA : IM_COL32(0x8c, 0x90, 0x96, 220), std::max(1.2f, QD(2.2f)));
    }
    if (ativo) dl->AddCircle(c, R * 1.04f, alfa(P_LARANJA, 130), 48, std::max(1.f, QD(2.f)));
    if ((sobre || ativo) && dica) ImGui::SetTooltip("%s", dica);
    return dIn != 0 || dOut != 0;
}

// rotulo de botao duplo do MB1: "AF -(o)- RF" sublinhado (a de cima) e, opcional, a 2a funcao embaixo
void rotKnobMb(float cxB, float y1, float y2, float cap, const char* a, const char* b, const char* a2, const char* b2)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float ws = cap * 1.25f;
    const float wa = txW(fP(), cap, a, 0.92f), wb = txW(fP(), cap, b, 0.92f);
    const float wa2 = a2 ? txW(fP(), cap, a2, 0.92f) : 0.f, wb2 = b2 ? txW(fP(), cap, b2, 0.92f) : 0.f;
    // o simbolo fica onde o grupo (o lado esquerdo maior + simbolo + o lado direito maior) fica centrado no botao
    const float wl = std::max(wa, wa2) + 2.f, wr = std::max(wb, wb2) + 2.f;
    const float cx = cxB - (wl + ws + wr) * 0.5f + wl + ws * 0.5f;
    tx(fP(), cap, cx - ws * 0.5f - 2.f, y1, M_ROT, a, 1.f, 0.92f);
    tx(fP(), cap, cx + ws * 0.5f + 2.f, y1, M_ROT, b, 0.f, 0.92f);
    const ImVec2 c = Q(cx, y1 + (a2 ? cap * 0.85f : 0.f));
    dl->AddCircle(c, QD(cap * 0.42f), M_ROT, 16, std::max(1.f, QD(1.3f)));
    dl->AddCircleFilled(c, QD(cap * 0.16f), M_ROT, 10);
    // a linha embaixo da de cima
    const float yl = y1 + cap * 0.85f;
    dl->AddLine(Q(cx - ws * 0.5f - 2.f - wa, yl), Q(cx - ws * 0.42f, yl), M_ROT, std::max(1.f, QD(1.2f)));
    dl->AddLine(Q(cx + ws * 0.42f, yl), Q(cx + ws * 0.5f + 2.f + wb, yl), M_ROT, std::max(1.f, QD(1.2f)));
    if (a2) tx(fP(), cap, cx - ws * 0.5f - 2.f, y2, M_ROT, a2, 1.f, 0.92f);
    if (b2) tx(fP(), cap, cx + ws * 0.5f + 2.f, y2, M_ROT, b2, 0.f, 0.92f);
}

// as teclas de canto em volta do dial (RX2, A/B, LOCK, D/I): o retangulo menos o
// circulo do dial. (cxD, cyD, rD) = o dial. Devolve 1 / 2 como as teclas.
int teclaCanto(const char* id, float x0, float y0, float x1, float y1, float cxD, float cyD, float rD, const char* dica)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // o contorno: anda pela borda do retangulo; onde a borda entra no circulo do dial, segue o arco
    // (de onde entrou ate onde saiu) - assim nada passa do retangulo
    std::vector<ImVec2> pf;    // em px da foto
    const ImVec2 cantos[4] = {ImVec2(x0, y0), ImVec2(x1, y0), ImVec2(x1, y1), ImVec2(x0, y1)};
    auto fora = [&](ImVec2 p) { const float dx = p.x - cxD, dy = p.y - cyD; return dx * dx + dy * dy >= rD * rD; };
    std::vector<ImVec2> borda;
    for (int i = 0; i < 4; ++i) {
        const ImVec2 p0 = cantos[i], p1 = cantos[(i + 1) % 4];
        for (int k = 0; k < 60; ++k) { const float u = k / 60.f; borda.push_back(ImVec2(p0.x + (p1.x - p0.x) * u, p0.y + (p1.y - p0.y) * u)); }
    }
    const int nb = (int)borda.size();
    int ini = 0;
    while (ini < nb && !fora(borda[ini])) ++ini;     // comeca num ponto fora do disco
    if (ini >= nb) ini = 0;
    auto corte = [&](ImVec2 a, ImVec2 b) {           // ponto da borda (entre a fora e b dentro, ou o contrario) no circulo
        for (int it = 0; it < 24; ++it) { const ImVec2 m((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f); if (fora(m) == fora(a)) a = m; else b = m; }
        return a;
    };
    float angEntra = 0.f; bool dentroAgora = false;
    for (int k = 0; k <= nb; ++k) {
        const ImVec2 p = borda[(ini + k) % nb], pa = borda[(ini + k - 1 + nb) % nb];
        const bool f = fora(p);
        if (k > 0 && !dentroAgora && !f) {          // entrou no disco
            const ImVec2 c = corte(pa, p); pf.push_back(c);
            angEntra = std::atan2(c.y - cyD, c.x - cxD); dentroAgora = true;
        } else if (dentroAgora && f) {               // saiu: o arco por fora do disco (o caminho mais curto)
            const ImVec2 c = corte(p, pa);
            float a1 = std::atan2(c.y - cyD, c.x - cxD), da = a1 - angEntra;
            while (da > kPi) da -= (float)(2 * kPi);
            while (da < -kPi) da += (float)(2 * kPi);
            for (int m = 1; m < 24; ++m) { const float an = angEntra + da * m / 24.f; pf.push_back(ImVec2(cxD + std::cos(an) * rD, cyD + std::sin(an) * rD)); }
            pf.push_back(c); dentroAgora = false;
        }
        if (k < nb && f) pf.push_back(p);
    }
    // o canto mais longe do dial (de la todo o contorno e visivel: leque)
    int il = 0; float dm = -1;
    for (int i = 0; i < 4; ++i) { const float dx = cantos[i].x - cxD, dy = cantos[i].y - cyD; if (dx * dx + dy * dy > dm) { dm = dx * dx + dy * dy; il = i; } }
    const ImVec2 cl = cantos[il];
    ImGui::SetCursorScreenPos(Q(x0, y0));
    ImGui::PushID(id);
    const ImVec2 sz(std::max(1.f, QD(x1 - x0)), std::max(1.f, QD(y1 - y0)));
    ImGui::InvisibleButton("##ct", sz);
    const ImGuiID iid = ImGui::GetItemID();
    const ImVec2 mp = ImGui::GetIO().MousePos;
    const ImVec2 mf((mp.x - M.ox) / (M.k * kK), (mp.y - M.oy) / (M.k * kK) - 24.f);
    const bool dentro = fora(mf);
    if (ImGui::IsItemActivated() && !dentro) ImGui::ClearActiveID();
    const bool sobre = ImGui::IsItemHovered() && dentro, ativo = ImGui::IsItemActive();
    int res = 0;
    if (ImGui::IsItemActivated()) longoFeito()[iid] = 0;
    if (ativo && ImGui::GetIO().MouseDownDuration[0] > 0.6f && longoFeito()[iid] == 0) { longoFeito()[iid] = 1; res = 2; }
    if (ImGui::IsItemDeactivated() && ImGui::IsItemHovered() && dentro && longoFeito()[iid] == 0) res = 1;
    ImGui::PopID();
    // desenho: rebaixo, moldura e face
    auto leque = [&](float enc, ImU32 c0, ImU32 c1) {
        const ImVec2 uv = dl->_Data->TexUvWhitePixel;
        const int n = (int)pf.size();
        // encolhe o contorno em direcao ao centro do retangulo (moldura -> face)
        const float mx = (x0 + x1) * 0.5f, my = (y0 + y1) * 0.5f;
        std::vector<ImVec2> p(n);
        for (int k = 0; k < n; ++k) {
            const float dx = pf[k].x - mx, dy = pf[k].y - my, d = std::max(0.001f, std::sqrt(dx * dx + dy * dy));
            p[k] = Q(pf[k].x - dx / d * enc, pf[k].y - dy / d * enc);
        }
        const ImVec2 cq = Q(cl.x - (cl.x - mx) / std::max(1.f, std::fabs(cl.x - mx)) * enc, cl.y - (cl.y - my) / std::max(1.f, std::fabs(cl.y - my)) * enc);
        const float yt = Q(0, y0).y, hh = std::max(1.f, Q(0, y1).y - yt);
        dl->PrimReserve(n * 3, n + 1);
        const ImDrawIdx i0 = (ImDrawIdx)dl->_VtxCurrentIdx;
        dl->PrimWriteVtx(cq, uv, mistura(c0, c1, (cq.y - yt) / hh));
        for (int k = 0; k < n; ++k) dl->PrimWriteVtx(p[k], uv, mistura(c0, c1, (p[k].y - yt) / hh));
        for (int k = 0; k < n; ++k) {
            dl->PrimWriteIdx(i0);
            dl->PrimWriteIdx((ImDrawIdx)(i0 + 1 + k));
            dl->PrimWriteIdx((ImDrawIdx)(i0 + 1 + (k + 1) % n));
        }
    };
    leque(-3.2f, IM_COL32(0x10, 0x10, 0x11, 255), IM_COL32(0x10, 0x10, 0x11, 255));
    leque(-1.8f, IM_COL32(0x5a, 0x5b, 0x5f, 255), IM_COL32(0x2a, 0x2b, 0x2e, 255));
    leque(0.6f, ativo ? IM_COL32(0x08, 0x08, 0x09, 255) : sobre ? IM_COL32(0x34, 0x35, 0x39, 255) : IM_COL32(0x25, 0x26, 0x29, 255),
          ativo ? IM_COL32(0x04, 0x04, 0x05, 255) : IM_COL32(0x10, 0x10, 0x12, 255));
    if (sobre && dica && !ativo) ImGui::SetTooltip("%s", dica);
    return res;
}

// o simbolo da antena do MB1 (Y com a haste), centro em x, yc (px da foto)
void antenaMb(float x, float yc, float h, ImU32 cor)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float t = std::max(1.f, QD(1.4f));
    dl->AddLine(Q(x - h * 0.45f, yc - h * 0.5f), Q(x, yc), cor, t);
    dl->AddLine(Q(x + h * 0.45f, yc - h * 0.5f), Q(x, yc), cor, t);
    dl->AddLine(Q(x - h * 0.45f, yc - h * 0.5f), Q(x + h * 0.45f, yc - h * 0.5f), cor, t);
    dl->AddLine(Q(x, yc - h * 0.5f), Q(x, yc + h * 0.5f), cor, t);
}

// 7 segmentos finos da tela do MB1 (frequencias): os zeros da esquerda ficam apagados
struct DigMb { float x0, x1; uint64_t peso; };
void freqMb(ImDrawList* dl, uint64_t hz, const float* xs, int nGrande, float wG, float hG, float yG, float tG,
            const float* xp, int nPeq, float wP, float hP, float yP, float tP, const float* xPonto, ImU32 on, ImU32 apagado,
            std::vector<DigMb>* caixas)
{
    // digitos da esquerda para a direita: nGrande grandes (MHz e kHz) e nPeq pequenos (Hz)
    const int n = nGrande + nPeq;
    uint64_t peso = 1;
    for (int i = 0; i < n - 1; ++i) peso *= 10;
    bool lider = true;       // zeros a esquerda (apagados), ate o digito do MHz
    const int nMhz = nGrande - 3;
    for (int i = 0; i < n; ++i) {
        const int dig = (int)((hz / peso) % 10);
        if (dig != 0 || i >= nMhz - 1) lider = false;
        const ImU32 c = lider ? apagado : on;
        if (i < nGrande) {
            seg7(dl, xs[i], yG, wG, hG, dig, c, 0u, tG, 0.f);
            if (caixas) caixas->push_back({xs[i], xs[i] + wG, peso});
        } else {
            seg7(dl, xp[i - nGrande], yP, wP, hP, dig, c, 0u, tP, 0.f);
            if (caixas) caixas->push_back({xp[i - nGrande], xp[i - nGrande] + wP, peso});
        }
        peso /= 10;
    }
    // os pontos (depois do MHz e depois do kHz)
    for (int k = 0; k < 2; ++k) {
        const float d = k == 0 ? tG * 1.05f : tP * 1.05f;
        const float yb = k == 0 ? yG + hG : yP + hP;
        dl->AddRectFilled(Q(xPonto[k], yb - d), Q(xPonto[k] + d, yb), on);
    }
}

// texto da tela do MB1: duas passadas (a letra da tela e grossa)
void txT(ImFont* f, float cap, float x, float yc, ImU32 cor, const char* s, float alin = 0.f, float cond = 1.f, float maxW = 0.f)
{
    tx(f, cap, x, yc, cor, s, alin, cond, maxW);
    tx(f, cap, x + 0.45f, yc, cor, s, alin, cond, maxW);
}

// passo da grade / regua de frequencia do MB1 (de 5 a 8 linhas na largura)
double passoGradeMb(double span)
{
    static const double ps[] = {100, 200, 250, 500, 1000, 2000, 2500, 5000, 10000, 20000, 25000, 50000, 100000, 200000, 250000, 500000, 1e6, 2e6};
    for (double p : ps) if (span / p <= 8.0) return p;
    return 5e6;
}

} // namespace

// ===========================================================================
//  MB1SDR: a caixa (tampa, frente, a placa com o nome, a moldura da tela)
// ===========================================================================
void Ui::mb1Chassi()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImDrawListFlags fl0 = dl->Flags;
    if (g_fundoChave) dl->Flags &= ~ImDrawListFlags_AntiAliasedFill;   // sem franja rosa na cor-chave
    // ---- a tampa de cima (vista um pouco de cima: mais estreita no fundo) ----
    quadCores(dl, Q(38, -23), Q(1462, -23), Q(1500, 0), Q(0, 0),
              IM_COL32(0x26, 0x26, 0x28, 255), IM_COL32(0x26, 0x26, 0x28, 255), IM_COL32(0x3a, 0x3a, 0x3d, 255), IM_COL32(0x3a, 0x3a, 0x3d, 255));
    // ---- a frente (cinza-escuro, como o radio) ----
    dl->AddRectFilled(Q(0, 0), Q(1500, 613), IM_COL32(0x2d, 0x2d, 0x2f, 255), QD(9), ImDrawFlags_RoundCornersBottom);
    dl->Flags = fl0;
    dl->AddRectFilledMultiColor(Q(2, 2), Q(1498, 260), IM_COL32(0x33, 0x33, 0x35, 255), IM_COL32(0x31, 0x31, 0x33, 255),
                                IM_COL32(0x2d, 0x2d, 0x2f, 0), IM_COL32(0x2d, 0x2d, 0x2f, 0));
    // a quina da tampa com a frente (brilho fino) e as beiradas da caixa
    dl->AddRectFilledMultiColor(Q(0, -1.5f), Q(1500, 2.5f), IM_COL32(0x6a, 0x6a, 0x6e, 255), IM_COL32(0x60, 0x60, 0x64, 255),
                                IM_COL32(0x2e, 0x2e, 0x30, 255), IM_COL32(0x2e, 0x2e, 0x30, 255));
    dl->AddRectFilledMultiColor(Q(0, 0), Q(7, 613), IM_COL32(0x22, 0x22, 0x24, 255), IM_COL32(0x2b, 0x2b, 0x2d, 255),
                                IM_COL32(0x2b, 0x2b, 0x2d, 255), IM_COL32(0x22, 0x22, 0x24, 255));
    dl->AddRectFilledMultiColor(Q(1493, 0), Q(1500, 613), IM_COL32(0x2b, 0x2b, 0x2d, 255), IM_COL32(0x22, 0x22, 0x24, 255),
                                IM_COL32(0x22, 0x22, 0x24, 255), IM_COL32(0x2b, 0x2b, 0x2d, 255));
    dl->AddLine(Q(9, 611.5f), Q(1491, 611.5f), IM_COL32(0x15, 0x15, 0x16, 255), std::max(1.f, QD(2.f)));
    // textura fininha da frente (pontinhos sempre nos mesmos lugares)
    {
        const float t = std::max(1.f, QD(1.1f));
        for (uint32_t i = 0; i < 5200; ++i) {
            const uint32_t h = hashIc(i * 2654435761u + 37u);
            const float x = 8 + (h & 0xffff) / 65535.f * 1484.f, y = 4 + (h >> 16) / 65535.f * 604.f;
            if (x > 152 && x < 882 && y > 48 && y < 524) continue;      // a tela
            const ImVec2 q = Q(x, y);
            dl->AddRectFilled(q, ImVec2(q.x + t, q.y + t), (h & 0x300) ? IM_COL32(0, 0, 0, 45) : IM_COL32(255, 255, 255, 16));
        }
    }
    // ---- no lugar do logotipo do fabricante: o que o programa e ----
    tx(fP(), 7.5f, 152, 24, M_ROT, "HF/VHF/UHF", 0.f, 0.95f);
    tx(fP(), 5.6f, 152, 37, IM_COL32(0xc8, 0xca, 0xce, 255), "SDR  RECEIVER", 0.f, 1.18f);
    // ---- a placa do nome: a grade de pontinhos e "MB1SDR" dourado ----
    {
        for (int cI = 0; cI < 8; ++cI)
            for (int rI = 0; rI < 6; ++rI) {
                const float px = 562 + cI * 5.6f, py = 19 + rI * 5.2f;
                const int al = 70 + cI * 22;
                dl->AddRectFilled(Q(px, py), Q(px + 3.2f, py + 3.2f), IM_COL32(0x4c, 0x8c, 0xc8, std::min(255, al)));
            }
        retGrad(dl, Q(605, 17), Q(884, 47), QD(2), IM_COL32(0x22, 0x23, 0x26, 255), IM_COL32(0x0e, 0x0e, 0x10, 255));
        dl->AddRect(Q(605, 17), Q(884, 47), IM_COL32(0x58, 0x5a, 0x60, 255), QD(2), 0, std::max(1.f, QD(1.f)));
        dl->AddLine(Q(607, 18.2f), Q(882, 18.2f), IM_COL32(255, 255, 255, 40), 1.f);
        // "SDR Transceiver" branco (menor) e "MB1SDR" dourado, como "SDR Transceiver MB1" da placa do radio
        const float wa = txW(fP(), 11.f, "SDR Transceiver", 0.96f), wb = txW(fP(), 16.f, "MB1SDR", 1.08f);
        const float xa = 745.5f - (wa + 8.f + wb) * 0.5f, xb = xa + wa + 8.f;
        tx(fP(), 11.f, xa + 1.2f, 33.5f, IM_COL32(0, 0, 0, 200), "SDR Transceiver", 0.f, 0.96f);         // sombra
        tx(fP(), 11.f, xa, 32.5f, IM_COL32(0xf4, 0xf6, 0xfa, 255), "SDR Transceiver", 0.f, 0.96f);          // branco
        tx(fP(), 16.f, xb + 1.5f, 33.5f, IM_COL32(0x14, 0x0c, 0x00, 230), "MB1SDR", 0.f, 1.08f);       // sombra
        tx(fP(), 16.f, xb, 32, IM_COL32(0xe8, 0xb8, 0x4c, 255), "MB1SDR", 0.f, 1.08f);                 // dourado
        tx(fP(), 16.f, xb, 31.3f, IM_COL32(0xff, 0xe0, 0x90, 70), "MB1SDR", 0.f, 1.08f);               // brilho
    }
    // ---- a moldura da tela: aro cinza, o vidro preto e a borda da tela ----
    dl->AddRectFilled(Q(155, 51), Q(879, 521), IM_COL32(0x40, 0x40, 0x43, 255), QD(7));
    retGrad(dl, Q(158, 54), Q(876, 518), QD(6), IM_COL32(0x16, 0x16, 0x18, 255), IM_COL32(0x07, 0x07, 0x08, 255));
    dl->AddLine(Q(166, 56), Q(866, 56), IM_COL32(255, 255, 255, 26), 1.f);
    dl->AddRectFilledMultiColor(Q(158, 512), Q(876, 518), IM_COL32(0x1c, 0x1c, 0x1e, 255), IM_COL32(0x1c, 0x1c, 0x1e, 255),
                                IM_COL32(0x34, 0x34, 0x37, 255), IM_COL32(0x34, 0x34, 0x37, 255));
    // ---- a tampa como "barra de titulo": versao, minimizar / maximizar / fechar, arrastar ----
    tx(fP(), 7.f, 70, -11.5f, IM_COL32(0xc0, 0xc2, 0xc6, 255), "MB1SDR  v" RXSDR_VERSAO, 0.f, 0.95f);
    {
        HWND hw = (HWND)ImGui::GetMainViewport()->PlatformHandleRaw;
        if (!hw) hw = GetActiveWindow();
        const bool max = hw && IsZoomed(hw);
        const float bw = 30.f, bh = 16.f, gap = 5.f, by = -20.f, bx0 = 1440.f - 3 * bw - 2 * gap;
        for (int i = 0; i < 3; ++i) {
            const float x = bx0 + i * (bw + gap);
            const ImVec2 a = Q(x, by), b = Q(x + bw, by + bh);
            ImGui::SetCursorScreenPos(a);
            ImGui::PushID(9300 + i);
            const bool clicou = ImGui::InvisibleButton("##jan", ImVec2(std::max(1.f, b.x - a.x), std::max(1.f, b.y - a.y)));
            const bool sob = ImGui::IsItemHovered(), at = ImGui::IsItemActive();
            ImGui::PopID();
            ImU32 fundo = at ? IM_COL32(0x10, 0x12, 0x14, 255) : sob ? IM_COL32(0x60, 0x64, 0x6a, 255) : IM_COL32(0x30, 0x31, 0x34, 255);
            if (i == 2 && (sob || at)) fundo = at ? IM_COL32(0x90, 0x1c, 0x14, 255) : IM_COL32(0xc4, 0x2b, 0x1c, 255);
            dl->AddRectFilled(a, b, fundo, QD(3));
            dl->AddRect(a, b, IM_COL32(0x76, 0x78, 0x7e, 200), QD(3), 0, 1.f);
            const ImU32 cs = IM_COL32(0xee, 0xf0, 0xf4, 255);
            const float cx = x + bw * 0.5f, cy = by + bh * 0.5f, t = std::max(1.f, QD(1.5f));
            if (i == 0) dl->AddLine(Q(cx - 6, cy + 3), Q(cx + 6, cy + 3), cs, t);
            else if (i == 1) {
                if (max) {
                    dl->AddRect(Q(cx - 5, cy - 2), Q(cx + 3, cy + 5), cs, 0.f, 0, t);
                    dl->AddLine(Q(cx - 2, cy - 5), Q(cx + 6, cy - 5), cs, t);
                    dl->AddLine(Q(cx + 6, cy - 5), Q(cx + 6, cy + 2), cs, t);
                } else dl->AddRect(Q(cx - 6, cy - 5), Q(cx + 6, cy + 5), cs, 0.f, 0, t);
            } else {
                dl->AddLine(Q(cx - 5, cy - 5), Q(cx + 5, cy + 5), cs, t);
                dl->AddLine(Q(cx + 5, cy - 5), Q(cx - 5, cy + 5), cs, t);
            }
            if (sob && !at) ImGui::SetTooltip("%s", i == 0 ? "Minimizar" : i == 1 ? (max ? "Restaurar" : "Maximizar") : "Fechar");
            if (clicou && hw) {
                if (i == 0) ShowWindow(hw, SW_MINIMIZE);
                else if (i == 1) ShowWindow(hw, max ? SW_RESTORE : SW_MAXIMIZE);
                else PostMessageW(hw, WM_CLOSE, 0, 0);
            }
        }
        // arrastar pela tampa (fora dos botoes); duplo clique maximiza / volta
        const ImVec2 t0 = Q(0, -24), t1 = Q(1500, 0);
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
    // botao direito no painel = MENU
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##funcFt");
}

// ===========================================================================
//  MB1SDR: a coluna da esquerda (POWER, TX/TUN, MUT/MON, AGC/ATT, PA/ANT, USB,
//  PHONES, MIC 1, MIC 2)
// ===========================================================================
void Ui::mb1Esquerda()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool lig = r_.ligado();
    Receptor& R = rx_[ativo_];
    // POWER (o simbolo vermelho)
    {
        const int k = teclaMb("mPwr", 30, 37, 120, 62, lig ? "POWER: desligar o rádio (para o dongle)" : "POWER: ligar o rádio (abre o dongle)");
        if (k == 1) ligarDesligar();
        const ImU32 cr = lig ? IM_COL32(0xff, 0x3c, 0x30, 255) : IM_COL32(0x9c, 0x2a, 0x24, 255);
        const ImVec2 c = Q(75, 50.5f);
        const float rr = QD(6.2f), t = std::max(1.2f, QD(1.9f));
        if (lig) dl->AddCircleFilled(c, rr * 1.8f, IM_COL32(0xff, 0x30, 0x20, 30), 24);
        dl->PathArcTo(c, rr, rad(-55), rad(235), 28);
        dl->PathStroke(cr, 0, t);
        dl->AddLine(Q(75, 42), Q(75, 50), cr, t);
    }
    // os dois LEDs: ligado (verde) e sinal no canal / squelch aberto (BUSY)
    {
        const bool busy = lig && r_.aoVivo() && sqlAberto_ && (R.sql > -90 || smAtual_ > 0.22f);
        ledMb(49, 85, 5.2f, IM_COL32(0x40, 0xe0, 0x60, 255), lig);
        ledMb(99, 85, 5.2f, IM_COL32(0xff, 0x9a, 0x30, 255), busy);
        ImGui::SetCursorScreenPos(Q(40, 76));
        ImGui::InvisibleButton("##ledl", ImVec2(QD(70), QD(18)));
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("LEDs: o da esquerda acende com o rádio ligado;\no da direita, com sinal no canal (squelch aberto)");
    }
    const float xa0 = 30, xa1 = 70, xb0 = 80, xb1 = 120;
    // TX (num receptor: MUTE geral) e TUN (ajuste automatico do escopo)
    if (teclaTxt("mTx", "TX", xa0, 97, xa1, 123, 9.5f, sendMudo_ ? IM_COL32(0xff, 0x50, 0x40, 255) : IM_COL32(0xe0, 0x5a, 0x50, 255),
                 sendMudo_ ? "TX: (num receptor) MUTE geral LIGADO - clique para ouvir de novo" : "TX: (num receptor) silencia todo o áudio - MUTE geral",
                 nullptr, sendMudo_) == 1)
        sendMudo_ = !sendMudo_;
    {
        const int k = teclaTxt("mTun", "TUN", xb0, 97, xb1, 123, 9.5f, IM_COL32(0xee, 0x96, 0x5c, 255),
                               "TUN: ajuste automático do espectro e da cachoeira (AUTO)\nSegurar: guarda a aparência atual como padrão",
                               nullptr, autoWf_);
        if (k == 1) autoCachoeira();
        if (k == 2) padrao();
    }
    // MUT (silencia o receptor) e MON (segurar: ouve o outro receptor)
    if (teclaTxt("mMut", "MUT", xa0, 137, xa1, 163, 9.5f, M_ROT, "MUT: silencia o receptor ouvido", nullptr, R.mute) == 1) R.mute = !R.mute;
    {
        bool seg = false;
        teclaTxt("mMon", "MON", xb0, 137, xb1, 163, 9.5f, M_ROT, "MON (monitor): enquanto segura, ouve o outro receptor (VFO-B / VFO-A)", &seg, xfcVolta_ >= 0);
        if (seg && xfcVolta_ < 0) { xfcVolta_ = ativo_; ativar(1 - ativo_); }
        if (!seg && xfcVolta_ >= 0) { ativar(xfcVolta_); xfcVolta_ = -1; }
    }
    // AGC (FAST / MID / SLOW / OFF) e ATT (0 / -6 / -12 / -18 dB)
    {
        static const char* nm[4] = {"OFF", "SLOW", "MID", "FAST"};
        char d[160];
        std::snprintf(d, sizeof d, "AGC: %s (clique: FAST > MID > SLOW > OFF)", R.agcOff ? "OFF" : nm[std::clamp(R.agc, 1, 3)]);
        if (teclaTxt("mAgc", "AGC", xa0, 175, xa1, 201, 9.5f, M_ROT, d) == 1) {
            if (R.agcOff) { R.agcOff = false; R.agc = 3; }
            else if (R.agc <= 1) R.agcOff = true;
            else R.agc -= 1;
            aviso(std::string("AGC: ") + (R.agcOff ? "OFF" : nm[std::clamp(R.agc, 1, 3)]), 1.0);
        }
        std::snprintf(d, sizeof d, "ATT: atenuador (agora %d dB)\nClique: 0 > -6 > -12 > -18 dB", -6 * att_);
        if (teclaTxt("mAtt", "ATT", xb0, 175, xb1, 201, 9.5f, M_ROT, d, nullptr, att_ > 0) == 1) {
            att_ = (att_ + 1) % 4;
            char a[32]; std::snprintf(a, sizeof a, "ATT %d dB", -6 * att_); aviso(a, 1.0);
        }
    }
    // PA (pre-amplificador: P.AMP) e ANT (o aparelho: RTL-SDR / RTL-TCP / SDRplay)
    {
        char d[200];
        std::snprintf(d, sizeof d, "PA: pré-amplificador (agora %s)\nClique: OFF > AMP1 > AMP2", pamp_ == 0 ? "OFF" : pamp_ == 1 ? "AMP1" : "AMP2");
        if (teclaTxt("mPa", "PA", xa0, 213, xa1, 240, 9.5f, M_ROT, d, nullptr, pamp_ > 0) == 1) {
            pamp_ = (pamp_ + 1) % 3;
            aviso(pamp_ == 0 ? "PREAMP OFF" : pamp_ == 1 ? "PREAMP AMP1" : "PREAMP AMP2", 1.0);
        }
        if (teclaTxt("mAnt", "ANT", xb0, 213, xb1, 240, 9.5f, M_ROT,
                     "ANT: o aparelho e a antena - RTL-SDR, RTL-TCP (rede) ou SDRplay\n(abre a CONFIGURAÇÃO)") == 1)
            abrirConfig();
    }
    // o simbolo USB e a porta (clique: captura da tela)
    {
        const ImU32 b = M_ROT;
        const float t = std::max(1.f, QD(1.4f));
        dl->AddCircleFilled(Q(67, 255), QD(2.4f), b, 10);
        dl->AddLine(Q(67, 255), Q(88, 255), b, t);
        dl->AddTriangleFilled(Q(88, 252), Q(88, 258), Q(92, 255), b);
        dl->AddLine(Q(72, 255), Q(76, 250), b, t); dl->AddLine(Q(76, 250), Q(82, 250), b, t);
        dl->AddCircleFilled(Q(83, 250), QD(1.7f), b, 8);
        dl->AddLine(Q(74, 255), Q(78, 259.5f), b, t); dl->AddLine(Q(78, 259.5f), Q(84, 259.5f), b, t);
        dl->AddRectFilled(Q(83.5f, 258), Q(86.5f, 261), b);
        // a porta USB em pe: aro de metal, o buraco preto e a lingueta
        retGrad(dl, Q(50, 266), Q(72, 330), QD(2), IM_COL32(0x8c, 0x8e, 0x92, 255), IM_COL32(0x4c, 0x4e, 0x52, 255));
        dl->AddRectFilled(Q(52.5f, 268.5f), Q(69.5f, 327.5f), IM_COL32(0x06, 0x06, 0x07, 255), QD(1));
        dl->AddRectFilled(Q(55, 272), Q(61, 324), IM_COL32(0x18, 0x18, 0x1a, 255));
        for (int k = 0; k < 4; ++k) dl->AddRectFilled(Q(61, 280 + k * 11.f), Q(63.5f, 286 + k * 11.f), IM_COL32(0xb8, 0x9c, 0x54, 255));
        dl->AddRectFilled(Q(74, 266), Q(102, 330), IM_COL32(0x1e, 0x1e, 0x20, 255), QD(3));
        dl->AddRect(Q(74, 266), Q(102, 330), IM_COL32(0x3c, 0x3c, 0x40, 255), QD(3), 0, 1.f);
        ImGui::SetCursorScreenPos(Q(48, 246));
        if (ImGui::InvisibleButton("##usbMb", ImVec2(QD(56), QD(86)))) g_pedirCaptura = true;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("USB: captura da tela (PNG na pasta Capturas) - o mesmo que F12");
    }
    // os jaques: PHONES (saida de som), MIC 1 (gravar) e MIC 2 (tocar a gravacao)
    auto jaque = [&](float cx, float cy, float r) {
        const ImVec2 c = Q(cx, cy);
        dl->AddCircleFilled(ImVec2(c.x + QD(1), c.y + QD(2)), QD(r * 1.04f), IM_COL32(0, 0, 0, 140), 48);
        anelLuz(dl, c, QD(r * 0.72f), QD(r), 0.f, (float)(2 * kPi), 64, [](float a, float) {
            return cinzaN(0.06f + 0.22f * sino(a, rad(-125), rad(50)) + 0.06f * sino(a, rad(50), rad(40)));
        });
        circuloSuave(dl, c, QD(r), 64, 1.f, [](float a) { return cinzaN(0.12f + 0.30f * sino(a, rad(-125), rad(50)), 220); });
        dl->AddCircleFilled(c, QD(r * 0.72f), IM_COL32(0x0a, 0x0a, 0x0b, 255), 48);
        anelLuz(dl, c, QD(r * 0.36f), QD(r * 0.50f), 0.f, (float)(2 * kPi), 48, [](float a, float) { return cinzaN(0.10f + 0.30f * sino(a, rad(60), rad(50))); });
        dl->AddCircleFilled(c, QD(r * 0.36f), IM_COL32(0x02, 0x02, 0x02, 255), 32);
    };
    tx(fP(), 11.f, 78, 345, M_ROT, "PHONES", 0.5f, 0.95f);
    jaque(78, 384, 19);
    tx(fP(), 11.f, 78, 427, M_ROT, "MIC 1", 0.5f, 0.95f);
    jaque(78, 467, 19);
    tx(fP(), 11.f, 80, 508, M_ROT, "MIC 2", 0.5f, 0.95f);
    {   // MIC 2: o conector redondo grande (8 pinos)
        const ImVec2 c = Q(79, 560);
        dl->AddCircleFilled(ImVec2(c.x + QD(1.5f), c.y + QD(2.5f)), QD(31), IM_COL32(0, 0, 0, 150), 64);
        anelLuz(dl, c, QD(24), QD(30), 0.f, (float)(2 * kPi), 96, [](float a, float) {
            return cinzaN(0.30f + 0.45f * sino(a, rad(-135), rad(55)) + 0.25f * sino(a, rad(45), rad(40)));
        });
        circuloSuave(dl, c, QD(30), 96, 1.f, [](float a) { return cinzaN(0.2f + 0.3f * sino(a, rad(-135), rad(55)), 220); });
        dl->AddCircleFilled(c, QD(24), IM_COL32(0x0c, 0x0c, 0x0d, 255), 64);
        anelLuz(dl, c, QD(20), QD(24), 0.f, (float)(2 * kPi), 64, [](float a, float) { return cinzaN(0.05f + 0.18f * sino(a, rad(60), rad(50))); });
        dl->AddRectFilled(Q(75, 538), Q(83, 543), IM_COL32(0x5a, 0x5c, 0x60, 255), QD(1.5f));
        for (int i = 0; i < 8; ++i) {
            const ImVec2 d = dirAng((float)(-kPi / 2 + (i + 0.5) * 2 * kPi / 8.6));
            const ImVec2 p = Q(79 + d.x * 13.f, 560 + d.y * 13.f);
            dl->AddCircleFilled(p, QD(2.6f), IM_COL32(0x02, 0x02, 0x02, 255), 12);
            dl->AddCircleFilled(p, QD(1.8f), IM_COL32(0xd0, 0xc8, 0xb0, 255), 12);
        }
    }
    {   // cliques nos jaques
        ImGui::SetCursorScreenPos(Q(56, 362));
        if (ImGui::InvisibleButton("##phMb", ImVec2(QD(44), QD(44)))) { saidas_ = SomNativo::listarSaidas(); ImGui::OpenPopup("##saidaIc"); }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("PHONES: escolher a saída de som (placa / fone)");
        const bool grav = r_.som().gravando();
        ImGui::SetCursorScreenPos(Q(56, 445));
        if (ImGui::InvisibleButton("##m1Mb", ImVec2(QD(44), QD(44)))) alternarGravacao();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", grav ? "MIC 1: parar a gravação do áudio" : "MIC 1: gravar o áudio (pasta Gravacoes) - o LED REC acende");
        ImGui::SetCursorScreenPos(Q(47, 528));
        if (ImGui::InvisibleButton("##m2Mb", ImVec2(QD(64), QD(64)))) tocarUltimaGravacao();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("MIC 2: tocar a última gravação");
        if (grav && std::fmod(agora(), 1.0) < 0.6) dl->AddCircleFilled(Q(78, 467), QD(4), IM_COL32(0xff, 0x30, 0x28, 255), 16);
    }
}

// ===========================================================================
//  MB1SDR: a tela (icones, S-meter, frequencias, indicadores, escopo e as
//  teclas de toque)
// ===========================================================================
void Ui::mb1Tela()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool lig = r_.ligado();
    Receptor& R = rx_[ativo_];
    Receptor& O = rx_[1 - ativo_];
    const float X0 = 176, Y0 = 79, X1 = 858, Y1 = 504;
    char b[96];
    // o fundo da tela e a borda fina clara
    dl->AddRectFilled(Q(X0 - 1, Y0 - 1), Q(X1 + 1, Y1 + 1), IM_COL32(0x80, 0x88, 0x98, 255));
    dl->AddRectFilledMultiColor(Q(X0, Y0), Q(X1, 238), IM_COL32(0x0c, 0x0f, 0x15, 255), IM_COL32(0x0c, 0x0f, 0x15, 255),
                                IM_COL32(0x10, 0x14, 0x1b, 255), IM_COL32(0x10, 0x14, 0x1b, 255));
    dl->AddRectFilled(Q(X0, 238), Q(X1, Y1), IM_COL32(0x0b, 0x0d, 0x12, 255));
    dl->PushClipRect(Q(X0, Y0), Q(X1, Y1), true);
    const ImU32 W = lig ? M_BRANCO : IM_COL32(0x80, 0x84, 0x8c, 255);
    const ImU32 CI = lig ? M_CIANO : IM_COL32(0x40, 0x60, 0x78, 255);
    // ---- a fileira de icones: MENU, tela cheia, configuracao e POWER ----
    {
        const float t = std::max(1.f, QD(1.4f));
        auto icone = [&](const char* id, float x0, float x1, const char* dica) {
            ImGui::SetCursorScreenPos(Q(x0, 81));
            const bool c = ImGui::InvisibleButton(id, ImVec2(QD(x1 - x0), QD(15)));
            if (ImGui::IsItemHovered()) { dl->AddRectFilled(Q(x0, 81), Q(x1, 96), IM_COL32(255, 255, 255, 30), QD(2)); ImGui::SetTooltip("%s", dica); }
            return c;
        };
        // 4 quadradinhos
        for (int i = 0; i < 4; ++i) {
            const float x = 179 + (i % 2) * 6.f, y = 84 + (i / 2) * 6.f;
            dl->AddRectFilled(Q(x, y), Q(x + 4, y + 4), W, QD(0.6f));
        }
        if (icone("##icMenu", 176, 194, "MENU: funções e configuração")) ImGui::OpenPopup("##funcFt");
        // monitor
        dl->AddRect(Q(215, 83.5f), Q(229, 93), W, QD(1), 0, t);
        dl->AddLine(Q(222, 93), Q(222, 96), W, t); dl->AddLine(Q(218, 96), Q(226, 96), W, t);
        if (icone("##icTela", 211, 233, "Escopo na tela inteira (ESC volta)")) scp_ = 2;
        // ferramentas
        dl->AddLine(Q(252, 83), Q(264, 95), W, t); dl->AddLine(Q(264, 83), Q(252, 95), W, t);
        dl->AddCircle(Q(253, 84), QD(2.2f), W, 10, t);
        if (icone("##icCfg", 247, 269, "CONFIGURAÇÃO: aparelho, som, S-meter, idioma")) abrirConfig();
        // POWER vermelho
        dl->AddRectFilled(Q(287, 82), Q(320, 97), lig ? IM_COL32(0xd8, 0x30, 0x30, 255) : IM_COL32(0x70, 0x24, 0x24, 255), QD(1.5f));
        dl->PathArcTo(Q(303.5f, 89.8f), QD(4.2f), rad(-55), rad(235), 20);
        dl->PathStroke(IM_COL32(0xff, 0xf0, 0xf0, 255), 0, t);
        dl->AddLine(Q(303.5f, 84), Q(303.5f, 89.5f), IM_COL32(0xff, 0xf0, 0xf0, 255), t);
        if (icone("##icPwr", 287, 320, lig ? "Desligar o rádio" : "Ligar o rádio")) ligarDesligar();
    }
    if (!expand_) {   // MB1SDR: com EXPAND (F1) o escopo ocupa esta parte
    // ---- ATU: (num receptor) o aparelho ----
    {
        const char* dev = !r_.temDispositivo() ? "--" : r_.tipo() == "rtltcp" ? "RTL-TCP" : r_.tipo() == "sdrplay" ? "SDRplay" : "RTL-SDR";
        std::snprintf(b, sizeof b, "SDR: %s", dev);
        txT(fL(), 7.2f, 180, 107, CI, b);
        ImGui::SetCursorScreenPos(Q(178, 100));
        if (ImGui::InvisibleButton("##devMb", ImVec2(QD(70), QD(14)))) abrirConfig();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("O aparelho em uso (clique: CONFIGURAÇÃO)\nTaxa: %.3f Msps", r_.taxa() / 1e6);
    }
    mb1Medidor();
    // ---- PA / LOCK / IQ / PLAY (acendem quando ligados) ----
    {
        txT(fL(), 6.f, 414, 107, pamp_ > 0 ? W : M_APAG, "PA", 0.5f);
        txT(fL(), 6.f, 414, 127, R.lock ? IM_COL32(0xff, 0x70, 0x50, 255) : M_APAG, "LOCK", 0.5f);
        const bool dc = Config::instance().dcRemove();
        txT(fL(), 6.f, 408, 182, dc ? W : M_APAG, "IQ", 0.5f);
        dl->AddCircleFilled(Q(421, 182), QD(3.2f), dc ? M_VERDE : M_APAG, 12);
        dl->AddTriangleFilled(Q(405, 193), Q(405, 201), Q(412, 197), M_APAG);
        dl->AddCircleFilled(Q(421, 197), QD(3.2f), r_.som().gravando() ? IM_COL32(0xff, 0x40, 0x30, 255) : M_APAG, 12);
        ImGui::SetCursorScreenPos(Q(400, 176));
        if (ImGui::InvisibleButton("##iqMb", ImVec2(QD(28), QD(12)))) { auto& c = Config::instance(); c.set("dc_remove", !c.dcRemove()); }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("IQ: tirar o risco do DC do centro do escopo (clique liga / desliga)");
    }
    // ---- a parte de cima, a direita: RX / TX, PRMP, data e hora ----
    {
        const uint64_t f = r_.vfo();
        const char* faixa = f >= 300000000ULL ? "UHF" : f >= 30000000ULL ? "VHF" : "HF";
        std::snprintf(b, sizeof b, "%s RX", faixa);
        txT(fL(), 7.f, 447, 110, CI, b);
        antenaMb(487, 110, 8.f, CI);
        txT(fL(), 7.f, 495, 110, CI, ativo_ == 0 ? "A1" : "B1");
        std::snprintf(b, sizeof b, "%s TX", faixa);
        txT(fL(), 7.f, 553, 110, CI, b);
        antenaMb(595, 110, 8.f, CI);
        txT(fL(), 7.f, 607, 110, CI, txVfo_ == 0 ? "A1" : "B1");
        if (att_ > 0) std::snprintf(b, sizeof b, "ATT: -%d dB", 6 * att_);
        else std::snprintf(b, sizeof b, "PRMP: %d dB", pamp_ * 10);
        txT(fL(), 7.6f, 647, 110, att_ > 0 ? M_LARANJA : CI, b);
        ImGui::SetCursorScreenPos(Q(645, 103));
        if (ImGui::InvisibleButton("##prmpMb", ImVec2(QD(70), QD(14)))) pamp_ = (pamp_ + 1) % 3;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("PRMP: pré-amplificador (clique troca; ATT na tecla ATT)");
        SYSTEMTIME lt, ut;
        GetLocalTime(&lt); GetSystemTime(&ut);
        std::snprintf(b, sizeof b, "%02d/%02d/%04d", lt.wDay, lt.wMonth, lt.wYear);
        txT(fL(), 8.2f, 857, 92, W, b, 1.f);
        std::snprintf(b, sizeof b, "%02d:%02d UTC  %02d:%02d LOC", ut.wHour, ut.wMinute, lt.wHour, lt.wMinute);
        txT(fL(), 7.8f, 857, 110, W, b, 1.f);
    }
    // ---- RX / TX / TUNE, AGC e o modo ----
    {
        auto caixa = [&](const char* id, float x0, float x1, const char* t, bool aceso, ImU32 corAceso, const char* dica) {
            const ImVec2 a = Q(x0, 122), c = Q(x1, 141);
            if (aceso) retGrad(dl, a, c, QD(3), mistura(corAceso, IM_COL32(255, 255, 255, 255), 0.25f), corAceso);
            dl->AddRect(a, c, aceso ? mistura(corAceso, IM_COL32(255, 255, 255, 255), 0.4f) : IM_COL32(0x6c, 0x7c, 0x96, 255), QD(3), 0, std::max(1.f, QD(1.4f)));
            txT(fL(), 7.f, (x0 + x1) * 0.5f, 131.5f, aceso ? IM_COL32(0x10, 0x30, 0x14, 255) : IM_COL32(0x9c, 0xb0, 0xcc, 255), t, 0.5f);
            ImGui::SetCursorScreenPos(a);
            const bool cl = ImGui::InvisibleButton(id, ImVec2(std::max(1.f, c.x - a.x), std::max(1.f, c.y - a.y)));
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", dica);
            return cl;
        };
        if (caixa("##rxMb", 447, 480, "RX", lig, IM_COL32(0x3c, 0xd8, 0x50, 255), lig ? "RX: recebendo (clique desliga o rádio)" : "RX: clique liga o rádio"))
            ligarDesligar();
        if (caixa("##txMb", 487, 519, "TX", sendMudo_, IM_COL32(0xe8, 0x40, 0x34, 255), "TX: (num receptor) MUTE geral - clique liga / desliga"))
            sendMudo_ = !sendMudo_;
        if (caixa("##tuMb", 527, 572, "TUNE", autoWf_, IM_COL32(0xe8, 0xa0, 0x30, 255), "TUNE: ajuste automático do espectro e da cachoeira (AUTO)"))
            autoCachoeira();
        static const char* nm[4] = {"OFF", "SLOW", "MID", "FAST"};
        std::snprintf(b, sizeof b, "AGC: %s", R.agcOff ? "OFF" : nm[std::clamp(R.agc, 1, 3)]);
        txT(fL(), 9.8f, 580, 131, W, b);
        ImGui::SetCursorScreenPos(Q(578, 123));
        if (ImGui::InvisibleButton("##agcMb", ImVec2(QD(70), QD(17)))) {
            if (R.agcOff) { R.agcOff = false; R.agc = 3; } else if (R.agc <= 1) R.agcOff = true; else R.agc -= 1;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("AGC: clique troca FAST > MID > SLOW > OFF");
        const std::string md = r_.modo();
        const std::string nmMd = md == "NFM" ? "FM" : md;
        {   // caixa do modo (clique: menu dos modos)
            const bool sobreMd = ImGui::IsMouseHoveringRect(Q(656, 122), Q(708, 141), false);
            dl->AddRectFilled(Q(656, 122), Q(708, 141), sobreMd ? IM_COL32(0x20, 0x50, 0x2c, 255) : IM_COL32(0x10, 0x2a, 0x18, 255), QD(3));
            dl->AddRect(Q(656, 122), Q(708, 141), lig ? IM_COL32(0x54, 0xc8, 0x6a, 255) : IM_COL32(0x30, 0x60, 0x3c, 255), QD(3), 0, std::max(1.f, QD(1.3f)));
            dl->AddTriangleFilled(Q(699, 129), Q(705, 129), Q(702, 134), lig ? M_VERDE : IM_COL32(0x30, 0x70, 0x40, 255));
        }
        txT(fL(), 10.f, 677, 131.5f, lig ? M_VERDE : IM_COL32(0x30, 0x70, 0x40, 255), nmMd.c_str(), 0.5f, 1.f, 38.f);
        ImGui::SetCursorScreenPos(Q(656, 122));
        if (ImGui::InvisibleButton("##mdMb", ImVec2(QD(52), QD(19)))) ImGui::OpenPopup("##modosIc");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("MODE: clique para escolher LSB, USB, CW, AM, FM, WFM e os digitais\n(também na tecla de toque MODE / F5)");
        txT(fL(), 8.f, 740, 131, M_APAG, "BREAK IN");
        txT(fL(), 8.f, 830, 131, M_APAG, "MON", 1.f);
    }
    // ---- as frequencias: a do receptor ouvido (grande) e a do outro ----
    {
        const ImU32 apag = IM_COL32(0x2c, 0x32, 0x3a, 255);
        static const float xs[6] = {460, 481, 503, 528, 550, 572};
        static const float xp[3] = {596, 610, 624};
        static const float pontos[2] = {524.5f, 591.5f};
        std::vector<DigMb> caixas;
        if (entrando_) {
            const std::string t = (entrada_.empty() ? std::string("_") : entrada_) + " kHz";
            txT(fL(), 20.f, 636, 165, P_AMBAR, t.c_str(), 1.f);
        } else {
            freqMb(dl, r_.vfo() % 1000000000ULL, xs, 6, 18.5f, 28.f, 151.f, 4.0f, xp, 3, 12.f, 19.5f, 160.f, 3.0f, pontos,
                   lig ? M_BRANCO : IM_COL32(0x60, 0x64, 0x6a, 255), apag, &caixas);
        }
        static const float xs2[6] = {680, 697, 714, 740, 757, 774};
        static const float xp2[3] = {797, 809, 821};
        static const float pontos2[2] = {735.5f, 792.f};
        std::vector<DigMb> caixas2;
        freqMb(dl, O.f % 1000000000ULL, xs2, 6, 14.5f, 23.f, 157.f, 3.3f, xp2, 3, 10.f, 15.f, 165.f, 2.6f, pontos2,
               lig ? IM_COL32(0xe4, 0xe8, 0xee, 255) : IM_COL32(0x5a, 0x5e, 0x64, 255), apag, &caixas2);
        // roda do mouse em cima de um digito: muda aquele digito
        auto roda = [&](const std::vector<DigMb>& cx, float yt, float yb, int rx) {
            if (!ImGui::IsWindowHovered()) return;
            ImGuiIO& io = ImGui::GetIO();
            for (const auto& d : cx) {
                const ImVec2 a = Q(d.x0, yt), c = Q(d.x1, yb);
                if (ImGui::IsMouseHoveringRect(a, c, false)) {
                    dl->AddLine(ImVec2(a.x, c.y + 1), ImVec2(c.x, c.y + 1), IM_COL32(0x40, 0xc8, 0xff, 255), std::max(1.5f, QD(1.6f)));
                    if (io.MouseWheel != 0) {
                        int n = (int)std::lround(io.MouseWheel);
                        if (n == 0) n = io.MouseWheel > 0 ? 1 : -1;
                        mudarFreqRx(rx, (int64_t)n * (int64_t)d.peso);
                        io.MouseWheel = 0;
                    }
                }
            }
        };
        roda(caixas, 149, 181, ativo_);
        roda(caixas2, 155, 182, 1 - ativo_);
        ImGui::SetCursorScreenPos(Q(455, 147));
        if (ImGui::InvisibleButton("##fqMb", ImVec2(QD(185), QD(36)))) { entrando_ = !entrando_; entrada_.clear(); }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("VFO-%s (o que se ouve): %s\nClique: digitar a frequência (kHz, Enter confirma, Esc cancela)\nRoda do mouse em cima de um dígito: muda aquele dígito",
                              ativo_ == 0 ? "A" : "B", fmtFreq(r_.vfo()).c_str());
        ImGui::SetCursorScreenPos(Q(676, 153));
        if (ImGui::InvisibleButton("##fq2Mb", ImVec2(QD(160), QD(30)))) ativar(1 - ativo_);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("VFO-%s (o outro receptor): %s\nClique: passar a ouvir este\nRoda do mouse em cima de um dígito: muda aquele dígito",
                              ativo_ == 0 ? "B" : "A", fmtFreq(O.f).c_str());
    }
    // ---- os indicadores (acendem quando a funcao esta ligada; clique liga / desliga) ----
    {
        auto ind = [&](const char* id, float cx, float y, const char* t, bool on, ImU32 cor, const char* dica) {
            txT(fL(), 7.2f, cx, y, on ? cor : M_APAG, t, 0.5f);
            const float w = txW(fL(), 7.2f, t) + 4.f;
            ImGui::SetCursorScreenPos(Q(cx - w * 0.5f, y - 7));
            const bool c = ImGui::InvisibleButton(id, ImVec2(std::max(1.f, QD(w)), QD(14)));
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", dica);
            return c;
        };
        const ImU32 w = W;
        const bool split = txVfo_ != ativo_;
        if (ind("##iRit", 455, 186, "RIT", R.ritOn, w, "RIT: deslocamento da escuta (gire o MULTI 3)\nClique liga / desliga")) R.ritOn = !R.ritOn;
        if (ind("##iXit", 479, 186, "XIT", O.ritOn, w, "XIT: (num receptor) deslocamento do outro VFO\nClique liga / desliga")) O.ritOn = !O.ritOn;
        if (ind("##iComp", 504, 186, "COMP", proc_, w, "COMP: compressor do áudio (PROC)")) proc_ = !proc_;
        if (ind("##iSql", 549, 186, "SQL", R.sql > -90.f, w, "SQL: squelch ligado (nível no MULTI 1 - anel)\nClique: abre / fecha")) {
            if (R.sql > -90.f) { sqlSalvo_[ativo_] = R.sql; R.sql = -90.f; }
            else R.sql = sqlSalvo_[ativo_] > -90.f ? sqlSalvo_[ativo_] : -60.f;
            sql_ = R.sql; sqlAplicado_ = R.sql; sqlAuto_[ativo_] = false;
        }
        if (ind("##iVox", 586, 186, "VOX", sqlAuto_[ativo_] || sqlMedindo_, w, "VOX: (num receptor) squelch automático\nClique: mede o ruído e fecha o squelch logo acima")) {
            if (sqlMedindo_) { sqlMedindo_ = false; R.sql = -90.f; sql_ = -90.f; sqlAplicado_ = -90.f; sqlAuto_[ativo_] = false; }
            else if (sqlAuto_[ativo_]) { R.sql = -90.f; sql_ = -90.f; sqlAplicado_ = -90.f; sqlAuto_[ativo_] = false; aviso("Squelch aberto"); }
            else if (r_.aoVivo()) { sqlMedindo_ = true; sqlAmostras_.clear(); sqlIni_ = agora(); sqlUlt_ = 0; }
        }
        if (ind("##iMute", 625, 186, "MUTE", R.mute || sendMudo_, IM_COL32(0xff, 0x60, 0x50, 255), "MUTE: receptor silenciado")) R.mute = !R.mute;
        if (ind("##iReq", 660, 186, "R.EQ", rxeq_ != 0, w, "R.EQ: equalização do áudio (clique troca)")) {
            static const float tons[5] = {50, 68, 85, 32, 15};
            rxeq_ = (rxeq_ + 1) % 5; tom_ = tons[rxeq_];
            static const char* nm[5] = {"R.EQ OFF", "R.EQ agudo", "R.EQ mais agudo", "R.EQ grave", "R.EQ mais grave"};
            aviso(nm[rxeq_], 1.0);
        }
        if (ind("##iTeq", 697, 186, "T.EQ", sleepAte_ > 0, w, "T.EQ: (num receptor) TIMER - desligar o rádio depois de um tempo")) ImGui::OpenPopup("##timerIc");
        if (ind("##iSub", 755, 186, "Sub RX", dual_, w, "Sub RX: o escopo do VFO-B ao lado (tecla SUB / RX2)")) dual_ = !dual_;
        if (ind("##iSplit", 812, 186, "SPLIT", split, w, "SPLIT: um VFO para ouvir e o outro marcado como TX")) txVfo_ = split ? ativo_ : 1 - ativo_;
        if (ind("##iBin", 455, 203, "BIN", R.bc, w, "BIN: (num receptor mono) BC - cancelador de vários apitos")) R.bc = !R.bc;
        if (ind("##iNr", 479, 203, "NR", R.nr2, w, "NR: redutor de ruído espectral")) R.nr2 = !R.nr2;
        if (ind("##iAnf", 506, 203, "ANF", R.anch, w, "ANF: notch automático (tira os apitos sozinho)")) R.anch = !R.anch;
        if (ind("##iApf", 550, 203, "APF", R.apf, w, "APF: filtro de pico de áudio para CW (centro no tom do CW)")) R.apf = !R.apf;
        if (ind("##iDse", 586, 203, "DSE", R.nr1, w, "DSE: (aqui) NR1 - o redutor de ruído simples")) R.nr1 = !R.nr1;
        if (ind("##iNb1", 619, 203, "NB1", R.nb1, w, "NB1: apagador de pulsos (no áudio)")) R.nb1 = !R.nb1;
        if (ind("##iNb2", 658, 203, "NB2", R.nb2, w, "NB2: apagador de pulsos no IQ (antes do demodulador)")) R.nb2 = !R.nb2;
        if (ind("##iNf", 697, 203, "NF", R.nch, w, "NF: notch manual (TNF - posição no MULTI 1 com SHIFT)")) R.nch = !R.nch;
        ind("##iCtcss", 756, 203, "CTCSS", false, w, "CTCSS: (não usado no receptor)");
        ind("##iSter", 813, 203, "STEREO", r_.modo() == "WFM", w, "STEREO: acende no FM largo (WFM)");
    }
    // ---- a linha de informacoes ----
    {
        if (passo_ >= 1000) std::snprintf(b, sizeof b, "STEP: %s", fmtPasso(passo_).c_str()); else std::snprintf(b, sizeof b, "STEP: %d Hz", passo_);
        txT(fL(), 7.6f, 179, 227, CI, b);
        ImGui::SetCursorScreenPos(Q(178, 220));
        if (ImGui::InvisibleButton("##stepMb", ImVec2(QD(78), QD(14)))) ImGui::OpenPopup("##passoMb");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("STEP: passo do FD / FU e da roda do mouse no escopo (clique escolhe)");
        const int g = grupoModo(r_.modo());
        const float bw = (float)r_.banda();
        const char* lado = g == 0 ? (r_.modo() == "USB" ? "U" : "L") : g == 1 ? "U" : "";
        if (bw >= 1000) std::snprintf(b, sizeof b, "FLTR %s%s %.1f kHz", lado, *lado ? ":" : ":", bw / 1000.f);
        else std::snprintf(b, sizeof b, "FLTR %s%s %.0f Hz", lado, *lado ? ":" : ":", bw);
        txT(fL(), 7.6f, 264, 227, CI, b);
        ImGui::SetCursorScreenPos(Q(262, 220));
        if (ImGui::InvisibleButton("##fltMb", ImVec2(QD(90), QD(14)))) ImGui::OpenPopup("##bwIc");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("FLTR: largura do filtro (clique escolhe; o MULTI 2 também ajusta)");
        std::snprintf(b, sizeof b, "RF: %d dB", (int)std::lround(49.6f * R.rf / 100.f));
        txT(fL(), 7.6f, 359, 227, CI, b);
        ImGui::SetCursorScreenPos(Q(357, 220));
        ImGui::InvisibleButton("##rfMb", ImVec2(QD(56), QD(14)));
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("RF: ganho do dongle (roda do mouse aqui, ou o anel do AF/RF)");
            if (ImGui::GetIO().MouseWheel != 0) { R.rf = std::clamp(R.rf + ImGui::GetIO().MouseWheel * 2.f, 0.f, 100.f); ImGui::GetIO().MouseWheel = 0; }
        }
        const float volDb = R.af <= 0.5f ? -99.f : 40.f * std::log10(R.af / 100.f);
        if (volDb <= -99.f) std::snprintf(b, sizeof b, "VOL: OFF"); else std::snprintf(b, sizeof b, "VOL: %d dB", (int)std::lround(volDb));
        txT(fL(), 7.6f, 427, 227, CI, b);
        ImGui::SetCursorScreenPos(Q(425, 220));
        ImGui::InvisibleButton("##volMb", ImVec2(QD(68), QD(14)));
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("VOL: volume (roda do mouse aqui, ou o miolo do AF/RF)");
            if (ImGui::GetIO().MouseWheel != 0) { R.af = std::clamp(R.af + ImGui::GetIO().MouseWheel * 2.f, 0.f, 100.f); ImGui::GetIO().MouseWheel = 0; }
        }
        std::snprintf(b, sizeof b, "CPU: %d %%", (int)std::lround(cpu_));
        txT(fL(), 7.6f, 499, 227, lig ? M_ROSA : IM_COL32(0x80, 0x40, 0x48, 255), b);
        const double sr = r_.taxa();
        std::snprintf(b, sizeof b, "SR: %.2f M", sr / 1e6);
        txT(fL(), 7.6f, 578, 227, lig ? IM_COL32(0xff, 0x90, 0x98, 255) : IM_COL32(0x80, 0x40, 0x48, 255), b);
        ImGui::SetCursorScreenPos(Q(576, 220));
        if (ImGui::InvisibleButton("##srMb", ImVec2(QD(66), QD(14)))) ImGui::OpenPopup("##spanMb");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("SR: a taxa de amostragem = a largura do escopo (0,25 a 3,2 MSPS)\nClique escolhe (o mesmo que a tecla SPAN / F6)");
        if (R.sql > -90.f) std::snprintf(b, sizeof b, "SQL: %d dB", (int)std::lround(R.sql)); else std::snprintf(b, sizeof b, "SQL: OFF");
        txT(fL(), 7.6f, 660, 227, CI, b);
        if (grupoModo(r_.modo()) == 1) std::snprintf(b, sizeof b, "CW: %d Hz", (int)std::lround(cwPitch_));
        else std::snprintf(b, sizeof b, "S: %s", lig ? leituraS(smAtual_).c_str() : "--");
        txT(fL(), 7.6f, 745, 227, lig ? M_AMAR : IM_COL32(0x70, 0x6c, 0x30, 255), b);
    }
    } else {
        // EXPAND: so uma linha com o essencial, embaixo dos icones
        const std::string md = r_.modo();
        std::snprintf(b, sizeof b, "VFO-%s  %s  %s", ativo_ == 0 ? "A" : "B", fmtFreqLcd(r_.vfo()).c_str(), (md == "NFM" ? std::string("FM") : md).c_str());
        txT(fL(), 9.f, 340, 89, lig ? M_BRANCO : IM_COL32(0x80, 0x84, 0x8c, 255), b);
        const float bw = (float)r_.banda();
        if (bw >= 1000) std::snprintf(b, sizeof b, "FLTR %.1f kHz   SR %.2f M   S: %s", bw / 1000.f, r_.taxa() / 1e6, lig ? leituraS(smAtual_).c_str() : "--");
        else std::snprintf(b, sizeof b, "FLTR %.0f Hz   SR %.2f M   S: %s", bw, r_.taxa() / 1e6, lig ? leituraS(smAtual_).c_str() : "--");
        txT(fL(), 7.6f, 855, 89, CI, b, 1.f);
        ImGui::SetCursorScreenPos(Q(338, 80));
        if (ImGui::InvisibleButton("##fqExp", ImVec2(QD(200), QD(18)))) { entrando_ = !entrando_; entrada_.clear(); }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Clique: digitar a frequência (kHz, Enter)\nF1 (EXPAND): volta a tela normal");
        if (entrando_) {
            const std::string tt = (entrada_.empty() ? std::string("_") : entrada_) + " kHz";
            dl->AddRectFilled(Q(560, 82), Q(700, 97), IM_COL32(0, 0, 0, 230), QD(2));
            txT(fL(), 9.f, 630, 89.5f, P_AMBAR, tt.c_str(), 0.5f);
        }
    }
    // ---- o escopo: espectro, regua e cachoeira (EXPAND: maior, ate embaixo dos icones) ----
    if (expand_) mb1Escopo(X0, 99, X1, 472, reguaE_);
    else mb1Escopo(X0, 240, X1, 472, reguaN_);
    // ---- as teclas de toque (as mesmas do F1..F10) ----
    {
        dl->AddRectFilledMultiColor(Q(X0, 473), Q(X1, Y1), IM_COL32(0x10, 0x12, 0x16, 255), IM_COL32(0x10, 0x12, 0x16, 255),
                                    IM_COL32(0x06, 0x07, 0x09, 255), IM_COL32(0x06, 0x07, 0x09, 255));
        for (int i = 0; i < 10; ++i) {
            const float cx = 205.f + 68.6f * i;
            const char* t = nomeSoftMb(i);
            const ImU32 ct = shift_ ? IM_COL32(0xff, 0xb0, 0x60, 255) : (i == 0 && expand_) ? P_AMBAR : W;
            if (!shift_ && i == 8) {          // alto-falante (MUTE)
                const ImU32 c = R.mute ? IM_COL32(0xff, 0x60, 0x50, 255) : W;
                dl->AddRectFilled(Q(cx - 7, 485), Q(cx - 3, 492), c);
                dl->AddTriangleFilled(Q(cx - 3, 485), Q(cx + 3, 480), Q(cx + 3, 497), c);
                dl->AddTriangleFilled(Q(cx - 3, 485), Q(cx + 3, 497), Q(cx - 3, 492), c);
                if (R.mute) { dl->AddLine(Q(cx + 5, 484), Q(cx + 11, 494), c, std::max(1.f, QD(1.5f))); dl->AddLine(Q(cx + 11, 484), Q(cx + 5, 494), c, std::max(1.f, QD(1.5f))); }
            } else if (!shift_ && i == 9) {   // teclado
                const ImU32 c = entrando_ ? P_AMBAR : W;
                dl->AddRectFilled(Q(cx - 11, 482), Q(cx + 11, 496), c, QD(1.5f));
                for (int rr = 0; rr < 3; ++rr)
                    for (int cc = 0; cc < 5; ++cc)
                        dl->AddRectFilled(Q(cx - 9.5f + cc * 4.f, 483.5f + rr * 4.f), Q(cx - 7.f + cc * 4.f, 485.5f + rr * 4.f), IM_COL32(0x10, 0x12, 0x16, 255));
            } else txT(fL(), 7.6f, cx, 488.5f, ct, t, 0.5f, 1.f, 62.f);
            ImGui::SetCursorScreenPos(Q(cx - 33, 475));
            char id[16]; std::snprintf(id, sizeof id, "##sk%d", i);
            ImGui::InvisibleButton(id, ImVec2(QD(66), QD(28)));
            const ImGuiID iid = ImGui::GetItemID();
            if (ImGui::IsItemHovered()) {
                dl->AddRectFilled(Q(cx - 33, 475), Q(cx + 33, 503), IM_COL32(255, 255, 255, 22), QD(3));
                if (!ImGui::IsItemActive()) ImGui::SetTooltip("%s", dicaSoftMb(i));
            }
            if (ImGui::IsItemActivated()) longoFeito()[iid] = 0;
            if (ImGui::IsItemActive() && ImGui::GetIO().MouseDownDuration[0] > 0.6f && longoFeito()[iid] == 0) { longoFeito()[iid] = 1; softMb(i, true); }
            if (ImGui::IsItemDeactivated() && ImGui::IsItemHovered() && longoFeito()[iid] == 0) softMb(i, false);
        }
    }
    dl->PopClipRect();
    if (!lig) {
        dl->AddRectFilled(Q(380, 300), Q(660, 330), IM_COL32(0, 0, 0, 215), QD(5));
        tx(fP(), 8.5f, 520, 315, P_AMBAR, "Aperte POWER (em cima, à esquerda) para ligar", 0.5f);
    }
    if (dim_ > 0) dl->AddRectFilled(Q(X0, Y0), Q(X1, Y1), IM_COL32(0, 0, 0, dim_ * 45));
}

// O S-meter da tela do MB1: duas escalas em arco (S e, embaixo, a de PWR/SWR/MIC)
// com o ponteiro verde que gira em volta de (300, 291) - o centro dos arcos.
void Ui::mb1Medidor()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool lig = r_.ligado();
    const float cx = 300.f, cy = 291.f, R1 = 163.f, R2 = 131.f;
    const ImU32 cw = lig ? IM_COL32(0xdc, 0xf6, 0xff, 255) : IM_COL32(0x70, 0x78, 0x80, 255);
    const ImU32 cv = lig ? IM_COL32(0xff, 0x4c, 0x66, 255) : IM_COL32(0x7c, 0x34, 0x3c, 255);
    const ImU32 cz = lig ? IM_COL32(0x44, 0xb6, 0xf2, 255) : IM_COL32(0x2c, 0x50, 0x70, 255);
    // S (unidades 0..9) e dB acima do S9 -> angulo (medidos na foto)
    auto angS = [](float s) { return -127.2f + (s - 1.f) * 5.47f; };
    auto angD = [](float d) {
        if (d <= 10.f) return -83.5f + 5.4f * d / 10.f;
        if (d <= 30.f) return -78.1f + 4.75f * (d - 10.f) / 10.f;
        return -68.6f + 4.97f * (std::min(d, 66.f) - 30.f) / 10.f;
    };
    auto P = [&](float a, float r) { return Q(polar(cx, cy, a, r).x, polar(cx, cy, a, r).y); };
    auto arco = [&](float r, float a0, float a1, ImU32 c0, ImU32 c1, float esp) {
        const int n = std::max(4, (int)(std::fabs(a1 - a0) / 1.2f));
        for (int k = 0; k < n; ++k) {
            const float u0 = (float)k / n, u1 = (float)(k + 1) / n;
            dl->AddLine(P(a0 + (a1 - a0) * u0, r), P(a0 + (a1 - a0) * u1, r), mistura(c0, c1, (u0 + u1) * 0.5f), std::max(1.f, QD(esp)));
        }
    };
    auto marca = [&](float r, float a, float l0, float l1, ImU32 c, float esp) { dl->AddLine(P(a, r + l0), P(a, r + l1), c, std::max(1.f, QD(esp))); };
    // ---- a escala S (branca ate o 9, rosa / vermelha depois) ----
    arco(R1, angS(1.f), angS(9.f), cw, cw, 4.6f);
    arco(R1, angS(9.f), angD(58.f), mistura(cw, cv, 0.45f), cv, 4.6f);
    for (int s = 1; s <= 9; ++s) marca(R1, angS((float)s), -3.f, (s % 2) ? 7.f : 4.5f, cw, 1.8f);
    for (int d = 10; d <= 60; d += 10) marca(R1, angD((float)d), -3.f, (d == 10 || d == 30 || d == 60) ? 7.f : 4.5f, cv, 1.8f);
    txT(fL(), 14.f, 195, 149, cw, "S", 0.5f);
    txT(fL(), 14.f, 221, 131, cw, "3", 0.5f);
    txT(fL(), 14.f, 250, 121, cw, "5", 0.5f);
    txT(fL(), 14.f, 285, 115, cw, "7", 0.5f);
    txT(fL(), 14.f, 320, 115, cw, "9", 0.5f);
    txT(fL(), 10.f, 337, 119, cv, "+10", 0.5f);
    txT(fL(), 10.f, 366, 128, cv, "+30", 0.5f);
    txT(fL(), 10.f, 410, 152, cv, "+60", 0.5f);
    // ---- a escala de baixo (PWR / SWR / MIC): azul na esquerda, clara no resto ----
    arco(R2, -122.f, -108.f, cz, cz, 3.6f);
    arco(R2, -108.f, -56.f, cw, cw, 3.6f);
    {
        const float am[9] = {-118, -112, -104, -96, -88, -80, -72, -64, -58};
        for (int i = 0; i < 9; ++i) marca(R2, am[i], 0.f, (i % 2) ? 5.f : 3.f, i < 2 ? cz : cw, 1.4f);
        const float ab[5] = {-110, -100, -88, -74, -66};
        for (int i = 0; i < 5; ++i) marca(R2, ab[i], -5.f, 0.f, cw, 1.4f);
    }
    txT(fL(), 6.f, 247, 185, cw, "-30", 0.5f);
    txT(fL(), 6.f, 276, 173, cw, "-20", 0.5f);
    txT(fL(), 6.f, 305, 172, cw, "-10", 0.5f);
    txT(fL(), 6.f, 340, 180, cw, "0", 0.5f);
    txT(fL(), 6.f, 352, 190, cw, "+5", 0.5f);
    txT(fL(), 6.f, 238, 165, cw, "1.5", 0.5f);
    txT(fL(), 6.f, 266, 142, cw, "8", 0.5f);
    txT(fL(), 6.f, 285, 141, cw, "10", 0.5f);
    txT(fL(), 6.f, 314, 142, cw, "20", 0.5f);
    txT(fL(), 6.f, 339, 155, cw, "10", 0.5f);
    txT(fL(), 6.f, 361, 151, cw, "50", 0.5f);
    txT(fL(), 6.f, 374, 163, cw, "100", 0.5f);
    txT(fL(), 6.f, 396, 179, cw, "200", 0.5f);
    txT(fL(), 5.4f, 200, 176, cw, "PWR", 0.5f);
    txT(fL(), 5.4f, 210, 187, cw, "SWR", 0.5f);
    txT(fL(), 5.4f, 222, 198, cw, "MIC", 0.5f);
    // ---- o ponteiro (verde): da posicao 0..1 do S-meter para o angulo ----
    {
        const float p = std::clamp(smAtual_, 0.f, 1.05f);
        static const float sp[10] = {0.06f, 0.117f, 0.17f, 0.224f, 0.282f, 0.34f, 0.392f, 0.444f, 0.495f, 0.546f};
        static const float dp[7] = {0.546f, 0.62f, 0.693f, 0.77f, 0.845f, 0.92f, 0.996f};
        float a;
        if (p <= sp[0]) a = angS(0.f) + (p / sp[0] - 1.f) * 2.f;
        else if (p < sp[9]) {
            int i = 0; while (i < 8 && p > sp[i + 1]) ++i;
            a = angS(i + (p - sp[i]) / (sp[i + 1] - sp[i]));
        } else {
            int i = 0; while (i < 5 && p > dp[i + 1]) ++i;
            a = angD(10.f * (i + std::min(1.2f, (p - dp[i]) / (dp[i + 1] - dp[i]))));
        }
        a = std::clamp(a, -131.f, -50.f);
        dl->PushClipRect(Q(176, 95), Q(400, 214), true);
        const ImU32 cp = lig ? IM_COL32(0x5c, 0xec, 0x78, 255) : IM_COL32(0x40, 0x6c, 0x4c, 255);
        dl->AddLine(P(a + 0.6f, 104.f), P(a + 0.6f, 178.f), IM_COL32(0, 0, 0, 140), std::max(1.f, QD(3.2f)));
        dl->AddLine(P(a, 104.f), P(a, 178.f), cp, std::max(1.f, QD(2.4f)));
        dl->PopClipRect();
    }
    // ---- clique: S / dB / PEAK na dica ----
    ImGui::SetCursorScreenPos(Q(180, 97));
    ImGui::InvisibleButton("##medMb", ImVec2(QD(222), QD(112)));
    if (ImGui::IsItemClicked()) medidorModo_ = (medidorModo_ + 1) % 3;
    if (ImGui::IsItemHovered()) {
        char b[48];
        if (!lig) std::snprintf(b, sizeof b, "--");
        else if (medidorModo_ == 1) std::snprintf(b, sizeof b, "%.0f dB", peakDb_);
        else if (medidorModo_ == 2) std::snprintf(b, sizeof b, "PEAK %.0f dB", peakDbSql_);
        else std::snprintf(b, sizeof b, "%s", leituraS(smAtual_).c_str());
        ImGui::SetTooltip("S-meter do VFO-%s (o que se ouve): %s\nClique: leitura em S / dB / PEAK", ativo_ == 0 ? "A" : "B", b);
    }
    // ---- o pre-amplificador (VHF LNA / HF LNA) ----
    {
        const bool vhf = r_.vfo() >= 30000000ULL;
        const ImU32 c = pamp_ > 0 && lig ? M_AMAR : M_APAG;
        txT(fL(), 9.f, 378, 111, c, vhf ? "VHF LNA" : "HF LNA", 0.5f);
    }
}

// O escopo da tela: espectro verde (com a coluna de dB por cima, na esquerda),
// a regua de frequencia e a cachoeira azul. Com Sub RX (DUAL), o do VFO-B ao lado.
void Ui::mb1Escopo(float X0, float Y0, float X1, float Y1, float& yRegua)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool lig = r_.ligado();
    char b[64];
    // a regua (entre o espectro e a cachoeira): arrastavel - sobe a cachoeira e diminui o espectro
    yRegua = std::clamp(yRegua, Y0 + 40.f, Y1 - 45.f);
    const float yR0 = yRegua, yR1 = yR0 + 20.f;
    auto fotoX = [&](float sx) { return (sx - M.ox) / (M.k * kK); };
    const double span = (taxaMostrada_ ? taxaMostrada_ : r_.taxa()) / (1.0 + zoom_);
    const float xm0 = (X0 + X1) * 0.5f;
    const bool dir = dual_ && ativo_ == 1;
    const float xA0 = !dual_ ? X0 : dir ? xm0 + 1 : X0, xA1 = !dual_ ? X1 : dir ? X1 : xm0 - 1;
    {   // trocou o receptor em uso: a cachoeira do outro comeca limpa
        static int ultAtivo = -1;
        if (ultAtivo != ativo_) { if (subTex_) criarTexturaSub(); ultAtivo = ativo_; }
    }
    auto regua = [&](float xa, float xb, double lo, double hi, double fMarca) {
        dl->AddRectFilledMultiColor(Q(xa, yR0), Q(xb, yR1), IM_COL32(0x44, 0x4c, 0x5a, 255), IM_COL32(0x44, 0x4c, 0x5a, 255),
                                    IM_COL32(0x30, 0x36, 0x42, 255), IM_COL32(0x30, 0x36, 0x42, 255));
        const double sp = hi - lo;
        const double st = passoGradeMb(sp);
        for (double f = std::ceil(lo / st) * st; f <= hi; f += st) {
            const float x = xa + (float)((f - lo) / sp) * (xb - xa);
            dl->AddCircleFilled(Q(x, yR0 + 2.5f), QD(1.3f), IM_COL32(0xe8, 0xec, 0xf2, 255), 8);
            if (x < xa + 14 || x > xb - 14) continue;
            if (st >= 1e6) std::snprintf(b, sizeof b, "%.0f", f / 1e6);
            else if (st >= 100000) std::snprintf(b, sizeof b, "%.1f", f / 1e6);
            else if (st >= 10000) std::snprintf(b, sizeof b, "%.2f", f / 1e6);
            else if (st >= 1000) std::snprintf(b, sizeof b, "%.3f", f / 1e6);
            else std::snprintf(b, sizeof b, "%.4f", f / 1e6);
            txT(fL(), 7.6f, x, yR0 + 11.5f, lig ? M_BRANCO : IM_COL32(0x80, 0x84, 0x8c, 255), b, 0.5f);
        }
        // o triangulo da sintonia
        const float xv = xa + (float)((fMarca - lo) / sp) * (xb - xa);
        if (xv >= xa && xv <= xb) dl->AddTriangleFilled(Q(xv - 5, yR0 + 7), Q(xv + 5, yR0 + 7), Q(xv, yR0 - 1), IM_COL32(0x6c, 0xc8, 0xff, 255));
    };
    // ---- o principal (o receptor ouvido) ----
    {
        const ImVec2 A = Q(xA0, Y0), B = Q(xA1, yR0);
        const float sx = A.x, sy = A.y, sw = B.x - A.x, hEsp = B.y - A.y;
        float xm = sx;
        if (ifOn_) {
            const float wIf = std::floor(sw * 0.26f);
            ifDisplay(sx, sy, wIf, Q(0, Y1).y - sy);
            xm = sx + wIf + 2.f;
        }
        const float wm = sx + sw - xm;
        wfVelDesenho_ = wfSpeedEsc_;
        const bool troca = pausa_ && congelado_.size() == suave_.size() && !suave_.empty();
        if (troca) suave_.swap(congelado_);
        semDb_ = true; gradeMb_ = true;
        dl->PushClipRect(ImVec2(xm, sy), ImVec2(xm + wm, sy + hEsp), true);
        espectro(xm, sy, wm, hEsp);
        dl->PopClipRect();
        semDb_ = false; gradeMb_ = false;
        if (troca) suave_.swap(congelado_);
        const double lo = xParaHz(xm, xm, wm), hi = xParaHz(xm + wm, xm, wm);
        // a coluna dos dB por cima do espectro (transparente) e o nome da banda
        if (!ifOn_) {
            const float xc1 = fotoX(xm) + 37.f;
            dl->AddRectFilled(ImVec2(xm, sy), Q(xc1, yR0), IM_COL32(0x4a, 0x6a, 0x74, 120));
            const float range = std::clamp(wfRange_, 10.f, 120.f), topo = refEsp_ > -900.f ? refEsp_ : (wfBrilho_ - 100.f) * 0.5f;
            const float y0f = Y0, hf = yR0 - Y0;
            const float st = range > 80.f ? 20.f : range > 40.f ? 10.f : 5.f;
            const float calib = -105.f;      // o 0 dBFS do FFT vira dBm aproximado na escala da tela
            for (float db = std::ceil((topo - range) / st) * st; db <= topo; db += st) {
                const float y = y0f + hf - (db - topo + range) / range * hf;
                if (y < Y0 + 18 || y > yR0 - 6) continue;
                std::snprintf(b, sizeof b, "%d", (int)std::lround(db + calib));
                txT(fL(), 6.8f, fotoX(xm) + 18.f, y, lig ? M_BRANCO : IM_COL32(0x80, 0x84, 0x8c, 255), b, 0.5f);
                dl->AddLine(Q(fotoX(xm) + 31.f, y), Q(fotoX(xm) + 35.f, y), M_BRANCO, 1.f);
            }
            const char* banda = nullptr;
            static const struct { uint64_t lo, hi; const char* n; } bd[] = {
                {1800000, 2000000, "160M"}, {3500000, 4000000, "80M"}, {5250000, 5450000, "60M"}, {7000000, 7300000, "40M"},
                {10100000, 10150000, "30M"}, {14000000, 14350000, "20M"}, {18068000, 18168000, "17M"}, {21000000, 21450000, "15M"},
                {24890000, 24990000, "12M"}, {28000000, 29700000, "10M"}, {50000000, 54000000, "6M"}, {70000000, 70500000, "4M"},
                {144000000, 148000000, "2M"}, {430000000, 440000000, "70CM"}};
            for (const auto& x : bd) if (r_.vfo() >= x.lo && r_.vfo() <= x.hi) banda = x.n;
            if (!banda) banda = "GEN";
            const float wB = txW(fL(), 8.f, banda) + 8.f;
            dl->AddRectFilled(Q(fotoX(xm) + 40.f, Y0 + 4), Q(fotoX(xm) + 40.f + wB, Y0 + 19), IM_COL32(0x1c, 0x4c, 0x7c, 220), QD(2));
            txT(fL(), 8.f, fotoX(xm) + 44.f, Y0 + 11.5f, IM_COL32(0x9c, 0xd4, 0xff, 255), banda);
        }
        // a marca do outro receptor (tracejada) e o NOTCH
        if (!mono_) {
            const float xo = (float)hzParaX((double)rx_[1 - ativo_].f, xm, wm);
            if (xo >= xm && xo <= xm + wm)
                for (float y = sy; y < sy + hEsp; y += QD(7)) dl->AddLine(ImVec2(xo, y), ImVec2(xo, std::min(sy + hEsp, y + QD(4))), IM_COL32(0xf0, 0xb0, 0x40, 200), std::max(1.f, QD(1.2f)));
        }
        const Receptor& R = rx_[ativo_];
        if ((R.nch || R.bef) && (r_.modo() == "USB" || r_.modo() == "LSB")) {
            const double nh = 150.0 * std::pow(3500.0 / 150.0, std::clamp(R.nch ? R.notch : R.cont, 0.f, 1.f));
            const double fn = r_.modo() == "USB" ? (double)r_.vfo() + nh : (double)r_.vfo() - nh;
            const float xn = (float)hzParaX(fn, xm, wm);
            dl->AddLine(ImVec2(xn, sy), ImVec2(xn, sy + hEsp), alfa(P_CIANO, 200), std::max(1.f, QD(1.5f)));
        }
        regua(fotoX(xm), xA1, lo, hi, (double)r_.vfo());
        // a cachoeira (azul)
        {
            const ImVec2 C = Q(0, yR1), D = Q(0, Y1);
            cachoeira(xm, C.y, wm, D.y - C.y);
        }
        if (pausa_ && std::fmod(agora(), 1.0) < 0.6) txT(fL(), 9.f, (xA0 + xA1) * 0.5f, Y0 + 26, P_AMBAR, "HOLD", 0.5f);
        if (zoom_ > 0 || modoCentro_) {
            std::snprintf(b, sizeof b, "%s%s", modoCentro_ ? "CENTER  " : "", zoom_ > 0 ? (std::string("ZOOM x") + std::to_string((int)std::lround(1 + zoom_))).c_str() : "");
            txT(fL(), 6.8f, xA1 - 6, Y0 + 11, IM_COL32(0x9c, 0xd4, 0xff, 255), b, 1.f);
        }
    }
    // ---- Sub RX (DUAL): o escopo do outro VFO ----
    if (dual_) {
        const int o = 1 - ativo_;
        Receptor& O = rx_[o];
        const float xB0 = dir ? X0 : xm0 + 1, xB1 = dir ? xm0 - 1 : X1;
        dl->AddLine(Q(xm0, Y0), Q(xm0, Y1), IM_COL32(0x80, 0x88, 0x98, 255), std::max(1.f, QD(1.2f)));
        subCentro_ = (double)O.f;
        subSpan_ = span;
        // o espectro ate a regua e a cachoeira depois dela (subGap_ = a altura da regua)
        subFrac_ = (yR0 - Y0) / (Y1 - Y0);
        subGap_ = QD(yR1 - yR0);
        const ImVec2 A = Q(xB0, Y0), B = Q(xB1, Y1);
        escopoSub(A.x, A.y, B.x - A.x, B.y - A.y);
        subGap_ = 0.f;
        const double lo = subCentro_ - span / 2, hi = subCentro_ + span / 2;
        regua(xB0, xB1, lo, hi, (double)O.f);
        {
            const float xc = (A.x + B.x) * 0.5f, hs = Q(0, yR0).y - A.y;
            for (float y = A.y; y < A.y + hs; y += QD(7)) dl->AddLine(ImVec2(xc, y), ImVec2(xc, std::min(A.y + hs, y + QD(4))), IM_COL32(0xf0, 0xb0, 0x40, 220), std::max(1.f, QD(1.2f)));
            txT(fL(), 7.f, xB0 + 6, Y0 + 11, IM_COL32(0xf0, 0xb0, 0x40, 255), o == 0 ? "VFO-A" : "VFO-B");
        }
        ImGui::SetCursorScreenPos(A);
        ImGui::InvisibleButton("##subMb", ImVec2(std::max(1.f, B.x - A.x), std::max(1.f, B.y - A.y)));
        if (ImGui::IsItemHovered()) {
            ImGuiIO& io = ImGui::GetIO();
            const double hz = lo + (io.MousePos.x - A.x) / std::max(1.f, B.x - A.x) * span;
            dl->AddLine(ImVec2(io.MousePos.x, A.y), ImVec2(io.MousePos.x, Q(0, Y1).y), IM_COL32(255, 255, 255, 60));
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
    }
    {   // arrastar a regua para cima / para baixo (duplo clique: volta ao tamanho padrao)
        ImGui::SetCursorScreenPos(Q(X0, yR0));
        ImGui::InvisibleButton("##reguaMb", ImVec2(std::max(1.f, QD(X1 - X0)), std::max(1.f, QD(yR1 - yR0))));
        const bool sob = ImGui::IsItemHovered(), at = ImGui::IsItemActive();
        if (sob || at) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            dl->AddRect(Q(X0, yR0), Q(X1, yR1), IM_COL32(0x6c, 0xc8, 0xff, at ? 220 : 120), 0.f, 0, std::max(1.f, QD(1.2f)));
            // os tracinhos de "pegar" no meio
            for (int k = -1; k <= 1; ++k) dl->AddLine(Q((X0 + X1) * 0.5f - 14, yR0 + 10 + k * 3.f), Q((X0 + X1) * 0.5f + 14, yR0 + 10 + k * 3.f), IM_COL32(0xd8, 0xf0, 0xff, 200), 1.f);
        }
        if (sob && !at) ImGui::SetTooltip("Arraste para cima ou para baixo: a cachoeira cresce e o espectro diminui (ou o contrário)\nDuplo clique: volta ao tamanho padrão");
        {   // pela posicao do mouse (nao pelo "delta": o primeiro quadro do clique pode trazer um pulo)
            static float pega = 0.f;
            const float myFoto = (ImGui::GetIO().MousePos.y - M.oy) / std::max(0.01f, M.k * kK) - 24.f;
            if (ImGui::IsItemActivated()) pega = myFoto - yRegua;
            if (at) yRegua = std::clamp(myFoto - pega, Y0 + 40.f, Y1 - 45.f);
        }
        if (sob && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) yRegua = expand_ ? 280.f : 398.f;
    }
}

// ===========================================================================
//  MB1SDR: o teclado do meio (bandas / numeros, REC, M.IN ... BIN, SUB ... SPLT),
//  as teclas de canto, o dial e as F1..F10 embaixo da tela
// ===========================================================================
void Ui::mb1Teclado()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    Receptor& R = rx_[ativo_];
    Receptor& O = rx_[1 - ativo_];
    const float cx[6] = {917, 968, 1020, 1070, 1122, 1173};
    const float cw = 36.f;
    const float ry[4] = {41, 82, 123, 163};
    const float rh = 24.f;
    // ---- as teclas de banda / numero ----
    struct Num { const char* banda; const char* dig; int band; int d; };
    static const Num nums[4][3] = {
        {{"160", "1", 0, 1}, {"80", "2", 1, 2}, {"40", "3", 3, 3}},
        {{"30", "4", 4, 4}, {"20", "5", 5, 5}, {"17", "6", 6, 6}},
        {{"15", "7", 7, 7}, {"12", "8", 8, 8}, {"10", "9", 9, 9}},
        {{"6", ".", 10, -1}, {"2", "0", -2, 0}, {nullptr, nullptr, 0, 0}}};
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 3; ++c) {
            const Num& n = nums[r][c];
            const float x0 = cx[c], y0 = ry[r];
            char id[16]; std::snprintf(id, sizeof id, "kN%d%d", r, c);
            if (!n.banda) {      // ENT
                const int k = teclaTxt("kEnt", "ENT", x0, y0, x0 + cw, y0 + rh, 10.f, M_ROT,
                                       "ENT: digitar a frequência (kHz) com as teclas de número / confirmar", nullptr, entrando_);
                if (k == 1) { if (entrando_) confirmarEntrada(); else { entrando_ = true; entrada_.clear(); } }
                continue;
            }
            char d[200];
            if (n.band >= 0) std::snprintf(d, sizeof d, "Banda de %s m (volta onde estava nesta banda)\nDigitando a frequência: o número %s", n.banda, n.dig);
            else std::snprintf(d, sizeof d, "Banda de 2 m (145 MHz, FM)\nDigitando a frequência: o número 0");
            const int k = teclaMb(id, x0, y0, x0 + cw, y0 + rh, d);
            tx(fP(), 6.6f, x0 + 4, y0 + 7.5f, M_ROT, n.banda, 0.f, 0.92f);
            tx(fP(), 11.f, x0 + cw - 4, y0 + rh * 0.56f, M_LARANJA, n.dig, 1.f, 0.95f);
            if (k == 1) {
                if (entrando_) {
                    if (n.d >= 0) digito(n.d);
                    else if (entrada_.find('.') == std::string::npos) entrada_ += '.';
                } else if (n.band >= 0) irBanda(n.band);
                else {             // 2 m
                    R.memModo = false;
                    mudarModoPainel("NFM");
                    sintonizar(145000000ULL, false);
                    muteAte_ = agora() + 0.25;
                }
            }
            if (k == 2 && n.d >= 0) { entrando_ = true; entrada_.clear(); digito(n.d); }
        }
    // ---- os LEDs (REC, IND1, IND2, SHIFT) ----
    {
        const bool lig = r_.ligado();
        const bool busy = lig && r_.aoVivo() && sqlAberto_ && (R.sql > -90 || smAtual_ > 0.22f);
        tx(fP(), 6.8f, 1088, 38, M_ROT, "REC", 0.5f, 0.92f);
        ledMb(1088, 55, 6.f, IM_COL32(0xff, 0x34, 0x2c, 255), r_.som().gravando());
        tx(fP(), 6.8f, 1088, 79, M_ROT, "IND1", 0.5f, 0.92f);
        ledMb(1088, 96, 6.f, IM_COL32(0x50, 0xf0, 0x60, 255), busy);
        tx(fP(), 6.8f, 1088, 120, M_ROT, "IND2", 0.5f, 0.92f);
        ledMb(1088, 138, 6.f, IM_COL32(0xff, 0xd0, 0x40, 255), decAberta_ || decRodando_);
        tx(fP(), 6.8f, 1088, 160, M_ROT, "SHIFT", 0.5f, 0.92f);
        ledMb(1088, 177, 6.f, IM_COL32(0xff, 0x9a, 0x30, 255), shift_);
        ImGui::SetCursorScreenPos(Q(1072, 30));
        ImGui::InvisibleButton("##ledsMb", ImVec2(QD(32), QD(156)));
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("REC: gravando o áudio\nIND1: sinal no canal (squelch aberto)\nIND2: decodificador aberto\nSHIFT: a segunda função (tecla WF segurada / clique no LED)");
        if (ImGui::IsItemClicked()) shift_ = !shift_;
    }
    // ---- REC, PLAY, memorias e frequencia ----
    {
        const bool grav = r_.som().gravando();
        const int k1 = teclaMb("kRec", cx[4], ry[0], cx[4] + cw, ry[0] + rh, grav ? "REC: parar a gravação" : "REC: gravar o áudio (pasta Gravacoes)", nullptr, grav);
        dl->AddCircleFilled(Q(cx[4] + cw * 0.5f, ry[0] + rh * 0.5f), QD(4.5f), grav && std::fmod(agora(), 1.0) < 0.6 ? IM_COL32(0xff, 0x40, 0x30, 255) : IM_COL32(0xe0, 0x50, 0x48, 255), 16);
        if (k1 == 1) alternarGravacao();
        const int k2 = teclaMb("kPlay", cx[5], ry[0], cx[5] + cw, ry[0] + rh, "PLAY: toca a última gravação\nSegurar: para de tocar");
        dl->AddTriangleFilled(Q(cx[5] + cw * 0.5f - 3.5f, ry[0] + rh * 0.5f - 5), Q(cx[5] + cw * 0.5f - 3.5f, ry[0] + rh * 0.5f + 5),
                              Q(cx[5] + cw * 0.5f + 5, ry[0] + rh * 0.5f), IM_COL32(0x80, 0xa0, 0x88, 255));
        if (k2 == 1) tocarUltimaGravacao();
        if (k2 == 2) { PlaySoundA(nullptr, nullptr, 0); aviso("PLAY parado"); }
        if (teclaTxt("kMin", "M.IN", cx[4], ry[1], cx[4] + cw, ry[1] + rh, 9.f, M_ROT, "M.IN: gravar a frequência nas memórias (abre a lista com o nome)") == 1) {
            memAberta_ = true;
            std::snprintf(memNome_, sizeof memNome_, "%s %s", fmtFreq(r_.vfo()).c_str(), r_.modo().c_str());
        }
        if (teclaTxt("kMrd", "M.RD", cx[5], ry[1], cx[5] + cw, ry[1] + rh, 9.f, M_ROT, "M.RD: o dial passa os canais de memória (V/M)", nullptr, R.memModo) == 1) {
            if (mem_.empty()) aviso("Nenhuma memória gravada (M.IN grava)");
            else {
                R.memModo = !R.memModo;
                if (R.memModo) {
                    int mais = 0; long long melhor = LLONG_MAX;
                    for (int i = 0; i < (int)mem_.size(); ++i) {
                        const long long dd = std::llabs((long long)mem_[i].f - (long long)r_.vfo());
                        if (dd < melhor) { melhor = dd; mais = i; }
                    }
                    R.memCanal = mais; sintonizarMemoria((size_t)mais);
                }
                aviso(R.memModo ? "MEMÓRIA" : "VFO", 1.0);
            }
        }
        if (teclaTxt("kMd", "MD", cx[4], ry[2], cx[4] + cw, ry[2] + rh, 9.5f, M_ROT, "MD: memória anterior") == 1) funcaoMulti(4, -1);
        if (teclaTxt("kMu", "MU", cx[5], ry[2], cx[5] + cw, ry[2] + rh, 9.5f, M_ROT, "MU: próxima memória") == 1) funcaoMulti(4, 1);
        auto passoF = [&](int sinal, bool grande) {
            if (R.lock) { aviso("LOCK: dial travado"); return; }
            const int64_t ps = grande ? 100000 : std::max(1, passo_);
            const int64_t nf = std::max<int64_t>(1000, ((int64_t)r_.vfo() / ps + sinal) * ps);
            R.memModo = false;
            sintonizar((uint64_t)nf, false); R.f = (uint64_t)nf;
        };
        char d[160];
        std::snprintf(d, sizeof d, "FD: desce um passo (%s)\nSegurar: desce 100 kHz", fmtPasso(passo_).c_str());
        const int k3 = teclaTxt("kFd", "FD", cx[4], ry[3], cx[4] + cw, ry[3] + rh, 9.5f, M_ROT, d);
        if (k3) passoF(-1, k3 == 2);
        std::snprintf(d, sizeof d, "FU: sobe um passo (%s)\nSegurar: sobe 100 kHz", fmtPasso(passo_).c_str());
        const int k4 = teclaTxt("kFu", "FU", cx[5], ry[3], cx[5] + cw, ry[3] + rh, 9.5f, M_ROT, d);
        if (k4) passoF(1, k4 == 2);
    }
    // ---- NR / NB1 / NB2 / ANF / TNF / BIN ----
    {
        const float y0 = 213, y1 = 237;
        auto dupla = [&](const char* id, int col, const char* a, const char* s, bool on, const char* dica) {
            const int k = teclaMb(id, cx[col], y0, cx[col] + cw, y1, dica, nullptr, on);
            tx(fP(), 7.f, cx[col] + 6, y0 + 8, M_ROT, a, 0.f, 0.92f);
            if (s) tx(fP(), 5.6f, cx[col] + cw - 5, y1 - 6.5f, IM_COL32(0xd0, 0x80, 0x58, 255), s, 1.f, 0.92f);
            return k;
        };
        {
            const int k = dupla("kNr", 0, "NR", "ESC", R.nr2 || R.nr1, "NR: redutor de ruído espectral\nSegurar: NR1 (o simples)\nDigitando a frequência: ESC (cancela)");
            if (k == 1) { if (entrando_) { entrando_ = false; entrada_.clear(); } else R.nr2 = !R.nr2; }
            if (k == 2) { R.nr1 = !R.nr1; aviso(R.nr1 ? "NR1 ligado" : "NR1 desligado"); }
        }
        if (dupla("kNb1", 1, "NB1", "DEL", R.nb1, "NB1: apagador de pulsos (no áudio)\nDigitando a frequência: DEL (apaga tudo)") == 1) {
            if (entrando_) entrada_.clear(); else R.nb1 = !R.nb1;
        }
        {
            const int k = teclaMb("kNb2", cx[2], y0, cx[2] + cw, y1, "NB2: apagador de pulsos no IQ (antes do demodulador)\nDigitando a frequência: apaga o último número", nullptr, R.nb2);
            tx(fP(), 7.f, cx[2] + 6, y0 + 8, M_ROT, "NB2", 0.f, 0.92f);
            const ImU32 co = IM_COL32(0xd0, 0x80, 0x58, 255);
            const float xb = cx[2] + cw - 6, yb = y1 - 6.5f;
            dl->AddRectFilled(Q(xb - 7, yb - 3), Q(xb, yb + 3), co, QD(0.8f));
            dl->AddTriangleFilled(Q(xb - 7, yb - 3), Q(xb - 7, yb + 3), Q(xb - 10, yb), co);
            if (k == 1) { if (entrando_) { if (!entrada_.empty()) entrada_.pop_back(); } else R.nb2 = !R.nb2; }
        }
        if (teclaTxt("kAnf", "ANF", cx[3], y0, cx[3] + cw, y1, 10.f, M_ROT, "ANF: notch automático (tira os apitos sozinho)", nullptr, R.anch) == 1) R.anch = !R.anch;
        {
            const int k = teclaTxt("kTnf", "TNF", cx[4], y0, cx[4] + cw, y1, 10.f, M_ROT, "TNF: notch manual (posição e largura: MULTI 1 com SHIFT)\nSegurar: CONT (contorno)", nullptr, R.nch || R.bef);
            if (k == 1) R.nch = !R.nch;
            if (k == 2) { R.bef = !R.bef; aviso(R.bef ? "CONT ligado" : "CONT desligado"); }
        }
        if (teclaTxt("kBin", "BIN", cx[5], y0, cx[5] + cw, y1, 10.f, M_ROT, "BIN: (num receptor mono) BC - cancelador de vários apitos", nullptr, R.bc) == 1) R.bc = !R.bc;
    }
    // ---- SUB, A>B, A<B, A<>B, TX B, SPLT ----
    {
        const float y0 = 265, y1 = 290;
        if (teclaTxt("kSub", "SUB", cx[0], y0, cx[0] + cw, y1, 10.f, M_ROT, "SUB: o VFO-B anda junto quando se gira o dial (DUAL-W)", nullptr, tracking_) == 1)
            tracking_ = !tracking_;
        auto seta = [&](const char* id, int col, int tipo, const char* dica) {
            const int k = teclaMb(id, cx[col], y0, cx[col] + cw, y1, dica);
            const float xc = cx[col] + cw * 0.5f, yc = (y0 + y1) * 0.5f;
            tx(fP(), 10.f, xc - 6.5f, yc, M_ROT, "A", 0.5f, 0.92f);
            tx(fP(), 10.f, xc + 6.5f, yc, M_ROT, "B", 0.5f, 0.92f);
            const float h = 2.8f;
            if (tipo == 0 || tipo == 2) dl->AddTriangleFilled(Q(xc - 1.2f, yc - h), Q(xc + 2.2f, yc), Q(xc - 1.2f, yc + h), M_ROT);
            if (tipo == 1 || tipo == 2) dl->AddTriangleFilled(Q(xc + 1.2f, yc - h), Q(xc - 2.2f, yc), Q(xc + 1.2f, yc + h), M_ROT);
            return k;
        };
        auto copia = [&](int de, int para) {
            sincronizarAtivo();
            rx_[para].f = rx_[de].f; rx_[para].modo = rx_[de].modo; rx_[para].fil = rx_[de].fil; rx_[para].memModo = false;
            if (para == ativo_) { r_.setModo(rx_[para].modo); sintonizar(rx_[para].f, false); aplicarFiltro(); muteAte_ = agora() + 0.25; }
        };
        if (seta("kAb", 1, 0, "A>B: copia o VFO-A para o VFO-B") == 1) { copia(0, 1); aviso("A > B", 1.0); }
        if (seta("kBa", 2, 1, "A<B: copia o VFO-B para o VFO-A") == 1) { copia(1, 0); aviso("B > A", 1.0); }
        if (seta("kAxb", 3, 2, "A<>B: troca o VFO-A com o VFO-B (frequência, modo e filtro)") == 1) { trocarMainSub(); aviso("A <> B", 1.0); }
        if (teclaTxt("kTxb", "TX B", cx[4], y0, cx[4] + cw, y1, 9.5f, M_ROT, "TX B: (num receptor) marca o VFO-B como o de TX (SPLIT)", nullptr, txVfo_ == 1) == 1)
            txVfo_ = txVfo_ == 1 ? 0 : 1;
        {
            const bool split = txVfo_ != ativo_;
            const int k = teclaTxt("kSplt", "SPLT", cx[5], y0, cx[5] + cw, y1, 9.5f, M_ROT,
                                   "SPLT: um VFO para ouvir e o outro marcado como TX\nSegurar: QUICK SPLIT (o outro VFO vai 5 kHz acima)", nullptr, split);
            if (k == 1) txVfo_ = split ? ativo_ : 1 - ativo_;
            if (k == 2) { sincronizarAtivo(); O.f = r_.vfo() + 5000; O.modo = r_.modo(); txVfo_ = 1 - ativo_; aviso("QUICK SPLIT +5 kHz"); }
        }
    }
    // ---- as teclas de canto e o dial ----
    const float cxD = 1077.f, cyD = 467.f, rCorte = 158.f;
    {
        const int k = teclaCanto("kcRx2", 912, 303, 998, 384, cxD, cyD, rCorte,
                                 "RX2: liga / desliga o escopo do VFO-B ao lado (Sub RX)");
        tx(fP(), 10.f, 932, 342, dual_ ? M_ROT : IM_COL32(0x80, 0x84, 0x8a, 255), "RX2", 0.f, 0.95f);
        ledMb(921, 318, 6.f, IM_COL32(0x50, 0xf0, 0x60, 255), dual_);
        if (k == 1) dual_ = !dual_;
    }
    {
        const int k = teclaCanto("kcAb", 1156, 303, 1242, 384, cxD, cyD, rCorte,
                                 "A/B: o dial, o som e os botões passam para o outro VFO (A ou B)\nSegurar: troca as frequências A e B");
        tx(fP(), 10.f, 1213, 346, ativo_ == 0 ? M_ROT : IM_COL32(0x80, 0x84, 0x8a, 255), "A", 1.f, 0.95f);
        tx(fP(), 10.f, 1217, 346, IM_COL32(0x80, 0x84, 0x8a, 255), "/", 0.5f, 0.95f);
        tx(fP(), 10.f, 1221, 346, ativo_ == 1 ? M_LARANJA : IM_COL32(0x90, 0x6c, 0x58, 255), "B", 0.f, 0.95f);
        ledMb(1233, 318, 6.f, IM_COL32(0xff, 0x9a, 0x30, 255), ativo_ == 1);
        if (k == 1) ativar(1 - ativo_);
        if (k == 2) { trocarMainSub(); aviso("A <> B"); }
    }
    {
        const int k = teclaCanto("kcLock", 912, 510, 998, 588, cxD, cyD, rCorte,
                                 R.lock ? "LOCK: destravar o dial" : "LOCK: travar o dial");
        tx(fP(), 8.f, 932, 576, R.lock ? IM_COL32(0xff, 0x6c, 0x40, 255) : IM_COL32(0xc0, 0x60, 0x40, 255), "LOCK", 0.5f, 0.95f);
        if (k == 1) { R.lock = !R.lock; aviso(R.lock ? "LOCK: dial travado" : "LOCK: dial livre"); }
    }
    {
        const int k = teclaCanto("kcDi", 1156, 510, 1242, 588, cxD, cyD, rCorte,
                                 "D/I: passo do dial (clique escolhe)\nSegurar: FAST (dial 10x mais rápido)");
        tx(fP(), 8.f, 1222, 576, M_ROT, "D/I", 0.5f, 0.95f);
        if (k == 1) ImGui::OpenPopup("##passoMb");
        if (k == 2) { fast_ = !fast_; aviso(fast_ ? "FAST ligado" : "FAST desligado"); }
    }
    {
        const int p = dialMb("dMb", cxD, cyD, 120.5f, angDialA_, R.lock,
                             "Dial: sintoniza o VFO ouvido (arraste girando ou use a roda do mouse)\nGirar rápido acelera. Passo: D/I");
        if (p) sintonizarRx(ativo_, p);
    }
    // ---- F1..F10 (as mesmas das teclas de toque da tela) ----
    for (int i = 0; i < 10; ++i) {
        const float x0 = 188.f + 66.9f * i, x1 = x0 + 46.f;
        char id[12], t[8];
        std::snprintf(id, sizeof id, "kF%d", i + 1);
        std::snprintf(t, sizeof t, "F%d", i + 1);
        char d[240];
        std::snprintf(d, sizeof d, "F%d = a tecla de toque de cima: %s%s", i + 1, dicaSoftMb(i), "\n(SHIFT troca a página)");
        const int k = teclaTxt(id, t, x0, 561, x1, 597, 10.f, M_ROT, d);
        if (k) softMb(i, k == 2);
    }
}

// o dial grande do MB1: saia de borracha, o aro fino claro, a face escura e a covinha
int Ui::dialMb(const char* id, float cx, float cy, float rFace, float& ang, bool travado, const char* dica)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 c = Q(cx, cy);
    const float F = QD(rFace), RO = F * 1.10f;
    ImGui::SetCursorScreenPos(ImVec2(c.x - RO, c.y - RO));
    ImGui::PushID(id);
    ImGui::InvisibleButton("##dmb", ImVec2(2 * RO, 2 * RO));
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
    // sombra
    dl->PushClipRect(Q(0, -24), Q(1500, 609.5f), true);     // nada do dial passa da beirada de baixo da frente
    dl->AddCircleFilled(ImVec2(c.x + F * 0.02f, c.y + F * 0.05f), RO * 1.02f, IM_COL32(0, 0, 0, 110), 128);
    // a saia de borracha (serrilhado fino, gira com o dial)
    auto luzS = [](float a) { return 0.075f + 0.09f * sino(a, rad(-125), rad(55)) + 0.03f * sino(a, rad(40), rad(40)); };
    anelLuz(dl, c, F * 1.02f, RO, 0.f, TWO_PI, 180, [&](float a, float rr) { return cinzaN(luzS(a) * (rr > RO * 0.985f ? 0.6f : 1.f)); });
    for (int k = 0; k < 150; ++k) {
        const float an = ang + k * TWO_PI / 150;
        const ImVec2 d = dirAng(an);
        dl->AddLine(ImVec2(c.x + d.x * F * 1.04f, c.y + d.y * F * 1.04f), ImVec2(c.x + d.x * RO * 0.98f, c.y + d.y * RO * 0.98f),
                    cinzaN(luzS(an) * 0.55f, 150), std::max(1.f, F * 0.010f));
    }
    circuloSuave(dl, c, RO, 180, 1.f, [&](float a) { return cinzaN(0.07f + 0.12f * sino(a, rad(-125), rad(50)), 200); });
    // o aro fino claro (a linha branca da foto)
    circuloSuave(dl, c, F * 1.015f, 180, std::max(1.f, QD(1.6f)), [](float a) {
        return cinzaN(0.55f + 0.40f * sino(a, rad(-110), rad(70)) + 0.15f * sino(a, rad(30), rad(40)), 255);
    });
    // a face escura (levemente conica)
    discoCone(dl, c, F, [](float a) { return 0.115f + 0.045f * sino(a, rad(-130), rad(50)) + 0.02f * sino(a, rad(45), rad(40)); }, 1.f);
    for (int i = 1; i <= 14; ++i) dl->AddCircle(c, F * i / 15.f, IM_COL32(0, 0, 0, (i & 1) ? 12 : 6), 0, 1.f);
    // a covinha do dedo (gira com o dial; parada embaixo, como na foto)
    {
        const ImVec2 d = dirAng(ang + (float)(kPi * 0.5) + rad(14.f));
        const ImVec2 f(c.x + d.x * F * 0.58f, c.y + d.y * F * 0.58f);
        const float rf = F * 0.27f;
        dl->AddCircleFilled(ImVec2(f.x, f.y + rf * 0.06f), rf * 1.06f, IM_COL32(0, 0, 0, 200), 64);
        anelLuz(dl, f, rf * 0.80f, rf, 0.f, TWO_PI, 96, [](float a, float) { return cinzaN(0.06f + 0.30f * sino(a, rad(-120), rad(45)) + 0.10f * sino(a, rad(60), rad(40))); });
        anelLuz(dl, f, 0.f, rf * 0.80f, 0.f, TWO_PI, 96, [&](float a, float rr) {
            const float t = rr / (rf * 0.80f);
            return cinzaN((0.03f + 0.10f * sino(a, rad(70), rad(40))) * (0.4f + 0.6f * t) + 0.25f * std::max(0.f, 1.f - t * 3.f) * sino(a, rad(-120), rad(60)));
        });
        // os reflexos (a lente brilhante no meio da covinha)
        anelLuz(dl, f, rf * 0.25f, rf * 0.62f, 0.f, TWO_PI, 72, [](float a, float) {
            return cinzaN(0.10f + 0.45f * sino(a, rad(-100), rad(35)) + 0.35f * sino(a, rad(70), rad(30)), 200);
        });
        dl->AddCircleFilled(f, rf * 0.25f, IM_COL32(0x08, 0x08, 0x09, 255), 32);
        dl->AddCircleFilled(ImVec2(f.x - rf * 0.12f, f.y - rf * 0.36f), rf * 0.10f, IM_COL32(255, 255, 255, 200), 16);
        dl->AddCircleFilled(ImVec2(f.x + rf * 0.16f, f.y + rf * 0.30f), rf * 0.08f, IM_COL32(255, 255, 255, 170), 12);
        dl->AddLine(ImVec2(f.x - rf * 0.30f, f.y + rf * 0.05f), ImVec2(f.x - rf * 0.05f, f.y + rf * 0.28f), IM_COL32(255, 255, 255, 120), std::max(1.f, rf * 0.05f));
        circuloSuave(dl, f, rf * 0.80f, 96, 1.f, [](float a) { return cinzaN(0.15f + 0.45f * sino(a, rad(-135), rad(40)), 200); });
    }
    if (ativo && dentro) dl->AddCircle(c, F * 1.02f, alfa(P_LARANJA, 110), 0, std::max(1.f, QD(2.f)));
    dl->PopClipRect();
    if (sobreB && dentro && !ativo && dica) ImGui::SetTooltip("%s", dica);
    return travado ? 0 : passos;
}

// ===========================================================================
//  MB1SDR: o lado direito - os quatro botoes duplos, MULTI 1 / 2, as teclas
//  pequenas (F11..F21, WF), RIT / XIT / RX2 e o MULTI 3
// ===========================================================================
void Ui::mb1Direita()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    Receptor& A = rx_[ativo_];
    Receptor& O = rx_[1 - ativo_];
    char d[360];
    float di, dO;
    // ---- MIC/SPD - DRIVE (VOX - COMP com SHIFT): o escopo e o squelch automatico ----
    rotKnobMb(1293, 20, 32, 7.3f, "MIC/SPD", "DRIVE", "VOX", "COMP");
    if (!shift_) {
        std::snprintf(d, sizeof d, "MIC/SPD (miolo): velocidade da cachoeira (SPEED %.0f)\nDRIVE (saia): LEVEL do espectro e da cachoeira (%.0f)\nSHIFT: VOX / COMP",
                      wfSpeedEsc_, wfBrilho_ - 100.f);
        if (knobMb("gSpd", 1293, 80, 43, (wfSpeedEsc_ - 1.f) / 9.f, wfBrilho_ / 200.f, di, dO, d, true)) {
            if (di != 0) { static float acc = 0; acc += di * 9.f; const int n = (int)acc; if (n) { wfSpeedEsc_ = std::clamp(wfSpeedEsc_ + n, 1.f, 10.f); acc -= n; } }
            if (dO != 0) { wfBrilho_ = std::clamp(wfBrilho_ + dO * 200.f, 0.f, 200.f); autoWf_ = false; }
        }
    } else {
        std::snprintf(d, sizeof d, "(SHIFT) VOX (miolo): margem do squelch automático (%.0f dB)\nCOMP (saia): força do compressor (%.0f%%)",
                      sqlMargem_, procNivel_ * 100.f);
        if (knobMb("gSpd", 1293, 80, 43, sqlMargem_ / 10.f, procNivel_, di, dO, d, true)) {
            sqlMargem_ = std::clamp(sqlMargem_ + di * 10.f, 0.f, 10.f);
            procNivel_ = std::clamp(procNivel_ + dO, 0.f, 1.f);
        }
    }
    // ---- AF - RF ----
    rotKnobMb(1427, 22, 34, 7.3f, "AF", "RF", nullptr, nullptr);
    std::snprintf(d, sizeof d, "AF (miolo): volume do VFO-%s (%d%%)\nRF (saia): ganho do dongle (%d dB)", ativo_ == 0 ? "A" : "B",
                  (int)std::lround(A.af), (int)std::lround(49.6f * A.rf / 100.f));
    if (knobMb("gAf", 1427, 80, 43, A.af / 100.f, A.rf / 100.f, di, dO, d, true)) {
        A.af = std::clamp(A.af + di * 100.f, 0.f, 100.f);
        A.rf = std::clamp(A.rf + dO * 100.f, 0.f, 100.f);
    }
    // ---- MON - SQL (TNF.P - TNF.W com SHIFT) = MULTI 1 ----
    rotKnobMb(1293, 149, 162, 7.3f, "MON", "SQL", "TNF.P", "TNF.W");
    tx(fP(), 7.3f, 1287, 275, M_ROT, "MULTI 1", 0.5f, 0.95f);
    if (!shift_) {
        char sq[24];
        if (A.sql <= -90) std::snprintf(sq, sizeof sq, "aberto"); else std::snprintf(sq, sizeof sq, "%+d dB", (int)std::lround(A.sql));
        std::snprintf(d, sizeof d, "MON (miolo): tonalidade do áudio (%s)\nSQL (saia): squelch (%s)\nSHIFT: TNF.P / TNF.W (o notch manual)",
                      tom_ < 45 ? "grave" : tom_ > 55 ? "agudo" : "plano", sq);
        if (knobMb("gMon", 1293, 215, 43, tom_ / 100.f, (A.sql + 90.f) / 120.f, di, dO, d, true)) {
            if (di != 0) { tom_ = std::clamp(tom_ + di * 100.f, 0.f, 100.f); rxeq_ = 0; }
            if (dO != 0) {
                A.sql = std::clamp(A.sql + dO * 120.f, -90.f, 30.f);
                sql_ = A.sql; sqlAplicado_ = A.sql;
                if (A.sql <= -90.f) sqlAuto_[ativo_] = false;
            }
        }
    } else {
        const double nh = 150.0 * std::pow(3500.0 / 150.0, std::clamp(A.notch, 0.f, 1.f));
        std::snprintf(d, sizeof d, "(SHIFT) TNF.P (miolo): posição do notch manual (%d Hz) - girar liga o TNF\nTNF.W (saia): largura do notch (%s)",
                      (int)std::lround(nh), A.nchLargo ? "largo" : "estreito");
        if (knobMb("gMon", 1293, 215, 43, A.notch, A.nchLargo ? 0.75f : 0.25f, di, dO, d, true)) {
            if (di != 0) { A.notch = std::clamp(A.notch + di, 0.f, 1.f); A.nch = true; }
            if (dO > 0.002f) A.nchLargo = true;
            if (dO < -0.002f) A.nchLargo = false;
        }
    }
    // ---- SHIFT - WIDTH (NB1 - NB2 com SHIFT) = MULTI 2 ----
    rotKnobMb(1427, 149, 162, 7.3f, "SHIFT", "WIDTH", "NB1", "NB2");
    tx(fP(), 7.3f, 1421, 275, M_ROT, "MULTI 2", 0.5f, 0.95f);
    if (!shift_) {
        const int g = grupoModo(r_.modo());
        float& hi = filHi_[g][A.fil];
        float& lo = filLo_[g][A.fil];
        float tIn = 0.5f, tOut = 0.5f;
        switch (g) {
        case 0: std::snprintf(d, sizeof d, "SHIFT (miolo): desloca o filtro (%.2f a %.2f kHz)\nWIDTH (saia): largura (%.2f kHz)\nSHIFT: NB1 / NB2", lo / 1000.f, hi / 1000.f, (hi - lo) / 1000.f);
                tIn = (lo + hi) * 0.5f / 3000.f; tOut = (hi - lo - 400.f) / 4600.f; break;
        case 1: std::snprintf(d, sizeof d, "SHIFT (miolo): %+.0f Hz\nWIDTH (saia): largura do CW (%.0f Hz)\nSHIFT: NB1 / NB2", lo, hi);
                tIn = (lo + 800) / 1600.f; tOut = (hi - 50) / 2450.f; break;
        case 2: std::snprintf(d, sizeof d, "SHIFT (miolo): corte baixo do AM (%.2f kHz)\nWIDTH (saia): corte alto (%.1f kHz)\nSHIFT: NB1 / NB2", lo / 1000.f, hi / 1000.f);
                tIn = lo / 1000.f; tOut = (hi - 2000) / 6000.f; break;
        case 3: std::snprintf(d, sizeof d, "WIDTH: largura do FM (%.1f kHz)\nSHIFT: NB1 / NB2", hi / 1000.f);
                tOut = (hi - 6000) / 24000.f; tIn = tOut; break;
        default: std::snprintf(d, sizeof d, "WIDTH: largura do FM largo (%.0f kHz)\nSHIFT: NB1 / NB2", hi / 1000.f);
                tOut = (hi - 100000) / 150000.f; tIn = tOut; break;
        }
        if (knobMb("gShw", 1427, 215, 43, std::clamp(tIn, 0.f, 1.f), std::clamp(tOut, 0.f, 1.f), di, dO, d, true)) {
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
    } else {
        std::snprintf(d, sizeof d, "(SHIFT) NB1 (miolo): nível do NB (%.0f)\nNB2 (saia): nível do NB2 (%.0f)", nbNivel_, nb2Nivel_ * 100.f);
        if (knobMb("gShw", 1427, 215, 43, nbNivel_ / 100.f, nb2Nivel_, di, dO, d, true)) {
            nbNivel_ = std::clamp(nbNivel_ + di * 100.f, 0.f, 100.f);
            nb2Nivel_ = std::clamp(nb2Nivel_ + dO, 0.f, 1.f);
        }
    }
    // ---- as teclas pequenas (F11..F21 e WF / SHIFT) ----
    {
        const float kx[4] = {1255, 1306, 1357, 1408};
        const float kw = 46.f;
        const float ky[3] = {290, 330, 370};
        const float kh = 25.f;
        auto pequena = [&](const char* id, int r, int c, const char* t, const char* f, bool on, const char* dica, ImU32 corF = 0) {
            const int k = teclaMb(id, kx[c], ky[r], kx[c] + kw, ky[r] + kh, dica, nullptr, on);
            tx(fP(), 7.f, kx[c] + kw * 0.44f, ky[r] + 9.f, M_ROT, t, 0.5f, 0.92f, kw - 6.f);
            tx(fP(), 4.8f, kx[c] + kw - 6.f, ky[r] + kh - 6.5f, corF ? corF : IM_COL32(0xe0, 0x78, 0x50, 255), f, 1.f, 0.95f);
            return k;
        };
        if (pequena("kMic", 0, 0, "MIC", "F11", false, "MIC: (num receptor) VOICE - fala a frequência (voz do Windows)") == 1) falarFrequencia(ativo_);
        if (pequena("kCmp", 0, 1, "CMP", "F12", proc_, "CMP: compressor do áudio (PROC) - força no botão VOX/COMP com SHIFT") == 1) proc_ = !proc_;
        {
            const bool lig = sqlAuto_[ativo_];
            if (pequena("kVox", 0, 2, "VOX", "F13", lig || sqlMedindo_, "VOX: (num receptor) squelch automático\nClique: mede o ruído por 4 s e fecha o squelch logo acima\nAceso: clique de novo para abrir") == 1) {
                if (sqlMedindo_) { sqlMedindo_ = false; A.sql = -90.f; sql_ = -90.f; sqlAplicado_ = -90.f; sqlAuto_[ativo_] = false; }
                else if (lig) { A.sql = -90.f; sql_ = -90.f; sqlAplicado_ = -90.f; sqlAuto_[ativo_] = false; aviso("Squelch aberto"); }
                else if (r_.aoVivo()) { sqlMedindo_ = true; sqlAmostras_.clear(); sqlIni_ = agora(); sqlUlt_ = 0; }
            }
        }
        if (pequena("kSql", 0, 3, "SQL", "F14", A.sql > -90.f, "SQL: liga / desliga o squelch (o nível fica no MULTI 1 - saia)") == 1) {
            if (A.sql > -90.f) { sqlSalvo_[ativo_] = A.sql; A.sql = -90.f; }
            else A.sql = sqlSalvo_[ativo_] > -90.f ? sqlSalvo_[ativo_] : -60.f;
            sql_ = A.sql; sqlAplicado_ = A.sql; sqlAuto_[ativo_] = false;
        }
        if (pequena("kReq", 1, 0, "R.EQ", "F15", rxeq_ != 0, "R.EQ: equalização do áudio (OFF, agudo, mais agudo, grave, mais grave)") == 1) {
            static const float tons[5] = {50, 68, 85, 32, 15};
            static const char* nm[5] = {"R.EQ OFF", "R.EQ agudo", "R.EQ mais agudo", "R.EQ grave", "R.EQ mais grave"};
            rxeq_ = (rxeq_ + 1) % 5; tom_ = tons[rxeq_]; aviso(nm[rxeq_], 1.0);
        }
        if (pequena("kTeq", 1, 1, "T.EQ", "F16", sleepAte_ > 0, "T.EQ: (num receptor) TIMER - desligar o rádio depois de um tempo") == 1) ImGui::OpenPopup("##timerIc");
        if (pequena("kIq", 1, 2, "IQ", "F17", Config::instance().dcRemove(), "IQ: tirar o risco do DC do centro do escopo") == 1) {
            auto& c = Config::instance(); c.set("dc_remove", !c.dcRemove());
        }
        if (pequena("kXvtr", 1, 3, "XVTR", "F18", r_.xvtr() != 0, "XVTR: conversor (upconverter) - Ham It Up, SpyVerter...") == 1) ImGui::OpenPopup("##xvtrMb");
        if (pequena("kStep", 2, 0, "STEP", "F19", false, "STEP: passo do FD / FU, do D/I e da roda do mouse no escopo") == 1) ImGui::OpenPopup("##passoMb");
        auto zoomPasso = [&](int s) {
            static const float z[6] = {0, 1, 3, 7, 15, 31};
            int i = 0;
            for (int k = 0; k < 6; ++k) if (std::fabs(zoom_ - z[k]) < 0.3f) i = k;
            i = std::clamp(i + s, 0, 5);
            zoom_ = z[i]; centrarParaZoom(true);
            char a[40]; const double sp = r_.taxa() / (1.0 + zoom_);
            if (sp >= 1e6) std::snprintf(a, sizeof a, "SPAN %.2f MHz", sp / 1e6); else std::snprintf(a, sizeof a, "SPAN %.0f kHz", sp / 1000.0);
            aviso(a, 1.0);
        };
        if (pequena("kZp", 2, 1, "Z+", "F20", false, "Z+: zoom no escopo (menos largura, mais detalhe)") == 1) zoomPasso(1);
        if (pequena("kZm", 2, 2, "Z-", "F21", false, "Z-: tira o zoom do escopo") == 1) zoomPasso(-1);
        {
            const int k = pequena("kWf", 2, 3, "WF", "SHIFT", shift_,
                                  "WF / SHIFT\nClique: SHIFT - a 2a função dos botões e a 2a página das teclas de toque (LED SHIFT)\nSegurar: WF - aparência da cachoeira (cores, LEVEL, CONTRAST, velocidade)",
                                  IM_COL32(0xee, 0x9c, 0x62, 255));
            if (k == 1) shift_ = !shift_;
            if (k == 2) ImGui::OpenPopup("##smenuIc");
        }
    }
    // ---- RIT / XIT / RX2 (com os LEDs em cima) ----
    {
        const float y0 = 430, y1 = 460;
        ledMb(1280, 417, 6.f, IM_COL32(0xff, 0x9a, 0x30, 255), A.ritOn);
        ledMb(1355, 417, 6.f, IM_COL32(0xff, 0x9a, 0x30, 255), O.ritOn);
        ledMb(1427, 417, 6.f, IM_COL32(0x50, 0xf0, 0x60, 255), ativo_ == 1);
        {
            std::snprintf(d, sizeof d, "RIT: desloca a escuta do VFO ouvido (%+.2f kHz) - gire o MULTI 3\nSegurar: zera o RIT", A.rit / 1000.0);
            const int k = teclaTxt("kRit", "RIT", 1258, y0, 1305, y1, 10.5f, M_ROT, d, nullptr, A.ritOn);
            if (k == 1) A.ritOn = !A.ritOn;
            if (k == 2) { A.rit = 0; aviso("RIT zerado", 1.0); }
        }
        {
            std::snprintf(d, sizeof d, "XIT: (num receptor) desloca o OUTRO VFO (%+.2f kHz) - gire o MULTI 3\nSegurar: zera", O.rit / 1000.0);
            const int k = teclaTxt("kXit", "XIT", 1331, y0, 1379, y1, 10.5f, M_ROT, d, nullptr, O.ritOn);
            if (k == 1) O.ritOn = !O.ritOn;
            if (k == 2) { O.rit = 0; aviso("XIT zerado", 1.0); }
        }
        if (teclaTxt("kRx2", "RX2", 1403, y0, 1452, y1, 10.5f, M_ROT, ativo_ == 0 ? "RX2: passar a ouvir o VFO-B (o dial vai junto)" : "RX2: voltar para o VFO-A", nullptr, ativo_ == 1) == 1)
            ativar(1 - ativo_);
    }
    // ---- MULTI 3: RIT / XIT quando ligados; senao a funcao escolhida (clique escolhe) ----
    {
        tx(fP(), 9.5f, 1354, 478, M_ROT, "MULTI 3", 0.5f, 0.95f);
        const ImU32 cs = M_ROT;
        const float t = std::max(1.f, QD(1.6f));
        // as setas curvas em volta
        dl->PathArcTo(Q(1355, 538), QD(76), rad(-150), rad(-132), 12); dl->PathStroke(cs, 0, t);
        dl->PathArcTo(Q(1355, 538), QD(76), rad(-48), rad(-30), 12); dl->PathStroke(cs, 0, t);
        {
            const ImVec2 p0 = Q(1355 + std::cos(rad(-150)) * 76.f, 538 + std::sin(rad(-150)) * 76.f);
            dl->AddTriangleFilled(ImVec2(p0.x - QD(4), p0.y - QD(3)), ImVec2(p0.x + QD(3), p0.y + QD(1)), ImVec2(p0.x - QD(2), p0.y + QD(6)), cs);
            const ImVec2 p1 = Q(1355 + std::cos(rad(-30)) * 76.f, 538 + std::sin(rad(-30)) * 76.f);
            dl->AddTriangleFilled(ImVec2(p1.x + QD(4), p1.y - QD(3)), ImVec2(p1.x - QD(3), p1.y + QD(1)), ImVec2(p1.x + QD(2), p1.y + QD(6)), cs);
        }
        bool clique = false;
        const char* oque = A.ritOn ? "RIT" : O.ritOn ? "XIT" : nomeFuncaoMulti(multiFunc_);
        std::snprintf(d, sizeof d, "MULTI 3: %s (gire)\nClique: escolher a função (LEVEL, SPEED, CONTRAST, MEM CH, DNR, NB, AF...)\nCom RIT / XIT ligado ele gira o RIT / XIT", oque);
        const int p = encMb("eM3", 1355, 550, 49, angMulti_, d, &clique);
        if (clique) ImGui::OpenPopup("##multiIc");
        if (p) {
            if (A.ritOn) A.rit = std::clamp(A.rit + p * 10, -9990, 9990);
            else if (O.ritOn) O.rit = std::clamp(O.rit + p * 10, -9990, 9990);
            else funcaoMulti(multiFunc_, p);
        }
    }
}

// MULTI 3: o botao grande sem fim (cada 15 graus um passo); clique = sem girar
int Ui::encMb(const char* id, float cx, float cy, float r, float& ang, const char* dica, bool* clique)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 c = Q(cx, cy);
    const float R = QD(r);
    ImGui::SetCursorScreenPos(ImVec2(c.x - R, c.y - R));
    ImGui::PushID(id);
    ImGui::InvisibleButton("##enmb", ImVec2(R * 2, R * 2));
    const ImGuiID iid = ImGui::GetItemID();
    const bool sobre = ImGui::IsItemHovered(), ativo = ImGui::IsItemActive();
    ImGui::PopID();
    int passos = 0;
    static std::unordered_map<ImGuiID, float> resto, andou;
    const float aM = std::atan2(io.MousePos.y - c.y, io.MousePos.x - c.x);
    if (ImGui::IsItemActivated()) { angArrasto()[iid] = aM; andou[iid] = 0; }
    if (ativo) {
        float dd = aM - angArrasto()[iid];
        while (dd > kPi) dd -= (float)(2 * kPi);
        while (dd < -kPi) dd += (float)(2 * kPi);
        angArrasto()[iid] = aM;
        ang += dd; andou[iid] += std::fabs(dd); resto[iid] += dd;
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
    dl->AddCircleFilled(ImVec2(c.x + R * 0.05f, c.y + R * 0.10f), R * 1.03f, IM_COL32(0, 0, 0, 150), 64);
    auto luz = [](float a) { return 0.07f + 0.26f * sino(a, rad(-125), rad(48)) + 0.07f * sino(a, rad(45), rad(35)); };
    anelLuz(dl, c, R * 0.86f, R, 0.f, TWO_PI, 128, [&](float a, float rr) { return cinzaN(luz(a) * (rr > R * 0.975f ? 0.55f : 1.f)); });
    for (int k = 0; k < 90; ++k) {
        const float an = ang + k * TWO_PI / 90;
        const ImVec2 dd = dirAng(an);
        dl->AddLine(ImVec2(c.x + dd.x * R * 0.87f, c.y + dd.y * R * 0.87f), ImVec2(c.x + dd.x * R * 0.98f, c.y + dd.y * R * 0.98f),
                    cinzaN(luz(an) * 0.4f, 220), std::max(1.f, R * 0.025f));
    }
    circuloSuave(dl, c, R, 128, 1.f, [&](float a) { return cinzaN(0.10f + 0.35f * sino(a, rad(-125), rad(48)), 230); });
    // o aro fino claro e a face
    circuloSuave(dl, c, R * 0.86f, 128, std::max(1.f, QD(1.6f)), [](float a) { return cinzaN(0.50f + 0.40f * sino(a, rad(-110), rad(70)), 255); });
    discoCone(dl, c, R * 0.85f, [](float a) { return 0.10f + 0.05f * sino(a, rad(-130), rad(45)) + 0.02f * sino(a, rad(45), rad(40)); }, 1.f);
    dl->AddCircleFilled(ImVec2(c.x - R * 0.20f, c.y - R * 0.22f), R * 0.30f, IM_COL32(255, 255, 255, 10), 32);
    if (ativo) dl->AddCircle(c, R * 1.03f, alfa(P_LARANJA, 140), 64, std::max(1.f, QD(2.f)));
    if ((sobre || ativo) && dica) ImGui::SetTooltip("%s", dica);
    return passos;
}

// ===========================================================================
//  MB1SDR: as teclas de toque da tela (= F1..F10). Duas paginas: a normal e a
//  do SHIFT.
// ===========================================================================
const char* Ui::nomeSoftMb(int i) const
{
    static const char* p0[10] = {"EXPAND", "BS", "<QMEM", "QMEM>", "MODE", "SPAN", "EiBi", "DECOD", "", ""};
    static const char* p1[10] = {"AUTO", "HOLD", "CENTER", "IF", "RADAR", "SAT", "FULL", "REC", "PLAY", "MENU"};
    i = std::clamp(i, 0, 9);
    return shift_ ? p1[i] : p0[i];
}

const char* Ui::dicaSoftMb(int i) const
{
    static const char* p0[10] = {
        "EXPAND: o espectro e a cachoeira crescem juntos e ocupam a tela inteira do rádio\n(esconde o S-meter e as informações). De novo: volta ao normal",
        "BS (band stack): as bandas de radioamador e a cobertura geral (HF, VHF, UHF)",
        "<QMEM: memória rápida anterior\nSegurar: grava a frequência atual na memória rápida",
        "QMEM>: próxima memória rápida\nSegurar: grava a frequência atual na memória rápida",
        "MODE: escolher o modo - LSB, USB, CW, AM, FM, WFM e os digitais (RTTY, PSK, CW...)",
        "SPAN: a largura do escopo - taxa de amostragem de 0,25 a 3,2 MSPS e o zoom",
        "EiBi: a lista de memórias (radiodifusão no ar agora e as utilitárias)",
        "DECOD: os decodificadores digitais (RTTY, PSK, FT8, CW, DMR, ADS-B, SSTV...)",
        "Alto-falante: silencia o receptor (MUTE)",
        "Teclado: digitar a frequência (kHz, Enter confirma, Esc cancela)"};
    static const char* p1[10] = {
        "(SHIFT) AUTO: ajuste automático do espectro e da cachoeira\nSegurar: guarda a aparência como padrão",
        "(SHIFT) HOLD: congela o espectro e a cachoeira",
        "(SHIFT) CENTER: o escopo acompanha a sintonia (centro) ou fica parado",
        "(SHIFT) IF: zoom de 20 kHz em volta da sintonia, dentro do escopo",
        "(SHIFT) RADAR: o RADAR AÉREO (ADS-B, ACARS, VDL2, HFDL)",
        "(SHIFT) SAT: rastreio de satélites",
        "(SHIFT) FULL: o escopo na tela inteira (ESC volta)",
        "(SHIFT) REC: grava / para de gravar o áudio",
        "(SHIFT) PLAY: toca a última gravação\nSegurar: para de tocar",
        "(SHIFT) MENU: funções e configuração"};
    i = std::clamp(i, 0, 9);
    return shift_ ? p1[i] : p0[i];
}

void Ui::softMb(int i, bool longo)
{
    Receptor& R = rx_[ativo_];
    if (!shift_) {
        switch (i) {
        case 0: if (!longo) expand_ = !expand_; break;
        case 1: ImGui::OpenPopup("##bsMb"); break;
        case 2: case 3:
            if (longo) {
                qm_.insert(qm_.begin(), Rapida{r_.vfo(), r_.modo()});
                if (qm_.size() > 10) qm_.resize(10);
                qmIdx_ = -1;
                aviso("Gravado na memória rápida (QMEM)");
            } else if (qm_.empty()) aviso("QMEM vazia (segure <QMEM ou QMEM> para gravar)");
            else {
                const int n = (int)qm_.size();
                qmIdx_ = i == 3 ? (qmIdx_ + 1) % n : (qmIdx_ <= 0 ? n - 1 : qmIdx_ - 1);
                R.memModo = false;
                mudarModoPainel(qm_[qmIdx_].modo);
                sintonizar(qm_[qmIdx_].f, false);
                char a[48]; std::snprintf(a, sizeof a, "QMEM %d", qmIdx_ + 1); aviso(a);
            }
            break;
        case 4: ImGui::OpenPopup("##modosIc"); break;
        case 5: ImGui::OpenPopup("##spanMb"); break;
        case 6: memFiltroTipo_ = 2; memAberta_ = true; break;
        case 7: decAberta_ = !decAberta_; break;
        case 8: R.mute = !R.mute; break;
        case 9: entrando_ = !entrando_; entrada_.clear(); break;
        }
    } else {
        switch (i) {
        case 0: if (longo) padrao(); else autoCachoeira(); break;
        case 1: pausa_ = !pausa_; if (pausa_) congelado_ = suave_; break;
        case 2: modoCentro_ = !modoCentro_; if (modoCentro_) r_.centralizar(r_.centroSemDc(r_.vfo())); break;
        case 3: ifOn_ = !ifOn_; break;
        case 4: abrirRadar(); break;
        case 5: escolherDecoder(Decoders::SATELITES, true); decAberta_ = true; break;
        case 6: scp_ = 2; break;
        case 7: alternarGravacao(); break;
        case 8: if (longo) { PlaySoundA(nullptr, nullptr, 0); aviso("PLAY parado"); } else tocarUltimaGravacao(); break;
        case 9: ImGui::OpenPopup("##funcFt"); break;
        }
    }
}

// Os menus proprios do MB1 (os outros sao os do IC7610SDR: icPopups / popupsPainel)
void Ui::mb1Popups()
{
    auto& c = Config::instance();
    if (ImGui::BeginPopup("##spanMb")) {
        ImGui::TextDisabled("SPAN - largura do escopo (taxa de amostragem)");
        ImGui::Separator();
        const unsigned tx[] = {250000, 1024000, 1400000, 1800000, 1920000, 2048000, 2400000, 2560000, 2880000, 3200000};
        for (unsigned t : tx) {
            char r[64];
            std::snprintf(r, sizeof r, "%.3f MSPS  (janela de %.2f MHz)%s", t / 1e6, t / 1e6, t > 2400000 ? "  *" : "");
            if (ImGui::MenuItem(r, nullptr, c.sampleRate() == t)) { c.set("sample_rate", (long long)t); r_.aplicarConfig(); aviso(r, 1.5); }
        }
        ImGui::TextDisabled("* acima de 2,4 MSPS alguns dongles RTL-SDR perdem amostras");
        ImGui::Separator();
        ImGui::TextDisabled("ZOOM (dentro da taxa)");
        const float z[6] = {0, 1, 3, 7, 15, 31};
        for (float zz : z) {
            const double sp = r_.taxa() / (1.0 + zz);
            char t[48];
            if (sp >= 1e6) std::snprintf(t, sizeof t, "%.2f MHz", sp / 1e6); else std::snprintf(t, sizeof t, "%.0f kHz", sp / 1000.0);
            if (ImGui::MenuItem(t, nullptr, std::fabs(zoom_ - zz) < 0.3f)) { zoom_ = zz; centrarParaZoom(true); }
        }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##bsMb")) {
        ImGui::TextDisabled("BS - bandas");
        ImGui::Separator();
        for (int k = 0; k < kNB; ++k) {
            char t[40]; std::snprintf(t, sizeof t, "%s MHz", kBandasHam[k].rot);
            if (ImGui::MenuItem(t)) irBanda(k);
        }
        if (ImGui::MenuItem("144 MHz (2 m, FM)")) { rx_[ativo_].memModo = false; mudarModoPainel("NFM"); sintonizar(145000000ULL, false); }
        ImGui::Separator();
        if (ImGui::BeginMenu("GEN - cobertura geral HF")) { itensGen(0); ImGui::EndMenu(); }
        if (ImGui::BeginMenu("GEN - VHF / UHF")) { itensGen(1); ImGui::EndMenu(); }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##passoMb")) {
        ImGui::TextDisabled("STEP - passo do FD / FU e da roda do mouse no escopo");
        const int ps[] = {10, 50, 100, 500, 1000, 2500, 5000, 6250, 8330, 9000, 10000, 12500, 25000, 100000};
        for (int p : ps) if (ImGui::MenuItem(fmtPasso(p).c_str(), nullptr, passo_ == p)) passo_ = p;
        ImGui::Separator();
        if (ImGui::MenuItem("FAST: dial 10x mais rápido", nullptr, fast_)) fast_ = !fast_;
        if (ImGui::MenuItem("FINE: dial em passos de 1 Hz", nullptr, rx_[ativo_].fine)) rx_[ativo_].fine = !rx_[ativo_].fine;
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("##xvtrMb")) {
        ImGui::TextDisabled("XVTR - conversor (upconverter)");
        const struct { const char* n; uint64_t hz; } op[] = {{"Desligado", 0}, {"125 MHz (Ham It Up, SpyVerter)", 125000000},
                                                             {"120 MHz", 120000000}, {"100 MHz", 100000000}, {"106,25 MHz", 106250000}};
        for (const auto& o : op)
            if (ImGui::MenuItem(o.n, nullptr, r_.xvtr() == o.hz)) {
                r_.setXvtr(o.hz);
                if (o.hz) c.set("qmode", std::string("off"));
                r_.aplicarConfig();
            }
        ImGui::EndPopup();
    }
}
