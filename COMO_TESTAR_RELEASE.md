# Compilacao por GitHub Releases

1. Substitua os arquivos do repositorio pelos deste pacote (ou apenas `.github/workflows/build.yml` se ja estiver usando a versao corrigida).
2. No GitHub: Settings > Actions > General > Workflow permissions: Read and write permissions. Salve.
3. Abra Actions > Build MangaCAP Switch > Run workflow.
4. Se terminar com sucesso, abra Releases e baixe `MangaCAP.nro` da pre-release mais recente.

**Atenção:** Releases de repositorios publicos ficam acessiveis ao publico, inclusive pre-releases. Cada execucao manual cria uma nova tag e uma nova Release de teste.

Se aparecer erro 403 / Resource not accessible by integration na publicacao, verifique a permissao de escrita do GITHUB_TOKEN nas configuracoes do repositorio.

Se a compilacao falhar em uma etapa anterior, a publicacao nao ocorrera.
