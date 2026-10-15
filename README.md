# MangaCAP Switch V0.2 — teste de inicialização

**Importante:** este pacote contém o código-fonte, **não** um `.nro` pronto. A compilação automática pelo GitHub Actions produz o `MangaCAP.nro` para instalar no Switch. Este ambiente não tem o devkitPro e **não validou a compilação**.

## Como obter o `.nro` usando GitHub (sem instalar compiladores)

1. Crie um repositório GitHub e envie **o conteúdo desta pasta** para a raiz do repositório, inclusive `.github/workflows/build.yml` (a pasta `.github` é oculta no Windows).
2. Abra **Actions** > **Build MangaCAP Switch** > **Run workflow**.
3. Aguarde a execução. Se ficar verde, abra a execução e baixe o artefato **MangaCAP-Switch-v0.2-NRO**.
4. Extraia `MangaCAP.nro` do artefato e copie para `sdmc:/switch/MangaCAP/MangaCAP.nro`.
5. Coloque um `.cbz` em `sdmc:/switch/MangaCAP/books/` e abra pelo Sphaira.

Se a compilação falhar, envie o log do GitHub Actions para corrigirmos. **Não renomeie o ZIP ou código-fonte para `.nro`**, pois o Sphaira não o reconhecerá.

## Teste

A tela deve mostrar "MangaCAP SWITCH | V0.2" e listar nomes de arquivos CBZ/ZIP. Cima/Baixo navegam; X atualiza a lista; + sai. **Ainda não abre páginas**: o objetivo desta versão é confirmar a inicialização, o ícone e o acesso ao microSD.

## Compilação local alternativa

Com `devkitPro`, `devkitA64` e `libnx` instalados: `make` na raiz do projeto. O `Makefile` segue o padrão `switch_rules` de projetos homebrew. Dependendo da versão do SDK, podem ser necessários ajustes.

Este protótipo não inclui nem reutiliza código do Themezer-NX ou HATS Tools.


## V0.3.1: correção de compilação
O GitHub Actions deve instalar as bibliotecas Switch SDL2, SDL2_image, libzip e zlib antes de compilar. O arquivo `source/main.cpp` usa os cabeçalhos `SDL2/SDL.h` e `SDL2/SDL_image.h`.

**Atenção:** substitua também `.github/workflows/build.yml` no GitHub. Se a pasta oculta não for enviada pelo navegador, abra o arquivo no site, clique no lápis (Edit), cole o conteúdo deste ZIP e faça Commit. O `build.yml` antigo não instala as dependências.

Não foi possível compilar para Switch neste ambiente; a correção precisa ser verificada pelo GitHub Actions.
