#include <stdio.h>
#include <stdlib.h>
#include "fat.h"
#include "buf.h"

void FatInit(void)
{
  int i;
  for(i=1; i<=NUM_FAT_BLOCK; i++) {
    int *buf = (int *) calloc(BLOCK_SIZE/sizeof(int), sizeof(int));
    BufWrite(i, (char *) buf);
  }
}

/* newBlkNum이 지정하는 FAT entry의 value가 0이 아니면 -1을 리턴함.
   lastBlkNum이 지정하는 FAT entry의 값이 -1이 아니면 -1을 리턴함. */
int FatAdd(int lastBlkNum, int newBlkNum)
{
    if(lastBlkNum == -1) {
      int *buf = (int *) malloc(BLOCK_SIZE);
      int blkNum = newBlkNum/(BLOCK_SIZE/sizeof(int));

      BufRead(blkNum, (char *) buf);

      int indexNum = newBlkNum%(BLOCK_SIZE/sizeof(int));
      if(*(buf+indexNum) != 0) return -1;
      *(buf+indexNum) = lastBlkNum;

      BufWrite(blkNum, (char *) buf);
      free(buf);
    }
    else if(lastBlkNum/(BLOCK_SIZE/sizeof(int)) != newBlkNum/(BLOCK_SIZE/sizeof(int))){
      int *buf1 = (int *) malloc(BLOCK_SIZE);
      int blkNum1 = lastBlkNum/(BLOCK_SIZE/sizeof(int));
      BufRead(blkNum1, (char *) buf1);

      int indexNum1 = lastBlkNum%(BLOCK_SIZE/sizeof(int));
      if(*(buf1+indexNum1) != -1) return -1;
      *(buf1+indexNum1) = newBlkNum;

      int *buf2 = (int *) malloc(BLOCK_SIZE);
      int blkNum2 = newBlkNum/(BLOCK_SIZE/sizeof(int));
      BufRead(blkNum2, (char *) buf2);

      int indexNum2 = newBlkNum%(BLOCK_SIZE/sizeof(int));
      if(*(buf2+indexNum2) != 0) return -1;
      *(buf2+indexNum2) = -1;

      BufWrite(blkNum1, (char *) buf1);
      BufWrite(blkNum2, (char *) buf2);

      free(buf1);
      free(buf2);
    }
    else {
      int *buf = (int *) malloc(BLOCK_SIZE);
      int blkNum = lastBlkNum/(BLOCK_SIZE/sizeof(int));
      BufRead(blkNum, (char *) buf);

      int indexNum1 = lastBlkNum%(BLOCK_SIZE/sizeof(int));
      if(*(buf+indexNum1) != -1) return -1;
      *(buf+indexNum1) = newBlkNum;

      int indexNum2 = newBlkNum%(BLOCK_SIZE/sizeof(int));
      if(*(buf+indexNum2) != 0) return -1;
      *(buf+indexNum2) = -1;

      BufWrite(blkNum, (char *) buf);

      free(buf);
    }
}

/* firstBlkNum이 지정하는 FAT entry의 value가 0이거나
   logicalBlkNum에 대응하는 physical block 번호가 -1이거나 0인 경우, -1을 리턴함 */
int FatGetBlockNum(int firstBlkNum, int logicalBlkNum)
{
  int *buf = (int *) malloc(BLOCK_SIZE);

  int i;
  int physicalBlkNum = firstBlkNum;
  int blkNum = physicalBlkNum/(BLOCK_SIZE/sizeof(int));
  BufRead(blkNum, (char *) buf);

  for(i=0; i<logicalBlkNum; i++) {
    if(blkNum != physicalBlkNum/(BLOCK_SIZE/sizeof(int))) {
      blkNum = physicalBlkNum/(BLOCK_SIZE/sizeof(int));
      BufRead(blkNum, (char *) buf);
    }
    int indexNum = physicalBlkNum%(BLOCK_SIZE/sizeof(int));
    if(*(buf+indexNum) == 0) return -1;
    physicalBlkNum = *(buf+indexNum);
  }

  free(buf);
  if(physicalBlkNum == -1 || physicalBlkNum == 0) return -1;
  return physicalBlkNum;
}

/* firstBlkNum이 지정하는 FAT entry의 value가 0이거나
   startBlkNm이 지정하는 FAT entry의 value가 0인 경우, -1을 리턴함.*/
int FatRemove(int firstBlkNum, int startBlkNum)
{
  int count = 0;
  int *buf = (int *) malloc(BLOCK_SIZE);
  int blkNum = firstBlkNum/(BLOCK_SIZE/sizeof(int));
  BufRead(blkNum, (char *) buf);
  int indexNum = firstBlkNum%(BLOCK_SIZE/sizeof(int));
  // if(*(buf+indexNum) == 0) return 0;

  if(startBlkNum == -1) {
    *(buf+indexNum) = 0;
    BufWrite(blkNum, (char *) buf);
    count++;
    free(buf);
    return count;
  }

  int nextBlkNum;
  for(;;) {
    if(*(buf+indexNum) == startBlkNum) {
      nextBlkNum = *(buf+indexNum);
      *(buf+indexNum) = -1;
      break;
    }
    nextBlkNum = *(buf+indexNum);
    indexNum = nextBlkNum%(BLOCK_SIZE/sizeof(int));

    if(blkNum != nextBlkNum/(BLOCK_SIZE/sizeof(int))) {
      blkNum = nextBlkNum/(BLOCK_SIZE/sizeof(int));
      BufRead(blkNum, (char *) buf);
    }
  }
  BufWrite(blkNum, (char *) buf);

  for(;;) {
    if(blkNum != nextBlkNum/(BLOCK_SIZE/sizeof(int))) {
      blkNum = nextBlkNum/(BLOCK_SIZE/sizeof(int));
      BufRead(blkNum, (char *) buf);
    }
    int indexNum = nextBlkNum%(BLOCK_SIZE/sizeof(int));
    if(*(buf+indexNum) == 0) break;

    nextBlkNum = *(buf+indexNum);
    *(buf+indexNum) = 0;
    count++;

    BufWrite(blkNum, (char *) buf);

    if(nextBlkNum == -1) break;
  }

  free(buf);
  return count;
}

int FatGetFreeEntryNum() {
  int *buf = (int *) malloc(BLOCK_SIZE);

  int freeEntry;
  int dataStartBlock = DATA_START_BLOCK;
  int blkNum = dataStartBlock/(BLOCK_SIZE/sizeof(int));
  BufRead(blkNum, (char *) buf);

  for(freeEntry = dataStartBlock; freeEntry < DATA_START_BLOCK + (FAT_BLOCKS-3)*(BLOCK_SIZE/sizeof(int)) - 2; freeEntry++) {
    if(blkNum != freeEntry/(BLOCK_SIZE/sizeof(int))) {
      blkNum = freeEntry/(BLOCK_SIZE/sizeof(int));
      BufRead(blkNum, (char *) buf);
    }
    int indexNum = freeEntry%(BLOCK_SIZE/sizeof(int));
    if(*(buf+indexNum) == 0) break;
  }

  free(buf);
  if(freeEntry == DATA_START_BLOCK + NUM_FAT_BLOCK) return -1;
  return freeEntry;
}

int FatGetNumOfFreeEntries() {
  int *buf = (int *) malloc(BLOCK_SIZE);

  int freeEntry, count = 0;
  int dataStartBlock = DATA_START_BLOCK;
  int blkNum = dataStartBlock/(BLOCK_SIZE/sizeof(int));
  BufRead(blkNum, (char *) buf);

  for(freeEntry = dataStartBlock; freeEntry < DATA_START_BLOCK + (FAT_BLOCKS-3)*(BLOCK_SIZE/sizeof(int)) - 2; freeEntry++) {
    if(blkNum != freeEntry/(BLOCK_SIZE/sizeof(int))) {
      blkNum = freeEntry/(BLOCK_SIZE/sizeof(int));
      BufRead(blkNum, (char *) buf);
    }
    int indexNum = freeEntry%(BLOCK_SIZE/sizeof(int));
    if(*(buf+indexNum) == 0) count++;
  }

  free(buf);
  return count;
}
