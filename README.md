# MangaCAP Switch V0.3.2 — tentativa de compilação sem libzip

**Não é um NRO compilado.** Código-fonte para o GitHub Actions, sem instalação de pacotes via devkitPro (servidor retornava 403).

## Correção
- Substituição de libzip por leitor ZIP interno em `source/cbz_zip.cpp` com descompressão DEFLATE via zlib.
- Suporta arquivos CBZ/ZIP comuns (ZIP32, entradas armazenadas ou DEFLATE); não suporta ZIP64, criptografia ou outros métodos de compressão.
- Limite de 24 MiB por página descompactada; verificação CRC32.
- Usa SDL2 e SDL2_image já encontrados no ambiente de build.

## Como atualizar
**Importante:** envie `Makefile`, `source/main.cpp`, `source/cbz_zip.cpp`, `source/cbz_zip.h` e atualize também `.github/workflows/build.yml`. O GitHub pode não enviar a pasta oculta `.github` ao arrastar arquivos. Abra o arquivo de workflow pelo site e substitua todo seu conteúdo pelo arquivo do ZIP.

Após build verde, baixe o artifact e copie `MangaCAP.nro` para `sdmc:/switch/MangaCAP/MangaCAP.nro`. Teste via Sphaira segurando R ao abrir jogo. Mangás em `sdmc:/switch/MangaCAP/books`.

**Aviso:** não foi possível compilar nem executar no Switch neste ambiente; outros erros de compilação podem aparecer. Envie o log completo se falhar.
