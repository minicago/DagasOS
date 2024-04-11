#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define uint8 unsigned char
#define uint16 unsigned short
#define uint32 unsigned int
#define uint64 unsigned long long

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header
typedef struct elf_hdr_struct {
  uint32 magic;  // must equal ELF_MAGIC
  char elf[12];
  uint16 type;
  uint16 machine;
  uint32 version;
  uint64 entry;
  uint64 phoff;
  uint64 shoff;
  uint32 flags;
  uint16 ehsize;
  uint16 phentsize;
  uint16 phnum;
  uint16 shentsize;
  uint16 shnum;
  uint16 shstrndx;
}elf_hdr_t;

// Program section header
typedef struct prog_hdr_struct{
  uint32 type;
  uint32 flags;
  uint64 off;
  uint64 vaddr;
  uint64 paddr;
  uint64 filesz;
  uint64 memsz;
  uint64 align;
}prog_hdr_t;

// Values for Proghdr type
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4

#define ELF_OFF 0x8000000ull

#define SBI_SIZE 200000

char buf[SBI_SIZE];

int load_elf(FILE* file){
    elf_hdr_t elf_hdr;
    fseek(file, 0, SEEK_SET);
    printf("%p\n", &elf_hdr);
    fread(&elf_hdr, sizeof(elf_hdr_t), 1, file) ;
    if(elf_hdr.magic != ELF_MAGIC)
        goto elf_hdr_err;
    printf("%llx\n",elf_hdr.phnum);

    for(uint64 i=0, off = elf_hdr.phoff; i < elf_hdr.phnum; i++, off += sizeof(prog_hdr_t)){
        
        prog_hdr_t prog_hdr;
        fseek(file, off, SEEK_SET);
        fread(&prog_hdr, sizeof(prog_hdr), 1, file);
        
        if(prog_hdr.type != ELF_PROG_LOAD) continue;
        printf("%llx\n",prog_hdr.vaddr);
        if(prog_hdr.vaddr >= ELF_OFF && prog_hdr.vaddr < SBI_SIZE + ELF_OFF ) {
            fseek(file, prog_hdr.off, SEEK_SET);
            fread(buf + prog_hdr.vaddr - ELF_OFF, prog_hdr.filesz, 1, file);
        } 
    } 
    return 1;

elf_hdr_err:
    return 0;
}

int main(int argc, char* argv[]){
    
    if(argc != 4) {
        printf("capture_elf: too many or too few arguments\n");
        return 1;
    }
    FILE *file, *output;
    if(strcmp(argv[2],"-o") == 0) {
        file = fopen(argv[1],"rb");
        output = fopen(argv[3],"wb");
    } else if(strcmp(argv[1],"-o") == 0) {
        file = fopen(argv[3],"rb");
        output = fopen(argv[2],"wb");
    } else {
        printf("capture_elf: format should be \"capture_elf elf -o output\"\n");
        return 1;    
    }
    if(load_elf(file)){
        
        fwrite(buf, SBI_SIZE, 1, output);
    }else {
        printf("capture_elf: fail to load elf\n");
    }

    return 0;
    

}