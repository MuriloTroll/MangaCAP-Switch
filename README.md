# MangaCAP Switch V0.7.8 (experimental)

Base: V0.7.7. A biblioteca privada requer PIN de 6 digitos.

- Pressione **-** na biblioteca para acessar a area privada. Na primeira vez, crie e confirme um PIN.
- Anote o codigo de recuperacao de 12 digitos exibido UMA VEZ.
- Use cima/baixo para escolher um digito, **B** para inserir, **Y** para apagar, **X** para confirmar e **A** para cancelar.
- Para recuperar PIN esquecido, na biblioteca normal pressione o analogico direito (R3), informe o codigo e crie outro PIN.
- Dentro da biblioteca privada, R3 abre configuracoes para alterar o PIN com o PIN atual.
- Voltar para biblioteca normal bloqueia a privada.
- Arquivo de verificacao: `sdmc:/switch/MangaCAP/security.dat`. Copie-o junto com o cartao se quiser manter a configuracao.
- Os arquivos na pasta private NAO SAO CRIPTOGRAFADOS. Quem tiver acesso ao SD pode le-los ou remover security.dat e configurar um novo PIN.
- Se perder tanto o PIN quanto o codigo, nao ha recuperacao autenticada: remover security.dat reinicializa a protecao sem apagar os livros.

ATENCAO: Codigo ainda nao compilado ou testado em um Switch; use como versao experimental e guarde o NRO V0.7.7.

## Correção V0.7.8.1
Corrigida a ordem de declaração de lockPrivate e refreshCovers, que impedia a compilação.


## V0.7.8.2 — teclado touchscreen
- Tela de PIN agora oferece teclado numerico 3x4 com botoes grandes.
- Acesso com PIN de 6 digitos confirma automaticamente no sexto toque.
- Cadastro, alteracao e recuperacao usam o mesmo teclado (OK para confirmar).
- Controles fisicos existentes permanecem disponiveis.
- O arquivo `security.dat` e o PIN existente nao sao alterados.
- **Ainda nao compilado/testado em hardware**; validar via GitHub Actions e Switch.


## V0.7.8.3 — Ícone oficial
Usa o arquivo mangacap_icon.png da versão PC V5.3.4, convertido para icon.jpg (256×256). Ajustados título e versão no Makefile. O código do leitor não foi alterado.


## V0.7.8.4 — icone embutido no NRO
- Corrige Makefile para gerar NACP explicitamente com nacptool.
- Passa --icon e --nacp explicitamente para elf2nro ao criar MangaCAP.nro.
- Mantem o icon.jpg oficial 256x256, sem alterar o leitor ou o PIN.
- Instale o MangaCAP.nro gerado pelo novo build em sdmc:/switch/MangaCAP/MangaCAP.nro (confira o caminho do seu atalho).
- Verifique se nao existe outra copia antiga do MangaCAP.nro no SD.
- Build ainda nao validado em ambiente devkitPro.


## V0.7.8.5 - Ícone sem corte

O ícone foi reconstruído a partir da arte horizontal original do MangaCAP (com a palavra MangaCAP completa), redimensionada e centralizada com margens. Nenhuma alteração no código do leitor.


## V0.7.8.6 — Ícone ajustado
- Arte aproximadamente 10% maior que na V0.7.8.5, com enquadramento centralizado.
- Mantidos o leitor, PIN e demais recursos sem alterações no código-fonte.
- A imagem original já apresenta a lua parcialmente fora do enquadramento superior; esta revisão preserva a arte original e aumenta sua visibilidade sem recriar elementos.
