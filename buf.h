#ifndef __BUF_H__

#include "queue.h"


#define MAX_BUFLIST_NUM (2)
#define MAX_BUF_NUM     (10)
#define BLKNO_INVALID   (-1)

typedef struct Buf Buf;

typedef enum __BufState
{
    BUF_STATE_CLEAN,
    BUF_STATE_DIRTY
} BufState;

typedef enum __BufList
{
    BUF_LIST_CLEAN = 0,
    BUF_LIST_DIRTY = 1
} StateList;

struct Buf
{
    int 		blkno;
    BufState		state;
    void*		pMem;
    TAILQ_ENTRY(Buf) 	blist;
    TAILQ_ENTRY(Buf) 	slist;
    TAILQ_ENTRY(Buf) 	llist;
};

TAILQ_HEAD(bufList, Buf) pBufList;

TAILQ_HEAD(stateList, Buf) ppStateListHead[MAX_BUFLIST_NUM];

TAILQ_HEAD(lrulList, Buf) pLruListHead;


extern void BufInit(void);
extern void BufRead(int blkno, char* pData);
extern void BufWrite(int blkno, char* pData);
extern void BufSync(void);
/*
 * GetBufInfoByListNum: Get all buffers in a list specified by listnum.
 *                      This function receives a memory pointer to "ppBufInfo" that can contain the buffers.
 */
extern void GetBufInfoByListNum(StateList listnum, Buf** ppBufInfo, int* pNumBuf);

/*
 * GetBufInfoInLruList: Get all buffers in a list specified at the LRU list.
 *                         This function receives a memory pointer to "ppBufInfo" that can contain the buffers.
 */
extern void GetBufInfoInLruList(Buf** ppBufInfo, int* pNumBuf);


/*
 * GetBufInfoInBufferList: Get all buffers in the buffer list.
 *                         This function receives a memory pointer to "ppBufInfo" that can contain the buffers.
 */
extern void GetBufInfoInBufferList(Buf** ppBufInfo, int* pNumBuf);


#endif /* BUF_H__ */


