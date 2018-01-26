#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
typedef unsigned char boolean; 
#define TRUE  1
#define FALSE 0

//int cards[13];
long setBuffer(FILE *f, char *buffer);

FILE *f;
unsigned char* readFileIntoByteArray(FILE *f);
unsigned char* readStringFromOffset(char* buffer, int currentByte, int offset);

struct Header {
    boolean isLittleEndian;
    boolean isAPP1;
    short offsetToTIFF;
    short APPBlockSize;
};

struct TIFF_Tag {
    unsigned short tag;
    unsigned short dataType;
    unsigned int dataItems;
    unsigned int valueOffset;
};

struct TIFF_Tag *tiffs;


/*******************************************************************************
* Will utilize rules provided by the EXIF format description to extract the
* desired fields from an image.
********************************************************************************/
int main(int argc, char *argv[]) {
    if (argc == 2) {
        f = fopen(argv[1],"rb");
    } else {
        printf("Assuming img1.jpg\n");
        f = fopen("img1.jpg","rb");
    }
    
    unsigned char *buffer = readFileIntoByteArray(f); // character buffer for file
    struct Header header;
    
    // This if conditional was left for debug
    if(buffer[0] == 0xFF && buffer[1] == 0xD8) {
        //printf("JPEG confirmed\n");
    } else {
        printf("JPEG file not found.\n");
        return -1;
    }
    
    // We are supposed to disallow APP0
    if (buffer[2] == 0xFF && buffer[3] == 0xE0) {
        header.isAPP1 = FALSE;
    }
    else if (buffer[2] == 0xFF && buffer[3] == 0xE1) {
        header.isAPP1 = TRUE;
    }
    if(!header.isAPP1) {
        printf("APP0 detected. Our program assumes APP1.");
        return 0;
    }
    
    header.APPBlockSize = (short)buffer[4];
    header.APPBlockSize = (header.APPBlockSize << 8) + (short)buffer[5];
    header.isLittleEndian = TRUE;
    
    if(buffer[12] == 0x4D && buffer[13 == 0x4D]) { // if 'I' (Intel, little endian)
        header.isLittleEndian = FALSE;
        printf("Big-Endian (Motorola) file detected.");
    }
    
    short TIFF_Count = buffer[20] | (buffer[21] << 8); // byte 20 & 21 of file indicate how many TIFF tags there are
    //printf("TIFF_Count: %d\n",TIFF_Count);
    tiffs = (struct TIFF_Tag*)malloc(TIFF_Count * sizeof(struct TIFF_Tag));
    
    int currentByte = 0x16; // Byte 22
    int i = 0;
    
    for(i = 0; i < TIFF_Count; i++) {
        tiffs[i].tag = buffer[currentByte] | (buffer[currentByte+1] << 8);
        currentByte += 2;
        //printf("TAG: %x\n",tiffs[i].tag);
        tiffs[i].dataType = buffer[currentByte] | (buffer[currentByte+1] << 8);
        currentByte += 2;
        tiffs[i].dataItems = buffer[currentByte] | (buffer[currentByte+1] << 8) | (buffer[currentByte+2] << 16) | (buffer[currentByte+3] << 24);
        currentByte += 4;
        tiffs[i].valueOffset = buffer[currentByte] | (buffer[currentByte+1] << 8) | (buffer[currentByte+2] << 16) | (buffer[currentByte+3] << 24);
        currentByte += 4;
    }
    
    currentByte = 0;
    
    for(i = 0; i < TIFF_Count; i++) {
        //printf("%d, %x\n", i, tiffs[i].tag);
        if(tiffs[i].tag == 0x010F) {
            unsigned char* string = readStringFromOffset(buffer, 12, tiffs[i].valueOffset);
            printf("Manufacturer:\t\t%s\n", string);
            free(string);
        }
        if(tiffs[i].tag == 0x0110) {
            unsigned char* string = readStringFromOffset(buffer, 12, tiffs[i].valueOffset);
            printf("Camera Model:\t\t%s\n", string);
            free(string);
        }
        if(tiffs[i].tag == 0x8769) {
            currentByte = tiffs[i].valueOffset;
            i=TIFF_Count; // break out of loop bc data supposed to be in order
        }
    }
    
    free(tiffs);
    
    if(currentByte > 0) {
        currentByte+=12; // add address
        TIFF_Count = buffer[currentByte] | (buffer[currentByte+1] << 8); // get TIFF count again
        //printf("Current byte %04x is representing TIFF count %d\n", currentByte, TIFF_Count);
        tiffs = (struct TIFF_Tag*)malloc(TIFF_Count * sizeof(struct TIFF_Tag));
        currentByte+=2; // TIFF_Tag
        int j = 0;
        
        for(j = 0; j < TIFF_Count; j++) {
            tiffs[j].tag = buffer[currentByte] | (buffer[currentByte+1] << 8);
            currentByte += 2;
            tiffs[j].dataType = buffer[currentByte] | (buffer[currentByte+1] << 8);
            currentByte += 2;
            tiffs[j].dataItems = buffer[currentByte] | (buffer[currentByte+1] << 8) | (buffer[currentByte+2] << 16) | (buffer[currentByte+3] << 24);
            currentByte += 4;
            tiffs[j].valueOffset = buffer[currentByte] | (buffer[currentByte+1] << 8) | (buffer[currentByte+2] << 16) | (buffer[currentByte+3] << 24);
            currentByte += 4;
        }
        
        for(j = 0; j < TIFF_Count; j++) {
            if(tiffs[j].tag == 0x9003) {
                unsigned char* string = readStringFromOffset(buffer, 12, tiffs[j].valueOffset);
                printf("Date Taken:\t\t%s\n", string);
                free(string);
            }
            
            if(tiffs[j].tag == 0xA002) {
                printf("Width in Pixels:\t%d\n", tiffs[j].valueOffset);
            }
            
            if(tiffs[j].tag == 0xA003) {
                printf("Height in Pixels:\t%d\n", tiffs[j].valueOffset);
            }

            if(tiffs[j].tag == 0x8827) {
                printf("ISO Speed:\t\tISO %d\n", tiffs[j].valueOffset);
            }
            
            if(tiffs[j].tag == 0x829a) {
                short addr = tiffs[j].valueOffset + 12;
                int int1 = buffer[addr] + (buffer[addr+1] << 8) + (buffer[addr+2] << 16) + (buffer[addr+3] << 24);
                addr += 4;
                int int2 = buffer[addr] + (buffer[addr+1] << 8) + (buffer[addr+2] << 16) + (buffer[addr+3] << 24);
                printf("Exposure Speed:\t\t%d/%d\n",int1,int2);
            }
            
            if(tiffs[j].tag == 0x829d) {
                short addr = tiffs[j].valueOffset + 12;
                int int1 = buffer[addr] + (buffer[addr+1] << 8) + (buffer[addr+2] << 16) + (buffer[addr+3] << 24);
                addr += 4;
                int int2 = buffer[addr] + (buffer[addr+1] << 8) + (buffer[addr+2] << 16) + (buffer[addr+3] << 24);
                float fstop = (float)int1/(float)int2;
                printf("F-Stop:\t\t\tf/%.1f\n",fstop);
            }
            
            if(tiffs[j].tag == 0x920a) {
                short addr = tiffs[j].valueOffset + 12;
                int int1 = buffer[addr] + (buffer[addr+1] << 8) + (buffer[addr+2] << 16) + (buffer[addr+3] << 24);
                addr += 4;
                int int2 = buffer[addr] + (buffer[addr+1] << 8) + (buffer[addr+2] << 16) + (buffer[addr+3] << 24);
                int fstop = int1/int2;
                printf("Focal Length:\t\t%d mm\n",fstop);
            }

        }
    }
    free(tiffs);
    return 0;
}

/*******************************************************************************
* Reads a string from a character offset in a given buffer.
********************************************************************************/
unsigned char* readStringFromOffset(char* buffer, int currentByte, int offset) {
    int bufferPosition = currentByte + offset;
    int count = 0;
    while(buffer[bufferPosition+count] != 0x00) {
        count++;
    }
    count += 1; // make room for NULL terminator
    unsigned char* string = (unsigned char *)malloc((count * sizeof(unsigned char)));
    int i = 0;
    for(i = 0; i < count; i++) {
        string[i] = buffer[bufferPosition+i];
    }
    return string;
}

/*******************************************************************************
* Takes a file, determines the file size, and reads the file into the returned
* buffer.
********************************************************************************/
unsigned char* readFileIntoByteArray(FILE *f)
{
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    //printf("Size of file in bytes: %i\n",size);
    unsigned char *ret = malloc(size);
    fseek(f, 0, SEEK_SET);
    fread(ret, size, 1, f);
    fclose(f);
    return ret;
}
