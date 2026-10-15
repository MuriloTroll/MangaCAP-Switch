#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "cbz_zip.h"
#include "font.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>

static const char* ROOT="sdmc:/switch/MangaCAP";
static const char* BOOKS="sdmc:/switch/MangaCAP/books";
static const char* PROGRESS="sdmc:/switch/MangaCAP/progress.tsv";
struct Book {std::string name,path;};
static std::string lower(std::string s){for(char& c:s)c=(char)tolower((unsigned char)c);return s;}
static bool ends(const std::string& s,const char* suffix){size_t n=strlen(suffix);return s.size()>=n&&lower(s.substr(s.size()-n))==suffix;}
static bool isImage(const std::string& s){return ends(s,".png")||ends(s,".jpg")||ends(s,".jpeg")||ends(s,".webp");}
static std::vector<Book> scan(){mkdir(ROOT,0777);mkdir(BOOKS,0777);std::vector<Book> b;DIR* d=opendir(BOOKS);if(!d)return b;dirent* e;while((e=readdir(d))&&b.size()<250){std::string n=e->d_name;if(ends(n,".cbz")||ends(n,".zip"))b.push_back({n,std::string(BOOKS)+"/"+n});}closedir(d);std::sort(b.begin(),b.end(),[](const Book&a,const Book&b){return lower(a.name)<lower(b.name);});return b;}
static int getSaved(const std::string& path){std::ifstream f(PROGRESS);std::string line;while(std::getline(f,line)){auto pos=line.find('\t');if(pos!=std::string::npos&&line.substr(pos+1)==path)return atoi(line.substr(0,pos).c_str());}return 0;}
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
static void fill(SDL_Renderer*r,int x,int y,int w,int h,Uint8 R,Uint8 G,Uint8 B){SDL_SetRenderDrawColor(r,R,G,B,255);SDL_Rect d{x,y,w,h};SDL_RenderFillRect(r,&d);}
static void label(SDL_Renderer*r,int x,int y,const std::string& s,int scale=2,Uint8 R=240,Uint8 G=240,Uint8 B=240){SDL_SetRenderDrawColor(r,R,G,B,255);for(unsigned char c:s){if(c<32||c>126)c='?';auto glyph=FONT[c-32];for(int row=0;row<15;row++)for(int col=0;col<9;col++)if(glyph[row]&(1<<col)){SDL_Rect d{x+col*scale,y+row*scale,scale,scale};SDL_RenderFillRect(r,&d);}x+=10*scale;if(x>1260)break;}}
int main(int argc,char**argv){(void)argc;(void)argv;
    // Log de diagnóstico antes de inicializar os componentes gráficos.
    mkdir(ROOT,0777);FILE* log=fopen("sdmc:/switch/MangaCAP/startup.log","w");
    auto mark=[&](const char*s){if(log){fprintf(log,"%s\n",s);fflush(log);}};
    mark("inicio");if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)!=0){mark(SDL_GetError());if(log)fclose(log);return 1;}mark("SDL OK");
    IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG|IMG_INIT_WEBP);mark("IMG init");
    SDL_Window*win=SDL_CreateWindow("MangaCAP Switch",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1280,720,SDL_WINDOW_SHOWN);if(!win){mark(SDL_GetError());return 1;}
    SDL_Renderer*r=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED);if(!r)r=SDL_CreateRenderer(win,-1,SDL_RENDERER_SOFTWARE);if(!r){mark(SDL_GetError());return 1;}mark("renderer OK");
    SDL_GameController*ctl=nullptr;for(int i=0;i<SDL_NumJoysticks();i++)if(SDL_IsGameController(i)){ctl=SDL_GameControllerOpen(i);break;}
    auto books=scan();mark("biblioteca OK");int selected=0,page=0,iw=1,ih=1;float zoom=1.0f;CbzZip*z=nullptr;SDL_Texture*tex=nullptr;std::vector<std::string> pages;std::string message;
    auto release=[&](){if(tex){SDL_DestroyTexture(tex);tex=nullptr;}if(z){delete z;z=nullptr;}pages.clear();};
    auto load=[&](){if(tex){SDL_DestroyTexture(tex);tex=nullptr;}if(z&&!pages.empty()){tex=decode(r,z,pages[page],iw,ih);if(!tex)message="Falha ao decodificar imagem";else message.clear();}};
    bool running=true;while(running){SDL_Event ev;while(SDL_PollEvent(&ev)){
        if(ev.type==SDL_QUIT){running=false;break;}
        if(ev.type!=SDL_CONTROLLERBUTTONDOWN&&ev.type!=SDL_KEYDOWN)continue;
        int b=ev.type==SDL_CONTROLLERBUTTONDOWN?ev.cbutton.button:-1;SDL_Keycode k=ev.type==SDL_KEYDOWN?ev.key.keysym.sym:0;
        if(b==SDL_CONTROLLER_BUTTON_START||k==SDLK_ESCAPE){running=false;break;}
        if(!z){if((b==SDL_CONTROLLER_BUTTON_DPAD_DOWN||k==SDLK_DOWN)&&selected+1<(int)books.size())selected++;
            if((b==SDL_CONTROLLER_BUTTON_DPAD_UP||k==SDLK_UP)&&selected>0)selected--;
            if(b==SDL_CONTROLLER_BUTTON_X||k==SDLK_F5){books=scan();selected=0;}
            if((b==SDL_CONTROLLER_BUTTON_A||k==SDLK_RETURN)&&!books.empty()){int err=0;z=new CbzZip(books[selected].path);if(!z->valid()){delete z;z=nullptr;message="Nao foi possivel abrir o CBZ";continue;}pages=listPages(z);if(pages.empty()){message="CBZ sem imagens compativeis";release();continue;}page=std::max(0,std::min(getSaved(books[selected].path),(int)pages.size()-1));zoom=1;load();mark("CBZ aberto");}}
        else {if(b==SDL_CONTROLLER_BUTTON_B||k==SDLK_BACKSPACE){save(books[selected].path,page);release();continue;}
            if((b==SDL_CONTROLLER_BUTTON_DPAD_RIGHT||k==SDLK_RIGHT)&&page+1<(int)pages.size()){page++;save(books[selected].path,page);load();}
            if((b==SDL_CONTROLLER_BUTTON_DPAD_LEFT||k==SDLK_LEFT)&&page>0){page--;save(books[selected].path,page);load();}
            if(b==SDL_CONTROLLER_BUTTON_RIGHTSHOULDER||k==SDLK_EQUALS)zoom=std::min(3.f,zoom+.15f);
            if(b==SDL_CONTROLLER_BUTTON_LEFTSHOULDER||k==SDLK_MINUS)zoom=std::max(.5f,zoom-.15f);}
    }
    fill(r,0,0,1280,720,12,14,20);
    if(!z){fill(r,0,0,1280,90,116,18,34);label(r,36,20,"MangaCAP SWITCH",3);label(r,920,28,"V0.3",2);
        label(r,42,108,"BIBLIOTECA  -  CBZ / ZIP",2,228,84,96);
        if(books.empty())label(r,55,220,"Nenhum arquivo. Copie CBZ para /switch/MangaCAP/books",2);
        int first=std::max(0,selected-8);for(int i=first;i<(int)books.size()&&i<first+10;i++){int y=165+(i-first)*48;bool active=i==selected;fill(r,36,y,1200,43,active?123:32,active?25:34,active?42:48);label(r,54,y+7,(active?"> ":"  ")+books[i].name,2);}
        fill(r,0,668,1280,52,20,22,29);label(r,24,678,"A: abrir   X: atualizar   +: sair",2);
    }else{if(tex){float scale=std::min(1280.f/iw,690.f/ih)*zoom;int w=(int)(iw*scale),h=(int)(ih*scale);SDL_Rect dst{(1280-w)/2,(690-h)/2,w,h};SDL_RenderCopy(r,tex,nullptr,&dst);}fill(r,0,697,1280,23,28,29,38);fill(r,0,713,(int)(1280.f*(page+1)/pages.size()),7,218,41,58);label(r,16,698,std::to_string(page+1)+" / "+std::to_string(pages.size())+"   B: biblioteca  L/R: zoom",1);}
    if(!message.empty()){fill(r,0,626,1280,42,128,24,40);label(r,30,634,message,2);}
    SDL_RenderPresent(r);SDL_Delay(16);
    }
    if(z&&!books.empty())save(books[selected].path,page);release();mark("saida normal");if(log)fclose(log);
    if(ctl)SDL_GameControllerClose(ctl);SDL_DestroyRenderer(r);SDL_DestroyWindow(win);IMG_Quit();SDL_Quit();return 0;
}
