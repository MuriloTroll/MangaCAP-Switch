# MangaCAP Switch 📚🎮

**Seus mangás e livros, do seu jeito, no Nintendo Switch.**

O **MangaCAP Switch** é um leitor gratuito de mangás e livros para Nintendo Switch com ambiente homebrew. Organize sua coleção, escolha uma capa e continue a leitura de onde parou — na horizontal ou na vertical.

> **Versão pública:** v1.0.0 · **Idioma:** português brasileiro · **Distribuição:** gratuita

## O que você encontra

- 📚 **Biblioteca em grade**, com capas grandes, rolagem vertical e indicador de posição.
- 🔄 **Leitura horizontal e vertical**, com botão para girar apenas a interface do aplicativo.
- 📖 **Suporte a CBZ, PDF, EPUB e TXT**.
- 🔖 **Progresso de leitura** para retomar os arquivos.
- ⬆️ **Botão Topo**, para voltar rapidamente ao início da biblioteca ou do capítulo.
- 🗂️ **Navegação por volumes e capítulos**, com controles na tela.
- 🔒 **Biblioteca privada com PIN**.
- ❤️ **Apoio voluntário por Pix**, acessível pelo coração na biblioteca. Todas as funções continuam gratuitas.

## Instalação

1. Tenha um Nintendo Switch configurado para executar aplicativos homebrew.
2. Na seção **Releases** deste repositório, baixe o arquivo **`MangaCAP.nro`** da versão `v1.0.0`.
3. Copie o arquivo para o cartão microSD em **`/switch/MangaCAP.nro`**.
4. Crie a pasta **`/switch/MangaCAP/books/`** e coloque nela seus arquivos CBZ, PDF, EPUB e TXT.
5. Inicie o MangaCAP pelo Homebrew Menu.

**Estrutura de exemplo:**

```text
microSD/
└── switch/
    ├── MangaCAP.nro
    └── MangaCAP/
        └── books/
            ├── Volume 01.cbz
            ├── Livro.pdf
            ├── Romance.epub
            └── Notas.txt
```

O aplicativo também utiliza a pasta `/switch/MangaCAP/` para dados de progresso, capas e biblioteca privada. **Não apague essa pasta ao atualizar**, para evitar perder dados locais.

## Como usar

Na biblioteca, toque na capa para abrir um título. Arraste para cima ou para baixo para explorar a coleção. Use a seta **Topo** para voltar ao início e o botão **Girar** para alternar a orientação do aplicativo. Durante a leitura, toque na tela para revelar os controles e o nome do arquivo atual.

## Apoie o projeto ❤️

O MangaCAP é gratuito. Se ele foi útil para você, toque no **coração** na biblioteca para visualizar um QR Code Pix de contribuição voluntária. O QR aparece mesmo sem internet; para efetuar uma contribuição, utilize o aplicativo do seu banco e **confira o destinatário antes de confirmar**. Não é necessário doar para usar qualquer recurso.

## Avisos

- O MangaCAP é um **leitor**: não inclui nem distribui mangás, livros ou outros conteúdos protegidos. Utilize arquivos que você tenha direito de acessar.
- É um projeto independente, **não oficial e não afiliado à Nintendo**.
- O uso requer um ambiente homebrew previamente configurado; não são fornecidas instruções para modificar o console.
- A compatibilidade pode variar conforme o arquivo e o ambiente do dispositivo.

## Desenvolvimento

Código-fonte, histórico de alterações e builds de teste podem ser consultados neste repositório. A primeira versão pública é a **v1.0.0**; os testes anteriores fazem parte do histórico de desenvolvimento.
