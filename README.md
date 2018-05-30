# FAT File System

> Implementing FAT File System

## int OpenFile(const char* szFileName, OpenFlag flag);

### Defining type
```c
typedef enum __openFlag{
  OPEN_FLAG_READWRITE, /* 파일을 read/write permission으로 open함. flag가 입력되었을 때, 리눅스와 달리 open file table 또는 file descriptor table에 셋팅을 하지 않음. 단순히 파일 open을 수행함 */
  OPEN_FLAG_CREATE /* 파일이 존재하지 않으면 생성 후 파일을 open함 */
} OpenFlag;
```

### Parameters
- szFileName[in]: open할 파일 이름. 파일 이름은 절대 경로

### Return
성공하면, file descriptor를 리턴한다. 이 file descriptor는 file descriptor table의entry의 index값으로 정의된다. 실패했을때는 -1을 리턴한다.

## int WriteFile(int fileDesc, char* pBuffer, int length); 
open된 파일에 데이터를 저장한다. 

###	Parameters
-	fileDesc[in]: file descriptor
-	pBuffer[in]: 저장할 데이터를 포함하는 메모리의 주소
-	length[in]: 저장될 데이터의 길이 

### Return
성공하면, 저장된 데이터의 길이 값을 리턴한다. 실패했을때는 -1을 리턴한다.

## int ReadFile(int fileDesc, char* pBuffer, int length); 
open된 파일에서 데이터를 읽는다. 

###	Parameters
-	fileDesc[in]: file descriptor
-	pBuffer[out]: 읽을 데이터를 저장할 메모리의 주소
-	length[in]: 읽을 데이터의 길이 

### Return
성공하면, 읽은 데이터의 길이 값을 리턴한다. 실패했을때는 -1을 리턴한다. 

## int CloseFile(int fileDesc); 
open된 파일을 닫는다. 

###	Parameters
fileDesc[in]: file descriptor

### Return
성공하면, 0을 리턴한다. 실패했을때는 -1을 리턴한다.

## int RemoveFile(const char* szFileName) 
파일을 제거한다. 단, open된 파일을 제거할 수 없다. 

###	Parameters
szFileName[in]: 제거할 파일 이름. 단, 파일 이름은 절대 경로임.  

### Return
성공하면, 0를 리턴한다. 실패했을때는 -1을 리턴한다. 실패 원인으로 (1) 제거할 파일 이름이 없을 경우, (2) 제거될 파일이 open되어 있을 경우.

## int MakeDir(const char* szDirName); 
디렉토리를 생성한다.

###	Parameters
szDirName[in]: 생성할 디렉토리 이름 (절대 경로 사용)

### Return
성공하면, 0을 리턴한다. 실패했을때는 -1을 리턴한다. 실패원인은 생성하고자 하는 디렉토리의 이름과 동일한 디렉토리 또는 파일이 존재할 경우.

## int RemoveDir( const char* szDirName); 
디렉토리를 제거한다. 단, 리눅스 파일 시스템처럼 빈 디렉토리만 제거가 가능하다. 

###	Parameters
szDirName[in]: 제거할 디렉토리 이름 (절대 경로 사용)

### Return
성공하면, 0을 리턴한다. 실패했을때는 -1을 리턴한다. 실패 원인으로, (1) 디렉토리에 파일 또는 하위 디렉토리가 존할 경우, (2) 제거하고자 하는 디렉토리가 없을 경우.

## int EnumerateDirStatus(const char* szDirName, DirEntry* pDirEntry, int dirEntrys );  
디렉토리에 포함된 파일 또는 디렉토리의 정보를 얻어낸다. 이 함수는 해당 디렉토리를 구성하고 있는 디렉토리 엔트리들의 묶음을 리턴한다. 

### Defining Types
```c
#define NAME_LEN_MAX 16
typedef enum __fileType {
  FILE_TYPE_FILE,
  FILE_TYPE_DIR,
  FILE_TYPE_DEV
} FileType; 

typedef struct __dirEntry {
  char name[16];
  int  startBlockNum; 
  FileType type;
  int numBlocks;
} DirEntry; 
```

###	Parameters
-	szDirName[in]: 디렉토리 이름 (절대 경로 사용)
-	pDirEntry[out]: 디렉토리 엔터리들을 저장할 메모리 주소
-	dirEntries[in]: 읽을 디렉토리 엔터리의 개수

### Return
성공하면, 읽어진 디렉토리 엔터리 개수를 리턴한다. 예로, 임의의 디렉토리의 전체 디렉토리 엔터리 개수가 40이지만, dirEntries가 60으로 입력했을 때, 리턴되는 값은 유효한 디렉토리 엔터리 개수인 40을 리턴해야 한다. 또한, 40개의 디렉토리 엔터리 내용을 pDirEntry 배열로 전달해야 한다. 또한, 전체 디렉토리 엔터리 개수가 40이지만 dirEntry가 20으로 입력되었을 때, 리틴되는 값은 20이며, 20개의 디렉토리 엔터리의 내용이 pDirEntry로 리턴되어야 한다. 에러 발생시, -1을 리턴한다.

### Example
```c
int i;
DirEntry pDirEntry[20];

if ((count = EnumeratreDirStatus(“/usr/home/kim”, pDirEntry, 20)) < 0)
  exit(0); // program terminated due to the error return of the function.

for (i=0 ; i<count ; i++)
  printf(“directory entry:%s, type:%d, fat entry number:%d\n”, pDirEntry[i].name, pDirEntry[i].type, pDirEntry[i].starFatEntry); 
```

 
## void Mount(MountType type);
가상 디스크의 파일 시스템을 초기화한다.

### Defining Types
```c
typedef enum __mountType {
  MT_TYPE_FORMAT,
  MT_TYPE_READWRITE,
} MountType;
```

## void Unmount(void); 
가상 디스크의 파일 시스템을 종료한다. 이 system call은 가상디스크 파일을 닫는다.
