#include "cbz_zip.h"
#include <zlib.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include <archive.h>
#include <archive_entry.h>
static uint16_t u16(const unsigned char*p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t u32(const unsigned char*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static bool at(FILE*f,uint64_t pos,void*data,size_t n){
    if(pos>0x7fffffff || fseek(f,(long)pos,SEEK_SET)!=0)return false;
    return fread(data,1,n,f)==n;
}
CbzZip::CbzZip(const std::string& path):path_(path){
    // RAR4/RAR5 via libarchive. Somente o indice fica em memoria.
    FILE*magic=fopen(path.c_str(),"rb");
    unsigned char signature[8]{};
    size_t got=magic?fread(signature,1,sizeof(signature),magic):0;
    if(magic)fclose(magic);
    if(got>=7 && memcmp(signature,"Rar!\x1a\x07",6)==0 && (signature[6]==0 || signature[6]==1)){
        rar_=true;
        struct archive*a=archive_read_new();
        archive_read_support_filter_all(a);
        archive_read_support_format_rar(a);
        archive_read_support_format_rar5(a);
        if(archive_read_open_filename(a,path.c_str(),65536)==ARCHIVE_OK){
            struct archive_entry*e=nullptr;
            while(entries_.size()<5000 && archive_read_next_header(a,&e)==ARCHIVE_OK){
                const char*name=archive_entry_pathname(e);
                auto size=archive_entry_size(e);
                if(name && strlen(name)<4096 && size>0 && size<=24*1024*1024 && archive_entry_filetype(e)==AE_IFREG){
                    entries_.push_back({name,0,(uint32_t)size,0,0,0,0});
                }
                archive_read_data_skip(a);
            }
            valid_=!entries_.empty();
        }
        archive_read_free(a);
        return;
    }
    FILE*f=fopen(path.c_str(),"rb");if(!f)return;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return;}
    long len=ftell(f);if(len<22){fclose(f);return;}
    size_t tailSize=(size_t)std::min<long>(len,65557);
    std::vector<unsigned char>tail(tailSize);
    if(!at(f,(uint64_t)len-tailSize,tail.data(),tailSize)){fclose(f);return;}
    size_t eocd=tailSize;
    for(size_t i=tailSize-22;;--i){
        if(u32(tail.data()+i)==0x06054b50 && i+22+u16(tail.data()+i+20)==tailSize){eocd=i;break;}
        if(i==0)break;
    }
    if(eocd==tailSize){fclose(f);return;}
    const unsigned char*end=tail.data()+eocd;
    uint16_t disk=u16(end+4),diskCD=u16(end+6),count=u16(end+10);
    uint32_t cdSize=u32(end+12),cdOff=u32(end+16);
    if(disk || diskCD || count==0xffff || cdSize==0xffffffff || cdOff==0xffffffff || count>5000 ||
       (uint64_t)cdOff+cdSize>(uint64_t)len){fclose(f);return;}
    uint64_t pos=cdOff;bool ok=true;
    for(unsigned i=0;i<count;i++){
        unsigned char h[46];if(pos+46>(uint64_t)len || !at(f,pos,h,46) || u32(h)!=0x02014b50){ok=false;break;}
        uint16_t nlen=u16(h+28),extra=u16(h+30),comment=u16(h+32);
        uint64_t next=pos+46+nlen+extra+comment;
        if(next>(uint64_t)len || nlen>4096){ok=false;break;}
        std::string name(nlen,'\0');if(nlen && !at(f,pos+46,name.data(),nlen)){ok=false;break;}
        uint32_t csize=u32(h+20),usize=u32(h+24),offset=u32(h+42);
        if(csize==0xffffffff || usize==0xffffffff || offset==0xffffffff){ok=false;break;}
        entries_.push_back({name,csize,usize,offset,u32(h+16),u16(h+10),u16(h+8)});
        pos=next;
    }
    valid_=ok && !entries_.empty();if(!valid_)entries_.clear();fclose(f);
}
bool CbzZip::read(const std::string& name,std::vector<unsigned char>&data,size_t limit)const{
    data.clear();if(!valid_)return false;
    if(rar_){
        struct archive*a=archive_read_new();
        archive_read_support_filter_all(a);
        archive_read_support_format_rar(a);
        archive_read_support_format_rar5(a);
        bool ok=false;
        if(archive_read_open_filename(a,path_.c_str(),65536)==ARCHIVE_OK){
            struct archive_entry*e=nullptr;
            while(archive_read_next_header(a,&e)==ARCHIVE_OK){
                const char*entryName=archive_entry_pathname(e);
                if(!entryName || name!=entryName){archive_read_data_skip(a);continue;}
                auto sz=archive_entry_size(e);
                if(sz<=0 || (uint64_t)sz>limit || archive_entry_filetype(e)!=AE_IFREG)break;
                data.resize((size_t)sz);
                size_t offset=0;
                while(offset<data.size()){
                    la_ssize_t n=archive_read_data(a,data.data()+offset,data.size()-offset);
                    if(n<=0)break;
                    offset+=(size_t)n;
                }
                ok=offset==data.size();
                break;
            }
        }
        archive_read_free(a);
        if(!ok)data.clear();
        return ok;
    }
    auto it=std::find_if(entries_.begin(),entries_.end(),[&](const CbzEntry&e){return e.name==name;});
    if(it==entries_.end() || (it->flags&1) || (it->method!=0 && it->method!=8) ||
       !it->uncompressed || it->uncompressed>limit || it->compressed>limit*2 || it->compressed==0)return false;
    FILE*f=fopen(path_.c_str(),"rb");if(!f)return false;
    unsigned char h[30];bool ok=at(f,it->offset,h,30) && u32(h)==0x04034b50;
    uint64_t pos=(uint64_t)it->offset+30;
    if(ok){pos+=u16(h+26)+u16(h+28);}
    std::vector<unsigned char> compressed;
    if(ok){compressed.resize(it->compressed);ok=at(f,pos,compressed.data(),compressed.size());}
    fclose(f);if(!ok)return false;
    data.resize(it->uncompressed);
    if(it->method==0){if(it->compressed!=it->uncompressed){data.clear();return false;}memcpy(data.data(),compressed.data(),data.size());}
    else{
        z_stream stream{};stream.next_in=compressed.data();stream.avail_in=(uInt)compressed.size();
        stream.next_out=data.data();stream.avail_out=(uInt)data.size();
        if(inflateInit2(&stream,-MAX_WBITS)!=Z_OK){data.clear();return false;}
        int result=inflate(&stream,Z_FINISH);
        ok=result==Z_STREAM_END && stream.total_out==data.size();inflateEnd(&stream);
        if(!ok){data.clear();return false;}
    }
    if(crc32(0,data.data(),(uInt)data.size())!=it->crc){data.clear();return false;}
    return true;
}
