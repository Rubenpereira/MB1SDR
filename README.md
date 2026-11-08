# MB1SDR

Receptor SDR para Windows com a cara de um transceptor SDR de HF/VHF de mesa no estilo do **MB1** (sem a marca do fabricante; **MB1SDR** dourado no lugar da placa): a tela grande com o **S-meter de ponteiro**, as duas frequências, os indicadores, o **espectro verde** e a **cachoeira azul**; o teclado de bandas, o **dial grande**, os quatro botões duplos, as teclas F1 a F21 e o MULTI 3. Rádio cinza-escuro, letras brancas e as molduras dos botões como no original.

A tela do rádio já tem o espectro e a cachoeira, então não há cachoeira separada: a **largura do escopo vai de 0,25 a 3,2 MSPS** (tecla de toque SPAN / F6), com zoom (Z+ / Z-). A tecla **EXPAND (F1)** faz o espectro e a cachoeira crescerem juntos e ocuparem a tela toda do rádio.

![MB1SDR](MB1SDR.png)

## Radar Aéreo

Tela de radar em tela cheia, com a agulha girando 360°, que mostra os aviões que o dongle está ouvindo pelo **ADSB, ACARS, VDL2 e HFDL**. Mostra também as estações de terra do HFDL que transmitiram e a posição pela internet (adsb.lol) de quem foi ouvido sem posição. O alcance vai de 10 km até o planeta inteiro.

## Download

Baixe o `.zip` em **[Releases](https://github.com/Rubenpereira/MB1SDR/releases)**, descompacte onde quiser e rode o `MB1SDR.exe`.

**Não precisa instalar.** A configuração fica no `MB1SDR.ini`, ao lado do programa. Roda no Windows 7 SP1, 8, 10 e 11.

As DLLs do RTL-SDR já vêm junto. Para o dongle RTL-SDR no USB, instale uma vez o driver WinUSB com o **Zadig**, que vem na pasta `Drivers` do pacote (passo a passo no `COMO_INSTALAR_DRIVER.txt`). O RTL-TCP não precisa de driver, e o SDRplay usa a [API oficial](https://www.sdrplay.com/api/) do fabricante.

## O que tem

- **Aparelhos:** RTL-SDR (USB), RTL-TCP (rede) e SDRplay (com a [API oficial](https://www.sdrplay.com/api/)), escolhidos na tecla ANT.
- **Largura do escopo:** 0,25 / 1,024 / 1,4 / 1,8 / 1,92 / 2,048 / 2,4 / 2,56 / 2,88 / 3,2 MSPS, mais o zoom.
- **ADS-B pela rede:** o Radar Aéreo também mostra os aviões de outro receptor da sua rede (FlightAware/PiAware, dump1090, readsb, FR24...) pela porta Beast 30005. O MB1SDR só lê, e o dongle fica livre.
- **Modos:** LSB, USB, CW, AM, FM estreito e FM largo - na caixa verde do modo, na tela, ou na tecla de toque MODE (F5).
- **Cada botão faz a função do rádio.** Os de transmissão, que num receptor não servem, ganharam funções de recepção. Pare o mouse em cima de qualquer botão: a dica diz o que ele faz.
- **SHIFT** (tecla WF): a segunda função dos botões duplos (VOX/COMP, TNF.P/TNF.W, NB1/NB2) e a segunda página das teclas de toque (AUTO, HOLD, CENTER, IF, RADAR, SAT, FULL, REC, PLAY, MENU).
- **Dois VFOs (A e B):** A/B, RX2, A>B, A<B, A<>B, TX B, SPLT, SUB (o B acompanha o A), RIT e XIT, e o escopo do VFO-B ao lado (RX2 / Sub RX).
- **Filtros:** SHIFT-WIDTH (MULTI 2), FLTR na tela, três filtros por modo.
- **Redução de ruído e interferência:** NB1 e NB2, NR (redutor espectral) e NR1, ANF (notch automático), TNF (notch manual com posição e largura), CONT, BIN (cancelador de apitos), APF e COMP.
- **Frequência:** teclas de banda (160 m a 6 m e 2 m) com memória por banda, digitação pelo teclado do rádio (ENT) ou do PC, roda do mouse em cada dígito, FD / FU, BS e a cobertura geral (HF, VHF e UHF).
- **Memórias:** M.IN, M.RD, MD / MU e as memórias rápidas (QMEM); EiBi abre a lista com a radiodifusão no ar.
- **Gravação:** REC (ou MIC 1) grava, PLAY (ou MIC 2) toca.
- **Outros:** squelch automático (VOX), TIMER (T.EQ), equalização (R.EQ), XVTR (upconverter), voz da frequência (MIC), captura da tela (F12 ou a porta USB).
- **Decodificadores:** FT8 e FT4 (lista igual à do WSJT-X, com SNR, DT e frequência), CW, RTTY, SITOR-B/NAVTEX, DSC, ALE, PACTOR, PSK (BPSK31/63/125/250, QPSK31/63/125, BPSK63F e BPSK125F), DMR/P25/NXDN/dPMR/YSF/D-STAR, TETRA, HFDL, AIS, APRS, ACARS, VDL2, SELCAL (chamada seletiva dos aviões), DRM, SSTV, WEFAX, satélites e analisador de sinal.

O guia completo dos botões (parte por parte do painel) está no [LEIA-ME.txt](LEIA-ME.txt).

## Compilar

Precisa do Visual Studio 2022 (ou Build Tools) com C++ e do CMake. Rode `COMPILAR.bat`: o programa sai em `pronto\MB1SDR.exe`.

Os decodificadores externos (dsd-fme, AIS-catcher, direwolf etc.) não estão neste repositório. Eles vêm no `.zip` da release; veja o [TERCEIROS.txt](TERCEIROS.txt).

## Licença

Software livre, **GNU GPL versão 3**. Veja o [LICENSE.txt](LICENSE.txt). Os programas de terceiros seguem as licenças deles ([TERCEIROS.txt](TERCEIROS.txt)).

## Privacidade / Privacy

This program will not transfer any information to other networked systems unless specifically requested by the user or the person installing or operating it. It connects to the RTL-TCP server the user configures (and, if the user turns it on, to the ADS-B receiver on the local network that the user types in Configuration), and opens web links (for example aprs.fi or FlightAware) only when the user clicks them.

Online lookups are **off by default**. They run only after the user turns them on, and the choice is saved in `MB1SDR.ini`:

- **"Look up registration and model on the web (hexdb.io)"** in the ADSB decoder sends the ICAO address or the registration of the aircraft heard to hexdb.io.
- **"Position from the internet (adsb.lol)"** on the Aerial Radar screen sends the ICAO address, registration or flight of the aircraft heard to adsb.lol, and also a position when completing an HFDL flight number. If the user enters their own ADS-B Exchange API key, the same lookups also go to ADS-B Exchange (RapidAPI).

The satellite tracker downloads public orbital elements (TLE) from Celestrak when the user opens it and the local file is missing or more than a day old, or when the user clicks "Update TLE". Nothing about the user is sent.

*Este programa não envia nenhuma informação para outros computadores, a não ser quando o próprio usuário pede. Ele se conecta ao servidor RTL-TCP que o usuário configurar (e, se o usuário ligar, ao receptor de ADS-B da rede local que ele digitar na Configuração) e só abre links da internet quando o usuário clica neles.*

*As consultas pela internet vêm **desligadas**. Só funcionam depois que o usuário liga, e a escolha fica guardada no `MB1SDR.ini`:*

- *"Buscar prefixo e modelo na internet (hexdb.io)", no decodificador ADSB, manda ao hexdb.io o código ICAO ou o prefixo dos aviões ouvidos.*
- *"Posição pela internet (adsb.lol)", na tela do Radar Aéreo, manda ao adsb.lol o código ICAO, o prefixo ou o voo dos aviões ouvidos, e uma posição quando completa o voo do HFDL. Com a chave do ADS-B Exchange do próprio usuário, as mesmas consultas vão também para o ADS-B Exchange (RapidAPI).*

*O rastreador de satélites baixa da Celestrak os elementos orbitais públicos (TLE) quando o usuário abre essa tela e o arquivo não existe ou tem mais de um dia, ou quando o usuário clica em "Atualizar TLE". Nada sobre o usuário é enviado.*

## Desinstalar / Uninstall

MB1SDR is portable: it is not installed and changes nothing in Windows. To remove it, delete its folder.

*O MB1SDR é portátil: não instala nada no Windows. Para remover, basta apagar a pasta dele.*
