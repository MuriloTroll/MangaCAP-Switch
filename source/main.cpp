#include <switch.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <vector>
#include <sys/stat.h>

static const char* BOOK_DIR = "sdmc:/switch/MangaCAP/books";
static std::vector<std::string> scan_books() {
    std::vector<std::string> out;
    mkdir("sdmc:/switch/MangaCAP", 0777);
    mkdir(BOOK_DIR, 0777);
    DIR* dir = opendir(BOOK_DIR);
    if (!dir) return out;
    while (dirent* entry = readdir(dir)) {
        if (out.size() >= 300) break;
        std::string name = entry->d_name;
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){return (char)std::tolower(c);});
        if ((lower.size() >= 4 && lower.substr(lower.size()-4) == ".cbz") ||
            (lower.size() >= 4 && lower.substr(lower.size()-4) == ".zip")) out.push_back(name);
    }
    closedir(dir);
    std::sort(out.begin(), out.end());
    return out;
}
int main(int argc, char** argv) {
    (void)argc; (void)argv;
    consoleInit(nullptr);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);
    auto books = scan_books();
    size_t selection = 0;
    bool running = true;
    while (appletMainLoop() && running) {
        padUpdate(&pad);
        u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Plus) running = false;
        if ((down & HidNpadButton_Down) && selection + 1 < books.size()) selection++;
        if ((down & HidNpadButton_Up) && selection > 0) selection--;
        if (down & HidNpadButton_X) { books = scan_books(); selection = 0; }
        consoleClear();
        printf("\x1b[1;31mMangaCAP SWITCH\x1b[0m  |  V0.2\n");
        printf("========================================\n");
        printf("Sphaira / Homebrew Launcher - teste inicial\n\n");
        printf("Pasta: sdmc:/switch/MangaCAP/books\n");
        printf("Volumes encontrados: %zu\n\n", books.size());
        if (books.empty()) printf("Nenhum CBZ/ZIP encontrado.\nCopie um arquivo para a pasta acima.\n");
        else {
            size_t start = selection > 12 ? selection - 12 : 0;
            for (size_t i = start; i < books.size() && i < start + 13; ++i)
                printf("%s %.65s\n", i == selection ? ">" : " ", books[i].c_str());
            printf("\nSelecionado: %zu/%zu\n", selection+1, books.size());
        }
        printf("\nCima/Baixo: navegar | X: atualizar\n");
        printf("+: sair\n\n");
        printf("Esta versao testa inicializacao e acesso ao SD.\n");
        printf("Leitura CBZ sera adicionada apos o teste.\n");
        consoleUpdate(nullptr);
        svcSleepThread(33'000'000);
    }
    consoleExit(nullptr);
    return 0;
}
