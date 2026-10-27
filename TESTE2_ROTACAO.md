# MangaCAP Switch v0.7.9 — Teste 2 (rotacao experimental)

Este pacote adiciona o botao **GIRAR 90** no canto superior direito (abaixo da barra superior). Toque para alternar entre retrato e horizontal. Atalho para teste em teclado: F9.

**Limitacao conhecida:** esta e uma primeira prova de conceito da rotacao da renderizacao e remapeamento do toque. O layout interno permanece em 1280 x 720, por isso a interface pode parecer deformada no modo retrato. A adaptacao real de biblioteca e leitor para 720 x 1280 ainda nao foi concluida.

O sistema operacional do Switch nao e alterado.

O codigo usa textura de renderizacao SDL; se nao houver suporte a SDL_TEXTUREACCESS_TARGET no renderer, a rotacao nao sera ativada.

**Teste:** mantenha backup do NRO aprovado. Compile pelo Actions; baixe em Releases (pre-release); verifique botao, toque, biblioteca, PIN e leitor. Se houver tela preta ou erro, volte ao NRO anterior e envie startup.log.

**Nao e release publica estavel.**
