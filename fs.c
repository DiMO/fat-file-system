#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "fat.h"
#include "fs.h"
#include "Disk.h"

FileDescTable *pFileDescTable = NULL;
FileSysInfo *pFileSysInfo = NULL;

void FileSysInit(void) {
  pFileDescTable = (FileDescTable *) malloc(sizeof(FileDescTable));
  memset(pFileDescTable, 0, sizeof(FileDescTable));
}

void Mount(MountType type) {
  int freeEntryNum;
  DirEntry *dirEntry;

	FileSysInit();
  BufInit();

  if(type == MT_TYPE_FORMAT) {
    DevCreateDisk();
    FatInit();
    freeEntryNum = FatGetFreeEntryNum();
    FatAdd(-1, freeEntryNum);

    dirEntry = (DirEntry *) malloc(BLOCK_SIZE);
    memset(dirEntry, 0, BLOCK_SIZE);
    strcpy(dirEntry[0].name, ".");
    dirEntry[0].mode = 486;
    dirEntry[0].startBlockNum = freeEntryNum;
    dirEntry[0].filetype = FILE_TYPE_DIR;
    dirEntry[0].numBlocks = 1;
    BufWrite(freeEntryNum, (char *) dirEntry);
    free(dirEntry);

    pFileSysInfo = (FileSysInfo *) malloc(BLOCK_SIZE);
    memset(pFileSysInfo, 0, BLOCK_SIZE);
    pFileSysInfo->blocks = (FAT_BLOCKS-3)*(BLOCK_SIZE/sizeof(int)) + FAT_BLOCKS + 1; // DATA_REGION_BLOCKS + FAT_BLOCKS + FILEINFO_BLOCK
    pFileSysInfo->rootFatEntryNum = freeEntryNum;
    pFileSysInfo->diskCapacity = FS_DISK_CAPACITY;
    pFileSysInfo->numAllocBlocks = 0;
    pFileSysInfo->numFreeBlocks = (FAT_BLOCKS-3)*(BLOCK_SIZE/sizeof(int)) - 2; // DATA_REGION_BLOCKS = 4030
    pFileSysInfo->numAllocFiles = 0;
    pFileSysInfo->fatTableStart = FAT_START_BLOCK;
    pFileSysInfo->dataStart = DATA_START_BLOCK;
    pFileSysInfo->numAllocBlocks++;
    pFileSysInfo->numFreeBlocks--;
    pFileSysInfo->numAllocFiles++;
    BufWrite(0, (char *) pFileSysInfo);
  }
  else if(type == MT_TYPE_READWRITE) {
    DevOpenDisk();
    pFileSysInfo = (FileSysInfo *) malloc(BLOCK_SIZE);
    memset(pFileSysInfo, 0, BLOCK_SIZE);
    BufRead(0, (char *) pFileSysInfo);
  }
}

void Unmount() {
  BufSync();
  DevCloseDisk();
}

int OpenFile(const char* szFileName, OpenFlag flag) {
  int i, nextEntryNum, entryIndexNum, freeEntryNum;
  DirEntry *dirEntry, *dirNextEntry;
  File *file;

  char *str = (char *) malloc(sizeof(szFileName));
  strcpy(str, szFileName);
  char *word, *temp;
  word = strtok(str, "/");
  nextEntryNum = pFileSysInfo->rootFatEntryNum;
  while (word != NULL)
  {
    temp = (char *) malloc(sizeof(word));
    strcpy(temp, word);
    word = strtok(NULL, "/");

    for(;;) {
      dirEntry = (DirEntry *) malloc(BLOCK_SIZE);
      memset(dirEntry, 0, BLOCK_SIZE);
      BufRead(nextEntryNum, (char *) dirEntry);

      for(i = 0; i<NUM_OF_DIRENT_PER_BLK; i++) {
        if(word == NULL) {
          if(dirEntry[i].filetype == FILE_TYPE_FILE && strcmp(dirEntry[i].name, temp) == 0) {
            entryIndexNum = i;
            break;
          }
          else if(flag == OPEN_FLAG_CREATE && strcmp(dirEntry[i].name, "") == 0) {
            entryIndexNum = i;
            strcpy(dirEntry[i].name, temp);
            dirEntry[i].mode = 486;
            dirEntry[i].startBlockNum = -1;
            dirEntry[i].filetype = FILE_TYPE_FILE;
            dirEntry[i].numBlocks = 0;
            BufWrite(nextEntryNum, (char *) dirEntry);

            pFileSysInfo->numAllocFiles++;
            BufWrite(0, (char *) pFileSysInfo);
            break;
          }
        }
        else if(word != NULL && dirEntry[i].filetype == FILE_TYPE_DIR && strcmp(dirEntry[i].name, temp) == 0) {
          nextEntryNum = dirEntry[i].startBlockNum;
          break;
        }
      }

      if(FatGetBlockNum(nextEntryNum, 1) == -1 && word == NULL && i == NUM_OF_DIRENT_PER_BLK) {
        freeEntryNum = FatGetFreeEntryNum();
        FatAdd(nextEntryNum, freeEntryNum);

        dirNextEntry = (DirEntry *) malloc(BLOCK_SIZE);
        memset(dirNextEntry, 0, BLOCK_SIZE);
        BufWrite(freeEntryNum, (char *) dirNextEntry);
        free(dirNextEntry);

        pFileSysInfo->numAllocBlocks++;
        pFileSysInfo->numFreeBlocks--;
        BufWrite(0, (char *) pFileSysInfo);
      }

      if(i == NUM_OF_DIRENT_PER_BLK) {
        nextEntryNum = FatGetBlockNum(nextEntryNum, 1);
        if(nextEntryNum == -1) break;
      }
      else {
        break;
      }
      free(dirEntry);
    }
    free(temp);
  }

  for(i = 0; i<DESC_ENTRY_NUM; i++) {
    if(pFileDescTable->pEntry[i].bUsed == 0) {
      file = (File *) malloc(sizeof(File));
      memset(file, 0, sizeof(File));
      file->dirBlkNum = nextEntryNum;
      file->entryIndex = entryIndexNum;
      file->fileOffset = 0;
      pFileDescTable->pEntry[i].bUsed = 1;
      pFileDescTable->pEntry[i].pFile = file;
      return i;
    }
  }

  return -1;
}

int WriteFile(int fileDesc, char* pBuffer, int length) {
  int freeEntryNum, dirBlkNum, entryIndexNum;
  DirEntry *dirEntry;
  char *pBuf;

  dirBlkNum = pFileDescTable->pEntry[fileDesc].pFile->dirBlkNum;
  entryIndexNum = pFileDescTable->pEntry[fileDesc].pFile->entryIndex;

  dirEntry = (DirEntry *) malloc(BLOCK_SIZE);
  memset(dirEntry, 0, BLOCK_SIZE);
  BufRead(dirBlkNum, (char *) dirEntry);
  if(dirEntry[entryIndexNum].startBlockNum == -1) {
    freeEntryNum = FatGetFreeEntryNum();
    FatAdd(-1, freeEntryNum);

    dirEntry[entryIndexNum].startBlockNum = freeEntryNum;
    dirEntry[entryIndexNum].numBlocks = 1;
    BufWrite(dirBlkNum, (char *) dirEntry);

    pFileSysInfo->numAllocBlocks++;
    pFileSysInfo->numFreeBlocks--;
    BufWrite(0, (char *) pFileSysInfo);
  }
  else {
    freeEntryNum = dirEntry[entryIndexNum].startBlockNum;
  }
  free(dirEntry);

  pFileDescTable->pEntry[fileDesc].pFile->fileOffset += BLOCK_SIZE;
  pBuf = (char *) malloc(BLOCK_SIZE);
  memcpy(pBuf, pBuffer, BLOCK_SIZE);
  BufWrite(freeEntryNum, (char *) pBuf);

  return strlen(pBuf);
}

int ReadFile(int fileDesc, char* pBuffer, int length) {
  int fileBlkNum, dirBlkNum, entryIndexNum, fileOffset, physicalBlkNum;
  DirEntry *dirEntry;
  char *pBuf;

  dirBlkNum = pFileDescTable->pEntry[fileDesc].pFile->dirBlkNum;
  entryIndexNum = pFileDescTable->pEntry[fileDesc].pFile->entryIndex;
  fileOffset = pFileDescTable->pEntry[fileDesc].pFile->fileOffset;

  dirEntry = (DirEntry *) malloc(BLOCK_SIZE);
  memset(dirEntry, 0, BLOCK_SIZE);
  BufRead(dirBlkNum, (char *) dirEntry);
  fileBlkNum = dirEntry[entryIndexNum].startBlockNum;
  free(dirEntry);

  physicalBlkNum = FatGetBlockNum(fileBlkNum, fileOffset/BLOCK_SIZE);
  pBuf = (char *) malloc(BLOCK_SIZE);
  BufRead(physicalBlkNum, (char *) pBuf);
  memcpy(pBuffer, pBuf, BLOCK_SIZE);
  free(pBuf);

  pFileDescTable->pEntry[fileDesc].pFile->fileOffset += BLOCK_SIZE;

  return strlen(pBuffer);
}

int CloseFile(int fileDesc) {
  if(pFileDescTable->pEntry[fileDesc].bUsed == 1) {
    free(pFileDescTable->pEntry[fileDesc].pFile);
    pFileDescTable->pEntry[fileDesc].bUsed = 0;
  }
  else return -1;

  return 0;
}

int RemoveFile(const char* szFileName) {
  int i, j, count, nextEntryNum;
  DirEntry *dirEntry;

  char *str = (char *) malloc(sizeof(szFileName));
  strcpy(str, szFileName);
  char *word, *temp;
  word = strtok(str, "/");
  nextEntryNum = pFileSysInfo->rootFatEntryNum;
  while (word != NULL)
  {
    temp = (char *) malloc(sizeof(word));
    strcpy(temp, word);
    word = strtok(NULL, "/");

    for(;;) {
      dirEntry = (DirEntry *) malloc(BLOCK_SIZE);
      memset(dirEntry, 0, BLOCK_SIZE);
      BufRead(nextEntryNum, (char *) dirEntry);

      for(i = 0; i<NUM_OF_DIRENT_PER_BLK; i++) {
        if(word == NULL) {
          if(dirEntry[i].filetype == FILE_TYPE_FILE && strcmp(dirEntry[i].name, temp) == 0) {
            count = FatRemove(dirEntry[i].startBlockNum, FatGetBlockNum(dirEntry[i].startBlockNum, 1));
            memset(&dirEntry[i], 0, sizeof(DirEntry));
            BufWrite(nextEntryNum, (char *) dirEntry);
            for(j = 0; j<NUM_OF_DIRENT_PER_BLK; j++) {
              if(strcmp(dirEntry[j].name, "") != 0)
                break;
            }
            if(j == NUM_OF_DIRENT_PER_BLK) {
              count += FatRemove(nextEntryNum, -1);
            }

            pFileSysInfo->numAllocBlocks-=count;
            pFileSysInfo->numFreeBlocks+=count;
            pFileSysInfo->numAllocFiles--;
            BufWrite(0, (char *) pFileSysInfo);
            return 0;
          }
        }
        else if(word != NULL && dirEntry[i].filetype == FILE_TYPE_DIR && strcmp(dirEntry[i].name, temp) == 0) {
          nextEntryNum = dirEntry[i].startBlockNum;
          break;
        }
      }
      free(dirEntry);
      if(i == NUM_OF_DIRENT_PER_BLK) {
        nextEntryNum = FatGetBlockNum(nextEntryNum, 1);
        if(nextEntryNum == -1) break;
      }
      else {
        break;
      }
    }
    free(temp);
  }
  return 0;
}

int MakeDir(const char* szDirName) {
  int i, freeEntryNum, nextEntryNum, physicalBlkNum;
  DirEntry *dirEntry, *dirNextEntry;

  char *str = (char *) malloc(sizeof(szDirName));
  strcpy(str, szDirName);
  char *word, *temp;
  word = strtok(str, "/");
  physicalBlkNum = pFileSysInfo->rootFatEntryNum;
  while (word != NULL)
  {
    temp = (char *) malloc(sizeof(word));
    strcpy(temp, word);
    word = strtok(NULL, "/");

    nextEntryNum = physicalBlkNum;
    for(;;) {
      dirEntry = (DirEntry *) malloc(BLOCK_SIZE);
      memset(dirEntry, 0, BLOCK_SIZE);
      BufRead(nextEntryNum, (char *) dirEntry);

      for(i = 0; i<NUM_OF_DIRENT_PER_BLK; i++) {
        if(word == NULL) {
          if(dirEntry[i].filetype == FILE_TYPE_DIR && strcmp(dirEntry[i].name, temp) == 0) {
            return -1;
          }
          else if(strcmp(dirEntry[i].name, "") == 0) {
            freeEntryNum = FatGetFreeEntryNum();
            FatAdd(-1, freeEntryNum);

            strcpy(dirEntry[i].name, temp);
            dirEntry[i].mode = 486;
            dirEntry[i].startBlockNum = freeEntryNum;
            dirEntry[i].filetype = FILE_TYPE_DIR;
            dirEntry[i].numBlocks = 1;
            BufWrite(nextEntryNum, (char *) dirEntry);

            break;
          }
        }
        else if(word != NULL && dirEntry[i].filetype == FILE_TYPE_DIR && strcmp(dirEntry[i].name, temp) == 0) {
          physicalBlkNum = dirEntry[i].startBlockNum;
          break;
        }
      }

      if(FatGetBlockNum(nextEntryNum, 1) == -1 && word == NULL && i == NUM_OF_DIRENT_PER_BLK) {
        freeEntryNum = FatGetFreeEntryNum();
        FatAdd(nextEntryNum, freeEntryNum);

        dirNextEntry = (DirEntry *) malloc(BLOCK_SIZE);
        memset(dirNextEntry, 0, BLOCK_SIZE);
        BufWrite(freeEntryNum, (char *) dirNextEntry);
        free(dirNextEntry);

        pFileSysInfo->numAllocBlocks++;
        pFileSysInfo->numFreeBlocks--;
        BufWrite(0, (char *) pFileSysInfo);
      }

      if(i == NUM_OF_DIRENT_PER_BLK) {
        nextEntryNum = FatGetBlockNum(nextEntryNum, 1);
        if(nextEntryNum == -1) break;
      }
      else {
        break;
      }
      free(dirEntry);
    }
    free(temp);
  }

  dirEntry = (DirEntry *) malloc(BLOCK_SIZE);
  memset(dirEntry, 0, BLOCK_SIZE);
  strcpy(dirEntry[0].name, ".");
  dirEntry[0].mode = 486;
  dirEntry[0].startBlockNum = freeEntryNum;
  dirEntry[0].filetype = FILE_TYPE_DIR;
  dirEntry[0].numBlocks = 1;
  strcpy(dirEntry[1].name, "..");
  dirEntry[1].mode = 486;
  dirEntry[1].startBlockNum = physicalBlkNum;
  dirEntry[1].filetype = FILE_TYPE_DIR;
  dirEntry[1].numBlocks = 1;
  BufWrite(freeEntryNum, (char *) dirEntry);
  free(dirEntry);

  pFileSysInfo->numAllocBlocks++;
  pFileSysInfo->numFreeBlocks--;
  pFileSysInfo->numAllocFiles++;
  BufWrite(0, (char *) pFileSysInfo);

  return 0;
}

int RemoveDir(const char* szDirName) {
  int i, j, removeEntryNum, nextEntryNum, physicalBlkNum, count;
  DirEntry *dirEntry, *dirRemoveEntry;

  char *str = (char *) malloc(sizeof(szDirName));
  strcpy(str, szDirName);
  char *word, *temp;
  word = strtok(str, "/");
  nextEntryNum = pFileSysInfo->rootFatEntryNum;
  while (word != NULL)
  {
    temp = (char *) malloc(sizeof(word));
    strcpy(temp, word);
    word = strtok(NULL, "/");

    for(;;) {
      dirEntry = (DirEntry *) malloc(BLOCK_SIZE);
      memset(dirEntry, 0, BLOCK_SIZE);
      BufRead(nextEntryNum, (char *) dirEntry);

      for(i = 0; i<NUM_OF_DIRENT_PER_BLK; i++) {
        if(word == NULL) {
          if(dirEntry[i].filetype == FILE_TYPE_DIR && strcmp(dirEntry[i].name, temp) == 0) {
            physicalBlkNum = dirEntry[i].startBlockNum;
            removeEntryNum = physicalBlkNum;

            for(;;) {
              dirRemoveEntry = (DirEntry *) malloc(BLOCK_SIZE);
              memset(dirRemoveEntry, 0, BLOCK_SIZE);
              BufRead(removeEntryNum, (char *) dirRemoveEntry);
              for(j = 0; j<NUM_OF_DIRENT_PER_BLK; j++) {
                if(strcmp(dirRemoveEntry[j].name, "") != 0 && strcmp(dirRemoveEntry[j].name, ".") != 0 && strcmp(dirRemoveEntry[j].name, "..") != 0)
                  return -1;
              }
              removeEntryNum = FatGetBlockNum(removeEntryNum, 1);
              if(removeEntryNum == -1) break;
            }
            count = FatRemove(physicalBlkNum, FatGetBlockNum(physicalBlkNum, 1));
            memset(&dirEntry[i], 0, sizeof(DirEntry));
            BufWrite(nextEntryNum, (char *) dirEntry);
            for(j = 0; j<NUM_OF_DIRENT_PER_BLK; j++) {
              if(strcmp(dirEntry[j].name, "") != 0)
                break;
            }
            if(j == NUM_OF_DIRENT_PER_BLK) {
              count += FatRemove(nextEntryNum, -1);
            }

            pFileSysInfo->numAllocBlocks-=count;
            pFileSysInfo->numFreeBlocks+=count;
            pFileSysInfo->numAllocFiles--;
            BufWrite(0, (char *) pFileSysInfo);

            free(dirRemoveEntry);
            return 0;
          }
        }
        else if(word != NULL && dirEntry[i].filetype == FILE_TYPE_DIR && strcmp(dirEntry[i].name, temp) == 0) {
          nextEntryNum = dirEntry[i].startBlockNum;
          break;
        }
      }
      free(dirEntry);
      if(i == NUM_OF_DIRENT_PER_BLK) {
        nextEntryNum = FatGetBlockNum(nextEntryNum, 1);
        if(nextEntryNum == -1) break;
      }
      else {
        break;
      }
    }
    free(temp);
  }
  return 0;
}

void EnumerateDirStatus(const char* szDirName, DirEntry* pDirEntry, int* pNum) {
  int i, j, count, nextEntryNum;
  DirEntry *dirEntry, *dirEnumerateEntry;

  count = 0;
  char *str = (char *) malloc(sizeof(szDirName));
  strcpy(str, szDirName);
  char *word, *temp;
  word = strtok(str, "/");
  nextEntryNum = pFileSysInfo->rootFatEntryNum;
  while (word != NULL)
  {
    temp = (char *) malloc(sizeof(word));
    strcpy(temp, word);
    word = strtok(NULL, "/");

    for(;;) {
      dirEntry = (DirEntry *) malloc(BLOCK_SIZE);
      memset(dirEntry, 0, BLOCK_SIZE);
      BufRead(nextEntryNum, (char *) dirEntry);

      for(i = 0; i<NUM_OF_DIRENT_PER_BLK; i++) {
        if(word == NULL) {
          if(dirEntry[i].filetype == FILE_TYPE_DIR && strcmp(dirEntry[i].name, temp) == 0) {
            nextEntryNum = dirEntry[i].startBlockNum;

            for(;;) {
              dirEnumerateEntry = (DirEntry *) malloc(BLOCK_SIZE);
              memset(dirEnumerateEntry, 0, BLOCK_SIZE);
              BufRead(nextEntryNum, (char *) dirEnumerateEntry);

              for(j = 0; j<NUM_OF_DIRENT_PER_BLK; j++) {
                if(strcmp(dirEnumerateEntry[j].name, "") == 0) continue;
                pDirEntry[count] = dirEnumerateEntry[j];
                count++;
              }
              nextEntryNum = FatGetBlockNum(nextEntryNum, 1);
              if(nextEntryNum == -1) break;
            }
            free(dirEnumerateEntry);
            *pNum = count;
            return;
          }
        }
        else if(word != NULL && dirEntry[i].filetype == FILE_TYPE_DIR && strcmp(dirEntry[i].name, temp) == 0) {
          nextEntryNum = dirEntry[i].startBlockNum;
          break;
        }
      }
      if(i == NUM_OF_DIRENT_PER_BLK) {
        nextEntryNum = FatGetBlockNum(nextEntryNum, 1);
        if(nextEntryNum == -1) break;
      }
      else {
        break;
      }
      free(dirEntry);
    }
    free(temp);
  }
  *pNum = -1;
}
