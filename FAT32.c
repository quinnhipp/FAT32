// Name: Quinnlan Hipp
// Date: 4/20/22
// Last modified: 5/4/22
// FAT32.c - FAT32 File System
// Course: EECS 3540 - Operating Systems and Systems Programming
//
// Purpose: This program is able emulate part of the FAT32 file system.
// It has the following commmand line parametes: "DIR", "EXTRACT" and "QUIT"
// "DIR" lists the .img contents which include both short and long names, file sizes, dates, and times
// "EXTRACT" requires a file inside the .img file and will copy that file from the inside to the outside of the file
// "QUIT" terminates the program
//
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <uchar.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

// Given in project document
typedef struct PartitionTableEntry
{
    unsigned char bootFlag;
    unsigned char CHSBegin[3];
    unsigned char typeCode;
    unsigned char CHSEnd[3];
    unsigned int LBABegin;
    unsigned int LBAEnd;
} __attribute__((__packed__)) PartitionTableEntry;

// Given in project document
typedef struct MBRStruct
{
    unsigned char bootCode[446];
    PartitionTableEntry part1;
    PartitionTableEntry part2;
    PartitionTableEntry part3;
    PartitionTableEntry part4;
    unsigned short flag;
} __attribute__((__packed__)) MBR;

// Given in project document
typedef struct BPBStruct
{
    unsigned char BS_jmpBoot[3];
    unsigned char BS_OEMName[8];
    unsigned short BPB_BytsPerSec;
    unsigned char BPB_SecPerClus;
    unsigned short BPB_RsvdSecCnt;
    unsigned char BPB_NumFATs;
    unsigned short BPB_RootEntCnt;
    unsigned short BPB_TotSec16;
    unsigned char BPB_Media;
    unsigned short BPB_FATSz16;
    unsigned short BPB_SecPerTrk;
    unsigned short BPB_NumHeads;
    unsigned int BPB_HiddSec;
    unsigned int BPB_TotSec32;
    unsigned int BPB_FATSz32;
    unsigned short BPB_Flags;
    unsigned short BPB_FSVer;
    unsigned int BPB_RootClus;
    unsigned short BPB_FSInfo;
    unsigned short BPB_BkBootSec;
    unsigned char BPB_Reserved[12];
    unsigned char BS_DrvNum;
    unsigned char BS_Reserved1;
    unsigned char BS_BootSig;
    unsigned char BS_VolID[4];
    unsigned char BS_VolLab[11];
    unsigned char BS_FilSysType[8];
    unsigned char unused[420];
    unsigned char signature[2];
} __attribute__((__packed__)) BPB;

typedef struct ShortDIRStruct
{
    unsigned char Name[8];
    unsigned char Ext[3];
    unsigned char Attr;
    unsigned char NTRes;
    unsigned char CrtTimeTenth;
    unsigned short CrtTime;
    unsigned short CrtDate;
    unsigned short LstAccDate;
    unsigned short FstClusHI;
    unsigned short WrtTime;
    unsigned short WrtDate;
    unsigned short FstClusLO;
    unsigned int FileSize;
} __attribute__((__packed__)) ShortDIREntry;

typedef struct LongDIRStruct
{
    unsigned char Ord;
    char16_t Name1[5];
    unsigned char Attr;
    unsigned char Type;
    unsigned char Chksum;
    char16_t Name2[6];
    unsigned short FstClusLO;
    char16_t Name3[2];
} __attribute__((__packed__)) LongDIREntry;

typedef union DIRUnion
{
    ShortDIREntry s;
    LongDIREntry l;
} __attribute__((__packed__)) DIRUnion;

void DisplaySector(unsigned char *sector);
void DisplayRoot();
void ExtractFile(char *input);
unsigned int FindFirstFileCluster(char *searchingFile);
unsigned int GetLongFilename(unsigned char *longFilename, union DIRUnion *dir, int i);
void DisplayDIRSector(DIRUnion dir[16]);
void PrintFile(DIRUnion dir);

BPB bpb;
MBR mbr;
DIRUnion dir[16];

FILE *inputFile;
unsigned int FATStart;
unsigned int DATAStart;
unsigned char SectorBuffer[512];
unsigned int totalDIRSize;
unsigned int totalNumFiles;
unsigned char *rolloverLFN[255] = {};

int main(int argc, char *argv[])
{
    setlocale(LC_NUMERIC, ""); // Allows outputs to have commas
    // Checks arguments
    if (argc < 2)
    {
        printf("usage: ./FAT32 FILENAME.img\n");
        return -1;
    }

    // Open File passed by argument and read the first 512 Bytes into a MBR struct
    inputFile = fopen(argv[1], "r");
    fread(&mbr, sizeof(mbr), 1, inputFile);

    // Use the MBR to locate the beginning LBA of the first partition. Then
    // read the first block of the partition into a BPB struct
    unsigned int part1Start = mbr.part1.LBABegin;
    fseek(inputFile, part1Start * 512, SEEK_SET);
    fread(&bpb, sizeof(bpb), 1, inputFile);

    // Calculate the start of FAT and start of RootDIR
    FATStart = part1Start + bpb.BPB_RsvdSecCnt;               // Sector of FAT
    DATAStart = FATStart + bpb.BPB_NumFATs * bpb.BPB_FATSz32; // Sector of DATA

    // Call desired function
    char inputString[512];
    char *input = inputString;
    while (strcmp(input, "QUIT") != 0)
    {
        printf(">");
        scanf("%[^\n]%*c", input);
        if (strcmp(input, "DIR") == 0) // DIR
        {
            DisplayRoot();
        }
        if (strncmp(input, "EXTRACT", 5) == 0) // EXTRACT <some filename>
        {
            ExtractFile(input);
        }
        if (strncmp(input, "CD", 5) == 0) // CD <some directory>
        {
            ExtractFile(input);
        }
        if (strncmp(input, "IMPORT", 5) == 0) // IMPORT <some filename>
        {
            ExtractFile(input);
        }
        if (strncmp(input, "DELETE", 5) == 0) // DELETE <some filename>
        {
            ExtractFile(input);
        }
    }
    return 0;
}

// Extract desired file from img file into the project directory
void ExtractFile(char *input)
{
    char *searchingFile;
    char *in;
    char *extension;
    char *temp;
    unsigned int ClusNum = 0;
    searchingFile = (char *)malloc(255);
    in = (char *)malloc(255);
    extension = (char *)malloc(255);
    temp = (char *)malloc(255);
    memset(searchingFile, 0, sizeof(searchingFile));
    memset(in, 0, sizeof(in));
    memset(extension, 0, sizeof(extension));
    memset(temp, 0, sizeof(temp));
    (char *)strcpy(in, input);
    (char *)strcpy(temp, input);
    temp = strtok(in, ".");
    while (temp != NULL)
    {
        temp = strtok(NULL, ".");       // Get the extension from the inputted filename
        if (temp == 0x0)
        {
            break;
        }
        strcpy(extension, temp);
    }
    bool isCaps = false;
    for (int i = 0; i < strlen(extension); i++)
    {
        if (isupper(extension[i]))      // Check if extension is all caps or not
        {
            isCaps = true;
        }
    }
    if (!isCaps) // If LFN is inputted
    {
        char *token = strtok(input, " ");
        while (1)
        {
            token = strtok(NULL, " ");
            if (token == NULL)
                break;
            strcat(searchingFile, token);
            strcat(searchingFile, " ");
        }
        searchingFile[strlen(searchingFile) - 1] = '\0';
    }
    else // If SFN is inputted
    {
        searchingFile = strtok(input, " ");
        searchingFile = strtok(NULL, " ");
    }

    ClusNum = FindFirstFileCluster(searchingFile); // Find desired file
    if (ClusNum == 0)                              // File not found
    {
        printf("File Not found\n");
        return;
    }
    // If file was found, open a new file in same directory and copy data
    FILE *fp = NULL;
    fp = fopen(searchingFile, "wb");

    while (ClusNum < 0x0FFFFFF8) // Break if Next Cluster = EOF
    {
        // Read and display cluster DATA
        fseek(inputFile, (DATAStart + (ClusNum - 2) * bpb.BPB_SecPerClus) * 512, SEEK_SET);
        for (int i = bpb.BPB_SecPerClus; i > 0; i--)
        {
            fread(&SectorBuffer, sizeof(SectorBuffer), 1, inputFile);
            fwrite(SectorBuffer, 1, sizeof(SectorBuffer), fp);
        }
        // After reading a full Cluster, refer to FAT for next cluster
        fseek(inputFile, FATStart * 512 + ClusNum * 4, SEEK_SET);
        fread(&ClusNum, sizeof(ClusNum), 1, inputFile);
    }
    fclose(fp);
}

// Find the cluster that the desired file is in
unsigned int FindFirstFileCluster(char *searchingFile)
{
    unsigned char longFileName[255] = {};
    unsigned char shortFileName[255] = {};
    unsigned int ClusNum = bpb.BPB_RootClus; // Set initial Cluster to the Root Cluster
    while (ClusNum < 0x0FFFFFF8)             // If Next Cluster = EOF, break
    {
        // Read each DIR entry and compare filenames
        fseek(inputFile, (DATAStart + (ClusNum - 2) * bpb.BPB_SecPerClus) * 512, SEEK_SET);
        for (int i = bpb.BPB_SecPerClus; i > 0; i--)
        {
            fread(dir, sizeof(dir), 1, inputFile);
            for (int j = 0; j < 16; j++)
            {
                if (dir[j].l.Attr == 0x0F)
                {
                    j += GetLongFilename(longFileName, dir, j) - 1;     // Build LFN

                    if (strcmp(longFileName, searchingFile) == 0) // Compare LFN
                    {
                        // If matching LFN found, calculate and return ClusNum
                        ClusNum = dir[j].s.FstClusHI << 16 | dir[j].s.FstClusLO;
                        return ClusNum;
                    }
                }
                else
                {
                    memset(shortFileName, 0, sizeof(shortFileName));
                    int k;
                    int count = 0;
                    for (k = 0; k < 8; k++)
                    {
                        if (dir[j].s.Name[k] != ' ')
                        {
                            shortFileName[count] = dir[j].s.Name[k];        // Build the SFN
                            count++;
                        }
                    }
                    shortFileName[count] = '.';
                    count++;
                    int a;
                    for (a = 0; a < 3; a++)
                    {
                        if (dir[j].s.Ext[a] != ' ')
                        {
                            shortFileName[count] = dir[j].s.Ext[a];
                            count++;
                        }
                    }

                    if (strncmp(shortFileName, searchingFile, 13) == 0) // Compare SFN
                    {
                        // If matching SFN found, calculate and return ClusNum
                        ClusNum = dir[j].s.FstClusHI << 16 | dir[j].s.FstClusLO;
                        return ClusNum;
                    }
                }
            }
        }
        // After reading a Cluster of ROOT, Check the FAT for the next cluster
        fseek(inputFile, FATStart * 512 + ClusNum * 4, SEEK_SET);
        fread(&ClusNum, sizeof(ClusNum), 1, inputFile);
    }
    return 0;
}

// Navigate through each cluster in the root starting at RootClus
void DisplayRoot()
{
    unsigned int ClusNum = bpb.BPB_RootClus;
    while (ClusNum <= 0x0FFFFFF8 && ClusNum >= 0x00000002) // Break if Next Cluster = EOF
    {
        fseek(inputFile, (DATAStart + (ClusNum - 2) * bpb.BPB_SecPerClus) * 512, SEEK_SET);
        for (int i = bpb.BPB_SecPerClus; i > 0; i--)
        {
            fread(dir, sizeof(dir), 1, inputFile);      // Read and display each sector
            DisplayDIRSector(dir);
        }
        // After reading a full Cluster, refer to FAT for next cluster
        fseek(inputFile, FATStart * 512 + ClusNum * 4, SEEK_SET);
        fread(&ClusNum, sizeof(ClusNum), 1, inputFile);
    }
    printf("\t\t%.1i File(s)\t%'10i bytes\n", totalNumFiles, totalDIRSize); // Print the summary
    totalDIRSize = 0;
    totalNumFiles = 0;
}

// Build each LFN into longFileName
unsigned int GetLongFilename(unsigned char *longFileName, union DIRUnion *dir, int i)
{
    unsigned int longNameEntries = (dir[i].l.Ord & 0x3F);
    int index = 0;
    for (int j = longNameEntries - 1; j >= 0; j--)
    {
        for (int k = 0; k < 5; k++)
        {
            longFileName[index] = dir[i + j].l.Name1[k];
            index++;
        }
        for (int k = 0; k < 6; k++)
        {
            longFileName[index] = dir[i + j].l.Name2[k];
            index++;
        }
        for (int k = 0; k < 2; k++)
        {
            longFileName[index] = dir[i + j].l.Name3[k];
            index++;
        }
    }
    return longNameEntries;
}

// Read and print each entry in the sector
void DisplayDIRSector(DIRUnion dir[16])
{
    unsigned char longFileName[255] = {};
    for (int i = 0; i < 16; i++)
    {
        if (dir[i].s.Name[0] == 0xE5) // If first bit is 0xE5, the sector is blank
            continue;

        if (dir[i].s.Name[0] == 0x00) // If first bit == 0x00, it's empty and doesn't need to be displayed
            break;

        if (dir[i].s.Attr == 0x02) // Skip Hidden files
            continue;

        if (dir[i].s.Attr == 0x08) // Skip Volume ID
            continue;

        memset(longFileName, 0, sizeof(longFileName)); // Clear longName string
        if (dir[i].l.Attr == 0x0F)                 // Long name Entry
        {
            i += GetLongFilename(longFileName, dir, i);
            memset(rolloverLFN, 0, sizeof(rolloverLFN)); // Clear rolloverLFN string
            memcpy(rolloverLFN, longFileName, strlen(longFileName));
        }
        if (dir[i].s.FileSize == 0)     // Skip if fileSize = 0
        {
            continue;
        }
        if (longFileName[0] == '\000' && i > 1)    // In case rolloverLFN is needed
        {
            memset(rolloverLFN, 0, sizeof(rolloverLFN));
        }
        unsigned int month, day, year, hours, minutes;
        char *ToD = "AM";
        day = dir[i].s.WrtDate & 0x1F;
        month = (dir[i].s.WrtDate & 0x1E0) >> 5;
        year = ((dir[i].s.WrtDate & 0xFE00) >> 9) + 1980;
        minutes = (dir[i].s.WrtTime & 0x3F0) >> 5;
        hours = (dir[i].s.WrtTime & 0x7C00) >> 11;
        if (hours > 12)
        {
            hours -= 12;
            ToD = "PM";
        }
        // Print all required data with rolloverLFN
        if (longFileName[0] == '\000')
        {
            printf("%02i/%02i/%4i   %02i:%02i %s  %'10i  %.8s.%.3s %s\n",
                   month, day, year,
                   hours, minutes, ToD,
                   dir[i].s.FileSize, dir[i].s.Name, dir[i].s.Ext, rolloverLFN); // longName
            totalDIRSize += dir[i].s.FileSize;  // Add fileSize to total size
        }
        // Print all required data with longFileName
        else
        {
            printf("%02i/%02i/%4i   %02i:%02i %s  %'10i  %.8s.%.3s %s\n",
                   month, day, year,
                   hours, minutes, ToD,
                   dir[i].s.FileSize, dir[i].s.Name, dir[i].s.Ext, longFileName); // longName
            totalDIRSize += dir[i].s.FileSize;
        }
        totalNumFiles++;           // Increment total files
    }
}

// Given in project document
void displaySector(unsigned char *sector)
{
    // Display the contents of sector[] as 16 rows of 32 bytes each. Each row is shown as 16 bytes,
    // a "-", and then 16 more bytes. The left part of the display is in hex; the right part is in
    // ASCII (if the character is printable; otherwise we display ".".
    //
    for (int i = 0; i < 16; i++)                 // for each of 16 rows
    {                                            //
        for (int j = 0; j < 32; j++)             // for each of 32 values per row
        {                                        //
            printf("%02X ", sector[i * 32 + j]); // Display the byte in hex
            if (j % 32 == 15)
                printf("- "); // At the half-way point? Show divider
        }
        printf(" ");                 // Separator space between hex & ASCII
        for (int j = 0; j < 32; j++) // For each of those same 32 bytes
        {                            //
            if (j == 16)
                printf(" ");                          // Halfway? Add a space for clarity
            int c = (unsigned int)sector[i * 32 + j]; // What IS this char's ASCII value?
            if (c >= 32 && c <= 127)
                printf("%1c", c); // IF printable, display it
            else
                printf("."); // Otherwise, display a "."
        }                    //
        printf("\n");        // That’s the end of this row
    }
}
