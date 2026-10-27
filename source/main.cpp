#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "cbz_zip.h"
#include "pdf_convert.h"
#include "font.h"
#include "rotate_cw_png.h"
#include "arrow_up_png.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstdlib>
#include <iterator>
#include <cmath>
#include <sys/stat.h>
#include <curl/curl.h>
#include <mbedtls/md.h>
#include <random>
#include <iomanip>

static const char* ROOT="sdmc:/switch/MangaCAP";
static const char* BOOKS="sdmc:/switch/MangaCAP/books";
static const char* PRIVATE_BOOKS="sdmc:/switch/MangaCAP/private";
static const char* PROGRESS="sdmc:/switch/MangaCAP/progress.tsv";
static const char* PAGE_COUNTS="sdmc:/switch/MangaCAP/page_counts.tsv";

static const char* SECURITY_FILE="sdmc:/switch/MangaCAP/security.dat";
struct SecurityData {std::string salt,pinHash,recoveryHash;bool valid=false;};
static std::string secureHex(const unsigned char* bytes,size_t n){
    const char* digits="0123456789abcdef";std::string out;out.reserve(n*2);
    for(size_t i=0;i<n;i++){out+=digits[bytes[i]>>4];out+=digits[bytes[i]&15];}return out;
}
static std::string randomDigits(size_t n){
    std::random_device rd;std::string out;
    for(size_t i=0;i<n;i++)out+=(char)('0'+(rd()%10));return out;
}
static std::string securityHash(const std::string& salt,const std::string& kind,const std::string& secret){
    const mbedtls_md_info_t* md=mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if(!md)return "";
    std::string data=salt+":"+kind+":"+secret;
    unsigned char hash[32]={};
    if(mbedtls_md_hmac(md,(const unsigned char*)salt.data(),salt.size(),(const unsigned char*)data.data(),data.size(),hash)!=0)return "";
    for(int i=0;i<12000;i++){
        unsigned char next[32];
        if(mbedtls_md_hmac(md,(const unsigned char*)data.data(),data.size(),hash,32,next)!=0)return "";
        memcpy(hash,next,32);
    }
    return secureHex(hash,32);
}
static SecurityData readSecurity(){
    SecurityData d;std::ifstream f(SECURITY_FILE);std::string version;
    if(std::getline(f,version)&&version=="MANGACAP_PIN_V1"&&std::getline(f,d.salt)&&std::getline(f,d.pinHash)&&std::getline(f,d.recoveryHash))
        d.valid=d.salt.size()==32&&d.pinHash.size()==64&&d.recoveryHash.size()==64;
    return d;
}
static bool writeSecurity(const SecurityData& d){
    const std::string temp=std::string(SECURITY_FILE)+".tmp";
    {std::ofstream f(temp,std::ios::trunc);if(!f)return false;
     f<<"MANGACAP_PIN_V1\n"<<d.salt<<"\n"<<d.pinHash<<"\n"<<d.recoveryHash<<"\n";
     f.flush();if(!f.good())return false;}
    return rename(temp.c_str(),SECURITY_FILE)==0;
}
static bool checkSecret(const std::string& entered,const std::string& expected){
    if(expected.size()!=64)return false;
    const std::string actual=entered;
    unsigned int diff=0;for(size_t i=0;i<64;i++)diff|=(unsigned char)actual[i]^(unsigned char)expected[i];
    return diff==0;
}

struct Book {std::string name,path,collection;};
static std::string lower(std::string s){for(char& c:s)c=(char)tolower((unsigned char)c);return s;}
static bool ends(const std::string& s,const char* suffix){size_t n=strlen(suffix);return s.size()>=n&&lower(s.substr(s.size()-n))==suffix;}
static bool isImage(const std::string& s){return ends(s,".png")||ends(s,".jpg")||ends(s,".jpeg")||ends(s,".webp");}
// Varredura limitada para nao percorrer acidentalmente todo o cartao SD.
// A primeira subpasta determina a obra; arquivos soltos usam o nome do CBZ.
static void scanFolder(const std::string& dir,const std::string& collection,
                       std::vector<Book>& out,int depth){
    if(depth>5 || out.size()>=500)return;
    DIR* d=opendir(dir.c_str());if(!d)return;
    struct dirent* e;
    while((e=readdir(d)) && out.size()<500){
        std::string n=e->d_name;
        if(n=="."||n==".."||n=="_private"||n.empty()||n[0]=='.')continue;
        std::string path=dir+"/"+n;
        struct stat st;
        if(stat(path.c_str(),&st)!=0)continue;
        if(S_ISDIR(st.st_mode)){
            if(depth<5)scanFolder(path,collection.empty()?n:collection,out,depth+1);
        }else if(S_ISREG(st.st_mode) && (ends(n,".cbz")||ends(n,".zip")||ends(n,".pdf")||ends(n,".epub")||ends(n,".txt"))){
            out.push_back({n,path,collection});
        }
    }
    closedir(d);
}
static std::vector<Book> scan(bool privateMode=false){
    mkdir(ROOT,0777);mkdir(BOOKS,0777);mkdir(PRIVATE_BOOKS,0777);
    std::vector<Book> b;scanFolder(privateMode?PRIVATE_BOOKS:BOOKS,"",b,0);
    std::sort(b.begin(),b.end(),[](const Book&a,const Book&b){
        if(lower(a.collection)!=lower(b.collection))return lower(a.collection)<lower(b.collection);
        return lower(a.name)<lower(b.name);
    });
    return b;
}
// Agrupamento simples por obra, preservando arquivos originais.
struct Work {std::string title;std::vector<int> volumes;};
static std::string stripExtension(std::string s){
    auto pos=s.find_last_of('.');if(pos!=std::string::npos)s.resize(pos);
    for(char&c:s)if(c=='_'||c=='.')c=' ';
    return s;
}
static std::string workTitle(const std::string& filename){
    std::string s=stripExtension(filename),low=lower(s);
    // Separa marcadores comuns: " - Capitulo", " - Volume", " - Cap", etc.
    const char* marks[]={" - capitulo"," - capítulo"," - captulo"," - cap "," - cap."," - vol "," - volume"," - chapter"," - ch "," - tomo"," - v0"," - v1"," - v2"," - v3"," - v4"," - v5"," - v6"," - v7"," - v8"," - v9"};
    size_t cut=s.size();
    for(auto m:marks){auto k=low.find(m);if(k!=std::string::npos)cut=std::min(cut,k);}
    // Remove sequencias finais como " 001", " 02", caso existam.
    if(cut==s.size()){
        size_t i=s.size();
        while(i>0 && std::isspace((unsigned char)s[i-1]))--i;
        size_t j=i;
        while(j>0 && std::isdigit((unsigned char)s[j-1]))--j;
        if(j<i && j>0 && (s[j-1]==' '||s[j-1]=='-'))cut=j-1;
    }
    s.resize(cut);
    while(!s.empty() && (s.back()==' '||s.back()=='-'))s.pop_back();
    return s.empty()?stripExtension(filename):s;
}
static std::vector<Work> groupBooks(const std::vector<Book>& books){
    std::vector<Work> works;
    std::map<std::string,int> ids;
    for(size_t i=0;i<books.size();++i){
        auto title=books[i].collection.empty()?workTitle(books[i].name):books[i].collection;
        auto key=lower(title);
        auto it=ids.find(key);
        if(it==ids.end()){
            int idx=(int)works.size();ids[key]=idx;
            works.push_back({title,{(int)i}});
        }else works[it->second].volumes.push_back((int)i);
    }
    return works;
}
struct Cover {std::string path;SDL_Texture* tex=nullptr;int w=0,h=0;bool attempted=false;};
static void clearCovers(std::vector<Cover>& covers){
    for(auto& c:covers)if(c.tex){SDL_DestroyTexture(c.tex);c.tex=nullptr;}
    covers.clear();
}

// Capas online (AniList): so baixar quando o titulo da obra corresponde
// claramente ao resultado. Arquivos locais sempre tem prioridade.
static std::string coverKey(std::string title){
    std::string out;
    for(unsigned char c:title){
        if(std::isalnum(c))out+=(char)std::tolower(c);
        else if(c==' '||c=='-'||c=='_'||c=='.')out+='-';
    }
    // Evita chaves diferentes por espacos duplicados e pontuacao.
    std::string clean;bool dash=false;
    for(char c:out){if(c=='-'){if(!clean.empty()&&!dash)clean+='-';dash=true;}
        else{clean+=c;dash=false;}}
    while(!clean.empty()&&clean.back()=='-')clean.pop_back();
    return clean.empty()?"obra":clean.substr(0,90);
}
static std::string searchTitle(std::string title){
    // Remove identificadores de grupos e marcadores comuns de capitulos/volumes.
    size_t at=title.find('@');if(at!=std::string::npos)title.resize(at);
    title=workTitle(title);
    std::string lo=lower(title);
    const char* markers[]={" volume "," vol "," capitulo "," capítulo "," captulo "," chapter "," cap "," ch "," tomo "," - "};
    size_t cut=title.size();
    for(const char* marker:markers){size_t pos=lo.find(marker);if(pos!=std::string::npos)cut=std::min(cut,pos);}
    title.resize(cut);
    while(!title.empty()&&(title.back()==' '||title.back()=='-'||title.back()=='_'))title.pop_back();
    return title;
}
static bool matchingTitle(const std::string& actual,const std::string& wanted){
    std::string a=coverKey(actual),b=coverKey(wanted);
    if(a==b)return true;
    // Aceita somente subtitulos depois de um titulo completo, nao palavras parciais.
    return (a.size()>b.size()+3 && a.compare(0,b.size()+1,b+"-")==0)
        || (b.size()>a.size()+3 && b.compare(0,a.size()+1,a+"-")==0);
}
static std::string jsonEscape(const std::string& s){
    std::string out;
    for(char c:s){if(c=='\\'||c=='"')out+='\\';if((unsigned char)c>=32)out+=c;}
    return out;
}
static std::string jsonStringAfter(const std::string& text,const std::string& key,size_t start=0){
    size_t p=text.find("\""+key+"\"",start);
    if(p==std::string::npos)return "";
    p=text.find(':',p+key.size()+2);
    if(p==std::string::npos)return "";
    p=text.find('"',p+1);
    if(p==std::string::npos)return "";
    std::string result;bool escape=false;
    for(++p;p<text.size();++p){
        char c=text[p];
        if(escape){if(c=='n')result+=' ';else result+=c;escape=false;continue;}
        if(c=='\\'){escape=true;continue;}
        if(c=='"')break;
        result+=c;
    }
    return result;
}
static size_t curlString(char* ptr,size_t size,size_t nmemb,void* data){
    auto* s=static_cast<std::string*>(data);
    size_t n=size*nmemb;
    if(s->size()+n>256*1024)return 0;
    s->append(ptr,n);return n;
}
static size_t curlImage(char* ptr,size_t size,size_t nmemb,void* data){
    auto* s=static_cast<std::vector<unsigned char>*>(data);
    size_t n=size*nmemb;
    if(s->size()+n>4*1024*1024)return 0;
    s->insert(s->end(),(unsigned char*)ptr,(unsigned char*)ptr+n);return n;
}
static bool curlOk(CURL* curl){
    curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1L);
    curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT,3L);
    curl_easy_setopt(curl,CURLOPT_TIMEOUT,6L);
    curl_easy_setopt(curl,CURLOPT_USERAGENT,"MangaCAP-Switch/0.7.2");
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);
    CURLcode code=curl_easy_perform(curl);
    long status=0;curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&status);
    return code==CURLE_OK&&status>=200&&status<300;
}
static std::vector<std::string> titleVariants(const std::string& input);
static bool downloadCoverOne(const std::string& title,const std::string& output){
    CURL* curl=curl_easy_init();if(!curl)return false;
    std::string response;
    std::string payload="{\"query\":\"query($search:String){Media(search:$search,type:MANGA){title{romaji english native}synonyms coverImage{large}}}\",\"variables\":{\"search\":\""+jsonEscape(title)+"\"}}";
    struct curl_slist* headers=nullptr;
    headers=curl_slist_append(headers,"Content-Type: application/json");
    curl_easy_setopt(curl,CURLOPT_URL,"https://graphql.anilist.co");
    curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,payload.c_str());
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,curlString);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,&response);
    bool ok=curlOk(curl);
    curl_slist_free_all(headers);curl_easy_cleanup(curl);
    if(!ok)return false;
    std::string romaji=jsonStringAfter(response,"romaji");
    std::string english=jsonStringAfter(response,"english");
    // Sem correspondencia segura, nao associa capa errada.
    std::string native=jsonStringAfter(response,"native");
    bool matched=false;
    for(const auto& variant:titleVariants(title)){
        if(matchingTitle(romaji,variant)||matchingTitle(english,variant)||matchingTitle(native,variant)){matched=true;break;}
    }
    if(!matched){
        size_t sy=response.find("\"synonyms\"");
        if(sy!=std::string::npos){
            size_t left=response.find('[',sy),right=left==std::string::npos?std::string::npos:response.find(']',left);
            if(right!=std::string::npos){
                std::string aliases=response.substr(left+1,right-left-1);
                size_t pos=0;
                while((pos=aliases.find('\"',pos))!=std::string::npos){
                    size_t end=aliases.find('\"',pos+1);if(end==std::string::npos)break;
                    for(const auto& variant:titleVariants(title))if(matchingTitle(aliases.substr(pos+1,end-pos-1),variant)){matched=true;break;}
                    if(matched)break;
                    pos=end+1;
                }
            }
        }
    }
    if(!matched)return false;
    size_t at=response.find("\"coverImage\"");
    if(at==std::string::npos)return false;
    std::string url=jsonStringAfter(response,"large",at);
    if(url.compare(0,8,"https://")!=0)return false;
    std::vector<unsigned char> image;
    curl=curl_easy_init();if(!curl)return false;
    curl_easy_setopt(curl,CURLOPT_URL,url.c_str());
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,curlImage);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,&image);
    ok=curlOk(curl);curl_easy_cleanup(curl);
    if(!ok||image.size()<256)return false;
    SDL_RWops* rw=SDL_RWFromConstMem(image.data(),(int)image.size());
    SDL_Surface* surf=rw?IMG_Load_RW(rw,1):nullptr;
    if(!surf)return false;
    SDL_FreeSurface(surf);
    std::string temp=output+".part";
    FILE* f=fopen(temp.c_str(),"wb");if(!f)return false;
    bool saved=fwrite(image.data(),1,image.size(),f)==image.size();
    fclose(f);
    if(!saved){remove(temp.c_str());return false;}
    if(rename(temp.c_str(),output.c_str())!=0){remove(temp.c_str());return false;}
    return true;
}
// Tenta variantes de busca sem comprometer a validacao do resultado.
static std::vector<std::string> titleVariants(const std::string& input){
    std::vector<std::string> out;
    auto add=[&](std::string value){
        value=searchTitle(value);
        while(!value.empty()&&value.back()==' ')value.pop_back();
        if(value.empty())return;
        for(const auto& previous:out)if(coverKey(previous)==coverKey(value))return;
        out.push_back(value);
    };
    add(input);
    std::string clean=input;
    // Edicoes de fas e informacoes de scanlation nao fazem parte do titulo.
    const char* tags[]={" full color"," full colour"," colored"," coloured"," colorido"," colorida"," em cores"," digital colored"," digital color"," fan colored"," fan color"," versão colorida"," versao colorida"," ["," ("};
    std::string low=lower(clean);
    for(const char* tag:tags){size_t at=low.find(tag);if(at!=std::string::npos){clean.resize(at);low.resize(at);}}
    add(clean);
    // Evita reduzir titulos compostos arbitrariamente: apenas sufixos conhecidos.
    return out;
}
static bool downloadCover(const std::string& title,const std::string& output){
    for(const auto& query:titleVariants(title))if(downloadCoverOne(query,output))return true;
    return false;
}
static int getSaved(const std::string& path){std::ifstream f(PROGRESS);std::string line;while(std::getline(f,line)){auto pos=line.find('\t');if(pos!=std::string::npos&&line.substr(pos+1)==path)return atoi(line.substr(0,pos).c_str());}return 0;}
// Quantidade total de paginas por arquivo, armazenada apos a primeira abertura.
// A barra da biblioteca nao precisa descompactar centenas de volumes a cada desenho.
static int knownPageCount(const std::string& path){
    std::ifstream f(PAGE_COUNTS);std::string line;
    while(std::getline(f,line)){
        auto tab=line.find('\t');
        if(tab!=std::string::npos && line.substr(tab+1)==path)return std::max(0,atoi(line.substr(0,tab).c_str()));
    }
    return 0;
}
static void rememberPageCount(const std::string& path,int count){
    if(count<=0)return;
    std::ifstream f(PAGE_COUNTS);std::vector<std::string> lines;std::string line;
    while(std::getline(f,line)){
        auto tab=line.find('\t');
        if(tab==std::string::npos||line.substr(tab+1)!=path)lines.push_back(line);
    }
    f.close();std::ofstream out(PAGE_COUNTS,std::ios::trunc);
    for(const auto& entry:lines)out<<entry<<'\n';
    out<<count<<'\t'<<path<<'\n';
}
static void save(const std::string& path,int page){std::ifstream f(PROGRESS);std::vector<std::string> lines;std::string line;while(std::getline(f,line)){auto p=line.find('\t');if(p==std::string::npos||line.substr(p+1)!=path)lines.push_back(line);}f.close();std::ofstream out(PROGRESS,std::ios::trunc);for(auto& l:lines)out<<l<<'\n';out<<page<<'\t'<<path<<'\n';}
static std::vector<std::string> listPages(CbzZip* z){
    std::vector<std::string> names;
    for(const auto& e:z->entries()) if(isImage(e.name) && e.name.find("__MACOSX/")!=0 && names.size()<2000) names.push_back(e.name);
    std::sort(names.begin(),names.end());return names;
}
static SDL_Texture* decode(SDL_Renderer*r,CbzZip*z,const std::string& name,int&w,int&h){
    std::vector<unsigned char> buf;
    if(!z->read(name,buf,24*1024*1024) || buf.empty())return nullptr;
    SDL_RWops*rw=SDL_RWFromConstMem(buf.data(),(int)buf.size());if(!rw)return nullptr;
    SDL_Surface*surf=IMG_Load_RW(rw,1);if(!surf)return nullptr;
    w=surf->w;h=surf->h;SDL_Texture*t=SDL_CreateTextureFromSurface(r,surf);SDL_FreeSurface(surf);return t;
}

// Capa automatica: primeiro imagens explicitamente identificadas como capa,
// depois primeira pagina. Aceita tambem cover.jpg/png/webp na pasta da obra.
static std::string basenameOf(const std::string& path){
    size_t p=path.find_last_of("/\\");
    return p==std::string::npos?path:path.substr(p+1);
}
static std::string dirnameOf(const std::string& path){
    size_t p=path.find_last_of("/\\");
    return p==std::string::npos?"":path.substr(0,p);
}
static std::string stemOf(const std::string& path){
    std::string name=basenameOf(path);
    size_t p=name.find_last_of('.');
    return p==std::string::npos?name:name.substr(0,p);
}
static bool fileExists(const std::string& path){
    struct stat st;
    return stat(path.c_str(),&st)==0 && S_ISREG(st.st_mode);
}
static int coverScore(const std::string& path){
    const std::string name=lower(basenameOf(path));
    const std::string stem=lower(stemOf(path));
    if(stem=="cover"||stem=="capa"||stem=="front"||stem=="folder"||
       stem=="poster"||stem=="thumbnail")return 100;
    if(stem.find("cover")!=std::string::npos||stem.find("capa")!=std::string::npos)
        return 90;
    if(stem.find("front")!=std::string::npos||stem.find("poster")!=std::string::npos)
        return 80;
    if(stem=="000"||stem=="0000"||stem=="001"||stem=="01")return 15;
    (void)name;
    return 0;
}
static SDL_Texture* coverSurface(SDL_Renderer*r,SDL_Surface*original,int&w,int&h){
    if(!original||original->w<=0||original->h<=0)return nullptr;
    const float ratio=std::min(214.f/original->w,356.f/original->h);
    w=std::max(1,(int)(original->w*ratio));
    h=std::max(1,(int)(original->h*ratio));
    SDL_Surface*thumb=SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_RGBA32);
    if(!thumb)return nullptr;
    SDL_BlitScaled(original,nullptr,thumb,nullptr);
    SDL_Texture*texture=SDL_CreateTextureFromSurface(r,thumb);
    SDL_FreeSurface(thumb);
    return texture;
}
static SDL_Texture* coverFromFile(SDL_Renderer*r,const std::string&path,int&w,int&h){
    if(!fileExists(path))return nullptr;
    SDL_Surface*original=IMG_Load(path.c_str());
    if(!original)return nullptr;
    SDL_Texture*texture=coverSurface(r,original,w,h);
    SDL_FreeSurface(original);
    return texture;
}
// Estima a proporcao de pixels realmente coloridos, ignorando quase preto/branco.
// Amostragem leve para evitar analisar todos os pixels no Switch.
static double coverColorFraction(SDL_Surface* surface){
    if(!surface||surface->w<=0||surface->h<=0)return 0.0;
    SDL_Surface* rgb=SDL_ConvertSurfaceFormat(surface,SDL_PIXELFORMAT_RGBA32,0);
    if(!rgb)return 0.0;
    if(SDL_MUSTLOCK(rgb)&&SDL_LockSurface(rgb)!=0){SDL_FreeSurface(rgb);return 0.0;}
    int colored=0,total=0;
    const int stepX=std::max(1,rgb->w/64),stepY=std::max(1,rgb->h/64);
    for(int y=0;y<rgb->h;y+=stepY){
        const Uint32* row=(const Uint32*)((const Uint8*)rgb->pixels+y*rgb->pitch);
        for(int x=0;x<rgb->w;x+=stepX){
            Uint8 r,g,b,a;SDL_GetRGBA(row[x],rgb->format,&r,&g,&b,&a);
            if(a<100)continue;
            const int hi=std::max((int)r,std::max((int)g,(int)b));
            const int lo=std::min((int)r,std::min((int)g,(int)b));
            if(hi-lo>=30 && hi>=65)++colored;
            ++total;
        }
    }
    if(SDL_MUSTLOCK(rgb))SDL_UnlockSurface(rgb);
    SDL_FreeSurface(rgb);
    return total?((double)colored/total):0.0;
}
static SDL_Texture* coverFromBook(SDL_Renderer*r,const Book&book,int&w,int&h,bool allowFolderCover=true){
    // 1. Capa na pasta da obra: books/Obra/cover.jpg
    // 2. Capa ao lado do CBZ: books/Obra/Volume 01.cover.jpg
    // 3. Capa dentro do CBZ, procurando "cover", "capa", "front".
    // 4. Primeira imagem do CBZ, sem configuracao manual.
    const std::string dir=dirnameOf(book.path),stem=stemOf(book.path);
    const char* extensions[]={".jpg",".jpeg",".png",".webp"};
    const char* names[]={"cover","Cover","COVER","capa","Capa","front","Front","folder","poster"};
    if(allowFolderCover)for(const char* ext:extensions){
        for(const char* name:names){
            SDL_Texture*texture=coverFromFile(r,dir+"/"+name+ext,w,h);
            if(texture)return texture;
        }
    }
    for(const char* ext:extensions){
        SDL_Texture*texture=coverFromFile(r,dir+"/"+stem+".cover"+ext,w,h);
        if(texture)return texture;
    }
    if(ends(book.path,".pdf")||ends(book.path,".txt"))return nullptr; // Evita conversao demorada ao percorrer capas.
    CbzZip zip(book.path);
    if(!zip.valid())return nullptr;
    if(ends(book.path,".epub")){
        std::string candidate;int best=-1;
        for(const auto&e:zip.entries())if(isImage(e.name)){
            int score=coverScore(e.name);if(score>best){best=score;candidate=e.name;}
        }
        if(candidate.empty())return nullptr;
        std::vector<unsigned char>bytes;
        if(!zip.read(candidate,bytes,24*1024*1024))return nullptr;
        SDL_RWops*rw=SDL_RWFromConstMem(bytes.data(),(int)bytes.size());
        if(!rw)return nullptr;
        SDL_Surface*surface=IMG_Load_RW(rw,1);if(!surface)return nullptr;
        SDL_Texture*t=coverSurface(r,surface,w,h);SDL_FreeSurface(surface);return t;
    }
    auto pages=listPages(&zip);
    if(pages.empty())return nullptr;
    // Procura nomes explicitos no arquivo inteiro; sem nome, compara as 4
    // primeiras paginas e prefere uma pagina vertical semelhante a capa.
    std::string chosen=pages.front();int best=coverScore(chosen);
    for(const auto& candidate:pages){
        int score=coverScore(candidate);
        if(score>best){best=score;chosen=candidate;}
    }
    SDL_Surface* selected=nullptr;
    if(best<80){
        // Sem capa nomeada: prefere a primeira pagina, especialmente se colorida.
        // Outras paginas precisam ter evidencias bem melhores para substitui-la.
        double bestQuality=-1e9;
        for(size_t i=0;i<pages.size()&&i<5;i++){
            std::vector<unsigned char> data;
            if(!zip.read(pages[i],data,24*1024*1024)||data.empty())continue;
            SDL_RWops*rw=SDL_RWFromConstMem(data.data(),(int)data.size());
            SDL_Surface* candidate=rw?IMG_Load_RW(rw,1):nullptr;
            if(!candidate)continue;
            const double ratio=(double)candidate->h/std::max(1,candidate->w);
            const double color=coverColorFraction(candidate);
            // Bonus pela posicao inicial e pelo formato de capa.
            // Colorido e apenas um sinal complementar, nunca obrigatorio.
            double quality=(i==0?2.0:0.45/(double)(i+1));
            if(ratio>=1.20&&ratio<=2.0)quality+=0.65;
            if(color>=0.08)quality+=0.85;
            if(color>=0.25)quality+=0.35;
            // Uma pagina muito larga provavelmente e dupla ou splash.
            if(ratio<0.85)quality-=0.75;
            if(quality>bestQuality){
                if(selected)SDL_FreeSurface(selected);
                selected=candidate;bestQuality=quality;
            }else SDL_FreeSurface(candidate);
        }
    }
    if(!selected){
        std::vector<unsigned char> bytes;
        if(!zip.read(chosen,bytes,24*1024*1024)||bytes.empty())return nullptr;
        SDL_RWops*rw=SDL_RWFromConstMem(bytes.data(),(int)bytes.size());
        selected=rw?IMG_Load_RW(rw,1):nullptr;
    }
    if(!selected)return nullptr;
    SDL_Texture*texture=coverSurface(r,selected,w,h);
    SDL_FreeSurface(selected);
    return texture;
}
static std::string xmlAttr(const std::string& tag,const std::string& name){
    size_t p=0;
    while((p=tag.find(name,p))!=std::string::npos){
        if(p && (std::isalnum((unsigned char)tag[p-1])||tag[p-1]=='-'||tag[p-1]==':')){p+=name.size();continue;}
        size_t q=p+name.size();
        while(q<tag.size() && std::isspace((unsigned char)tag[q]))++q;
        if(q>=tag.size() || tag[q]!='='){p=q;continue;}
        ++q;
        while(q<tag.size() && std::isspace((unsigned char)tag[q]))++q;
        if(q>=tag.size() || (tag[q]!='"' && tag[q]!=39)){p=q;continue;}
        char quote=tag[q++];size_t e=tag.find(quote,q);
        return e==std::string::npos?"":tag.substr(q,e-q);
    }
    return "";
}
static std::string zipDirectory(const std::string&path){size_t p=path.find_last_of('/');return p==std::string::npos?"":path.substr(0,p+1);}
static std::string normalizeZipPath(const std::string&path){
    std::vector<std::string>parts;std::stringstream ss(path);std::string x;
    while(std::getline(ss,x,'/')){if(x=="..") {if(!parts.empty())parts.pop_back();}else if(x!="."&&!x.empty())parts.push_back(x);}
    std::string out;for(auto &p:parts){if(!out.empty())out+="/";out+=p;}return out;
}
static bool readZipText(CbzZip&zip,const std::string&name,std::string&out,size_t max=4*1024*1024){
    std::vector<unsigned char>bytes;if(!zip.read(name,bytes,max))return false;
    out.assign(bytes.begin(),bytes.end());return true;
}
static std::string decodeEntities(const std::string&s){
    std::string out;out.reserve(s.size());
    for(size_t i=0;i<s.size();){
        if(s[i]=='&'){
            size_t e=s.find(';',i+1);
            if(e!=std::string::npos&&e-i<16){
                std::string v=s.substr(i+1,e-i-1);int code=0;
                if(v=="amp")code='&';else if(v=="lt")code='<';else if(v=="gt")code='>';
                else if(v=="quot")code='"';else if(v=="apos")code='\'';
                else if(v=="nbsp")code=' ';
                else if(v.size()>1&&v[0]=='#')code=(int)strtol(v.c_str()+(v[1]=='x'||v[1]=='X'?2:1),nullptr,(v[1]=='x'||v[1]=='X')?16:10);
                if(code){if(code<128)out+=(char)code;else if(code<2048){out+=(char)(0xc0|(code>>6));out+=(char)(0x80|(code&63));}
                    else if(code<65536){out+=(char)(0xe0|(code>>12));out+=(char)(0x80|((code>>6)&63));out+=(char)(0x80|(code&63));}
                    i=e+1;continue;}
            }
        }
        out+=s[i++];
    }return out;
}
static std::string htmlToText(const std::string&html){
    std::string out;out.reserve(std::min((size_t)200000,html.size()));
    bool tag=false,skip=false;std::string t;
    for(size_t i=0;i<html.size()&&out.size()<1500000;i++){
        char c=html[i];
        if(c=='<') {tag=true;t.clear();continue;}
        if(tag){if(c=='>'){
                std::string n=lower(t);while(!n.empty()&&isspace((unsigned char)n[0]))n.erase(n.begin());
                if(n.find("script")==0||n.find("style")==0)skip=true;
                if(n.find("/script")==0||n.find("/style")==0)skip=false;
                if(!skip && (n.find("/p")==0||n.find("/div")==0||n.find("/h")==0||n.find("br")==0||n.find("/li")==0))out+='\n';
                tag=false;
            }else if(t.size()<80)t+=c;
        }else if(!skip)out+=c;
    }
    return decodeEntities(out);
}
static std::string displayAscii(const std::string&utf){
    std::string out;out.reserve(utf.size());
    for(size_t i=0;i<utf.size();i++){
        unsigned char c=utf[i];
        if(c<128){if(c=='\r'||c=='\t')out+=' ';else if(c=='\n'||c>=32)out+=(char)c;continue;}
        if(c==0xc3 && i+1<utf.size()){
            unsigned char n=utf[++i];
            
            switch(n){case 0xa1:case 0xa0:case 0xa2:case 0xa3:case 0x81:case 0x80:case 0x82:case 0x83:out+='a';break;
            case 0xa9:case 0xaa:case 0x89:out+='e';break;case 0xad:case 0x8d:out+='i';break;
            case 0xb3:case 0xb4:case 0xb5:case 0x93:out+='o';break;
            case 0xba:case 0xbc:case 0x9a:out+='u';break;
            case 0xa7:case 0x87:out+='c';break;default:out+='?';break;}
        }else{while(i+1<utf.size() && ((unsigned char)utf[i+1]&0xc0)==0x80)i++;out+='?';}
    }return out;
}
static std::string findZipEntry(const CbzZip&zip,const std::string&wanted){
    const std::string low=lower(wanted);
    for(const auto&e:zip.entries())if(lower(e.name)==low)return e.name;
    return "";
}
static bool epubText(const std::string&path,std::string&out,std::string&err){
    CbzZip zip(path);if(!zip.valid()){err="EPUB ZIP invalido";return false;}
    std::string container,opf,opfPath;
    const std::string containerPath=findZipEntry(zip,"META-INF/container.xml");
    if(!containerPath.empty()&&readZipText(zip,containerPath,container,128*1024)){
        // Procura a primeira entrada rootfile, com ou sem prefixo de namespace.
        size_t r=container.find("rootfile");
        if(r!=std::string::npos){
            size_t end=container.find('>',r);
            if(end!=std::string::npos)
                opfPath=normalizeZipPath(xmlAttr(container.substr(r,end-r),"full-path"));
        }
    }
    // Alguns EPUBs nao convencionais possuem container.xml incorreto.
    // Recupera o pacote OPF a partir do ZIP, sem precisar de conversao externa.
    if(!opfPath.empty()){
        const std::string actual=findZipEntry(zip,opfPath);
        if(!actual.empty()&&readZipText(zip,actual,opf))opfPath=actual;
    }
    if(opf.empty()){
        for(const auto&e:zip.entries()){
            if(ends(lower(e.name),".opf")&&readZipText(zip,e.name,opf)){
                opfPath=e.name;break;
            }
        }
    }
    if(opf.empty()){err="EPUB sem pacote OPF";return false;}
    std::map<std::string,std::string>manifest;
    // Reconhece <item>, <opf:item> e atributos com espacamento variavel.
    for(size_t p=0;(p=opf.find("<",p))!=std::string::npos;){
        size_t e=opf.find('>',p);if(e==std::string::npos)break;
        std::string tag=opf.substr(p,e-p+1);
        size_t nameEnd=tag.find_first_of(" \t\r\n/>",1);
        std::string name=lower(tag.substr(1,nameEnd==std::string::npos?std::string::npos:nameEnd-1));
        size_t colon=name.rfind(':');if(colon!=std::string::npos)name=name.substr(colon+1);
        if(name=="item"){
            std::string id=xmlAttr(tag,"id"),href=xmlAttr(tag,"href");
            if(!id.empty()&&!href.empty()){
                href=href.substr(0,href.find('#'));
                std::string target=normalizeZipPath(zipDirectory(opfPath)+href);
                manifest[id]=target;
            }
        }
        p=e+1;
    }
    out.clear();int count=0;
    for(size_t p=0;(p=opf.find("<",p))!=std::string::npos;){
        size_t e=opf.find('>',p);if(e==std::string::npos)break;
        std::string tag=opf.substr(p,e-p+1);
        size_t nameEnd=tag.find_first_of(" \t\r\n/>",1);
        std::string name=lower(tag.substr(1,nameEnd==std::string::npos?std::string::npos:nameEnd-1));
        size_t colon=name.rfind(':');if(colon!=std::string::npos)name=name.substr(colon+1);
        p=e+1;
        if(name!="itemref")continue;
        std::string id=xmlAttr(tag,"idref");
        auto it=manifest.find(id);if(it==manifest.end())continue;
        std::string actual=findZipEntry(zip,it->second);
        std::string html;if(actual.empty()||!readZipText(zip,actual,html))continue;
        out+="\n\n"+htmlToText(html);
        if(++count>250||out.size()>8*1024*1024)break;
    }
    if(out.empty()){err="EPUB sem texto legivel";return false;}
    return true;
}
static bool textFile(const std::string&path,std::string&out){
    std::ifstream f(path,std::ios::binary);if(!f)return false;
    out.assign(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());
    if(out.size()>8*1024*1024)out.resize(8*1024*1024);
    return !out.empty();
}
static std::vector<std::string> paginate(const std::string&text){
    std::string plain=displayAscii(text);
    const int width=72,rows=23;std::vector<std::string>lines;lines.reserve(1024);
    std::istringstream ss(plain);std::string para;
    while(std::getline(ss,para)&&lines.size()<20000){
        if(para.empty()){lines.push_back("");continue;}
        std::istringstream words(para);std::string word,line;
        while(words>>word){
            while(word.size()>width){if(!line.empty()){lines.push_back(line);line.clear();}lines.push_back(word.substr(0,width));word.erase(0,width);}
            if(line.empty())line=word;
            else if(line.size()+1+word.size()<=width)line+=" "+word;
            else{lines.push_back(line);line=word;}
        }
        if(!line.empty())lines.push_back(line);
    }
    std::vector<std::string>pages;
    for(size_t i=0;i<lines.size();i+=rows){
        std::string page;for(size_t j=i;j<lines.size()&&j<i+rows;j++)page+=lines[j]+"\n";
        pages.push_back(page);
    }
    return pages;
}
static SDL_Texture* textTexture(SDL_Renderer*r,const std::string&text,int&w,int&h){
    w=1280;h=720;
    SDL_Surface*s=SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_RGBA32);if(!s)return nullptr;
    SDL_FillRect(s,nullptr,SDL_MapRGBA(s->format,245,239,225,255));
    const Uint32 ink=SDL_MapRGBA(s->format,37,38,43,255);
    std::istringstream ss(text);std::string line;int y=34;
    while(std::getline(ss,line)&&y<690){int x=48;
        for(unsigned char c:line){if(x>1220)break;if(c<32||c>126)c='?';
            const uint16_t*glyph=FONT[c-32];
            for(int row=0;row<15;row++)for(int col=0;col<9;col++)if(glyph[row]&(1<<col)){
                SDL_Rect rect{x+col*2,y+row*2,2,2};SDL_FillRect(s,&rect,ink);
            }x+=16;
        }y+=29;
    }
    SDL_Texture*t=SDL_CreateTextureFromSurface(r,s);SDL_FreeSurface(s);return t;
}
static void drawCover(SDL_Renderer*r,SDL_Texture*t,int iw,int ih,int x,int y,int w,int h){
    if(!t||iw<=0||ih<=0)return;
    float ratio=std::min((float)w/iw,(float)h/ih);
    int rw=std::max(1,(int)(iw*ratio)),rh=std::max(1,(int)(ih*ratio));
    SDL_Rect dst{x+(w-rw)/2,y+(h-rh)/2,rw,rh};
    SDL_RenderCopy(r,t,nullptr,&dst);
}
static std::string compactTitle(const std::string&s,size_t maxChars=20){
    if(s.size()<=maxChars)return s;
    return s.substr(0,maxChars-3)+"...";
}
// Divide titulo em ate duas linhas grandes. Para titulos longos, preserva
// o inicio (obra) e o fim (volume/capitulo), com reticencias no meio.
static std::vector<std::string> libraryTitleLines(const std::string& title, size_t lineChars=11){
    if(title.size()<=lineChars)return {title};
    if(title.size()<=lineChars*2){
        size_t split=title.rfind(' ',lineChars);
        if(split==std::string::npos||split<lineChars/2)split=lineChars;
        std::string first=title.substr(0,split);
        size_t secondStart=split;
        while(secondStart<title.size()&&title[secondStart]==' ')++secondStart;
        return {first,title.substr(secondStart)};
    }
    // Tres pontos ocupam espaco; manter o sufixo mais informativo.
    const size_t prefixChars=lineChars;
    const size_t suffixChars=lineChars-3;
    return {title.substr(0,prefixChars),"..."+title.substr(title.size()-suffixChars)};
}
// Nome exibido na biblioteca: nao altera a pasta, os arquivos ou a busca de capas.
static std::string cleanLibraryWorkTitle(std::string title){
    title=stripExtension(title);
    // Remove marcadores entre parenteses/colchetes usados em edicoes e scanlations.
    // Mantem parenteses que fazem parte do titulo quando nao forem metadados.
    const char* tags[]={"colored","colour","colorido","colorida","full color","full colour",
        "official color","official coloured","official colored","digital color","digital colored",
        "pt-br","pt br","portugues","português","english","scan","scanlation",
        "completo","complete","versao colorida","versão colorida","edition","edicao","edição"};
    for(size_t i=0;i<title.size();){
        if(title[i]!='('&&title[i]!='['&&title[i]!='{'){++i;continue;}
        char end=title[i]=='('?')':title[i]=='['?']':'}';
        size_t j=title.find(end,i+1);if(j==std::string::npos){++i;continue;}
        std::string inside=lower(title.substr(i+1,j-i-1));
        bool meta=false;
        for(auto tag:tags)if(inside.find(tag)!=std::string::npos){meta=true;break;}
        if(meta){title.erase(i,j-i+1);continue;}
        i=j+1;
    }
    // Remove sufixos descritivos sem apagar subtitulos legitimos.
    std::string lo=lower(title);
    const char* suffixes[]={" - official colored"," - official coloured"," - colored",
        " - colorido"," - colorida"," - full color"," - full colour"," - pt-br",
        " - scanlation"," - edição especial"," - edicao especial"};
    for(auto suffix:suffixes){auto pos=lo.find(suffix);if(pos!=std::string::npos){title.resize(pos);break;}}
    title=workTitle(title);
    // Alguns nomes de pastas incluem "Capitulo 01" sem separador " - ".
    std::string lowTitle=lower(title);
    const char* chapterMarkers[]={" capitulo "," capítulo "," captulo "," chapter "," volume "," vol. "," cap. "};
    for(auto marker:chapterMarkers){
        size_t at=lowTitle.find(marker);
        if(at!=std::string::npos && at+strlen(marker)<lowTitle.size()
           && std::isdigit((unsigned char)lowTitle[at+strlen(marker)])){
            title.resize(at);break;
        }
    }
    size_t first=title.find_first_not_of(" \t-_"),last=title.find_last_not_of(" \t-_");
    if(first==std::string::npos)return "SEM TITULO";
    return title.substr(first,last-first+1);
}
// Escala intermediaria de fonte bitmap: 14 px por caractere, em vez de 10 ou 20.
static void labelMedium(SDL_Renderer*r,int x,int y,const std::string& str,
                        Uint8 R=244,Uint8 G=244,Uint8 B=248){
    SDL_SetRenderDrawColor(r,R,G,B,255);
    for(unsigned char c:str){
        if(c<32||c>126)c='?';
        const uint16_t* glyph=FONT[c-32];
        for(int row=0;row<15;row++)for(int col=0;col<9;col++)if(glyph[row]&(1<<col)){
            SDL_Rect d{x+(col*14)/10,y+(row*21)/15,2,2};
            SDL_RenderFillRect(r,&d);
        }
        x+=14;if(x>1250)break;
    }
}
static std::vector<std::string> cleanWorkTitleLines(const std::string& title){
    // Ate 15 caracteres por linha, maximo duas linhas.
    if(title.size()<=15)return {title};
    if(title.size()<=30){
        size_t split=title.rfind(' ',15);
        if(split==std::string::npos||split<7)split=15;
        size_t next=split;while(next<title.size()&&title[next]==' ')++next;
        return {title.substr(0,split),title.substr(next)};
    }
    return {title.substr(0,15),"..."+title.substr(title.size()-12)};
}
static void fill(SDL_Renderer*r,int x,int y,int w,int h,Uint8 R,Uint8 G,Uint8 B){SDL_SetRenderDrawColor(r,R,G,B,255);SDL_Rect d{x,y,w,h};SDL_RenderFillRect(r,&d);}
static void label(SDL_Renderer*r,int x,int y,const std::string& s,int scale=2,Uint8 R=240,Uint8 G=240,Uint8 B=240){SDL_SetRenderDrawColor(r,R,G,B,255);for(unsigned char c:s){if(c<32||c>126)c='?';auto glyph=FONT[c-32];for(int row=0;row<15;row++)for(int col=0;col<9;col++)if(glyph[row]&(1<<col)){SDL_Rect d{x+col*scale,y+row*scale,scale,scale};SDL_RenderFillRect(r,&d);}x+=10*scale;if(x>1260)break;}}
// Fonte de biblioteca mais encorpada, preservando o desenho bitmap original.
static void labelBold(SDL_Renderer*r,int x,int y,const std::string& text,
                      Uint8 red=244,Uint8 green=244,Uint8 blue=248){
    label(r,x,y,text,1,red,green,blue);
    label(r,x+1,y,text,1,red,green,blue);
}
// Botao de rotacao com visual discreto; area de toque mantida no codigo existente.
static void roundedFill(SDL_Renderer* r,int x,int y,int w,int h,int rad,Uint8 R,Uint8 G,Uint8 B){
    SDL_SetRenderDrawColor(r,R,G,B,255);
    for(int yy=0;yy<h;yy++){
        int inset=0;
        if(yy<rad || yy>=h-rad){
            int d=(yy<rad)?rad-yy-1:yy-(h-rad);
            inset=rad-(int)std::sqrt((double)(rad*rad-d*d));
        }
        SDL_Rect line{x+inset,y+yy,w-2*inset,1};SDL_RenderFillRect(r,&line);
    }
}
// Icone PNG antialias, carregado uma unica vez por renderer (sem depender do SD).
static SDL_Texture* rotateIconTexture(SDL_Renderer* r){
    static SDL_Texture* icon=nullptr;
    static SDL_Renderer* owner=nullptr;
    if(owner!=r){icon=nullptr;owner=r;}
    if(!icon){
        SDL_RWops* rw=SDL_RWFromConstMem(ROTATE_CW_PNG,(int)ROTATE_CW_PNG_SIZE);
        SDL_Surface* surface=rw?IMG_Load_RW(rw,1):nullptr;
        if(surface){icon=SDL_CreateTextureFromSurface(r,surface);SDL_FreeSurface(surface);}
        if(icon)SDL_SetTextureBlendMode(icon,SDL_BLENDMODE_BLEND);
    }
    return icon;
}
static SDL_Texture* upIconTexture(SDL_Renderer* r){
    static SDL_Texture* icon=nullptr;static SDL_Renderer* owner=nullptr;
    if(owner!=r){icon=nullptr;owner=r;}
    if(!icon){
        SDL_RWops* rw=SDL_RWFromConstMem(ARROW_UP_PNG,(int)ARROW_UP_PNG_SIZE);
        SDL_Surface* surface=rw?IMG_Load_RW(rw,1):nullptr;
        if(surface){icon=SDL_CreateTextureFromSurface(r,surface);SDL_FreeSurface(surface);}
        if(icon)SDL_SetTextureBlendMode(icon,SDL_BLENDMODE_BLEND);
    }
    return icon;
}
static void drawTopButton(SDL_Renderer* r,int x,int y,int w,int h){
    const int bw=76,bh=54,bx=x+(w-bw)/2,by=y+(h-bh)/2;
    roundedFill(r,bx,by,bw,bh,13,127,28,57);
    roundedFill(r,bx+2,by+2,bw-4,bh-4,11,153,31,67);
    if(SDL_Texture* icon=upIconTexture(r)){
        const SDL_Rect dst{bx+(bw-40)/2,by+(bh-40)/2,40,40};
        SDL_RenderCopy(r,icon,nullptr,&dst);
    }
}
static void drawRotateButton(SDL_Renderer* r,int x,int y,int w,int h){
    // Mesmo tamanho e hitbox do Teste 5; volta a paleta vinho do Teste 4.
    const int bw=76,bh=54,bx=x+(w-bw)/2,by=y+(h-bh)/2;
    roundedFill(r,bx,by,bw,bh,13,127,28,57);
    roundedFill(r,bx+2,by+2,bw-4,bh-4,11,153,31,67);
    if(SDL_Texture* icon=rotateIconTexture(r)){
        const SDL_Rect dst{bx+(bw-40)/2,by+(bh-40)/2,40,40};
        SDL_RenderCopy(r,icon,nullptr,&dst);
    }
}

int main(int argc,char**argv){(void)argc;(void)argv;
    // Log de diagnóstico antes de inicializar os componentes gráficos.
    mkdir(ROOT,0777);FILE* log=fopen("sdmc:/switch/MangaCAP/startup.log","w");
    auto mark=[&](const char*s){if(log){fprintf(log,"%s\n",s);fflush(log);}};
    mark("inicio");if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)!=0){mark(SDL_GetError());if(log)fclose(log);return 1;}mark("SDL OK");
    IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG|IMG_INIT_WEBP);mark("IMG init");
    SDL_Window*win=SDL_CreateWindow("MangaCAP Switch",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1280,720,SDL_WINDOW_SHOWN);if(!win){mark(SDL_GetError());return 1;}
    SDL_Renderer*r=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED);if(!r)r=SDL_CreateRenderer(win,-1,SDL_RENDERER_SOFTWARE);if(!r){mark(SDL_GetError());return 1;}mark("renderer OK");
    // Teste 2: renderizacao isolada em textura para rotacao somente do aplicativo.
    SDL_Texture*portraitFrame=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,720,1280);
    SDL_Texture*landscapeFrame=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,1280,720);
    if(portraitFrame)SDL_SetTextureBlendMode(portraitFrame,SDL_BLENDMODE_NONE);
    bool portraitMode=false;
    const SDL_Rect portraitButton{1050,112,210,62};
    const SDL_Rect topButton{850,112,200,62};
    const SDL_Rect portraitTopButton{300,40,200,60};
    auto togglePortrait=[&](){
        if(!portraitFrame || !landscapeFrame){mark("Rotacao indisponivel: sem textura de destino");return;}
        portraitMode=!portraitMode;
        mark(portraitMode?"Modo retrato ativado":"Modo horizontal ativado");
    };
    SDL_GameController*ctl=nullptr;for(int i=0;i<SDL_NumJoysticks();i++)if(SDL_IsGameController(i)){ctl=SDL_GameControllerOpen(i);break;}
    // Inicializa rede somente para busca opcional de capas.
    bool onlineEnabled=(socketInitializeDefault()==0);
    if(onlineEnabled && curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK){socketExit();onlineEnabled=false;}
    mkdir("sdmc:/switch/MangaCAP/covers",0777);
    auto books=scan();auto works=groupBooks(books);mark("biblioteca OK");
    int selected=0,selectedVolume=0,activeBook=-1,libraryScreen=0,page=0,iw=1,ih=1;
    int libraryPortraitScroll=0;
    bool privateMode=false;
    SecurityData security=readSecurity();
    // 0=normal, 1=criar, 2=confirmar, 3=desbloquear, 4=mostrar recuperacao,
    // 5=recuperar, 6=menu seguranca, 7=PIN atual, 8=novo PIN, 9=confirmar novo.
    int securityScreen=0,securityDigit=0,securityFailures=0;
    std::string securityEntry,securityFirst,securityRecovery,securityNotice;
    bool privateUnlocked=false;
    auto beginSecurity=[&](int screen){securityScreen=screen;securityEntry.clear();securityDigit=0;securityNotice.clear();};
    std::map<std::string,int> pageCountCache;
    auto totalPages=[&](const std::string& path)->int{
        auto it=pageCountCache.find(path);if(it!=pageCountCache.end())return it->second;
        return pageCountCache[path]=knownPageCount(path);
    };
    std::vector<Cover> covers;
    auto refreshCovers=[&](){clearCovers(covers);covers.resize(books.size()+works.size());for(size_t i=0;i<books.size();++i)covers[i].path=books[i].path;};
    refreshCovers();
    auto lockPrivate=[&](){privateUnlocked=false;privateMode=false;libraryScreen=0;selected=selectedVolume=0;refreshCovers();};
    std::map<std::string,bool> coverTried;
    auto coverAt=[&](int idx,const std::string& title,bool isWork,int workIndex)->Cover*{
        const int coverIndex=isWork?(int)books.size()+workIndex:idx;
        if(coverIndex<0||coverIndex>=(int)covers.size()||idx<0||idx>=(int)books.size())return nullptr;
        Cover& c=covers[coverIndex];
        if(!c.attempted){
            c.attempted=true;
            const std::string workName=isWork?title:works[workIndex].title;
            const std::string workCache="sdmc:/switch/MangaCAP/covers/"+coverKey(workName)+".jpg";
            if(isWork){
                c.tex=coverFromFile(r,workCache,c.w,c.h);
                const std::string query=searchTitle(workName);
                if(!c.tex && onlineEnabled && !query.empty() && !coverTried[query]){
                    coverTried[query]=true;
                    if(downloadCover(workName,workCache))c.tex=coverFromFile(r,workCache,c.w,c.h);
                }
                if(!c.tex)c.tex=coverFromBook(r,books[idx],c.w,c.h);
            }else{
                // Volume/capitulo: prioriza a capa interna ou ao lado do arquivo.
                c.tex=coverFromBook(r,books[idx],c.w,c.h,false);
                // Caso nao tenha imagem propria (ex.: PDF), usa capa da obra.
                if(!c.tex)c.tex=coverFromFile(r,workCache,c.w,c.h);
            }
        }
        return &c;
    };
    float zoom=1.f;float panX=0.f,panY=0.f;bool continuous=true;bool showReaderHud=false;SDL_Texture*nextTex=nullptr;int nextW=1,nextH=1;CbzZip*z=nullptr;bool textReading=false;std::vector<std::string>textPages;SDL_Texture*tex=nullptr;std::vector<std::string> pages;std::string message;
    auto release=[&](){if(nextTex){SDL_DestroyTexture(nextTex);nextTex=nullptr;}if(tex){SDL_DestroyTexture(tex);tex=nullptr;}if(z){delete z;z=nullptr;}pages.clear();textPages.clear();textReading=false;};
    auto load=[&](){
        if(nextTex){SDL_DestroyTexture(nextTex);nextTex=nullptr;}
        if(tex){SDL_DestroyTexture(tex);tex=nullptr;}
        if((z||textReading)&&!pages.empty()){
            tex=textReading?textTexture(r,textPages[page],iw,ih):decode(r,z,pages[page],iw,ih);
            if(!tex)message="Falha ao decodificar imagem";else message.clear();
        }
    };
    auto ensureNext=[&](){
        if(continuous && (z||textReading) && !nextTex && page+1<(int)pages.size())
            nextTex=textReading?textTexture(r,textPages[page+1],nextW,nextH):decode(r,z,pages[page+1],nextW,nextH);
    };
    auto movePage=[&](int step,bool fromBottom){
        int newPage=page+step;
        if(newPage<0||newPage>=(int)pages.size())return;
        page=newPage;
        load();
        panX=0;
        if(continuous){
            float scale=((portraitMode?720.f:1280.f)/(float)iw)*zoom;
            float height=ih*scale;
            panY=fromBottom?std::max(0.f,height-(showReaderHud?697.f:720.f)):0.f;
        }else{
            // Pagina nova sempre comeca pelo topo, inclusive com zoom.
            float scale=std::min((portraitMode?720.f:1280.f)/(float)iw,(portraitMode?1120.f:697.f)/(float)ih)*zoom;
            panY=std::max(0.f,(ih*scale-(showReaderHud?697.f:720.f))*.5f);
        }
        if(activeBook>=0&&activeBook<(int)books.size())save(books[activeBook].path,page);
    };
    auto openBook=[&](int idx){
        if(idx<0||idx>=(int)books.size())return;
        release();
        activeBook=idx;
        std::string readPath=books[idx].path;
        if(ends(readPath,".epub")||ends(readPath,".txt")){
            std::string content,error;
            bool ok=ends(readPath,".epub")?epubText(readPath,content,error):textFile(readPath,content);
            if(!ok){message="Livro nao suportado: "+error;activeBook=-1;return;}
            textPages=paginate(content);
            if(textPages.empty()){message="Livro sem texto";activeBook=-1;return;}
            textReading=true;
            pages.resize(textPages.size());
            rememberPageCount(books[idx].path,(int)pages.size());
            pageCountCache[books[idx].path]=(int)pages.size();
            page=std::max(0,std::min(getSaved(books[idx].path),(int)pages.size()-1));
            zoom=1.f;panX=panY=0;showReaderHud=false;load();mark("EPUB/TXT aberto");return;
        }
        if(ends(readPath,".pdf")){
            std::string error;
            std::string cached;
            if(!pdfToCbzCache(readPath,ROOT,cached,error)){
                message="PDF nao suportado: "+error;
                mark("PDF: conversao falhou");
                return;
            }
            readPath=cached;
        }
        z=new CbzZip(readPath);
        if(!z->valid()){release();message="Nao foi possivel abrir arquivo";return;}
        pages=listPages(z);
        if(pages.empty()){release();message="Arquivo sem imagens";return;}
        rememberPageCount(books[idx].path,(int)pages.size());
        pageCountCache[books[idx].path]=(int)pages.size();
        page=std::max(0,std::min(getSaved(books[idx].path),(int)pages.size()-1));
        zoom=1.f;panX=panY=0;showReaderHud=false;load();mark("Livro aberto");
    };
    auto resumeVolume=[&](int workIndex)->int{
        if(workIndex<0||workIndex>=(int)works.size())return 0;
        const auto& v=works[workIndex].volumes;
        for(int i=(int)v.size()-1;i>=0;--i){
            const auto& path=books[v[i]].path;
            const int saved=getSaved(path),total=knownPageCount(path);
            if(saved>0 && (total<=0||saved<total-1))return i;
        }
        for(int i=(int)v.size()-1;i>=0;--i)if(getSaved(books[v[i]].path)>0)return i;
        return 0;
    };
    auto adjacentBook=[&](int step)->int{
        if(activeBook<0||selected<0||selected>=(int)works.size())return -1;
        const auto& vols=works[selected].volumes;
        for(size_t i=0;i<vols.size();++i)if(vols[i]==activeBook){
            int next=(int)i+step;
            return next>=0&&next<(int)vols.size()?vols[next]:-1;
        }
        return -1;
    };
    auto refreshLibrary=[&](){
        release();
        clearCovers(covers);
        books=scan(privateMode);works=groupBooks(books);
        selected=0;selectedVolume=0;libraryScreen=0;libraryPortraitScroll=0;activeBook=-1;
        refreshCovers();coverTried.clear();pageCountCache.clear();
    };
    auto goTop=[&](){
        if(page!=0){
            page=0;load();
            if(activeBook>=0&&activeBook<(int)books.size())save(books[activeBook].path,page);
        }
        panX=0.f;
        if(continuous)panY=0.f;
        else if(iw>0 && ih>0){
            const float viewH=portraitMode?(showReaderHud?1120.f:1280.f):(showReaderHud?697.f:720.f);
            const float viewW=portraitMode?720.f:1280.f;
            const float scale=std::min(viewW/(float)iw,viewH/(float)ih)*zoom;
            panY=std::max(0.f,(ih*scale-viewH)*.5f);
        }
        showReaderHud=false;
    };
    bool touchDragging=false, touchMoved=false,libraryTouchDragging=false;
    int libraryHorizontalScroll=0;
    SDL_FingerID activeFinger=0;
    float touchStartX=0, touchStartY=0, touchLastX=0, touchLastY=0;
    auto portraitLibraryCount=[&]()->int{return libraryScreen==0?(int)works.size():(works.empty()?0:(int)works[selected].volumes.size());};
    auto portraitLibraryMaxScroll=[&]()->int{int rows=(portraitLibraryCount()+2)/3;return std::max(0,rows*340-1000);};
    auto horizontalLibraryMaxScroll=[&]()->int{int rows=(portraitLibraryCount()+4)/5;return std::max(0,rows*285-460);};
    auto logicalTouch=[&](float px,float py)->SDL_FPoint{
        if(!portraitMode)return SDL_FPoint{px*1280.f,py*720.f};
        float x=py*720.f,y=(1.f-px)*1280.f;
        if(z||textReading)return SDL_FPoint{x,y};
        return SDL_FPoint{x*1280.f/720.f,(y-437.5f)*720.f/405.f};
    };
    bool running=true;while(running){SDL_Event ev;while(SDL_PollEvent(&ev)){
        if(ev.type==SDL_QUIT){running=false;break;}
        if(ev.type==SDL_KEYDOWN && ev.key.keysym.sym==SDLK_F9){togglePortrait();continue;}
        if(ev.type==SDL_FINGERDOWN || ev.type==SDL_FINGERMOTION || ev.type==SDL_FINGERUP){
            // No retrato, a biblioteca e desenhada em uma textura horizontal
            // reduzida, mas o botao de rotacao e desenhado na textura vertical.
            // Testar esse botao com coordenadas verticais ANTES de remapear
            // os demais toques para a biblioteca evita perder a area clicavel.
            const float portraitX=ev.tfinger.y*720.f;
            const float portraitY=(1.f-ev.tfinger.x)*1280.f;
            if(ev.type==SDL_FINGERDOWN && portraitMode && !(z||textReading) &&
               portraitX>=500.f && portraitX<700.f &&
               portraitY>=40.f && portraitY<100.f){
                togglePortrait();continue;
            }
            // Grade vertical: arrasto rola a biblioteca; toque curto seleciona uma capa.
            if(portraitMode && !(z||textReading) && !securityScreen){
                if(ev.type==SDL_FINGERDOWN){
                    libraryTouchDragging=true;touchMoved=false;activeFinger=ev.tfinger.fingerId;
                    touchStartX=touchLastX=portraitX;touchStartY=touchLastY=portraitY;
                    continue;
                }
                if(libraryTouchDragging && ev.tfinger.fingerId==activeFinger){
                    if(std::abs(portraitY-touchStartY)>14.f)touchMoved=true;
                    if(ev.type==SDL_FINGERMOTION){
                        if(touchMoved){libraryPortraitScroll=std::max(0,std::min(portraitLibraryMaxScroll(),libraryPortraitScroll+(int)(touchLastY-portraitY)));}
                        touchLastY=portraitY;continue;
                    }
                    if(ev.type==SDL_FINGERUP){
                        libraryTouchDragging=false;
                        if(touchMoved)continue;
                        if(portraitY<145.f && libraryScreen==1){libraryScreen=0;libraryPortraitScroll=0;continue;}
                        const int col=((int)portraitX-20)/230;
                        const int row=((int)portraitY-190+libraryPortraitScroll)/340;
                        if(portraitX>=20 && portraitX<710 && portraitY>=190 && portraitY<1220 && col>=0 && col<3 && row>=0){
                            int idx=row*3+col;
                            if(idx<portraitLibraryCount()){
                                if(libraryScreen==0){selected=idx;selectedVolume=resumeVolume(selected);libraryScreen=1;libraryPortraitScroll=0;}
                                else{selectedVolume=idx;openBook(works[selected].volumes[idx]);}
                            }
                        }
                        continue;
                    }
                }
                continue;
            }
            // A mesma grade com rolagem vertical no modo horizontal (5 colunas).
            if(!portraitMode && !(z||textReading) && !securityScreen){
                const float hx=ev.tfinger.x*1280.f,hy=ev.tfinger.y*720.f;
                if(ev.type==SDL_FINGERDOWN &&
                   hx>=portraitButton.x && hx<portraitButton.x+portraitButton.w &&
                   hy>=portraitButton.y && hy<portraitButton.y+portraitButton.h){
                    togglePortrait();continue;
                }
                if(ev.type==SDL_FINGERDOWN){
                    libraryTouchDragging=true;touchMoved=false;activeFinger=ev.tfinger.fingerId;
                    touchStartX=touchLastX=hx;touchStartY=touchLastY=hy;
                    continue;
                }
                if(libraryTouchDragging && ev.tfinger.fingerId==activeFinger){
                    if(std::abs(hy-touchStartY)>14.f)touchMoved=true;
                    if(ev.type==SDL_FINGERMOTION){
                        if(touchMoved)libraryHorizontalScroll=std::max(0,std::min(horizontalLibraryMaxScroll(),libraryHorizontalScroll+(int)(touchLastY-hy)));
                        touchLastY=hy;continue;
                    }
                    if(ev.type==SDL_FINGERUP){
                        libraryTouchDragging=false;
                        if(touchMoved)continue;
                        if(hy>=112.f && hy<165.f && libraryScreen==1){libraryScreen=0;libraryHorizontalScroll=0;continue;}
                        if(hy>=165.f && hy<655.f && hx>=36.f && hx<1260.f){
                            int col=((int)hx-36)/244;
                            int row=((int)hy-174+libraryHorizontalScroll)/285;
                            if(col>=0&&col<5&&row>=0){
                                int idx=row*5+col;
                                if(idx<portraitLibraryCount()){
                                    if(libraryScreen==0){selected=idx;selectedVolume=resumeVolume(selected);libraryScreen=1;libraryHorizontalScroll=0;}
                                    else{selectedVolume=idx;openBook(works[selected].volumes[idx]);}
                                }
                            }
                        }
                        continue;
                    }
                }
                continue;
            }
            SDL_FPoint pt=logicalTouch(ev.tfinger.x,ev.tfinger.y);
            if(ev.type==SDL_FINGERDOWN){
                touchDragging=(z||textReading);touchMoved=false;activeFinger=ev.tfinger.fingerId;
                touchStartX=touchLastX=pt.x;touchStartY=touchLastY=pt.y;
                // No leitor, processar o toque apenas ao soltar para distinguir arrasto de clique.
                if(touchDragging)continue;
            }else if(touchDragging && ev.tfinger.fingerId==activeFinger){
                float dx=pt.x-touchLastX,dy=pt.y-touchLastY;
                if(std::abs(pt.x-touchStartX)>13.f || std::abs(pt.y-touchStartY)>13.f)touchMoved=true;
                if(ev.type==SDL_FINGERMOTION && touchMoved){
                    panX+=dx;
                    // Arrastar o conteudo para cima avanca a leitura.
                    panY+=continuous?-dy:dy;
                }
                touchLastX=pt.x;touchLastY=pt.y;
                if(ev.type==SDL_FINGERUP){
                    touchDragging=false;
                    if(touchMoved)continue;
                    // Reutiliza o tratamento de clique abaixo.
                    ev.type=SDL_FINGERDOWN;
                }else continue;
            }
            if(ev.type==SDL_FINGERDOWN){
                const SDL_Rect button=portraitMode?SDL_Rect{500,40,200,60}:portraitButton;
                bool visible=!(z||textReading)||showReaderHud;
                if(visible && pt.x>=button.x && pt.x<button.x+button.w &&
                   pt.y>=button.y && pt.y<button.y+button.h){
                    togglePortrait();showReaderHud=false;continue;
                }
                // As demais areas de toque continuam usando as coordenadas originais.
                if(portraitMode && (z||textReading)){
                    if(showReaderHud && pt.x>=portraitTopButton.x && pt.x<portraitTopButton.x+portraitTopButton.w &&
                       pt.y>=portraitTopButton.y && pt.y<portraitTopButton.y+portraitTopButton.h){
                        goTop();continue;
                    }
                    // Botao de capitulo no rodape do layout retrato.
                    if(showReaderHud && pt.y>=1130.f && pt.y<=1220.f){
                        int target=adjacentBook(pt.x<360.f?-1:1);
                        if(target>=0){
                            if(activeBook>=0)save(books[activeBook].path,page);
                            openBook(target);
                            const auto& vols=works[selected].volumes;
                            for(size_t i=0;i<vols.size();++i)if(vols[i]==target)selectedVolume=(int)i;
                        }
                        continue;
                    }
                    showReaderHud=!showReaderHud;
                    continue;
                }
                if(portraitMode){
                    ev.tfinger.x=pt.x/1280.f;
                    ev.tfinger.y=pt.y/720.f;
                }
            }
        }
        // Toque revela os controles. Com controles visiveis, tocar em
        // "TOPO" retorna ao inicio do volume; outros toques ocultam.
        if(ev.type==SDL_FINGERDOWN && securityScreen){
            const int tx=(int)(ev.tfinger.x*1280.f);
            const int ty=(int)(ev.tfinger.y*720.f);
            auto submitSecurity=[&](){
                SDL_Event key={};key.type=SDL_KEYDOWN;key.key.keysym.sym=SDLK_RETURN;
                SDL_PushEvent(&key);
            };
            if(securityScreen==4){
                if(tx>=380&&tx<=900&&ty>=555&&ty<=680)submitSecurity();
            }else if(securityScreen==6){
                if(ty>=270&&ty<=385){
                    if(tx<550)beginSecurity(7);
                    else if(tx<1000)beginSecurity(5);
                }else if(ty>=610)beginSecurity(0);
            }else{
                // Teclado 3x4: 1-9, apagar, 0, confirmar.
                // Coordenadas correspondem aos retangulos desenhados abaixo.
                const int left=350,top=280,cellW=188,cellH=79,gap=12;
                int col=(tx-left)/(cellW+gap),row=(ty-top)/(cellH+gap);
                bool inside=tx>=left&&ty>=top&&col>=0&&col<3&&row>=0&&row<4
                    &&tx<left+col*(cellW+gap)+cellW
                    &&ty<top+row*(cellH+gap)+cellH;
                if(inside){
                    int digit=-1;
                    if(row<3)digit=row*3+col+1;
                    else if(col==1)digit=0;
                    if(digit>=0){
                        const size_t limit=securityScreen==5?12u:6u;
                        if(securityEntry.size()<limit)securityEntry+=(char)('0'+digit);
                        // Acesso diario: sexto toque ja confirma.
                        if(securityScreen==3&&securityEntry.size()==6)submitSecurity();
                    }else if(row==3&&col==0){
                        if(!securityEntry.empty())securityEntry.pop_back();
                    }else if(row==3&&col==2){submitSecurity();}
                }
                if(tx>=90&&tx<=310&&ty>=620&&ty<=690)beginSecurity(0);
            }
            continue;
        }
        if(ev.type==SDL_FINGERDOWN){
            if(z||textReading){
                const float touchX=ev.tfinger.x*1280.f;
                const float touchY=ev.tfinger.y*720.f;
                if(showReaderHud && !pages.empty() && touchX>=260.f && touchX<=1020.f && touchY>=625.f && touchY<=690.f){
                    int step=touchX<640.f?-1:1;
                    int target=adjacentBook(step);
                    if(target>=0){
                        if(activeBook>=0)save(books[activeBook].path,page);
                        openBook(target);
                        const auto& vols=works[selected].volumes;
                        for(size_t i=0;i<vols.size();++i)if(vols[i]==target)selectedVolume=(int)i;
                    }
                    continue;
                }
                if(showReaderHud && touchX>=topButton.x && touchY>=topButton.y &&
                   touchX<topButton.x+topButton.w && touchY<topButton.y+topButton.h){
                    goTop();
                }else{
                    showReaderHud=!showReaderHud;
                }
            }else if(libraryScreen==0 && !works.empty()){
                // 4 colunas, 2 linhas. Toque no cartao abre volumes.
                int x=(int)(ev.tfinger.x*1280.f),y=(int)(ev.tfinger.y*720.f);
                if(y>=164 && y<604 && x>=36 && x<1256){
                    int col=(x-36)/244,row=0;
                    int first=(selected/5)*5;
                    int idx=first+row*5+col;
                    if(col>=0 && col<5 && row<1 && idx<(int)works.size()){
                        selected=idx;selectedVolume=resumeVolume(selected);libraryScreen=1;
                    }
                }
            }else if(libraryScreen==1 && !works.empty()){
                int x=(int)(ev.tfinger.x*1280.f),y=(int)(ev.tfinger.y*720.f);
                if(y<100){libraryScreen=0;}
                else if(y>=164 && y<604 && x>=36 && x<1256){
                    int col=(x-36)/244,row=0;
                    int first=(selectedVolume/5)*5;
                    int idx=first+row*5+col;
                    if(col>=0 && col<5 && row<1 && idx<(int)works[selected].volumes.size()){
                        selectedVolume=idx;openBook(works[selected].volumes[idx]);
                    }
                }
            }
            continue;
        }
        if(ev.type!=SDL_CONTROLLERBUTTONDOWN&&ev.type!=SDL_KEYDOWN)continue;
        int b=ev.type==SDL_CONTROLLERBUTTONDOWN?ev.cbutton.button:-1;SDL_Keycode k=ev.type==SDL_KEYDOWN?ev.key.keysym.sym:0;
        if(b==SDL_CONTROLLER_BUTTON_START||k==SDLK_ESCAPE){running=false;break;}
        if(securityScreen){
            if(securityScreen==4){
                if(b==SDL_CONTROLLER_BUTTON_B||k==SDLK_RETURN){securityScreen=0;securityRecovery.clear();privateUnlocked=true;privateMode=true;refreshLibrary();}
                continue;
            }
            if(securityScreen==6){
                if(b==SDL_CONTROLLER_BUTTON_B||k==SDLK_RETURN){beginSecurity(7);}
                else if(b==SDL_CONTROLLER_BUTTON_X||k==SDLK_F6){beginSecurity(5);}
                else if(b==SDL_CONTROLLER_BUTTON_A||k==SDLK_BACKSPACE){beginSecurity(0);}
                continue;
            }
            if(b==SDL_CONTROLLER_BUTTON_A||k==SDLK_BACKSPACE){
                if(securityScreen==3||securityScreen==5){beginSecurity(0);}
                else if(securityScreen==1||securityScreen==2){beginSecurity(0);}
                else if(securityScreen==7||securityScreen==8||securityScreen==9){beginSecurity(6);}
                continue;
            }
            if(b==SDL_CONTROLLER_BUTTON_DPAD_UP||k==SDLK_UP){securityDigit=(securityDigit+1)%10;continue;}
            if(b==SDL_CONTROLLER_BUTTON_DPAD_DOWN||k==SDLK_DOWN){securityDigit=(securityDigit+9)%10;continue;}
            if(b==SDL_CONTROLLER_BUTTON_Y||k==SDLK_BACKSPACE){if(!securityEntry.empty())securityEntry.pop_back();continue;}
            if((b==SDL_CONTROLLER_BUTTON_B||k==SDLK_RIGHT)&&securityEntry.size()<(securityScreen==5?12u:6u)){
                securityEntry+=(char)('0'+securityDigit);continue;
            }
            if(k>=SDLK_0&&k<=SDLK_9&&securityEntry.size()<(securityScreen==5?12u:6u)){
                securityEntry+=(char)('0'+(k-SDLK_0));continue;
            }
            if(b==SDL_CONTROLLER_BUTTON_X||k==SDLK_RETURN){
                if(securityScreen==1&&securityEntry.size()==6){securityFirst=securityEntry;beginSecurity(2);}
                else if(securityScreen==2&&securityEntry==securityFirst){
                    security.salt=randomDigits(32);security.pinHash=securityHash(security.salt,"pin",securityEntry);
                    securityRecovery=randomDigits(12);security.recoveryHash=securityHash(security.salt,"recovery",securityRecovery);
                    security.valid=writeSecurity(security);
                    if(security.valid){beginSecurity(4);}else{securityNotice="ERRO AO SALVAR. TENTE NOVAMENTE";beginSecurity(1);}
                }
                else if(securityScreen==3&&securityEntry.size()==6&&checkSecret(securityHash(security.salt,"pin",securityEntry),security.pinHash)){
                    securityFailures=0;securityScreen=0;privateUnlocked=true;privateMode=true;refreshLibrary();
                }
                else if(securityScreen==5&&securityEntry.size()==12&&checkSecret(securityHash(security.salt,"recovery",securityEntry),security.recoveryHash)){
                    securityFailures=0;beginSecurity(8);
                }
                else if(securityScreen==7&&securityEntry.size()==6&&checkSecret(securityHash(security.salt,"pin",securityEntry),security.pinHash))beginSecurity(8);
                else if(securityScreen==8&&securityEntry.size()==6){securityFirst=securityEntry;beginSecurity(9);}
                else if(securityScreen==9&&securityEntry==securityFirst){
                    security.pinHash=securityHash(security.salt,"pin",securityEntry);
                    securityRecovery=randomDigits(12);security.recoveryHash=securityHash(security.salt,"recovery",securityRecovery);
                    if(writeSecurity(security))beginSecurity(4);else beginSecurity(6);
                }
                else {securityNotice="CODIGO INCORRETO OU INCOMPLETO";securityEntry.clear();securityFailures++;}
                continue;
            }
            continue;
        }
        if(!z&&!textReading){
            bool back=(b==SDL_CONTROLLER_BUTTON_A||k==SDLK_BACKSPACE);
            bool accept=(b==SDL_CONTROLLER_BUTTON_B||k==SDLK_RETURN);
            if(b==SDL_CONTROLLER_BUTTON_BACK||k==SDLK_F7){
                if(privateMode){privateMode=false;privateUnlocked=false;refreshLibrary();}
                else if(privateUnlocked){privateMode=true;refreshLibrary();}
                else beginSecurity(security.valid?3:1);
                continue;
            }
            if((b==SDL_CONTROLLER_BUTTON_RIGHTSTICK||k==SDLK_F8)&&privateMode&&privateUnlocked){beginSecurity(6);continue;}
            if((b==SDL_CONTROLLER_BUTTON_RIGHTSTICK||k==SDLK_F8)&&!privateMode&&security.valid){beginSecurity(5);continue;}
            if(b==SDL_CONTROLLER_BUTTON_Y||k==SDLK_F5){refreshLibrary();continue;}
            // X: descarta a capa online da obra selecionada e pesquisa novamente.
            if((b==SDL_CONTROLLER_BUTTON_X||k==SDLK_F6)&&!works.empty()){
                std::string key=coverKey(works[selected].title);
                std::string cache=std::string("sdmc:/switch/MangaCAP/covers/")+key+".jpg";
                remove(cache.c_str());
                coverTried.clear();
                refreshCovers();
                continue;
            }
            if(back && libraryScreen==1){libraryScreen=0;libraryPortraitScroll=0;libraryHorizontalScroll=0;continue;}
            int count=libraryScreen==0?(int)works.size():(works.empty()?0:(int)works[selected].volumes.size());
            int& pos=libraryScreen==0?selected:selectedVolume;
            if(count>0){
                int navCols=portraitMode?3:5;
                if(b==SDL_CONTROLLER_BUTTON_DPAD_RIGHT||k==SDLK_RIGHT)pos=std::min(count-1,pos+1);
                if(b==SDL_CONTROLLER_BUTTON_DPAD_LEFT||k==SDLK_LEFT)pos=std::max(0,pos-1);
                if(b==SDL_CONTROLLER_BUTTON_DPAD_DOWN||k==SDLK_DOWN)pos=std::min(count-1,pos+navCols);
                if(b==SDL_CONTROLLER_BUTTON_DPAD_UP||k==SDLK_UP)pos=std::max(0,pos-navCols);
                if(!portraitMode){
                    int y=(pos/5)*285;
                    if(y<libraryHorizontalScroll)libraryHorizontalScroll=y;
                    if(y+275>libraryHorizontalScroll+460)libraryHorizontalScroll=y+275-460;
                    libraryHorizontalScroll=std::max(0,std::min(horizontalLibraryMaxScroll(),libraryHorizontalScroll));
                }
                if(portraitMode){
                    int y=(pos/3)*340;
                    if(y<libraryPortraitScroll)libraryPortraitScroll=y;
                    if(y+320>libraryPortraitScroll+1000)libraryPortraitScroll=y+320-1000;
                    libraryPortraitScroll=std::max(0,std::min(portraitLibraryMaxScroll(),libraryPortraitScroll));
                }
                if(accept){
                    if(libraryScreen==0){libraryScreen=1;selectedVolume=resumeVolume(selected);libraryPortraitScroll=0;libraryHorizontalScroll=0;}
                    else openBook(works[selected].volumes[selectedVolume]);
                }
            }
        }
        else {
            // No Switch, SDL2 reporta as teclas fisicas A/B invertidas.
            if(b==SDL_CONTROLLER_BUTTON_A||k==SDLK_BACKSPACE){
                if(activeBook>=0&&activeBook<(int)books.size()) {
                    save(books[activeBook].path,page);
                }
                release();
                continue;
            }
            if(b==SDL_CONTROLLER_BUTTON_X||k==SDLK_TAB){
                continuous=!continuous;panX=panY=0;load();continue;
            }
            if(b==SDL_CONTROLLER_BUTTON_DPAD_RIGHT||k==SDLK_RIGHT)movePage(1,false);
            if(b==SDL_CONTROLLER_BUTTON_DPAD_LEFT||k==SDLK_LEFT)movePage(-1,true);
            if(b==SDL_CONTROLLER_BUTTON_RIGHTSHOULDER||k==SDLK_EQUALS)
                zoom=std::min(3.f,zoom+.15f);
            if(b==SDL_CONTROLLER_BUTTON_LEFTSHOULDER||k==SDLK_MINUS)
                zoom=std::max(.5f,zoom-.15f);
        }
    }
    // Movimento continuo dos analogicos, com zona morta e rolagem vertical.
    float axisX=0.f,axisY=0.f;
    if(ctl){
        auto norm=[](Sint16 v)->float{
            const float f=v<0?v/32768.f:v/32767.f;
            const float dead=.18f;
            return f>dead?(f-dead)/(1.f-dead):(f< -dead?(f+dead)/(1.f-dead):0.f);
        };
        float lx=norm(SDL_GameControllerGetAxis(ctl,SDL_CONTROLLER_AXIS_LEFTX));
        float ly=norm(SDL_GameControllerGetAxis(ctl,SDL_CONTROLLER_AXIS_LEFTY));
        float rx=norm(SDL_GameControllerGetAxis(ctl,SDL_CONTROLLER_AXIS_RIGHTX));
        float ry=norm(SDL_GameControllerGetAxis(ctl,SDL_CONTROLLER_AXIS_RIGHTY));
        axisX=std::abs(lx)>std::abs(rx)?lx:rx;
        axisY=std::abs(ly)>std::abs(ry)?ly:ry;
        if(SDL_GameControllerGetButton(ctl,SDL_CONTROLLER_BUTTON_DPAD_UP))axisY=-1.f;
        if(SDL_GameControllerGetButton(ctl,SDL_CONTROLLER_BUTTON_DPAD_DOWN))axisY=1.f;
    }
    const Uint8*keys=SDL_GetKeyboardState(nullptr);
    if(keys[SDL_SCANCODE_W])axisY=-1.f;
    if(keys[SDL_SCANCODE_S])axisY=1.f;
    if(keys[SDL_SCANCODE_A])axisX=-1.f;
    if(keys[SDL_SCANCODE_D])axisX=1.f;
    if(!(z||textReading) && !securityScreen && !portraitMode && std::abs(axisY)>.18f){
        libraryHorizontalScroll=std::max(0,std::min(horizontalLibraryMaxScroll(),libraryHorizontalScroll+(int)(axisY*13.f)));
    }
    if((z||textReading) && tex && iw>0 && ih>0){
        if(continuous){
            // Pagina ajustada a largura, rolando de cima para baixo.
            float scale=((portraitMode?720.f:1280.f)/(float)iw)*zoom;
            float h=ih*scale;
            float w=iw*scale;
            float maxX=std::max(0.f,(w-(portraitMode?720.f:1280.f))*.5f);
            panX=std::max(-maxX,std::min(maxX,panX-axisX*10.f));
            panY+=axisY*10.f;
            const float gap=20.f;
            // Quando o final de uma pagina passa do topo, a seguinte entra.
            if(panY>=h+gap && page+1<(int)pages.size()){
                float carry=panY-(h+gap);
                movePage(1,false);
                panY=carry;
            }else if(panY<0.f && page>0){
                float carry=panY;
                movePage(-1,false);
                panY=std::max(0.f,ih*((portraitMode?720.f:1280.f)/(float)iw)*zoom+gap+carry);
            }
            float currentH=ih*((portraitMode?720.f:1280.f)/(float)iw)*zoom;
            if(page+1==(int)pages.size())
                panY=std::min(panY,std::max(0.f,currentH-(portraitMode?(showReaderHud?1120.f:1280.f):(showReaderHud?697.f:720.f))));
            else
                panY=std::min(panY,currentH+gap);
            panY=std::max(0.f,panY);
        }else{
            float scale=std::min((portraitMode?720.f:1280.f)/(float)iw,(portraitMode?1120.f:697.f)/(float)ih)*zoom;
            float maxX=std::max(0.f,(iw*scale-(portraitMode?720.f:1280.f))*.5f);
            float maxY=std::max(0.f,(ih*scale-(portraitMode?(showReaderHud?1120.f:1280.f):(showReaderHud?697.f:720.f)))*.5f);
            panX=std::max(-maxX,std::min(maxX,panX-axisX*10.f));
            panY=std::max(-maxY,std::min(maxY,panY-axisY*10.f));
        }
    }
    if(portraitMode && landscapeFrame)SDL_SetRenderTarget(r,landscapeFrame);
    fill(r,0,0,1280,720,12,14,20);
    if(securityScreen){
        fill(r,0,0,1280,720,12,14,20);
        label(r,80,55,"MangaCAP  /  BIBLIOTECA PRIVADA",2,245,245,248);
        fill(r,80,115,1120,3,192,43,61);
        const char* prompt=securityScreen==1?"CRIAR PIN DE 6 DIGITOS":
            securityScreen==2?"CONFIRMAR PIN":securityScreen==3?"DIGITE O PIN":
            securityScreen==4?"GUARDE SEU CODIGO DE RECUPERACAO":
            securityScreen==5?"DIGITE CODIGO DE RECUPERACAO":
            securityScreen==6?"CONFIGURACOES DE SEGURANCA":
            securityScreen==7?"INFORME O PIN ATUAL":securityScreen==8?"NOVO PIN DE 6 DIGITOS":"CONFIRMAR NOVO PIN";
        label(r,110,165,prompt,2,240,240,245);
        if(securityScreen==4){
            label(r,130,275,securityRecovery,3,80,220,130);
            label(r,110,365,"ANOTE FORA DO SWITCH. ESTE CODIGO NAO SERA MOSTRADO DE NOVO.",1);
            label(r,110,610,"B: ENTENDI / ABRIR BIBLIOTECA",1);
        }else if(securityScreen==6){
            label(r,110,280,"B: ALTERAR PIN    X: RECUPERAR COM CODIGO    A: VOLTAR",1);
        }else{
            std::string masked(securityEntry.size(),'*');
            label(r,110,215,masked+" _",2,245,245,248);
            // Numeros grandes para toque; controles fisicos continuam ativos.
            const int left=350,top=280,cellW=188,cellH=79,gap=12;
            for(int row=0;row<4;row++)for(int col=0;col<3;col++){
                const int x=left+col*(cellW+gap),y=top+row*(cellH+gap);
                fill(r,x,y,cellW,cellH,35,42,56);
                fill(r,x,y,cellW,2,76,89,110);
                std::string txt;
                if(row<3)txt=std::to_string(row*3+col+1);
                else if(col==0)txt="APAGAR";
                else if(col==1)txt="0";
                else txt="OK";
                label(r,x+(col==0&&row==3?27:65),y+18,txt,(row==3&&col!=1)?2:3,245,245,248);
            }
            label(r,100,642,"A: VOLTAR",1,210,210,220);
            if(securityScreen==3)label(r,850,290,"6 DIGITOS",1,210,210,220);
            if(!securityNotice.empty())label(r,820,640,securityNotice,1,240,90,90);
        }
    }else if(!z&&!textReading){
        // Biblioteca horizontal em grade: 5 colunas, varias linhas e rolagem vertical.
        fill(r,0,0,1280,112,16,17,23);
        fill(r,0,108,1280,3,192,43,61);
        label(r,36,20,"MangaCAP",3,245,245,248);
        label(r,36,75,libraryScreen==0?(privateMode?"BIBLIOTECA PRIVADA":"MINHA BIBLIOTECA"):"VOLUMES DA OBRA",1,196,196,208);
        label(r,1060,36,"V0.7.8.2",2,211,72,88);
        if(libraryScreen==0)
            label(r,36,130,std::to_string(works.size())+" OBRAS  /  "+std::to_string(books.size())+" VOLUMES",1,175,178,188);
        else
            label(r,36,130,"< VOLTAR  /  "+compactTitle(works[selected].title,44),1,215,216,225);
        int count=portraitLibraryCount();
        if(count==0)label(r,55,290,"Copie EPUB/TXT/PDF/CBZ para /switch/MangaCAP/books",2);
        SDL_Rect libraryClip{0,170,1280,485};SDL_RenderSetClipRect(r,&libraryClip);
        int first=std::max(0,(libraryHorizontalScroll/285)*5);
        int last=std::min(count,first+20);
        for(int idx=first;idx<last;idx++){
            int col=idx%5,row=idx/5;
            int x=36+col*244,y=174+row*285-libraryHorizontalScroll;
            int bookIndex=libraryScreen==0?works[idx].volumes.front():works[selected].volumes[idx];
            bool active=idx==(libraryScreen==0?selected:selectedVolume);
            fill(r,x-3,y-3,218,230,active?213:30,active?60:31,active?77:39);
            fill(r,x,y,212,224,7,8,12);
            Cover*c=coverAt(bookIndex,libraryScreen==0?works[idx].title:books[bookIndex].name,libraryScreen==0,libraryScreen==0?idx:selected);
            if(c&&c->tex)drawCover(r,c->tex,c->w,c->h,x,y,212,224);
            else label(r,x+42,y+100,"SEM CAPA",1,170,170,182);
            std::string title=libraryScreen==0?cleanLibraryWorkTitle(works[idx].title):stripExtension(books[bookIndex].name);
            labelMedium(r,x,y+234,compactTitle(title,17));
            if(libraryScreen!=0){
                int total=totalPages(books[bookIndex].path);
                int saved=std::max(0,getSaved(books[bookIndex].path));
                int current=total>0?std::min(total,saved+1):saved+1;
                int percent=total>0?(int)(100.f*current/total+.5f):0;
                label(r,x,y+257,"PAG. "+std::to_string(current)+(total>0?" / "+std::to_string(total):""),1,177,183,195);
                fill(r,x,y+273,212,5,54,58,65);
                if(total>0)fill(r,x,y+273,(int)(212.f*percent/100.f),5,58,199,106);
            }
        }
        SDL_RenderSetClipRect(r,nullptr);
        fill(r,0,665,1280,55,22,23,30);
        label(r,28,680,libraryScreen==0?"ARRASTE PARA NAVEGAR   A: VOLUMES   X: CAPA   Y: ATUALIZAR":
              "ARRASTE PARA NAVEGAR   A: LER   B: VOLTAR   D-PAD: NAVEGAR",1);
    }else{
        SDL_Rect viewport{0,0,1280,showReaderHud?697:720};
        SDL_RenderSetClipRect(r,&viewport);
        if(tex){
            if(continuous){
                float scale=((portraitMode?720.f:1280.f)/(float)iw)*zoom;
                int w=(int)(iw*scale),h=(int)(ih*scale);
                SDL_Rect dst{(1280-w)/2+(int)panX,-(int)panY,w,h};
                SDL_RenderCopy(r,tex,nullptr,&dst);
                if(page+1<(int)pages.size() && dst.y+dst.h+20<viewport.h){
                    ensureNext();
                    if(nextTex){
                        float nscale=(1280.f/(float)nextW)*zoom;
                        int nw=(int)(nextW*nscale),nh=(int)(nextH*nscale);
                        SDL_Rect nextDst{(1280-nw)/2+(int)panX,dst.y+dst.h+20,nw,nh};
                        SDL_RenderCopy(r,nextTex,nullptr,&nextDst);
                    }
                }
            }else{
                float scale=std::min(1280.f/iw,(showReaderHud?697.f:720.f)/ih)*zoom;
                int w=(int)(iw*scale),h=(int)(ih*scale);
                SDL_Rect dst{(1280-w)/2+(int)panX,(viewport.h-h)/2+(int)panY,w,h};
                SDL_RenderCopy(r,tex,nullptr,&dst);
            }
        }
        SDL_RenderSetClipRect(r,nullptr);
        if(showReaderHud){
            if(!pages.empty()){
                fill(r,260,625,760,62,27,31,42);
                fill(r,275,634,342,44,63,69,83);
                fill(r,663,634,342,44,63,69,83);
                label(r,292,645,adjacentBook(-1)>=0?"< ANTERIOR":"PRIMEIRO CAP.",2);
                label(r,687,645,adjacentBook(1)>=0?"PROXIMO >":"EM DIA COM A OBRA",2);
            }
            // Botao flutuante visivel somente com a interface de leitura.
            drawTopButton(r,topButton.x,topButton.y,topButton.w,topButton.h);
            fill(r,0,697,1280,23,28,29,38);
            fill(r,0,713,(int)(1280.f*(page+1)/pages.size()),7,218,41,58);
            label(r,16,698,std::to_string(page+1)+" / "+std::to_string(pages.size())+
                  (continuous?" CONTINUO ":" PAGINA ")+
                  " B: voltar  Y: modo  L/R: zoom  Stick: rolar",1);
        }
    }
    if(!message.empty()){fill(r,0,626,1280,42,128,24,40);label(r,30,634,message,2);}
    // Botao da orientacao segue a visibilidade do HUD durante a leitura.
    if(!portraitMode && (!(z||textReading)||showReaderHud)){
        drawRotateButton(r,portraitButton.x,portraitButton.y,portraitButton.w,portraitButton.h);
    }
    if(portraitMode && portraitFrame){
        SDL_SetRenderTarget(r,portraitFrame);
        SDL_SetRenderDrawColor(r,12,14,20,255);SDL_RenderClear(r);
        if(z||textReading){
            const int viewH=showReaderHud?1120:1280;
            SDL_Rect clip{0,0,720,viewH};SDL_RenderSetClipRect(r,&clip);
            if(tex && iw>0 && ih>0){
                if(continuous){
                    float scale=720.f/(float)iw*zoom;
                    int w=(int)(iw*scale),h=(int)(ih*scale);
                    SDL_Rect dst{(720-w)/2+(int)panX,-(int)panY,w,h};
                    SDL_RenderCopy(r,tex,nullptr,&dst);
                    if(page+1<(int)pages.size() && dst.y+dst.h+20<viewH){
                        ensureNext();
                        if(nextTex){float ns=720.f/(float)nextW*zoom;
                            SDL_Rect nd{(720-(int)(nextW*ns))/2+(int)panX,dst.y+dst.h+20,(int)(nextW*ns),(int)(nextH*ns)};
                            SDL_RenderCopy(r,nextTex,nullptr,&nd);
                        }
                    }
                }else{
                    float scale=std::min(720.f/(float)iw,(float)viewH/(float)ih)*zoom;
                    int w=(int)(iw*scale),h=(int)(ih*scale);
                    SDL_Rect dst{(720-w)/2+(int)panX,(viewH-h)/2+(int)panY,w,h};
                    SDL_RenderCopy(r,tex,nullptr,&dst);
                }
            }
            SDL_RenderSetClipRect(r,nullptr);
            if(showReaderHud){
                fill(r,0,1120,720,160,27,31,42);
                fill(r,15,1140,335,75,63,69,83);
                fill(r,370,1140,335,75,63,69,83);
                label(r,28,1161,adjacentBook(-1)>=0?"< ANTERIOR":"PRIMEIRO",2);
                label(r,390,1161,adjacentBook(1)>=0?"PROXIMO >":"ULTIMO",2);
                label(r,24,1240,std::to_string(page+1)+" / "+std::to_string(pages.size()),1);
            }
        }else if(!securityScreen){
            // Biblioteca nativa 720x1280: 3 colunas, linhas ilimitadas, rolagem vertical.
            fill(r,0,0,720,145,16,17,23);
            fill(r,0,141,720,3,192,43,61);
            label(r,24,20,"MangaCAP",3,245,245,248);
            label(r,24,83,libraryScreen==0?(privateMode?"BIBLIOTECA PRIVADA":"MINHA BIBLIOTECA"):"< VOLUMES",1,196,196,208);
            int count=portraitLibraryCount();
            label(r,24,156,std::to_string(count)+(libraryScreen==0?" OBRAS":" VOLUMES"),1,175,178,188);
            SDL_Rect clip{0,188,720,1035};SDL_RenderSetClipRect(r,&clip);
            int start=std::max(0,libraryPortraitScroll/340*3);
            int end=std::min(count,start+15);
            for(int idx=start;idx<end;++idx){
                int col=idx%3,row=idx/3;
                int x=20+col*230,y=195+row*340-libraryPortraitScroll;
                int bookIndex=libraryScreen==0?works[idx].volumes.front():works[selected].volumes[idx];
                bool active=idx==(libraryScreen==0?selected:selectedVolume);
                fill(r,x-3,y-3,216,287,active?213:30,active?60:31,active?77:39);
                fill(r,x,y,210,280,7,8,12);
                Cover*c=coverAt(bookIndex,libraryScreen==0?works[idx].title:books[bookIndex].name,libraryScreen==0,libraryScreen==0?idx:selected);
                if(c&&c->tex)drawCover(r,c->tex,c->w,c->h,x,y,210,280);
                else label(r,x+45,y+120,"SEM CAPA",1,170,170,182);
                std::string title=libraryScreen==0?cleanLibraryWorkTitle(works[idx].title):stripExtension(books[bookIndex].name);
                labelMedium(r,x,y+290,compactTitle(title,17));
            }
            SDL_RenderSetClipRect(r,nullptr);
            fill(r,0,1230,720,50,22,23,30);
            label(r,18,1246,"ARRASTE PARA NAVEGAR",1,190,190,205);
        }else{
            // PIN permanece no layout original, sem alterar o fluxo de seguranca.
            SDL_Rect scaled{0,437,720,405};
            SDL_RenderCopy(r,landscapeFrame,nullptr,&scaled);
        }
        if(!(z||textReading)||showReaderHud){
            if((z||textReading) && showReaderHud)
                drawTopButton(r,portraitTopButton.x,portraitTopButton.y,portraitTopButton.w,portraitTopButton.h);
            drawRotateButton(r,500,40,200,60);
        }
        SDL_SetRenderTarget(r,nullptr);
        SDL_SetRenderDrawColor(r,0,0,0,255);SDL_RenderClear(r);
        const SDL_Rect dest{280,-280,720,1280};
        SDL_RenderCopyEx(r,portraitFrame,nullptr,&dest,90.0,nullptr,SDL_FLIP_NONE);
    }
    SDL_RenderPresent(r);SDL_Delay(16);
    }
    if((z||textReading)&&activeBook>=0&&activeBook<(int)books.size()) {
        save(books[activeBook].path,page);
    }
    release();
    if(portraitFrame)SDL_DestroyTexture(portraitFrame);
    if(landscapeFrame)SDL_DestroyTexture(landscapeFrame);
    mark("saida normal");
    if(log)fclose(log);
    clearCovers(covers);if(onlineEnabled){curl_global_cleanup();socketExit();}if(ctl)SDL_GameControllerClose(ctl);SDL_DestroyRenderer(r);SDL_DestroyWindow(win);IMG_Quit();SDL_Quit();return 0;
}
