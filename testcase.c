#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include "buf.h"
#include "fs.h"

#define DIR_NUM_MAX		100

#define BLK_SIZE		(128)

FileSysInfo		*pFileSysInfo;



void PrintFileSysInfo(void)
{
	void* pBuf = NULL;

	pBuf = malloc(BLK_SIZE);
	BufRead(0, pBuf);
	pFileSysInfo = (FileSysInfo*)pBuf;
	printf("File system info: # of allocated files:%d, # of allocated blocks:%d, #of free blocks:%d\n", 
		pFileSysInfo->numAllocFiles, pFileSysInfo->numAllocBlocks, pFileSysInfo->numFreeBlocks);
	printf("FAT info: # of free entries in FAT:%d\n", FatGetNumOfFreeEntries());
	printf("\n");
}


void ListDirContents(const char* dirName)
{
	int i ;
	int count;
	DirEntry pDirEntry[DIR_NUM_MAX];

	EnumerateDirStatus(dirName, pDirEntry, &count);

	printf("[%s]Sub-directory:\n", dirName);
	for (i = 0;i < count;i++)
	{
		if (pDirEntry[i].filetype == FILE_TYPE_FILE){
			printf("\t name:%s, start block:%d, type:file, blocks:%d\n", pDirEntry[i].name, pDirEntry[i].startBlockNum, pDirEntry[i].numBlocks);
		}
		else if (pDirEntry[i].filetype == FILE_TYPE_DIR)
				printf("\t name:%s, start block:%d, type:directory, blocks:%d\n", pDirEntry[i].name, pDirEntry[i].startBlockNum, pDirEntry[i].numBlocks);
		else
		{
			assert(0);
		}
	}
}



void TestCase1(void)
{
	int i;
	char dirName[NAME_LEN];

	printf(" ---- Test Case 1 ----\n");

	MakeDir("/tmp");
	MakeDir("/usr");
	MakeDir("/etc");
	MakeDir("/home");
	/* make home directory */
	for (i = 0;i < 8;i++)
	{
		memset(dirName, 0, NAME_LEN);
		sprintf(dirName, "/home/u%d", i);
		MakeDir(dirName);
	}
	/* make etc directory */
	for (i = 0;i < 24;i++)
	{
		memset(dirName, 0, NAME_LEN);
		sprintf(dirName, "/etc/dev%d", i);
		MakeDir(dirName);
	}
	ListDirContents("/home");
	ListDirContents("/etc");

	/* remove subdirectory of etc directory */
	for (i = 23;i >= 0;i--)
	{
		memset(dirName, 0, NAME_LEN);
		sprintf(dirName, "/etc/dev%d", i);
		RemoveDir(dirName);
	}
	ListDirContents("/etc");
}


void TestCase2(void)
{
	int i, j;
	int fd;
	char fileName[NAME_LEN];
	char dirName[NAME_LEN];

	printf(" ---- Test Case 2 ----\n");

	ListDirContents("/home");
	/* make home directory */
	for (i = 0;i < 8;i++)
	{
		for (j = 0;j < 9;j++)
		{
			memset(fileName, 0, NAME_LEN);
			sprintf(fileName, "/home/u%d/f%d", i,j);
			fd = OpenFile(fileName, OPEN_FLAG_CREATE);
			CloseFile(fd);
		}
	}

	for (i = 0;i < 8;i++)
	{
		memset(dirName, 0, NAME_LEN);
		sprintf(dirName, "/home/u%d", i);
		ListDirContents(dirName);
	}
}


void TestCase3(void)
{
	int i;
	int fd;
	char fileName[NAME_LEN];
	char pBuffer1[BLK_SIZE];
	char pBuffer2[BLK_SIZE];

	printf(" ---- Test Case 3 ----\n");
	for (i = 0;i < 9;i++)
	{
		memset(fileName, 0, NAME_LEN);
		sprintf(fileName, "/home/u7/f%d", i);
		fd = OpenFile(fileName, OPEN_FLAG_CREATE);
		memset(pBuffer1, 0, BLK_SIZE);
		strcpy(pBuffer1, fileName);
		WriteFile(fd, pBuffer1, BLK_SIZE);
		CloseFile(fd);
	}
	for (i = 0;i < 9;i++)
	{
		memset(fileName, 0, NAME_LEN);
		sprintf(fileName, "/home/u7/f%d", i);
		fd = OpenFile(fileName, OPEN_FLAG_READWRITE);

		memset(pBuffer1, 0, BLK_SIZE);
		strcpy(pBuffer1, fileName);

		memset(pBuffer2, 0, BLK_SIZE);
		ReadFile(fd, pBuffer2, BLK_SIZE);

		if (strcmp(pBuffer1, pBuffer2))
		{
			printf("TestCase 3: error\n");
			exit(0);
		}
		CloseFile(fd);
	}
	printf(" ---- Test Case 3: files written/read----\n");
	ListDirContents("/home/u7");
}


void TestCase4(void)
{
	int i;
	int fd;
	char fileName[NAME_LEN];
	char pBuffer[BLK_SIZE*2];

	printf(" ---- Test Case 4 ----\n");
	for (i = 0;i < 9;i++)
	{
		if (i%2 == 0)
		{
			memset(fileName, 0, NAME_LEN);
			sprintf(fileName, "/home/u7/f%d", i);
			RemoveFile(fileName);
		}
	}
	printf(" ---- Test Case 4: files of even number removed ----\n");

	for (i = 0;i < 9;i++)
	{
		if (i%2)
		{
			memset(fileName, 0, NAME_LEN);
			sprintf(fileName, "/home/u7/f%d", i);
			fd = OpenFile(fileName, OPEN_FLAG_READWRITE);

			memset(pBuffer, 0, BLK_SIZE*2);
			strcpy(pBuffer, fileName);
			WriteFile(fd, pBuffer, BLK_SIZE + 1);
			CloseFile(fd);
		}
	}

	printf(" ---- Test Case 4: files of odd number overwritten ----\n");
	ListDirContents("/home/u7");

	for (i = 0;i < 9;i++)
	{
		if (i%2 == 0)
		{
			memset(fileName, 0, NAME_LEN);
			sprintf(fileName, "/home/u7/f%d", i);
			fd = OpenFile(fileName, OPEN_FLAG_CREATE);

			memset(pBuffer, 0, BLK_SIZE*2);
			strcpy(pBuffer, fileName);
			WriteFile(fd, pBuffer, BLK_SIZE + 1);
			WriteFile(fd, pBuffer, BLK_SIZE + 1);
			CloseFile(fd);
		}
	}
	printf(" ---- Test Case 4: files of even number re-created & written ----\n");
	ListDirContents("/home/u7");
}

void TestCase5(void)
{
	printf(" ---- Test Case 5 ----\n");

	ListDirContents("/home/u7");
}

int main(int argc, char** argv)
{
	int TcNum;

	if (argc < 3)
	{
ERROR:
		printf("usage: a.out [format | readwrite] [1-5])\n");
		return -1;
	}
	if (strcmp(argv[1], "format") == 0)
		Mount(MT_TYPE_FORMAT);
	else if (strcmp(argv[1], "readwrite") == 0)
		Mount(MT_TYPE_READWRITE);
	else
		goto ERROR;

	TcNum = atoi(argv[2]);

	DevResetDiskAccessCount();	

	switch (TcNum)
	{
	case 1:
		TestCase1();
		PrintFileSysInfo(); 
		break;
	case 2:
		TestCase2();
		PrintFileSysInfo(); 
		break;
	case 3:
		TestCase3();
		PrintFileSysInfo(); 
		break;
	case 4:
		TestCase4();
		PrintFileSysInfo(); 
		break;
	case 5:
		TestCase5();
		PrintFileSysInfo(); 
		break;
	default:
		Unmount();
		goto ERROR;
	}
	Unmount();

	printf("the number of disk access counts is %d\n", DevGetDiskReadCount() + DevGetDiskWriteCount());

	return 0;
}

