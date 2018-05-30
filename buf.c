#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "queue.h"
#include "Disk.h"

void BufInit(void)
{
  TAILQ_INIT(&pBufList);
  int i;
  for(i=0; i<MAX_BUFLIST_NUM; i++) {
    TAILQ_INIT(ppStateListHead+i);
  }
  TAILQ_INIT(&pLruListHead);
}

void BufRead(int blkno, char* pData)
{
  int i;
  Buf *buf, *next_buf;
  for(i=0, buf=TAILQ_FIRST(&pBufList); i<MAX_BUF_NUM && buf!=NULL; i++, buf=next_buf) {
  	next_buf=TAILQ_NEXT(buf, blist);
    if (buf->blkno == blkno) {
      memcpy(pData, buf->pMem, BLOCK_SIZE);
      TAILQ_REMOVE(&pLruListHead, buf, llist);
      TAILQ_INSERT_TAIL(&pLruListHead, buf, llist);
      return;
    }
  }
  if(i == MAX_BUF_NUM) {
    Buf *victim = TAILQ_FIRST(&pLruListHead);
    TAILQ_REMOVE(&pLruListHead, victim, llist);
    TAILQ_REMOVE(&pBufList, victim, blist);
    if(victim->state == BUF_STATE_CLEAN) {
      TAILQ_REMOVE(&ppStateListHead[BUF_LIST_CLEAN], victim, slist);
    }
    else if(victim->state == BUF_STATE_DIRTY) {
      TAILQ_REMOVE(&ppStateListHead[BUF_LIST_DIRTY], victim, slist);
      DevOpenDisk();
      DevWriteBlock(victim->blkno, (char *) victim->pMem);
    }
    free(victim);
  }
  buf = malloc(sizeof(Buf));
  buf->blkno = blkno;
  buf->state = BUF_STATE_CLEAN;
  buf->pMem = malloc(BLOCK_SIZE);
  DevOpenDisk();
  DevReadBlock(blkno, (char *) buf->pMem);
  TAILQ_INSERT_HEAD(&pBufList, buf, blist);
  TAILQ_INSERT_TAIL(&ppStateListHead[BUF_LIST_CLEAN], buf, slist);
  TAILQ_INSERT_TAIL(&pLruListHead, buf, llist);
  memcpy(pData, buf->pMem, BLOCK_SIZE);
}

void BufWrite(int blkno, char* pData)
{
  int i;
  Buf *buf, *next_buf;
  for(i=0, buf=TAILQ_FIRST(&pBufList); i<MAX_BUF_NUM && buf!=NULL; i++, buf=next_buf) {
  	next_buf=TAILQ_NEXT(buf, blist);
    if (buf->blkno == blkno) {
      memcpy(buf->pMem, pData, BLOCK_SIZE);
      if (buf->state == BUF_STATE_CLEAN) {
        buf->state = BUF_STATE_DIRTY;
        TAILQ_REMOVE(&ppStateListHead[BUF_LIST_CLEAN], buf, slist);
        TAILQ_INSERT_TAIL(&ppStateListHead[BUF_LIST_DIRTY], buf, slist);
        TAILQ_REMOVE(&pLruListHead, buf, llist);
        TAILQ_INSERT_TAIL(&pLruListHead, buf, llist);
      }
      return;
    }
  }
  if(i == MAX_BUF_NUM) {
    Buf *victim = TAILQ_FIRST(&pLruListHead);
    TAILQ_REMOVE(&pLruListHead, victim, llist);
    TAILQ_REMOVE(&pBufList, victim, blist);
    if(victim->state == BUF_STATE_CLEAN) {
      TAILQ_REMOVE(&ppStateListHead[BUF_LIST_CLEAN], victim, slist);
    }
    else if(victim->state == BUF_STATE_DIRTY) {
      TAILQ_REMOVE(&ppStateListHead[BUF_LIST_DIRTY], victim, slist);
      DevOpenDisk();
      DevWriteBlock(victim->blkno, (char *) victim->pMem);
    }
    free(victim);
  }
  buf = malloc(sizeof(Buf));
  buf->blkno = blkno;
  buf->state = BUF_STATE_DIRTY;
  buf->pMem = malloc(BLOCK_SIZE);
  memcpy(buf->pMem, pData, BLOCK_SIZE);
  TAILQ_INSERT_HEAD(&pBufList, buf, blist);
  TAILQ_INSERT_TAIL(&ppStateListHead[BUF_LIST_DIRTY], buf, slist);
  TAILQ_INSERT_TAIL(&pLruListHead, buf, llist);
}

void BufSync(void)
{
  Buf *buf, *next_buf;
  for(buf=TAILQ_FIRST(&ppStateListHead[BUF_LIST_DIRTY]); buf!=NULL; buf=next_buf) {
  	next_buf=TAILQ_NEXT(buf, slist);
    DevOpenDisk();
    DevWriteBlock(buf->blkno, (char *) buf->pMem);
    buf->state = BUF_STATE_CLEAN;
    TAILQ_REMOVE(&ppStateListHead[BUF_LIST_DIRTY], buf, slist);
    TAILQ_INSERT_TAIL(&ppStateListHead[BUF_LIST_CLEAN], buf, slist);
  }
}

/*
 * GetBufInfoByListNum: Get all buffers in a list specified by listnum.
 *                      This function receives a memory pointer to "ppBufInfo" that can contain the buffers.
 */
void GetBufInfoByListNum(StateList listnum, Buf** ppBufInfo, int* pNumBuf)
{
  Buf *buf;
  *pNumBuf = 0;
  TAILQ_FOREACH(buf, &ppStateListHead[listnum], slist) {
    ppBufInfo[*pNumBuf] = buf;
    (*pNumBuf)++;
  }
}

/*
 * GetBufInfoInLruList: Get all buffers in a list specified at the LRU list.
 *                         This function receives a memory pointer to "ppBufInfo" that can contain the buffers.
 */
void GetBufInfoInLruList(Buf** ppBufInfo, int* pNumBuf)
{
  Buf *buf;
  *pNumBuf = 0;
  TAILQ_FOREACH(buf, &pLruListHead, llist) {
    ppBufInfo[*pNumBuf] = buf;
    (*pNumBuf)++;
  }
}

/*
 * GetBufInfoInBufferList: Get all buffers in the buffer list.
 *                         This function receives a memory pointer to "ppBufInfo" that can contain the buffers.
 */
void GetBufInfoInBufferList(Buf** ppBufInfo, int* pNumBuf)
{
  Buf *buf;
  *pNumBuf = 0;
  TAILQ_FOREACH(buf, &pBufList, blist) {
    ppBufInfo[*pNumBuf] = buf;
    (*pNumBuf)++;
  }
}
