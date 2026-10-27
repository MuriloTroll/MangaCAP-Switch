<div align="center">

# 📚 MangaCAP Switch

### Seus mangás e livros, do seu jeito.

**v1.0.0 · Gratuito · Português brasileiro**

Leitor de mangás e livros para consoles com ambiente homebrew, com biblioteca organizada, leitura em duas orientações e progresso salvo.

**[⬇️ Baixar a versão 1.0.0](../../releases/tag/v1.0.0)** · **[📖 Manual completo](GUIA_INICIANTE.md)**

</div>

---

## ✨ O que o MangaCAP oferece

- 📚 **Biblioteca com capas grandes**, organizada em grade, com rolagem por toque e indicador lateral.
- 🔄 **Leitura horizontal e vertical**, sem alterar a orientação do sistema.
- 📖 **CBZ, PDF, EPUB e TXT** para ler seus arquivos locais.
- 🔖 **Progresso salvo** para continuar de onde parou.
- ⬆️ **Botão Topo**, navegação entre volumes e controles que aparecem ao tocar na tela.
- 🔍 **Zoom, deslocamento e modo de leitura contínua ou paginada.**
- 🔒 **Biblioteca privada com PIN de seis dígitos** e código de recuperação.

> **Importante:** o MangaCAP é um leitor de arquivos locais. Não inclui nem distribui mangás ou livros. O console precisa estar previamente preparado para executar homebrew.

## 🚀 Como instalar

1. Acesse **[Releases](../../releases)** e baixe `MangaCAP.nro` da versão `v1.0.0`.
2. Utilize seu método habitual e seguro de acesso ao microSD.
3. Abra a pasta `switch` no cartão.
4. Copie o arquivo para **`switch/MangaCAP.nro`**.
5. O aplicativo criará suas pastas de trabalho na primeira execução.
6. No gerenciador homebrew (por exemplo, **Sphaira**), encontre o ícone do **MangaCAP 1.0.0** e abra-o.
7. Depois da primeira execução, reconecte o cabo USB e abra **`switch/MangaCAP/`**. Copie seus arquivos para a pasta **`books`**.
8. Desconecte com segurança e inicie o MangaCAP. Sua coleção aparecerá na biblioteca.

### 📂 Onde colocar os mangás?

Organize os arquivos por obra, com os volumes dentro de cada pasta:

```text
switch/
├── MangaCAP.nro
└── MangaCAP/
    ├── books/
    │   ├── Berserk/
    │   │   ├── Volume 01.cbz
    │   │   └── Volume 02.cbz
    │   ├── Dandadan/
    │   │   └── Volume 01.cbz
    │   └── Meu Livro.epub
    └── private/
        └── (arquivos da biblioteca privada)
```

Não precisa extrair os arquivos `.cbz` compatíveis. Para adicionar novos títulos, copie-os para `books` e reinicie o aplicativo.

## 🎮 Controles principais

| Na biblioteca | Ação |
|---|---|
| **Direcional ← → ↑ ↓** | Navegar entre capas e linhas |
| **A** | Abrir obra ou volume |
| **B** | Voltar |
| **–** | Acessar/alternar biblioteca privada |
| **+** | Sair do aplicativo |
| **Toque e arrasto** | Selecionar capas ou rolar a biblioteca |
| **⬆️ Topo** | Voltar ao início da lista |
| **🔄 Girar** | Alternar orientação |

| Durante a leitura | Ação |
|---|---|
| **← / →** | Página anterior / próxima página |
| **Y** | Alternar leitura contínua e paginada |
| **L / R** | Diminuir / aumentar zoom |
| **Analógicos** | Deslocar a página |
| **B** | Voltar à biblioteca |
| **Toque na tela** | Mostrar/ocultar controles e nome do arquivo |
| **⬆️ Topo** | Voltar ao início do volume |

As funções de atualização por **Y** e limpeza de capas por **X** foram removidas da biblioteca na versão final; **Y continua funcionando durante a leitura**.

## 🔐 Biblioteca privada

O MangaCAP permite manter arquivos separados na pasta `switch/MangaCAP/private/`. No primeiro acesso, pressione **–** e siga as instruções na tela para criar um **PIN de seis dígitos**. Guarde o **código de recuperação de 12 dígitos** apresentado pelo aplicativo.

**Atenção:** o PIN protege o acesso pela interface, mas **não criptografa** os arquivos armazenados no microSD.

Para o passo a passo de criação do PIN, recuperação, atalhos e organização de volumes, consulte o **[Guia completo do iniciante](GUIA_INICIANTE.md)**.

## 💾 Atualizações e backup

Ao atualizar, substitua somente `switch/MangaCAP.nro`, preservando a pasta `switch/MangaCAP/` com livros, dados privados e progresso. Faça backup antes de qualquer atualização.

## 📄 Sobre o projeto

O MangaCAP é um projeto independente, gratuito e não oficial. Utilize arquivos que você tenha direito de acessar. Contribuições são opcionais e não desbloqueiam funcionalidades.

---

**Feito para quem quer ler no Nintendo Switch.** Se o MangaCAP foi útil, o botão ❤️ permite agradecer com uma contribuição voluntária. Boa leitura! 📚
