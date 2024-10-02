#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#define RESP_PIPE "RESP_PIPE_28253"
#define REQ_PIPE "REQ_PIPE_28253"
//---------------------------------------------------------------------------------------------

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
int findSectionOffset(char *path, unsigned int sectionNoArg){
    int fd = open(path,O_RDONLY);
    if (fd == -1) {
        printf("ERROR\ninvalid file\n");
        return -1;
    }
    SF sf;

    read(fd,&sf.magic,1);
    read(fd,&sf.header_size,2);
    read(fd,&sf.version,1);
    read(fd,&sf.no_of_sections,1);

    sf.section_headers = (SectionHeader*) calloc(sf.no_of_sections, sizeof(SectionHeader));
    unsigned char cnt = 0;
    // printf("sf.no_of_sections = %u AND sectionNoArg = %u\n", sf.no_of_sections, sectionNoArg);
    while(cnt < sf.no_of_sections){
        SectionHeader *sec = sf.section_headers + cnt;
            if(cnt == sectionNoArg){
                read(fd,sec->sect_name,15);
                sec->sect_name[15] = '\0';
                read(fd,&sec->sect_type,1);
                read(fd,&sec->sect_offset,4);
                read(fd,&sec->sect_size,4);

                return sec->sect_offset; //returnam offsetul sectiunii cautate din SF
            }

            read(fd,sec->sect_name,15);
            sec->sect_name[15] = '\0';
            read(fd,&sec->sect_type,1);
            read(fd,&sec->sect_offset,4);
            read(fd,&sec->sect_size,4);
        cnt++;
    }
    return -1;
}
//---------------------------------------------------------------------------------------------
void makeConnection(int *respFd, int *reqFd){
    unlink(RESP_PIPE); // in case it was previously created
    if (mkfifo(RESP_PIPE, 0666) == -1) {
        printf("ERROR\ncannot create the response pipe | cannot open the request pipe");
    }
    
    *reqFd = open(REQ_PIPE, O_RDONLY);
    if(*reqFd == -1) {
        printf("ERROR\ncannot create the response pipe | cannot open the request pipe");
        unlink(RESP_PIPE);
        exit(-1);
    }

    *respFd = open(RESP_PIPE, O_WRONLY);
    if (*respFd == -1) {
        printf("ERROR\ncannot create the response pipe | cannot open the request pipe");
        unlink(RESP_PIPE);
        exit(-1);
    }

    char helloMessage[] = "HELLO";
    helloMessage[strlen(helloMessage)] = '#';
    if (write(*respFd, helloMessage, 6) != 6) {
        printf("ERROR\ncannot create the response pipe | cannot open the request pipe");
        unlink(RESP_PIPE);
        exit(-1);
    }
    printf("SUCCESS\n");
}
void echoReq(int respFd, int reqFd, char buffer[]){
    char echo[] = "ECHO";
    echo[strlen(echo)] = '#';
    write(respFd, echo, sizeof(echo));
    int var = 28253;
    write(respFd, &var, sizeof(int));
    char variant[] = "VARIANT";
    variant[strlen(variant)] = '#';
    write(respFd, variant, sizeof(variant));
}
void createShmReq(int respFd, int* shmFd, unsigned char **sharedMem, unsigned int nrBytes){
    *shmFd = shm_open("/bAq7qS5", O_CREAT | O_RDWR, 0664);
    if(shmFd < 0) {
        write(respFd, "CREATE_SHM#ERROR#", strlen("CREATE_SHM#ERROR#"));
        return;
    }
    ftruncate(*shmFd, nrBytes);
    *sharedMem = (unsigned char*)mmap(0, nrBytes, PROT_READ | PROT_WRITE, MAP_SHARED, *shmFd, 0);
    if(*sharedMem == (void*)-1){
        write(respFd, "CREATE_SHM#ERROR#", strlen("CREATE_SHM#ERROR#"));
        close(*shmFd);
        return;
    }
    write(respFd, "CREATE_SHM#SUCCESS#", strlen("CREATE_SHM#SUCCESS#"));
}
void writeShmReq(int respFd, unsigned char *sharedMem, unsigned int nrBytesArg, unsigned int valueArg, unsigned int offsetArg){
    if(offsetArg <= (nrBytesArg - sizeof(valueArg) + 1)){
        // int k = 0;
        // for(int i = 3; i >= 0; --i){
        //     sharedMem[offsetArg + i] = (valueArg >> (k * 8)) & 0xFF;
        //     k++;
        // }
        memcpy(sharedMem + offsetArg, &valueArg, sizeof(valueArg));
        // for(int i = 0; i < 4; ++i)
        //     printf("%d-",sharedMem[offsetArg + i]);
        // printf("\n");
        write(respFd, "WRITE_TO_SHM#SUCCESS#", strlen("WRITE_TO_SHM#SUCCESS#"));
    }else{
        write(respFd, "WRITE_TO_SHM#ERROR#", strlen("WRITE_TO_SHM#ERROR#"));
    }
}
void mapFileReq(int respFd, unsigned int *prevNrBytes, char fileName[], unsigned char **sharedMem) {
    printf("FILE_NAME: %s\n",fileName);
    int fd;
    fd = open(fileName, O_RDONLY);
    if(fd == -1) {
        write(respFd, "MAP_FILE#ERROR#", strlen("MAP_FILE#ERROR#"));
        return;
    }
    off_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    munmap((void *)(*sharedMem), *prevNrBytes);//scapam de orice am avut inainte in memorie
    (*sharedMem) = NULL;
    *prevNrBytes = size;
    *sharedMem = (unsigned char*)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (*sharedMem == (void*)-1) {
        write(respFd, "MAP_FILE#ERROR#", strlen("MAP_FILE#ERROR#"));
        close(fd);
        return;
    }
    write(respFd, "MAP_FILE#SUCCESS#", strlen("MAP_FILE#SUCCESS#"));

    close(fd);
}
void readFromFileReq(int respFd, unsigned char *sharedMem, unsigned int toatalNrBytes, unsigned int offsetArg, unsigned int noOfBytesArg){
    char *readData = (char*) calloc(noOfBytesArg, sizeof(char));
    char shiftData[offsetArg];

    printf("offsetArg + noOfBytesArg = %u\n toatalNrBytes = %u\n", offsetArg + noOfBytesArg, toatalNrBytes);
    if((offsetArg + noOfBytesArg) > toatalNrBytes ){
        write(respFd, "READ_FROM_FILE_OFFSET#ERROR#", strlen("READ_FROM_FILE_OFFSET#ERROR#"));
        free(readData);
    }else{
        for(int i = 0; i < noOfBytesArg; ++i)
            readData[i] = sharedMem[offsetArg + i];
        for(int i = 0; i < offsetArg; ++i)
            shiftData[i] = sharedMem[i];
            
        int k = 0;
        for(int j = 0; j < toatalNrBytes; ++j){
            if(j < noOfBytesArg)
                sharedMem[j] = readData[j];
            else if(j < (offsetArg + noOfBytesArg))
                sharedMem[j] = shiftData[k++];
        }

        write(respFd, "READ_FROM_FILE_OFFSET#SUCCESS#", strlen("READ_FROM_FILE_OFFSET#SUCCESS#"));
        free(readData);
    }
}
void readFromSFReq(int respFd, unsigned char *sharedMem, char fileName[], unsigned int sectionNoArg, unsigned int offsetArg, unsigned int noOfBytesArg){
    char *readData = (char*) calloc(noOfBytesArg, sizeof(char));
    unsigned int sectionOffset = findSectionOffset(fileName, sectionNoArg);
    if(sectionOffset == -1){
        write(respFd, "READ_FROM_FILE_SECTION#ERROR#", strlen("READ_FROM_FILE_SECTION#ERROR#"));
        return;
    }

    if(!((offsetArg + sectionOffset) >= 0 && (offsetArg + sectionOffset) <= (sizeof(sharedMem) - noOfBytesArg + 1))){
        write(respFd, "READ_FROM_FILE_SECTION#ERROR#", strlen("READ_FROM_FILE_SECTION#ERROR#"));
        return;
    }

    for(int i = 0; i < noOfBytesArg; ++i){
        readData[i] = sharedMem[(offsetArg + sectionOffset) + i];
    }
    memcpy(sharedMem, readData, noOfBytesArg);
    write(respFd, "READ_FROM_FILE_SECTION#SUCCESS#", strlen("READ_FROM_FILE_SECTION#SUCCESS#"));
}
void readFromLogicalSpaceReq(int respFd, unsigned char *sharedMem, unsigned int logicalOffsetArg, unsigned int logicalnoOfBytesArg){
    //code here
    write(respFd, "READ_FROM_LOGICAL_SPACE_OFFSET#ERROR#", strlen("READ_FROM_LOGICAL_SPACE_OFFSET#ERROR#"));
}

void generateResponse(int respFd, int reqFd) {
    int shmFd = 0;
    unsigned char *sharedMem = NULL;
    unsigned int nrBytesArg = 0;
    char fileName[256];

    while(1){
        char req[256];
        int i=0;
        while(1){
            ssize_t byte = read(reqFd, &req[i++], 1);
            if(byte != 1)
                return;
            if(req[i-1] == '#')
                break;
        }
        req[i] = '\0';
        // printf("REQUEST: %s\n", req);

        if (strcmp(req, "EXIT#") == 0)
            return;

        if (strcmp(req, "ECHO#") == 0)
            echoReq(respFd, reqFd, req);
        
        if(strcmp(req, "CREATE_SHM#") == 0){
            read(reqFd, &nrBytesArg, sizeof(unsigned int));
            createShmReq(respFd, &shmFd, &sharedMem, nrBytesArg);
        }

        if(strcmp(req, "WRITE_TO_SHM#") == 0){
            unsigned int offsetArg = 0;
            unsigned int valueArg;
            read(reqFd, &offsetArg, sizeof(unsigned int));
            read(reqFd, &valueArg, sizeof(unsigned int));
            // printf("Write offset: %d\n",offsetArg);
            // printf("Write value: %d\n",valueArg);
            writeShmReq(respFd, sharedMem, nrBytesArg, valueArg, offsetArg);
        }

        if(strcmp(req, "MAP_FILE#") == 0){
            int i = 0;
            while(1){
                read(reqFd, &fileName[i++],1);
                if(fileName[i-1] == '#'){
                    fileName[i-1] = '\0';
                    break;
                }
            }
            mapFileReq(respFd, &nrBytesArg, fileName, &sharedMem);
        }

        if(strcmp(req, "READ_FROM_FILE_OFFSET#") == 0){
            unsigned int offsetArg = 0;
            unsigned int noOfBytesArg = 0;
            read(reqFd, &offsetArg, sizeof(unsigned int));
            read(reqFd, &noOfBytesArg, sizeof(unsigned int));
            readFromFileReq(respFd, sharedMem, nrBytesArg, offsetArg, noOfBytesArg);
        }

        if(strcmp(req, "READ_FROM_FILE_SECTION#") == 0){
            unsigned int sectionNoArg = 0;
            unsigned int offsetArg = 0;
            unsigned int noOfBytesArg = 0;
            read(reqFd, &sectionNoArg, sizeof(unsigned int));
            read(reqFd, &offsetArg, sizeof(unsigned int));
            read(reqFd, &noOfBytesArg, sizeof(unsigned int));
            readFromSFReq(respFd, sharedMem, fileName, sectionNoArg, offsetArg, noOfBytesArg);
        }

        if(strcmp(req, "READ_FROM_LOGICAL_SPACE_OFFSET#") == 0){
            unsigned int logicalOffsetArg = 0;
            unsigned int logicalnoOfBytesArg = 0;
            read(reqFd, &logicalOffsetArg, sizeof(unsigned int));
            read(reqFd, &logicalnoOfBytesArg, sizeof(unsigned int));
            readFromLogicalSpaceReq(respFd, sharedMem, logicalOffsetArg, logicalnoOfBytesArg);
        }

        usleep(1000);
    }
    close(shmFd);
    munmap((void *)sharedMem, nrBytesArg);
}
//---------------------------------------------------------------------------------------------
int main(int argc, char **argv){
    int respFd = -1, reqFd = -1;
    makeConnection(&respFd, &reqFd);
    generateResponse(respFd, reqFd);

    close(respFd);
    close(reqFd);
    unlink(RESP_PIPE);
    return 0;  
}