#include "cbz_zip.h"
#include <zlib.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
static uint16_t u16(const unsigned char*p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t u32(const unsigned char*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static bool at(FILE*f,uint64_t pos,void*data,size_t n){
    if(pos>0x7fffffff || fseek(f,(long)pos,SEEK_SET)!=0)return false;
    return fread(data,1,n,f)==n;
}
CbzZip::CbzZip(const std::string& path):path_(path){
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
