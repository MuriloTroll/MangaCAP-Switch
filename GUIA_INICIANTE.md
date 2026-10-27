# Guia completo do usuário — MangaCAP Switch 📚🎮

**Seus mangás e livros, do seu jeito, no Nintendo Switch.**

**Versão pública:** 1.0.0 · **Idioma:** português brasileiro · **Preço:** gratuito · **Plataforma:** Nintendo Switch com ambiente homebrew.

O MangaCAP é um leitor independente de arquivos locais. Permite organizar obras e volumes em uma biblioteca com capas, ler em modo horizontal ou vertical, continuar de onde parou e guardar arquivos em uma biblioteca privada protegida por PIN. Não acompanha mangás nem livros.

## Índice

1. [Recursos](#recursos)
2. [Antes de instalar](#antes-de-instalar)
3. [Instalação passo a passo](#instalação-passo-a-passo)
4. [Como transferir mangás e livros](#como-transferir-mangás-e-livros)
5. [Como organizar obras e volumes](#como-organizar-obras-e-volumes)
6. [Biblioteca e botões da tela](#biblioteca-e-botões-da-tela)
7. [Controles e atalhos](#controles-e-atalhos)
8. [Como ler e continuar a leitura](#como-ler-e-continuar-a-leitura)
9. [Biblioteca privada e criação do PIN](#biblioteca-privada-e-criação-do-pin)
10. [Recuperação e alteração do PIN](#recuperação-e-alteração-do-pin)
11. [Doações por Pix](#doações-por-pix)
12. [Atualização e backup](#atualização-e-backup)
13. [Problemas comuns](#problemas-comuns)
14. [Informações e avisos](#informações-e-avisos)

## Recursos

- Biblioteca em grade de três colunas, com capas grandes e rolagem vertical em ambas as orientações.
- Indicador de posição na lateral direita da biblioteca.
- Leitura em horizontal e vertical: o aplicativo gira a própria interface, sem alterar a orientação do sistema do Switch.
- Arquivos **CBZ, PDF, EPUB e TXT**; o scanner também reconhece arquivos **ZIP** compatíveis com o leitor de imagens.
- Abertura de obras e volumes, navegação entre volumes adjacentes e retorno ao topo.
- Progresso salvo localmente e retomada de leitura.
- Biblioteca privada com PIN de seis dígitos e código de recuperação.
- Botão de apoio voluntário por Pix, sem exigir conexão do Switch à internet.

## Antes de instalar

Você precisa de um Nintendo Switch que **já consiga executar aplicativos homebrew**, de um cartão microSD acessível e de um computador ou gerenciador de arquivos para copiar os dados. O MangaCAP não modifica o console nem instala o ambiente homebrew.

## Instalação passo a passo

1. Acesse **Releases** neste repositório e baixe **`MangaCAP.nro`** da versão `v1.0.0`.
2. Desligue o console ou utilize seu método habitual e seguro de acesso ao microSD.
3. No cartão, abra a pasta `switch` (crie-a se não existir).
4. Copie `MangaCAP.nro` diretamente para `switch/MangaCAP.nro`.
5. Crie `switch/MangaCAP/books/` para a biblioteca comum. O aplicativo também cria suas pastas de trabalho quando é executado.
6. Copie os arquivos de leitura para `books` e abra o **Homebrew Menu**.
7. Inicie **MangaCAP**. Sua coleção aparecerá na biblioteca.

**Exemplo de estrutura do cartão:**

```text
microSD/
└── switch/
    ├── MangaCAP.nro
    └── MangaCAP/
        ├── books/
        │   ├── Berserk/
        │   │   ├── Berserk - Volume 01.cbz
        │   │   └── Berserk - Volume 02.cbz
        │   ├── Bleach/
        │   │   └── Bleach - Volume 01.cbz
        │   ├── Livro.pdf
        │   ├── Romance.epub
        │   └── Anotacoes.txt
        ├── private/
        │   └── (arquivos da biblioteca privada)
        ├── covers/
        │   └── (capas em cache)
        ├── security.dat
        ├── progress.tsv
        └── page_counts.tsv
```

Os arquivos `.dat` e `.tsv` são criados e utilizados pelo aplicativo conforme necessário; você **não precisa criá-los manualmente**.

## Como transferir mangás e livros

1. No computador, localize seus arquivos **CBZ, PDF, EPUB ou TXT**.
2. Acesse o microSD e entre em `switch/MangaCAP/books/`.
3. Arraste os arquivos para essa pasta ou crie uma subpasta com o nome da obra.
4. Ejete o cartão corretamente e abra o MangaCAP.
5. Se o programa já estiver aberto, pressione **Y** na biblioteca para atualizar a lista.

**Dica para CBZ:** normalmente é um arquivo que contém imagens de páginas organizadas em sequência. Não é preciso extrair um CBZ compatível antes de copiá-lo. Para evitar páginas fora de ordem, use nomes como `001.jpg`, `002.jpg`, `003.jpg` ao criar seu próprio arquivo.

**Não coloque os mangás na pasta `switch/` ao lado do `.nro`:** os arquivos da biblioteca comum pertencem a `switch/MangaCAP/books/`.

## Como organizar obras e volumes

A maneira mais previsível de agrupar uma série é criar uma pasta para ela:

```text
books/
├── Dandadan/
│   ├── Dandadan - Volume 01.cbz
│   ├── Dandadan - Volume 02.cbz
│   └── Dandadan - Volume 03.cbz
├── Berserk/
│   ├── Berserk - Volume 01.cbz
│   └── Berserk - Volume 02.cbz
└── Livros/
    ├── Frankenstein.epub
    └── Outro livro.pdf
```

Na tela inicial você seleciona a **obra**; na tela seguinte escolhe o **volume**. Arquivos soltos também são reconhecidos, mas organizar em pastas ajuda a manter a coleção limpa. Os arquivos são listados por nome: prefira numeração com zeros (`01`, `02`, `03`...). A varredura tem limites de profundidade e quantidade de arquivos; evite estruturas excessivamente aninhadas.

## Biblioteca e botões da tela

| Ícone ou elemento | Função |
|---|---|
| ❤️ **Coração** | Abre a janela de contribuição voluntária por Pix na biblioteca principal. |
| ⬆️ **Topo** | Volta imediatamente ao início da lista de obras ou volumes. |
| 🔄 **Girar** | Alterna a orientação da interface entre horizontal e vertical. |
| **Capa** | Abre a obra; na lista de volumes, abre o arquivo selecionado. |
| **Barra lateral direita** | Indica a posição atual ao rolar a biblioteca. |
| **Voltar** | Retorna da lista de volumes à lista de obras. |

Arraste o dedo **para cima ou para baixo** para rolar a biblioteca. Um toque curto abre a capa; arrastar não deve abrir um volume. A grade e a rolagem funcionam tanto na horizontal quanto na vertical.

## Controles e atalhos

> **Observação importante:** no código do Switch, o mapeamento físico de **A/B** chega invertido pelo SDL2. A tabela abaixo usa o comportamento documentado na interface do aplicativo, e não somente os nomes internos do código.

### Na biblioteca

| Controle | Ação |
|---|---|
| **Direcional ← / →** | Move a seleção entre capas. |
| **Direcional ↑ / ↓** | Move a seleção entre linhas. |
| **B (confirmar na interface)** | Entra na obra selecionada ou abre o volume. |
| **A (voltar na interface)** | Volta da lista de volumes à biblioteca de obras. |
| **Y** | Atualiza a biblioteca, útil após copiar novos arquivos. |
| **X** | Descarta a capa em cache da obra selecionada e tenta carregá-la novamente. |
| **L** | Abre a janela de apoio por Pix na biblioteca principal. |
| **Botão – / Back (quando reconhecido)** | Abre ou alterna a biblioteca privada; pode solicitar o PIN. |
| **Clique no analógico direito** | Acessa funções de segurança, conforme a tela e o estado do PIN. |
| **+ / Start** | Sai do aplicativo quando a janela de doação não está aberta. |

### Durante a leitura

| Controle | Ação |
|---|---|
| **Toque curto na tela** | Mostra ou esconde os controles de leitura. |
| **Setas ← / →** | Página anterior ou próxima página. |
| **Y** | Alterna modo de leitura contínua/paginada. |
| **L / R** | Diminui ou aumenta o zoom. |
| **Analógicos** | Ajustam o deslocamento da página conforme o modo de leitura. |
| **A (voltar na interface)** | Fecha o leitor e volta à biblioteca. |
| **Topo (ícone ⬆️)** | Volta ao início do volume atual. |
| **Anterior / Próximo na tela** | Navega entre volumes ou capítulos adjacentes, quando disponíveis. |
| **Girar (🔄)** | Alterna a orientação do leitor. |

Os botões na tela são especialmente úteis no modo touchscreen. Alguns atalhos podem variar conforme o mapeamento do controle ou a configuração do ambiente homebrew.

## Como ler e continuar a leitura

1. Escolha uma obra na biblioteca.
2. Selecione o volume desejado.
3. Toque na tela durante a leitura para revelar o painel de controles.
4. Na parte superior aparece **`LENDO: nome-do-arquivo`**, indicando qual volume está aberto.
5. Use as setas de capítulo, o botão Topo, o zoom e o modo contínuo conforme sua preferência.
6. Ao sair do leitor, o aplicativo registra o progresso para permitir retomar a leitura posteriormente.

A posição é salva localmente no microSD. O comportamento de renderização pode variar conforme o formato e a estrutura do arquivo.

## Biblioteca privada e criação do PIN

A biblioteca privada serve para **separar arquivos da biblioteca comum e controlar o acesso dentro do MangaCAP**. Ela **não criptografa** os arquivos: quem tiver acesso direto ao cartão microSD poderá encontrar os arquivos na pasta privada.

### 1. Copiar os arquivos privados

No microSD, crie ou abra:

```text
switch/MangaCAP/private/
```

Copie para ela os livros ou mangás que deseja manter fora da biblioteca comum. Você pode criar subpastas para organizar as obras. **Não basta criar uma pasta chamada `private` dentro de `books`:** use a pasta `private` ao lado de `books`.

### 2. Criar a senha pela primeira vez

1. Abra o MangaCAP e vá para a biblioteca principal.
2. Pressione o **botão – / Back** (quando reconhecido pelo controle) para acessar a área privada.
3. Se ainda não houver PIN configurado, aparecerá **`CRIAR PIN DE 6 DIGITOS`**.
4. Digite seis números no teclado exibido na tela e confirme em **OK**.
5. Repita os mesmos seis números na tela **`CONFIRMAR PIN`**.
6. O aplicativo exibirá um **código de recuperação de 12 dígitos**. **Anote-o e guarde fora do Switch.**
7. Confirme que guardou o código para entrar na biblioteca privada.

No teclado da senha, use **APAGAR** para corrigir e **OK** para confirmar. Pelo controle, as setas ↑/↓ alteram o dígito selecionado, **B** adiciona o dígito, **Y** apaga e **X** confirma a entrada; **A** volta.

### 3. Acessar e sair da biblioteca privada

- Ao tentar entrar novamente, informe seu **PIN de seis dígitos**.
- Para retornar à biblioteca comum, use o atalho de alternância da área privada.
- A biblioteca comum e a privada possuem conjuntos separados de arquivos, em `books/` e `private/`.

**Segurança:** o aplicativo armazena verificadores da senha no arquivo `security.dat`, mas não criptografa os livros. Não divulgue o PIN nem o código de recuperação.

## Recuperação e alteração do PIN

**Esqueceu o PIN?** Na tela de segurança, use a opção **RECUPERAR COM CODIGO** (atalho **X**, quando exibido). Informe o **código de recuperação de 12 dígitos**, confirme e cadastre um novo PIN de seis dígitos. Ao redefinir, o aplicativo gera **um novo código de recuperação**, que também deve ser guardado.

**Quer mudar o PIN?** Abra as **Configurações de segurança** pela função do clique no analógico direito quando disponível. Escolha **ALTERAR PIN**, informe o PIN atual, cadastre e confirme o novo. Guarde o novo código de recuperação.

Se você perder **tanto o PIN quanto o código de recuperação**, não há uma recuperação garantida dentro do aplicativo. **Não apague `security.dat` por tentativa e erro sem fazer backup do microSD.**

## Doações por Pix

O MangaCAP é **inteiramente gratuito**. Na biblioteca principal, toque no botão ❤️ ou pressione **L** para abrir o QR Code Pix. A janela **permanece aberta** até você tocar fora dela ou usar o comando de voltar. O QR Code é incorporado ao aplicativo: o Switch não precisa de internet para exibi-lo.

A doação é opcional e de valor livre. Use o aplicativo do seu banco para escanear e **confira o destinatário e os dados antes de confirmar**. Nenhum recurso é condicionado à contribuição.

## Atualização e backup

1. Faça uma cópia da pasta `switch/MangaCAP/` do cartão microSD, especialmente `books/`, `private/`, `security.dat` e `progress.tsv`.
2. Baixe o novo `.nro` de uma Release oficial do projeto.
3. Substitua **apenas** `switch/MangaCAP.nro`, salvo orientação específica da Release.
4. Preserve as pastas de livros, a biblioteca privada e os arquivos de progresso.
5. Abra o aplicativo e confira se os títulos e a leitura continuam acessíveis.

**Não exclua `switch/MangaCAP/` para “reinstalar” o aplicativo sem antes fazer backup.**

## Problemas comuns

| Problema | O que verificar |
|---|---|
| **O aplicativo não aparece no Homebrew Menu** | Confirme que o arquivo está em `switch/MangaCAP.nro` e que o console já executa homebrew. |
| **A biblioteca está vazia** | Verifique `switch/MangaCAP/books/`, as extensões e se os arquivos estão realmente no microSD; pressione **Y** para atualizar. |
| **Uma obra não agrupa os volumes** | Coloque todos os volumes dentro da mesma subpasta da obra e use nomes consistentes. |
| **Capas não aparecem** | Selecione a obra e pressione **X** para descartar a capa em cache e tentar novamente. Alguns arquivos não fornecem capa automaticamente. |
| **O toque abre um volume em vez de rolar** | Arraste verticalmente o dedo por uma distância perceptível; confirme que está usando a versão pública mais recente. |
| **Não consigo acessar a área privada** | Verifique o atalho de alternância e seu PIN. Se esqueceu, utilize o código de recuperação. |
| **Perdi o progresso após atualizar** | Confira se a pasta `switch/MangaCAP/` e o arquivo `progress.tsv` foram preservados. |
| **O Pix não fecha** | Toque fora da janela ou use o comando de voltar; o QR foi feito para permanecer aberto após um toque simples. |
| **Um PDF/EPUB/CBZ não abre** | Teste outro arquivo válido e confirme se não está corrompido. A compatibilidade varia conforme o conteúdo. |

## Informações e avisos

- **Projeto independente**, não oficial e não afiliado à Nintendo.
- O MangaCAP é um **leitor local**: não fornece, hospeda ou distribui mangás ou livros.
- Utilize somente arquivos que você tenha direito de acessar.
- O programa não instala homebrew nem ensina a modificar o console.
- O PIN restringe acesso **na interface**, mas **não criptografa** os arquivos do microSD.
- Não há garantia de compatibilidade com todo arquivo ou configuração de Switch.
- O histórico de desenvolvimento pode conter versões de teste; para instalar, escolha a **Release pública estável**.

---

**Feito para quem quer ler no Nintendo Switch.** Se o MangaCAP foi útil, o botão ❤️ permite agradecer com uma contribuição voluntária. Boa leitura! 📚
