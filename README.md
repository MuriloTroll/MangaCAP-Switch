# MangaCAP Switch V0.3 — primeiro leitor experimental

**Não é um NRO pronto.** Este ZIP contém o projeto para o GitHub compilar, como ocorreu na V0.2. O build da V0.3 ainda **não foi executado/validado**; pode exigir ajustes nas dependências e no código.

## Mudanças
- Biblioteca gráfica simples preto/vermelho com nomes de arquivos CBZ/ZIP.
- Abrir imagens PNG/JPEG/WebP contidas em arquivos CBZ/ZIP (dependente dos decodificadores presentes no build).
- Direita/esquerda muda página; L/R altera zoom; B retorna à biblioteca.
- Progresso por volume em `sdmc:/switch/MangaCAP/progress.tsv`.
- Log de inicialização em `sdmc:/switch/MangaCAP/startup.log` para investigar falhas.
- Não extrai arquivos para o microSD e limita imagens descompactadas a 24 MB.

## Controles
- Biblioteca: cima/baixo para selecionar, A para abrir, X para atualizar, + para sair.
- Leitor: esquerda/direita para páginas, L/R zoom, B voltar, + sair.

## Instalação após build bem-sucedido
1. Envie **todos** os arquivos desta versão para a raiz do seu repositório `MangaCAP-Switch`, substituindo os existentes, inclusive `.github/workflows/build.yml`.
2. Em GitHub Actions, execute `Build MangaCAP Switch` ou aguarde a execução do push.
3. Em `Artifacts`, baixe `MangaCAP-Switch-v0.3-NRO`.
4. Extraia o `MangaCAP.nro` e substitua o anterior em `sdmc:/switch/MangaCAP/MangaCAP.nro`.
5. Mantenha os CBZ em `sdmc:/switch/MangaCAP/books`.
6. Abra pelo Sphaira usando **segurar R ao iniciar um jogo**, que foi o método validado para a V0.2.

## Limitações
- Interface simples, sem capas em grade, sem rolagem contínua, sem suporte a toque, sem ordenação natural de páginas.
- ZIPs com imagens muito grandes podem falhar por limite de memória/decodificação.
- Apenas arquivos CBZ/ZIP diretamente na pasta books, não subpastas.
- Se o app fechar, procure `startup.log` no cartão microSD e compartilhe seu conteúdo.
- Este código ainda não foi compilado com o toolchain do Switch neste ambiente. O GitHub Actions será nosso teste de compilação.
