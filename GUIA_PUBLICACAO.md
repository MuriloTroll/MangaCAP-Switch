# Checklist de publicação — MangaCAP v1.0.0

1. **Não substitua o código RC3 aprovado por um build antigo.** Use exatamente o código-fonte aprovado e gere um `MangaCAP.nro` novo no GitHub Actions.
2. O workflow atual do RC3 publica uma **pré-release** com tag `v1.0.0-rc2-<número>`. Isso não é a versão pública final.
3. Baixe o `MangaCAP.nro` da última compilação aprovada, teste-o no console e guarde uma cópia.
4. No GitHub, acesse **Releases → Draft a new release**. Crie a tag **`v1.0.0`** apontando para o commit da RC3 aprovada.
5. Título sugerido: **MangaCAP Switch v1.0.0 — Primeira versão pública**.
6. Anexe o **`MangaCAP.nro`** testado, cole o conteúdo de `RELEASE_v1.0.0.md` e **desmarque** a opção de pré-release.
7. Substitua o `README.md` da raiz do repositório pelo arquivo deste pacote. Confira a visualização do Markdown antes de salvar.
8. **Não exclua** `source/`, `assets/`, `Makefile`, `icon.jpg` ou `.github/workflows/`. Os arquivos antigos `TESTE*.md` podem ser removidos da raiz separadamente, se desejar; removê-los de um ZIP não os exclui do GitHub.
9. Após publicar, verifique o link da Release, o download do `.nro` e a instrução da pasta `books`.

**Atenção:** este pacote contém somente materiais de apresentação/documentação. Ele não recompila o programa, não publica a Release automaticamente e não altera o código do RC3.
