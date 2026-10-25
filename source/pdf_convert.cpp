#include "pdf_convert.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <zlib.h>

namespace {
void u16(FILE*f,uint16_t x){fputc(x&255,f);fputc((x>>8)&255,f);}
void u32(FILE*f,uint32_t x){u16(f,x&65535);u16(f,x>>16);}
struct Entry{std::string name;uint32_t offset,crc,size;};
std::string hashName(const std::string&path,const struct stat&st){
    uint64_t h=14695981039346656037ULL;
    std::string key=path+"/"+std::to_string((long long)st.st_size)+"/"+std::to_string((long long)st.st_mtime);
    for(unsigned char c:key){h^=c;h*=1099511628211ULL;}
    char buf[32];snprintf(buf,sizeof(buf),"%016llx",(unsigned long long)h);return buf;
}
bool writeZip(const std::string&path,const std::vector<std::vector<unsigned char>>& images){
    FILE*f=fopen(path.c_str(),"wb");if(!f)return false;
    std::vector<Entry> entries;
    bool ok=true;
    for(size_t i=0;i<images.size()&&ok;i++){
        const auto&data=images[i];char filename[32];snprintf(filename,sizeof(filename),"%05u.jpg",(unsigned)(i+1));
        Entry e{filename,(uint32_t)ftell(f),(uint32_t)crc32(0,data.data(),(uInt)data.size()),(uint32_t)data.size()};
        u32(f,0x04034b50);u16(f,20);u16(f,0);u16(f,0);u16(f,0);u16(f,0);
        u32(f,e.crc);u32(f,e.size);u32(f,e.size);u16(f,(uint16_t)e.name.size());u16(f,0);
        fwrite(e.name.data(),1,e.name.size(),f);ok=fwrite(data.data(),1,data.size(),f)==data.size();entries.push_back(e);
    }
    uint32_t start=(uint32_t)ftell(f);
    for(const auto&e:entries){
        u32(f,0x02014b50);u16(f,20);u16(f,20);u16(f,0);u16(f,0);u16(f,0);u16(f,0);
        u32(f,e.crc);u32(f,e.size);u32(f,e.size);u16(f,(uint16_t)e.name.size());
        u16(f,0);u16(f,0);u16(f,0);u16(f,0);u32(f,0);u32(f,e.offset);
        fwrite(e.name.data(),1,e.name.size(),f);
    }
    uint32_t finish=(uint32_t)ftell(f);
    u32(f,0x06054b50);u16(f,0);u16(f,0);u16(f,(uint16_t)entries.size());u16(f,(uint16_t)entries.size());
    u32(f,finish-start);u32(f,start);u16(f,0);
    bool ioOk=!ferror(f);
    bool closeOk=fclose(f)==0;
    ok=ok&&ioOk&&closeOk;
    if(!ok)remove(path.c_str());
    return ok;
}
}
bool pdfToCbzCache(const std::string&pdf,const std::string&root,std::string&cbz,std::string&error){
    struct stat st;if(stat(pdf.c_str(),&st)!=0||st.st_size<=0){error="arquivo inacessivel";return false;}
    std::string dir=root+"/cache";mkdir(dir.c_str(),0777);
    cbz=dir+"/"+hashName(pdf,st)+".cbz";
    struct stat cached;if(stat(cbz.c_str(),&cached)==0&&cached.st_size>100)return true;
    // Limite de memoria; PDFs maiores precisam de um renderizador por streaming.
    if(st.st_size>90LL*1024*1024){error="PDF maior que 90 MB";return false;}
    std::ifstream file(pdf,std::ios::binary);if(!file){error="falha ao ler PDF";return false;}
    std::string bytes((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    if(bytes.size()<8||bytes.compare(0,5,"%PDF-")!=0){error="nao e PDF valido";return false;}
    std::vector<std::vector<unsigned char>> images;
    // Extrai apenas fluxos /Subtype /Image codificados em JPEG (/DCTDecode).
    // Este nao e um parser PDF completo. A ordem pode variar em documentos complexos.
    size_t at=0;
    while(at<bytes.size()&&images.size()<500){
        size_t stream=bytes.find("stream",at);if(stream==std::string::npos)break;
        size_t end=bytes.find("endstream",stream+6);if(end==std::string::npos)break;
        size_t dictStart=bytes.rfind("<<",stream);size_t dictEnd=bytes.rfind(">>",stream);
        if(dictStart!=std::string::npos&&dictEnd!=std::string::npos&&dictStart<dictEnd&&stream-dictStart<4096){
            std::string dict=bytes.substr(dictStart,dictEnd-dictStart+2);
            if(dict.find("/Subtype")!=std::string::npos&&dict.find("/Image")!=std::string::npos&&dict.find("/DCTDecode")!=std::string::npos){
                size_t start=stream+6;if(start<bytes.size()&&bytes[start]=='\r')start++;
                if(start<bytes.size()&&bytes[start]=='\n')start++;
                size_t first=bytes.find("\xff\xd8",start);
                size_t last=bytes.rfind("\xff\xd9",end);
                if(first!=std::string::npos&&last!=std::string::npos&&first>=start&&last>=first&&last+2<=end&&last-first<24*1024*1024){
                    images.emplace_back(bytes.begin()+first,bytes.begin()+last+2);
                }
            }
        }
        at=end+9;
    }
    if(images.empty()){error="PDF nao contem imagens JPEG extraiveis";return false;}
    if(!writeZip(cbz,images)){error="falha ao gravar cache no SD";return false;}
    return true;
}
