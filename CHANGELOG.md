# MB1SDR - histórico

## 1.1.1 (09/11/2026)
- "Pouco ganho" no espectro e na cachoeira: o ganho de RF estava certo (medido: RF 100% = tuner 44,5 dB + ganho
  digital; em HF o ruído fica em -10 dBFS, em VHF -45 dBFS; RF 50% derruba 25-30 dB, o PRMP soma 6 dB por degrau),
  o desenho é que escondia o sinal: o topo da escala ficava 45 dB acima do ruído (o ruído colado embaixo) e a cachoeira
  azul só clareava 23 dB acima do ruído. Agora o topo fica 40 dB acima do ruído (o ruído a 1/4 da altura) e a cachoeira
  fica branca ~14 dB acima do ruído, amarela ~22 dB e vermelha ~33 dB, como a Eclipse. Quem já usava ganha o ajuste
  sozinho na primeira vez que abre.
- PRMP da tela mostra o ganho real do pré: 6 / 12 dB (SDRplay 10 / 20 dB).

## 1.1.0 (09/11/2026)
- Dois aparelhos: o VFO-B pode ter o seu próprio dongle (RTL-SDR, RTL-TCP ou SDRplay), recebendo ao mesmo tempo que o VFO-A.
  Áudio dos dois juntos (padrão), volume de cada um (AF; SHIFT + AF = AF2 do outro VFO) ou só o selecionado (MON da tela).
  Escopo do VFO-B com a janela inteira do aparelho dele; S-meter, BUSY e squelch de cada VFO no aparelho dele.
- Com dois aparelhos, botões de dois receptores: AF A / RF A, AF B / RF B e SQL A / SQL B (MULTI 1).
- A tecla Shift do teclado do PC faz o SHIFT do painel enquanto está apertada.
- Um slider de volume em pé ao lado de cada frequência da tela (VFO-A e VFO-B).
- EXPAND (F1): espectro e cachoeira na tela toda. Régua do escopo arrastável. MODE (F5) e a caixa do modo na tela.

## 1.0.0 (08/11/2026)
- Primeira versão. Mesmo motor do FTdx9000D-SDR / IC7610SDR (RTL-SDR, RTL-TCP, SDRplay, todos os decodificadores,
  Radar Aéreo, Satélites; S-meter que acompanha o áudio), com o painel novo no estilo do MB1, medido na foto do
  rádio (endireitada):
  - coluna da esquerda: POWER com os dois LEDs, TX (MUTE geral), TUN (AUTO), MUT, MON (ouve o outro VFO enquanto
    segura), AGC, ATT, PA (pré-amplificador), ANT (aparelho), a porta USB (captura), PHONES, MIC 1 (grava) e MIC 2
    (toca);
  - a tela: ícones (MENU, tela cheia, configuração, POWER), o S-meter com as duas escalas em arco e o ponteiro verde,
    LNA / PA / LOCK / IQ, HF RX / HF TX, PRMP, data e hora, RX / TX / TUNE, AGC, o modo, as duas frequências em
    7 segmentos finos (zeros da esquerda apagados), os indicadores clicáveis (RIT, XIT, COMP, SQL, VOX, MUTE, R.EQ,
    T.EQ, Sub RX, SPLIT, BIN, NR, ANF, APF, DSE, NB1, NB2, NF, STEREO), a linha de informações (STEP, FLTR, RF, VOL,
    CPU, SR, SQL, S) e o escopo: espectro verde preenchido com a coluna de dB e o nome da banda, a régua e a
    cachoeira azul (paleta nova "MB1"); Sub RX põe o escopo do VFO-B ao lado;
  - teclas de toque = F1 ... F10, com duas páginas (SHIFT): START, BS, <QMEM, QMEM>, RX ANT, SPAN, EiBi, DECOD,
    MUTE, teclado / AUTO, HOLD, CENTER, IF, RADAR, SAT, FULL, REC, PLAY, MENU;
  - teclado do meio: bandas de 160 m a 6 m e 2 m / números com ENT, REC, PLAY, M.IN, M.RD, MD, MU, FD, FU, os LEDs
    REC / IND1 / IND2 / SHIFT, NR/ESC, NB1/DEL, NB2, ANF, TNF, BIN, SUB, A>B, A<B, A<>B, TX B, SPLT;
  - as teclas de canto em volta do dial (RX2, A/B, LOCK, D/I) e o dial grande com a covinha;
  - lado direito: MIC/SPD-DRIVE, AF-RF, MON-SQL (MULTI 1) e SHIFT-WIDTH (MULTI 2), cada um com a 2a função no
    SHIFT; MIC ... XVTR, STEP, Z+, Z-, WF/SHIFT; RIT, XIT, RX2 com os LEDs e o MULTI 3.
- Sem cachoeira separada (a tela do rádio já tem): a largura do escopo vai de 0,25 a 3,2 MSPS (SPAN / F6 ou "SR:").
- Placa do modelo: "SDR Transceiver" branco (menor) + "MB1SDR" dourado; "HF/VHF/UHF SDR RECEIVER" no lugar do logotipo do fabricante;
  rádio cinza-escuro, letras brancas, moldura cinza das teclas, pés cromados.
- Sem barra de título do Windows: a tampa do rádio tem a versão e minimizar / maximizar / fechar; fundo transparente.
- Ícone próprio. Portátil (sem instalação), com as DLLs, os decodificadores e o Zadig na pasta Drivers.
- Licença GNU GPL versão 3.
