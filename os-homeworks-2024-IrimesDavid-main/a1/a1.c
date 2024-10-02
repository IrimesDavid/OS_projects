#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
//---------------------------------------------------------------------------------------------
typedef struct arguments
{
    int listOp;
    int parseOp;
    int extractOp;
    int findallOp;
    char *path;
    int recursive;
    int fDim;
    char *fSuffix;
    int section;
    int line;

} ARGs;
typedef struct section_header{
    unsigned char sect_name[16];
    unsigned char sect_type;
    unsigned int sect_offset;
    unsigned int sect_size;
} SectionHeader;
typedef struct sf_file
{
    unsigned char magic;
    unsigned char version;
    unsigned char no_of_sections;
    unsigned short header_size;
    SectionHeader *section_headers;

}SF;
//---------------------------------------------------------------------------------------------
void list(char *path, int dim, char *suffix, int rec, int succes){
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;

    dir = opendir(path);
    if(dir == NULL) {
    printf("ERROR\ninvalid directory path\n");
    exit(-1);
    }
    if(succes)
        printf("SUCCESS\n");
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            if(path[strlen(path)-1] == '/')
                snprintf(fullPath, 512, "%s%s", path, entry->d_name);
            else
                snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if(lstat(fullPath, &statbuf) == 0) {
                int ok1 = 1, ok2 = 1; // consideram true (in caz ca nu avem filtre)
                int justFiles = 0;
                if(dim != 0){
                    justFiles = 1;
                    ok1 = 0; //filtrul activ
                    if(statbuf.st_size < dim) 
                        ok1 = 1;
                }
                if(suffix != NULL){
                    ok2 = 0;
                    int name_len = strlen(entry->d_name);
                    int suffix_len = strlen(suffix);
                    if (name_len >= suffix_len) {
                    char *name_suffix = entry->d_name + name_len - suffix_len; // punctul de început al sufixului în numele fișierului
                    if (strcmp(name_suffix, suffix) == 0) // verificăm dacă sufixul este exact la sfârșit
                        ok2 = 1;
                    }
                }
                if(ok1 == 1 && ok2 == 1){
                    if(justFiles && S_ISREG(statbuf.st_mode))
                        printf("%s\n",fullPath);
                    else if(!justFiles)
                        printf("%s\n",fullPath);
                }
                if(rec != 0 && S_ISDIR(statbuf.st_mode))
                    list(fullPath, dim, suffix, rec, 0);
            }
        }
    }
    closedir(dir);
}
void parse(char *path){
    int fd = open(path,O_RDONLY);
    if (fd == -1) {
        printf("ERROR\ninvalid file name\n");
        exit(-1);
    }
    SF sf;

    read(fd,&sf.magic,1);
    if(sf.magic != 't'){
        printf("ERROR\nwrong magic\n");
        exit(-1);
    }
    read(fd,&sf.header_size,2);
    read(fd,&sf.version,1);
    if(sf.version < 108 || sf.version > 184){
        printf("ERROR\nwrong version\n");
        exit(-1);
    }
    read(fd,&sf.no_of_sections,1);
    if(sf.no_of_sections != 2 && (sf.no_of_sections < 5 || sf.no_of_sections > 15)){
        printf("ERROR\nwrong sect_nr\n");
        exit(-1);
    }

    sf.section_headers = (SectionHeader*) calloc(sf.no_of_sections, sizeof(SectionHeader));
    unsigned char cnt = 0;
    while(cnt < sf.no_of_sections){
        SectionHeader *sec = sf.section_headers + cnt;

            read(fd,sec->sect_name,15);
            sec->sect_name[15] = '\0';
            read(fd,&sec->sect_type,1);
            if(sec->sect_type != 23 &&
            sec->sect_type != 71 &&
            sec->sect_type != 43 &&
            sec->sect_type != 10 &&  
            sec->sect_type != 87 &&
            sec->sect_type != 68){
                printf("ERROR\nwrong sect_types\n");
                free(sf.section_headers);
                exit(-1);
            }
            read(fd,&sec->sect_offset,4);
            read(fd,&sec->sect_size,4);
        cnt++;
    }
    cnt = 0;
    printf("SUCCESS\n");
    printf("version=%hhu\n",sf.version);
    printf("nr_sections=%hhu\n",sf.no_of_sections);
    while(cnt < sf.no_of_sections){
        SectionHeader sec = sf.section_headers[cnt];
        printf("section%d: %s %hhu %u\n",cnt+1,sec.sect_name,sec.sect_type,sec.sect_size);
        cnt++;
    }
    free(sf.section_headers);
}
void extract(char *path, int section, int line){
    int fd = open(path,O_RDONLY);
    if (fd == -1) {
        printf("ERROR\ninvalid file\n");
        exit(-1);
    }
    SF sf;

    read(fd,&sf.magic,1);
    read(fd,&sf.header_size,2);
    read(fd,&sf.version,1);
    read(fd,&sf.no_of_sections,1);
    if(sf.no_of_sections < section || section <= 0){
        printf("ERROR\ninvalid section\n");
        exit(-1);
    }

    sf.section_headers = (SectionHeader*) calloc(sf.no_of_sections, sizeof(SectionHeader));
    unsigned char cnt = 0;
    while(cnt < sf.no_of_sections){
        SectionHeader *sec = sf.section_headers + cnt;

            read(fd,sec->sect_name,15);
            sec->sect_name[15] = '\0';
            read(fd,&sec->sect_type,1);
            read(fd,&sec->sect_offset,4);
            read(fd,&sec->sect_size,4);
        cnt++;
    }
    SectionHeader sec = sf.section_headers[section-1];
    unsigned char buff[sec.sect_size];
    cnt = 1;
    int success = 1;
        lseek(fd,sec.sect_offset, SEEK_SET);
        read(fd, buff, sec.sect_size);
        for(int i=0;i<sec.sect_size-1;++i){
            if(line == cnt){
                if(success){
                    printf("SUCCESS\n");
                    success = 0;
                }
                if(buff[i] == 0x0D && buff[i+1] == 0x0A){
                    printf("%c%c",buff[i],buff[i+1]);
                    break;
                }
                else
                    printf("%c",buff[i]);
            }
            else if(buff[i] == 0x0D && buff[i+1] == 0x0A) {
                cnt++;
                i++;
            }
        }
    free(sf.section_headers);
    if(line > cnt || line <= 0){
        printf("ERROR\ninvalid line\n");
        exit(-1);
    }
}
char* checkSF(char *path){
    int fd = open(path,O_RDONLY);
    if (fd == -1) {
        printf("ERROR\ninvalid file\n");
        exit(-1);
    }
    SF sf;

    read(fd,&sf.magic,1);
    read(fd,&sf.header_size,2);
    read(fd,&sf.version,1);
    read(fd,&sf.no_of_sections,1);

    sf.section_headers = (SectionHeader*) calloc(sf.no_of_sections, sizeof(SectionHeader));
    unsigned char cnt = 0;
    while(cnt < sf.no_of_sections){
        SectionHeader *sec = sf.section_headers + cnt;

            read(fd,sec->sect_name,15);
            sec->sect_name[15] = '\0';
            read(fd,&sec->sect_type,1);
            read(fd,&sec->sect_offset,4);
            read(fd,&sec->sect_size,4);
        cnt++;
    }
    while(cnt){
        SectionHeader sec = sf.section_headers[cnt-1];
        unsigned char buff[sec.sect_size];
        lseek(fd,sec.sect_offset, SEEK_SET);
        read(fd, buff, sec.sect_size);
        int counter = 1;
        for(int i=0;i<sec.sect_size-1;++i){
            if(buff[i] == 0x0D && buff[i+1] == 0x0A) {
                counter++;
                i++;
            }
        }
        if(counter == 15){
            free(sf.section_headers);
            close(fd);
            return path;
        } 
        cnt--;
    }
    free(sf.section_headers);
    close(fd);
    return NULL;
}
void findall(char *path, int *success){
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;

    dir = opendir(path);
    if(dir == NULL) {
    printf("ERROR\ninvalid directory path\n");
    exit(-1);
    }
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            if(path[strlen(path)-1] == '/')
                snprintf(fullPath, 512, "%s%s", path, entry->d_name);
            else
                snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if(lstat(fullPath, &statbuf) == 0) {
                if(S_ISREG(statbuf.st_mode)){
                    if(checkSF(fullPath) != NULL){
                        if(*success){
                            printf("SUCCESS\n");
                            *success = 0;
                        }
                        printf("%s\n",fullPath);
                    }
                }
                if(S_ISDIR(statbuf.st_mode))
                    findall(fullPath,success);
            }
        }
    }
    closedir(dir);
}
//---------------------------------------------------------------------------------------------
void initializeARGs(ARGs *a)
{
    a->listOp = 0;
    a->parseOp = 0;
    a->extractOp = 0;
    a->findallOp = 0;
    a->path = NULL;
    a->recursive = 0;
    a->fDim = 0;
    a->fSuffix = NULL;
    a->section = 0;
    a->line = 0;
}
void readARGs(int argc, char **argv, ARGs *a)
{
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "variant") == 0)
        {
            printf("28253\n");
        }
        if (strcmp(argv[i], "list") == 0)
        {
            a->listOp = 1;
        }
        if (strcmp(argv[i], "parse") == 0)
        {
            a->parseOp = 1;
        }
        if (strcmp(argv[i], "extract") == 0)
        {
            a->extractOp = 1;
        }
        if (strcmp(argv[i], "findall") == 0)
        {
            a->findallOp = 1;
        }
        if (strcmp(argv[i], "recursive") == 0)
            a->recursive = 1;
        if (strncmp(argv[i], "path=", 5) == 0)
        {
            if (a->path == NULL)
                a->path = "";
            a->path = argv[i] + 5;
        }
        if (strncmp(argv[i], "size_smaller=", 13) == 0)
        {
            a->fDim = atoi(argv[i] + 13);
            if (a->fDim == 0)
                a->fDim = -1; // conventie
        }
        if (strncmp(argv[i], "name_ends_with=", 15) == 0)
        {
            if (a->fSuffix == NULL)
                a->fSuffix = "";
            a->fSuffix = argv[i] + 15;
        }
        if (strncmp(argv[i], "section=", 8) == 0)
        {
            a->section = atoi(argv[i] + 8);
            if (a->section == 0)
                a->section = -1;
        }
        if (strncmp(argv[i], "line=", 5) == 0)
        {
            a->line = atoi(argv[i] + 5);
            if (a->line == 0)
                a->line = -1;
        }
    }
}
void generateResponse(ARGs *a)
{
    if (a->listOp + a->parseOp + a->extractOp > 1)
    {
        printf("ERROR\ncannot execute both list and parse operations\n");
        exit(-1);
    }
    if (a->listOp)
    {
        char *nmk = "";
        if (a->path == NULL || strcmp(a->path, nmk) == 0){
            printf("ERROR\ninvalid directory path\n");
            exit(-1);
        }
        if (a->fDim < 0){
            printf("ERROR\ninvalid size limit\n");
            exit(-1);
        }
        if (a->fSuffix != NULL && strcmp(a->fSuffix, nmk) == 0){
            printf("ERROR\ninvalid suffix\n");
            exit(-1);
        }
        list(a->path, a->fDim, a->fSuffix, a->recursive, 1);
        exit(0);
    }
    if (a->parseOp)
    {
        if (a->recursive || a->fDim != 0 || a->fSuffix != NULL)
            printf("ERROR\ninvalid filters for parse option, only use path\n");
        char *nmk = "";
        if (a->path == NULL || strcmp(a->path, nmk) == 0){
            printf("ERROR\ninvalid directory path\n");
            exit(-1);
        }
        parse(a->path);
        exit(0);
    }
    if (a->extractOp)
    {
        if (a->recursive || a->fDim != 0 || a->fSuffix != NULL){
            printf("ERROR\ninvalid filters for extract option, only use path, section and line\n");
            exit(-1);
        }
        char *nmk = "";
        if (a->path == NULL || strcmp(a->path, nmk) == 0){
            printf("ERROR\ninvalid directory path\n");
            exit(-1);
        }
        if (a->section < 0){
            printf("ERROR\ninvalid section\n");
            exit(-1);
        }
        if (a->section == 0){
            printf("ERROR\nmissing section option\n");
            exit(-1);
        }
        if (a->line < 0){
            printf("ERROR\ninvalid line\n");
            exit(-1);
        }
        if (a->line == 0){
            printf("ERROR\nmissing line option\n");
            exit(-1);
        }
        extract(a->path, a->section, a->line);
        exit(0);
    }
    if (a->findallOp)
    {
        if (a->recursive || a->fDim != 0 || a->fSuffix != NULL || a->section || a->line){
            printf("ERROR\ninvalid filters for findall option, only use path\n");
            exit(-1);
        }
        char *nmk = "";
        if (a->path == NULL || strcmp(a->path, nmk) == 0){
            printf("ERROR\ninvalid directory path\n");
            exit(-1);
        }
        int success = 1;
        findall(a->path, &success);
        if(success == 1) printf("SUCCESS\n"); //inseamna ca nu exista rezultate dar comanda s-a executat cu succes
        exit(0);
    }

    if (!a->listOp && !a->parseOp && !a->extractOp && !a->findallOp &&
        (a->recursive ||
         a->path != NULL ||
         a->fDim != 0 ||
         a->fSuffix != NULL ||
         a->section != 0 ||
         a->line != 0))
    {
        printf("ERROR\nmissing list/parse/extract/findall option\n");
        exit(-1);
    }
}
//---------------------------------------------------------------------------------------------
int main(int argc, char **argv){
    ARGs a;
    initializeARGs(&a);

    if (argc >= 2)
    {
        readARGs(argc, argv, &a);
        generateResponse(&a);
    }
    else
        printf("USECASE: <variant> | <list [recursive] <filtering opritons> path=<dir_path>>\n");

    return 0;
}