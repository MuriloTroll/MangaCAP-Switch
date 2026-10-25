#pragma once
#include <string>
// Converte apenas PDFs que contenham paginas como imagens JPEG DCT.
// Retorna falso para outros tipos de PDF; nao altera o original.
bool pdfToCbzCache(const std::string& pdf,const std::string&root,std::string&cbz,std::string&error);
