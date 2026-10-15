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
