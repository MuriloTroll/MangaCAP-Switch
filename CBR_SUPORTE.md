# Suporte CBR (RAR)

O leitor abre .cbr diretamente usando libarchive, com leitura sob demanda de cada pagina. CBZ, EPUB, PDF e TXT permanecem suportados.

A compilacao requer `switch-libarchive` no devkitPro. O workflow tenta instalar esse pacote. Se o pacote nao existir no repositorio utilizado, sera necessario compilar libarchive para Switch antes de executar `make`.

Nao foi possivel compilar ou testar o NRO neste ambiente (devkitPro ausente). O arquivo AKIRA RAR4 ainda precisa ser validado no console.
