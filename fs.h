#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "Disk.h"


// ------- Caution -----------------------------------------
#define FS_DISK_CAPACITY	(BLOCK_SIZE*BLOCK_SIZE/sizeof(int)*BLOCK_SIZE)

#define FILEINFO_START_BLOCK	(0)
#define FAT_BLOCKS		(129)
#define FAT_START_BLOCK		(1)
#define DATA_START_BLOCK	(FAT_START_BLOCK + FAT_BLOCKS)


#define NUM_OF_DIRENT_PER_BLK	(BLOCK_SIZE / sizeof(DirEntry))


#define NAME_LEN			(16)
#define DESC_ENTRY_NUM			(128)
// ----------------------------------------------------------


typedef enum __openFlag {
	OPEN_FLAG_READWRITE,
	OPEN_FLAG_CREATE
} OpenFlag;


typedef enum __fileType {
    FILE_TYPE_FILE,
    FILE_TYPE_DIR,
    FILE_TYPE_DEV
} FileType;



typedef enum __fileMode {
	FILE_MODE_READONLY,
	FILE_MODE_READWRITE,
	FILE_MODE_EXEC
}FileMode;




typedef enum __mountType {
    MT_TYPE_FORMAT,
    MT_TYPE_READWRITE
} MountType;



typedef struct _FileSysInfo {
    int blocks;
    int rootFatEntryNum;
    int diskCapacity;
    int numAllocBlocks;
    int numFreeBlocks;
    int numAllocFiles;
    int fatTableStart;
    int dataStart;
} FileSysInfo;



typedef struct  __DirEntry {
     char name[NAME_LEN];
     int mode;
     int startBlockNum;
     FileType filetype;
     int numBlocks;
} DirEntry;


typedef struct _File {
    int   dirBlkNum;
    int   entryIndex;
    int   fileOffset;
} File;



typedef struct _DescEntry {
    int bUsed;
    File* pFile;
} DescEntry;

typedef struct  __FileDescTable {
    int        numUsedDescEntry;
    DescEntry  pEntry[DESC_ENTRY_NUM];
} FileDescTable;

extern int		OpenFile(const char* szFileName, OpenFlag flag);
extern int		WriteFile(int fileDesc, char* pBuffer, int length);
extern int		ReadFile(int fileDesc, char* pBuffer, int length);
extern int		CloseFile(int fileDesc);
extern int		RemoveFile(const char* szFileName);
extern int		MakeDir(const char* szDirName);
extern int		RemoveDir(const char* szDirName);
extern void		EnumerateDirStatus(const char* szDirName, DirEntry* pDirEntry, int* pNum);
extern void		Mount(MountType type);
extern void		Unmount(void);
extern void 		FileSysInit(void);


extern FileDescTable* pFileDescTable;
extern FileSysInfo* pFileSysInfo;


#endif /* FILESYSTEM_H_ */
