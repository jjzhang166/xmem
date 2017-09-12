#include "xconfig.h"
#include "platform.h"
#include "xtypes.h"

#define XMEM_VER "1.0.0"

/***************************************************************************
                         X-Memory Physical Sketch
----------------------------------------------------------------------------
|                                  Pool                                    |
|                                                                          |
|    ------------------    --------------------    --------------------    |
|    |  SuperBlock    |    |       Block      |    |       Block      |    |
|    |                |    |                  |    |                  |    |
|    |  4-bytes list  |    |  xx-bytes block  |    |  xxx-bytes block |    |
|    |                |    |                  |    |                  |    |
|    ------------------    --------------------    --------------------    |
|                                                                          |
|    ------------------    --------------------    --------------------    |
|    |  SuperBlock    |    |       Block      |    |       Block      |    |
|    |                |    |                  |    |                  |    |
|    |  8-bytes list  |    | xxxx-bytes block |    | xxxxx-bytes block|    |
|    |                |    |                  |    |                  |    |
|    ------------------    --------------------    --------------------    |
|                                                                          |
|    ------------------                                                    |
|    |  SuperBlock    |                                                    |
|    |                |             ...                     ...            |
|    |  16-bytes list |                                                    |
|    |                |                                                    |
|    ------------------                                                    |
|                                                                          |
----------------------------------------------------------------------------
****************************************************************************/


static pxMemMgrHdr xMemMgrHdrList;
static pxMemBlock xMemBlkList = NULL;
static u32 xMemMgrHdrListEnd=0;
static u32 xMemBlkPoolStart=0;

#if defined(__MT7681)
extern unsigned long _RAM_SIZE;
extern unsigned long _BSS_END;
extern u32 __OS_Heap_Start;

#define XMEM_POOL_START ((u32)&_BSS_END)
#define XMEM_POOL_END ((u32)&_RAM_SIZE)
#else
static u8 xmempool[XMEM_POOL_SIZE] = {0};
#define XMEM_POOL_START ((u32)&xmempool[0])
#define XMEM_POOL_END (XMEM_POOL_START+XMEM_POOL_SIZE)
#endif


#if XMEM_HEADER_PROTECT_ENABLE
/***************************************************************************
 * FUNCTION
 * xMemMgrHdrListInit
 * DESCRIPTION
 * init Header List
 * PARAMETERS
 * void
 * RETURNS
 * void
 * *************************************************************************/
static void xMemMgrHdrListInit(void)
{
    /*
      -------------------------------------------------------------------------------
      | Header List|  xMemHdrEnd--->|  ...   |<---xMemBlkStart  |Blocks,Super Blocks|
      -------------------------------------------------------------------------------
      This Structure avoid Header List overwrote by pointer which allocated from X-Memory Pool
     */
    xMemMgrHdrList = NULL;
    xMemMgrHdrListEnd = XMEM_POOL_START;
}

/***************************************************************************
 * FUNCTION
 * xMemMgrHdrGet
 * DESCRIPTION
 * get a Header
 * PARAMETERS
 * type  [IN]    XMEM_LIST_TYPE_BLOCK or XMEM_LIST_TYPE_SUPERBLOCK
 * RETURNS
 * void
 * *************************************************************************/
static void * xMemMgrHdrGet(u8 type)
{
    pxMemMgrHdr hdr = NULL,hdrnew = NULL, hdrprev = NULL;
    u16 allocsize;

    /*
     -------------------------------------------
     |  ...  |  prev  |  blk  |  next  |  ...  |
     -------------------------------------------
    */

    hdr = xMemMgrHdrList;
    if(type == XMEM_LIST_TYPE_BLOCK)
        allocsize = XMEM_NODE_SIZE(xMemBlock);
    else if(type == XMEM_LIST_TYPE_SUPERBLOCK)
        allocsize = XMEM_NODE_SIZE(xMemSuperBlock);
    else
        return NULL;

    while(hdr)
    {
        if(hdr->type == XMEM_LIST_TYPE_FREE)
        {
           if(hdr->size == allocsize)
           {
               //most fitable, block size equals to required size
               hdr->type = type;
               return (void*)hdr+XMEM_HEADER_SIZE;
           }
           else if(hdr->size > allocsize)
           {
               //split into 2 hdrs
               hdrnew = (pxMemMgrHdr)((void*)hdr+allocsize+XMEM_HEADER_SIZE);
               hdrnew->type = XMEM_LIST_TYPE_FREE;
               hdrnew->size = hdr->size - allocsize - XMEM_HEADER_SIZE;
               hdrnew->next = hdr->next;
               hdr->next = hdrnew;
               hdr->size = allocsize;
               hdr->type = type;
               return (void*)hdr+XMEM_HEADER_SIZE;
           }
        }
        hdrprev = hdr;
        hdr = hdr->next;
    }

    if(xMemMgrHdrListEnd+allocsize+XMEM_HEADER_SIZE<xMemBlkPoolStart)
    {
        hdrnew = (pxMemMgrHdr)xMemMgrHdrListEnd;
        if(hdrprev)
        {
            hdrprev->next = hdrnew;
        }
        else
        {
            xMemMgrHdrList = hdrnew;
        }

        hdrnew->type = type;
        hdrnew->size = allocsize;
        hdrnew->next = NULL;
        xMemMgrHdrListEnd += allocsize+XMEM_HEADER_SIZE;
        hdr = hdrnew;

        return (void*)hdr+XMEM_HEADER_SIZE;
    }

    return NULL;
}

/***************************************************************************
 * FUNCTION
 * xMemMgrHdrPut
 * DESCRIPTION
 * put a Header
 * PARAMETERS
 * header  [IN]    the header be put
 * RETURNS
 * void
 * *************************************************************************/
static void xMemMgrHdrPut(void * header)
{
    pxMemMgrHdr hdrfree=NULL,hdrprev=NULL,hdrprev2=NULL;
    u16 headersize;

    /*
     -------------------------------------------
     |  ...  |  prev  |  blk  |  next  |  ...  |
     -------------------------------------------
    */

    hdrfree = xMemMgrHdrList;
    while(hdrfree)
    {
        if((u32)hdrfree+XMEM_HEADER_SIZE == header)
        {
            xMemAssert(hdrfree->type!=XMEM_LIST_TYPE_FREE);

            if(hdrfree->type == XMEM_LIST_TYPE_BLOCK)
                headersize = XMEM_NODE_SIZE(xMemBlock);
            else if(hdrfree->type == XMEM_LIST_TYPE_SUPERBLOCK)
                headersize = XMEM_NODE_SIZE(xMemSuperBlock);

            hdrfree->type = XMEM_LIST_TYPE_FREE;
            if(hdrprev&&hdrprev->type == XMEM_LIST_TYPE_FREE)
            {
                hdrprev->next = hdrfree->next;
                hdrprev->size += headersize+XMEM_HEADER_SIZE;
                hdrfree = hdrprev;
                hdrprev = hdrprev2;
            }
            if(hdrfree->next&&hdrfree->next->type == XMEM_LIST_TYPE_FREE)
            {
                hdrfree->size += hdrfree->next->size + XMEM_HEADER_SIZE;
                hdrfree->next = hdrfree->next->next;
            }

            if((u32)hdrfree + hdrfree->size+XMEM_HEADER_SIZE==xMemMgrHdrListEnd)
            {
                xMemMgrHdrListEnd -= hdrfree->size+XMEM_HEADER_SIZE;
                if(hdrprev) hdrprev->next = NULL;
                else xMemMgrHdrList = NULL;
            }

            return;
        }

        hdrprev2 = hdrprev;
        hdrprev = hdrfree;
        hdrfree = hdrfree->next;
    }

    xMemAssert(0);
}

static const char xMemDumpMsgMgrHdrLst[]="-----xMemMgrHdrLst Info-----\n";
static const char xMemDumpFmtMgrHdrLst[]="hdr:%u,hdrsize:%d,hdrnext:%u,type:%d\n";

/***************************************************************************
 * FUNCTION
 * xMemMgrHdrListInfoDump
 * DESCRIPTION
 * Dump Header List information for debug or test
 * PARAMETERS
 * void
 * RETURNS
 * void
 * *************************************************************************/
static void xMemMgrHdrListInfoDump(void)
{
    pxMemMgrHdr phdrlst;

    phdrlst=xMemMgrHdrList;
    xMemPrintf(xMemDumpMsgMgrHdrLst);

    while(phdrlst)
    {
        xMemPrintf(xMemDumpFmtMgrHdrLst,(u32)phdrlst,phdrlst->size,(u32)phdrlst->next,phdrlst->type);
        phdrlst=phdrlst->next;
    }
    xMemPrintf(xMemDumpMsgMgrHdrLst);
    return;
}
#endif

/***************************************************************************
 * FUNCTION
 * xMemBlockListInit
 * DESCRIPTION
 * Init Block List
 * PARAMETERS
 * void
 * RETURNS
 * void
 * *************************************************************************/
static void xMemBlockListInit(void)
{
    #if XMEM_HEADER_PROTECT_ENABLE
    xMemBlkPoolStart = XMEM_POOL_END;
    xMemBlkList = NULL;
    #else

    xMemBlkList = (pxMemBlock)XMEM_POOL_START;
    xMemBlkList->blksize=XMEM_POOL_SIZE-XMEM_BLOCK_SIZE;
    xMemBlkList->next=NULL;
    xMemBlkList->free=1;
    #endif
}


/***************************************************************************
 * FUNCTION
 * xMemBlockListCheck
 * DESCRIPTION
 * check memory block boundry
 * PARAMETERS
 * void
 * RETURNS
 * void
 * *************************************************************************/
#if XMEM_BOUNDRY_CHECK_ENABLE
const char xMemDumpFmtBlockList[]="blk:%u,blksize:%d,blknext:%u,free:%d\n";
void dump(unsigned char * mem,size_t size)
{
    int i;

    for(i=0;i<size;i++)
    {
        printf("%02x",mem[i]);
        if((i%4)==3) printf(" ");
        if((i%16)==15) printf("\n");
    }
    if((i%16)!=0) printf("\n");
}

void xMemBlockListCheck()
{
    pxMemBlock pmemblk,preblk;
    preblk=pmemblk=xMemBlkList;

    while(pmemblk)
    {
        if(pmemblk->free>1
                ||(pmemblk->next&&
                   ((u32)pmemblk+XMEM_BLOCK_SIZE+pmemblk->blksize!=(u32)pmemblk->next
                   ||pmemblk->next>=XMEM_POOL_END
                   ||pmemblk->next<=XMEM_POOL_START)
                   )
           )
        {
            xMemPrintf(xMemDumpFmtBlockList,(u32)pmemblk,pmemblk->blksize,(u32)pmemblk->next,pmemblk->free);
            dump(preblk,preblk->blksize+XMEM_BLOCK_SIZE);
            dump(pmemblk,128);
            xMemAssert(0);
        }
        preblk=pmemblk;
        pmemblk=pmemblk->next;
    }

    return;
}
#endif

/***************************************************************************
 * FUNCTION
 * xMemBlockAlloc
 * DESCRIPTION
 * allocate a memory bock
 * PARAMETERS
 * size  [IN] block size that required
 * RETURNS
 * void * memory block address that alocated
 * *************************************************************************/
#if XMEM_BOUNDRY_CHECK_ENABLE

static void * xMemBlockAlloc(size_t size)
{
    pxMemBlock blkprev=NULL,blk=NULL,blknew=NULL,blkalloc=NULL;
    s16 allocsize,remainsize;

    /*
     --------------------------------------------------------------
     |       |     prev     |     blk      |    next      |       |
     |  ...  |--------------|--------------|--------------|  ...  |
     |       |header|  mem  |header|  mem  |header|  mem  |       |
     --------------------------------------------------------------
    */

    blk=xMemBlkList;
    remainsize=XMEM_POOL_SIZE;
    blkalloc=NULL;
    allocsize=size+((size%4)==0?0:(4-size%4));

    while(blk)
    {
        if(blk->free)
        {
           if(blk->blksize==allocsize)
           {//most fitable, block size equals to required size
               blk->free=0;
               return (void*)blk+XMEM_BLOCK_SIZE;
           }
           else if(blk->blksize>allocsize&&(blk->blksize-allocsize)<remainsize)
           {// find minimal fitable size block
                remainsize=blk->blksize-allocsize;
                blkalloc=blk;
           }
        }
        blkprev=blk;
        blk=blk->next;
    }

    if(blkalloc)
    {
        if(remainsize>(XMEM_BLOCK_SIZE+XMEM_BALLANCE_SIZE))
        {
            //split into 2 blocks
            blknew=(pxMemBlock)((void *)blkalloc+allocsize+XMEM_BLOCK_SIZE);
            blknew->blksize=remainsize-XMEM_BLOCK_SIZE;
            blknew->free=1;
            blknew->next=blkalloc->next;
            blkalloc->next=blknew;
            blkalloc->blksize=allocsize;
        }

        blkalloc->free=0;
        return (void*)blkalloc+XMEM_BLOCK_SIZE;
    }

    xMemBlockListCheck();
    return NULL;
}
#else
static void * xMemBlockAlloc(size_t size)
{
    pxMemBlock blkprev=NULL,blk=NULL,blknew=NULL,blkalloc=NULL;
    u16 allocsize,remainsize;

    /*
     -------------------------------------------
     |  ...  |  next  |  blk  |  prev  |  ...  |
     -------------------------------------------
    */

    blk = xMemBlkList;
    blkprev = xMemBlkList;
    remainsize = xMemBlkPoolStart-xMemMgrHdrListEnd;
    blkalloc=NULL;
    allocsize=size+((size%4)==0?0:(4-size%4));

    while(blk)
    {
        if(blk->free)
        {
           if(blk->blksize==allocsize)
           {//most fitable, block size equals to required size
               blk->free=0;
               return (void*)blk->addr;
           }
           else if(blk->blksize>allocsize&&(blk->blksize-allocsize)<remainsize)
           {// find minimal fitable size block 
                remainsize=blk->blksize-allocsize;
                blkalloc=blk;
            }
        }
        blkprev=blk;
        blk=blk->next;    
    }

    if(blkalloc)
    {
        if(remainsize>=XMEM_BALLANCE_SIZE)
        {
            //split into 2 blocks
            blknew=(pxMemBlock)xMemMgrHdrGet(XMEM_LIST_TYPE_BLOCK);
            if(blknew != NULL)
            {
                blknew->blksize=remainsize;
                blknew->free=1;
                blknew->next=blkalloc->next;
                blknew->addr=(void*)blkalloc->addr+allocsize;
                blkalloc->next=blknew;
                blkalloc->blksize=allocsize;
            }
        }
        
        blkalloc->free=0;
        return (void*)blkalloc->addr;
    }
    else if(remainsize>allocsize)
    {
        blknew=(pxMemBlock)xMemMgrHdrGet(XMEM_LIST_TYPE_BLOCK);
        if(blknew != NULL)
        {
            if(blkprev)
            {
                blkprev->next = blknew;
            }
            else
            {
                xMemBlkList = blknew;
            }

            blknew->next = NULL;
            blknew->blksize = allocsize;
            blknew->free = 0;
            xMemBlkPoolStart -= allocsize;
            blknew->addr = xMemBlkPoolStart;
            blkalloc = blknew;
            return (void*)blkalloc->addr;
        }
    }

    return NULL;
}

#endif
/***************************************************************************
 * FUNCTION
 * xMemBlockFree
 * DESCRIPTION
 * free a memory bock
 * PARAMETERS
 * ptr  [IN] block address that be free
 * RETURNS
 * u8 0-success,1-failure
 * *************************************************************************/
#if XMEM_BOUNDRY_CHECK_ENABLE
static u8 xMemBlockFree(void *ptr){
    pxMemBlock blkprev=NULL,blkfree=NULL;

    /*
     --------------------------------------------------------------
     |       |     prev     |     blk      |    next      |       |
     |  ...  |--------------|--------------|--------------|  ...  |
     |       |header|  mem  |header|  mem  |header|  mem  |       |
     --------------------------------------------------------------
    */

    blkfree = xMemBlkList;
    while(blkfree)
    {
        if((u32)blkfree+XMEM_BLOCK_SIZE==ptr)
        {
            blkfree->free=1;
            //merge physical neighbor blocks, previous or next, assure block will not overlap reserve space
            if(blkprev&&blkprev->free)
            {
                blkprev->blksize += (blkfree->blksize+XMEM_BLOCK_SIZE);
                blkprev->next = blkfree->next;
                blkfree = blkprev;
            }

            if(blkfree->next&&blkfree->next->free)
            {
                blkfree->blksize += (blkfree->next->blksize+XMEM_BLOCK_SIZE);
                blkfree->next = blkfree->next->next;
            }

            return 0;
        }

        blkprev = blkfree;
        blkfree = blkfree->next;
    }

    return 1;
}
#else
static u8 xMemBlockFree(void *ptr)
{
    pxMemBlock blkprev=NULL,blkfree=NULL,blkprev2=NULL;

    /*
     -------------------------------------------
     |  ...  |  next  |  blk  |  prev  |  ...  |
     -------------------------------------------
    */

    blkfree=xMemBlkList;
    while(blkfree)
    {
        if(blkfree->addr==ptr)
        {
            blkfree->free = 1;
            //may move this block of code to memory collection
            if(blkprev&&blkprev->free)
            {
                blkprev->blksize += blkfree->blksize;
                blkprev->next = blkfree->next;
                blkprev->addr = blkfree->addr;
                xMemMgrHdrPut(blkfree);
                blkfree = blkprev;
                blkprev = blkprev2;
            }

            if(blkfree->addr==xMemBlkPoolStart)
            {
                xMemBlkPoolStart += blkfree->blksize;
                if(blkprev)
                {
                    blkprev->next = blkfree->next;
                }
                else
                {
                    blkfree->addr = NULL;
                    blkfree->blksize = 0;
                    blkfree->next = NULL;
                }
                xMemMgrHdrPut(blkfree);
            }
            else  if(blkfree->next&&blkfree->next->free)
            {
                blkprev = blkfree;
                blkfree = blkfree->next;

                blkprev->blksize += blkfree->blksize;
                blkprev->addr = blkfree->addr;
                blkprev->next = blkfree->next;
                xMemMgrHdrPut(blkfree);
            }
            //end
            return 0;
        }
        blkprev2 = blkprev;
        blkprev = blkfree;
        blkfree = blkfree->next;
    }

    return 1;
}


#endif
static const char xMemDumpMsgBlock[]="-----Block Info-----\n";
static const char xMemDumpFmtBlock[]="blk:%u,blksize:%d,blknext:%u,free:%d\n";

/***************************************************************************
 * FUNCTION
 * xMemBLockListInfoDump
 * DESCRIPTION
 * Dump block lists information
 * PARAMETERS
 * void
 * RETURNS
 * void
 * *************************************************************************/
static void xMemBlockListInfoDump(void)
{
    pxMemBlock pmemblk;

    pmemblk=xMemBlkList;
    xMemPrintf(xMemDumpMsgBlock);

    while(pmemblk)
    {
        #if XMEM_HEADER_PROTECT_ENABLE
        xMemPrintf(xMemDumpFmtBlock,(u32)pmemblk->addr,pmemblk->blksize,(u32)pmemblk->next,pmemblk->free);
        #else
        xMemPrintf(xMemDumpFmtBlock,(u32)pmemblk,pmemblk->blksize,(u32)pmemblk->next,pmemblk->free);
        #endif
        pmemblk=pmemblk->next;
    }
    xMemPrintf(xMemDumpMsgBlock);
    return;
}


#if XMEM_SUPERBLOCK_ENABLE
static xMemSuperBlock xMemSuperBlockList[XMEM_SUPERBLOCK_LIST_COUNT]={0};

static const char xMemSuperBlkInitFailureFmt[]="Init Super Block [Failed], super block:%u, address:%u, blocks:%d,block size:%d\n";
/***************************************************************************
 * FUNCTION
 * xMemSuperBlockInit
 * DESCRIPTION
 * Init super block
 * PARAMETERS
 * psuperblock  [IN/OUT]    super block be initial
 * addr         [IN]    meta blocks address
 * nblks        [IN]    blocks count
 * blksize      [IN]    block size
 * RETURNS
 * void
 * *************************************************************************/
static void xMemSuperBlockInit (xMemSuperBlock * psuperblock,void *addr, u8 nblks, u16 blksize)
{  
    xMemAssert(nblks<=XMEM_SUPERBLOCK_BLKS_MAX);
 
    if (addr == NULL||nblks < 2||blksize < sizeof(void *)||psuperblock == NULL)
    {
        XMEM_LOG(xMemSuperBlkInitFailureFmt,(u32)psuperblock,(u32)addr,nblks,blksize);
        return ;
    }
    psuperblock->addr     = addr;
    psuperblock->freeList = (nblks==XMEM_SUPERBLOCK_BLKS_MAX)?0xFFFFFFFF:((1<<nblks)-1);
    psuperblock->nfree    = nblks;
    psuperblock->nblk    = nblks;
    psuperblock->blksize  = blksize;
    psuperblock->next = NULL;

    return ;
}

/***************************************************************************
 * FUNCTION
 * xMemSuperBlockAppend
 * DESCRIPTION
 * Append a new super block
 * PARAMETERS
 * psuperblock  [IN/OUT] super block list
 * RETURNS
 * xMemSuperBlock *  new super block
 * *************************************************************************/
static xMemSuperBlock * xMemSuperBlockAppend(xMemSuperBlock * superblocklist)
{
    xMemSuperBlock * pmemtail,*pmemnew=NULL;
    void *blk;

    pmemtail=superblocklist;
    while(pmemtail->next) pmemtail=pmemtail->next;
    #if XMEM_BOUNDRY_CHECK_ENABLE
    pmemnew=(xMemSuperBlock *)xMemBlockAlloc(XMEM_LIST_TYPE_SUPERBLOCK);
    #else
    pmemnew=(xMemSuperBlock *)xMemMgrHdrGet(XMEM_LIST_TYPE_SUPERBLOCK);
    #endif
    if(pmemnew)
    {
        blk=xMemBlockAlloc(superblocklist->blksize*(superblocklist->nblk/2));
        if(blk)
        {
            xMemSuperBlockInit(pmemnew,blk,(superblocklist->nblk/2),superblocklist->blksize);
            pmemtail->next=pmemnew;
        }else
        {
            #if XMEM_BOUNDRY_CHECK_ENABLE
            xMemBlockFree(pmemnew);
            #else
            xMemMgrHdrPut(pmemnew);
            #endif
            pmemnew=NULL;
        }
    }
    return pmemnew;
}

/***************************************************************************
 * FUNCTION
 * xMemSuperBlockListInit
 * DESCRIPTION
 * Init super block list
 * PARAMETERS
 * void
 * RETURNS
 * void
 * *************************************************************************/
static void xMemSuperBlockListInit(void)
{
    void * blk;
    #if XMEM_8META_ENABLE
    blk=xMemBlockAlloc(XMEM_SUPERBLOCK_1META_MIN_SIZE+XMEM_SUPERBLOCK_2META_MIN_SIZE+XMEM_SUPERBLOCK_4META_MIN_SIZE+XMEM_SUPERBLOCK_8META_MIN_SIZE);
    #else
    blk=xMemBlockAlloc(XMEM_SUPERBLOCK_1META_MIN_SIZE+XMEM_SUPERBLOCK_2META_MIN_SIZE+XMEM_SUPERBLOCK_4META_MIN_SIZE);
    #endif
    if(blk)
    {
        xMemSuperBlockInit(&xMemSuperBlockList[0],
                blk,
                XMEM_SUPERBLOCK_1META_CNT,
                XMEM_META_BLOCK_SIZE);
        xMemSuperBlockInit(&xMemSuperBlockList[1],
                blk+XMEM_SUPERBLOCK_1META_MIN_SIZE,
                XMEM_SUPERBLOCK_2META_CNT,
                XMEM_2META_BLOCK_SIZE);
        xMemSuperBlockInit(&xMemSuperBlockList[2],
                blk+XMEM_SUPERBLOCK_1META_MIN_SIZE+XMEM_SUPERBLOCK_2META_MIN_SIZE,
                XMEM_SUPERBLOCK_4META_CNT,
                XMEM_4META_BLOCK_SIZE);
        #if XMEM_8META_ENABLE
        xMemSuperBlockInit(&xMemSuperBlockList[3],
                blk+XMEM_SUPERBLOCK_1META_MIN_SIZE+XMEM_SUPERBLOCK_2META_MIN_SIZE+XMEM_SUPERBLOCK_4META_MIN_SIZE,
                XMEM_SUPERBLOCK_8META_CNT,
                XMEM_8META_BLOCK_SIZE);
        #endif
    }
}

static const char xMemDumpMsgSuperBblock[]="-----SuperBlock Info-----\n";
static const char xMemDumpFmtSuperBlock[]="size:%d,free:%d,total:%d\n";

/***************************************************************************
 * FUNCTION
 * xMemSuperBlockInfoDump
 * DESCRIPTION
 * Dump super block lists information
 * PARAMETERS
 * void
 * RETURNS
 * void
 * *************************************************************************/
static void xMemSuperBlockInfoDump(void)
{
    u8 i;

    xMemSuperBlock * psuperblock;
    xMemPrintf(xMemDumpMsgSuperBblock);
    for(i=0;i<XMEM_SUPERBLOCK_LIST_COUNT;i++)
    {
        psuperblock=&xMemSuperBlockList[i];
        while(psuperblock)
        {
            xMemPrintf(xMemDumpFmtSuperBlock,psuperblock->blksize,psuperblock->nfree,psuperblock->nblk);
            psuperblock=psuperblock->next;
        }
    }
    xMemPrintf(xMemDumpMsgSuperBblock);
}

/***************************************************************************
 * FUNCTION
 * xMallocMetaBlockGet
 * DESCRIPTION
 * Get a meta block
 * PARAMETERS
 * superblocklist   [IN/OUT]    super block list
 * size             [IN]    Meta block size required
 * RETURNS
 * void * meta block
 * *************************************************************************/
static void * xMallocMetaBlockGet(xMemSuperBlock * superblocklist,size_t size)
{
    u32 i;
    void      *pblk;
    xMemSuperBlock *pmemiter;
    
    if (superblocklist == NULL||size>superblocklist->blksize)    return NULL;

    pmemiter=superblocklist;

    while(pmemiter)
    {
        if (pmemiter->nfree > 0)
        {
            break;
        }
        
        pmemiter=pmemiter->next;
    }
    
    if(pmemiter==NULL)pmemiter=xMemSuperBlockAppend(superblocklist);

    if(pmemiter)
    {
        for(i=0;i<pmemiter->nblk;i++)
        {
           if((1<<i)&(pmemiter->freeList))
           {
              pmemiter->freeList&=(~(1<<i));
              break;
           }
        }
        xMemAssert(i<pmemiter->nblk);
        pblk = (void *)(pmemiter->addr+i*(u32)(pmemiter->blksize));
        //xMemPrintf("get %u\n",pblk);
        pmemiter->nfree--;        
        return (pblk);
    }
                
    return NULL;   

}

/***************************************************************************
 * FUNCTION
 * xMallocMetaBlockPut
 * DESCRIPTION
 * put a meta block
 * PARAMETERS
 * superblocklist   [IN/OUT]    super block list
 * pblk             [IN]    meta block be put
 * RETURNS
 * void
 * *************************************************************************/
static void  xMallocMetaBlockPut (xMemSuperBlock  *superblocklist, void *pblk)
{
    u8 i;

    if (superblocklist == NULL||pblk == NULL||superblocklist->nfree >= superblocklist->nblk)  return ;

    i=(u32)(pblk-superblocklist->addr)/superblocklist->blksize;
    superblocklist->freeList|=(1<<i);
    superblocklist->nfree++;
    return;
}

/***************************************************************************
 * FUNCTION
 * xMallocMetaBlockAlloc
 * DESCRIPTION
 * Allocate a meta block
 * PARAMETERS
 * size     [IN]    meta block size that required
 * RETURNS
 * void * meta block
 * *************************************************************************/
static void * xMallocMetaBlockAlloc(size_t size)
{
    void * ptr;

    #if XMEM_8META_ENABLE
    if(size>XMEM_4META_BLOCK_SIZE)
    {
        ptr=xMallocMetaBlockGet(&xMemSuperBlockList[3],size);
    }
    else
    #endif
    if(size>XMEM_2META_BLOCK_SIZE)
    {
        ptr=xMallocMetaBlockGet(&xMemSuperBlockList[2],size);
    }
    else if(size>XMEM_META_BLOCK_SIZE)
    {
        ptr=xMallocMetaBlockGet(&xMemSuperBlockList[1],size);
    }
    else if(size>0)
    {
        ptr=xMallocMetaBlockGet(&xMemSuperBlockList[0],size);
    }
    else
    {
        return NULL;
    }

    if(ptr==NULL)
    {
        xMemSuperBlockInfoDump();
    }

    return ptr;
}

/***************************************************************************
 * FUNCTION
 * xMemMetaBlockFree
 * DESCRIPTION
 * free a meta block
 * PARAMETERS
 * pblk     [IN] meta block be free
 * RETURNS
 * u8 0-success, 1-failure
 * *************************************************************************/
static u8 xMemMetaBlockFree(void * pblk)
{
    int i;
    u32 start,end,p;
    xMemSuperBlock  *pmem,*pmemprev=NULL;

    if (pblk == NULL)   return 0;

    for(i=0;i<XMEM_SUPERBLOCK_LIST_COUNT;i++)
    {
        pmem=&xMemSuperBlockList[i];
        while(pmem)
        {
            p=(u32)pblk;
            start=(u32)pmem->addr;
            end=start+pmem->blksize*pmem->nblk;
            if(p>=start&&p<end)
            {
                xMallocMetaBlockPut(pmem,pblk);
                if(pmem->nfree==pmem->nblk&&pmemprev)
                {
                    pmemprev->next=pmem->next;
                    xMemBlockFree(pmem->addr);
                    xMemBlockFree(pmem);
                }
                return 0;
            }
            pmemprev=pmem;
            pmem=pmem->next;        
        }
    }
    return 1;
}
#endif
static u8 xmem_init_flag=0;

/***************************************************************************
 * FUNCTION
 * xMemInit
 * DESCRIPTION
 * Init X-Memory Pool
 * PARAMETERS
 * void
 * RETURNS
 * void
 * *************************************************************************/
void xMemInit(void)
{
    xMemPrintf("xMem Version: %s\n",XMEM_VER);
    xMemAssert(XMEM_POOL_END-XMEM_POOL_START>=XMEM_POOL_SIZE);
    #if defined(__MT7681)
    __OS_Heap_Start += XMEM_POOL_SIZE;//reserve space for other using
    #endif

    #if XMEM_HEADER_PROTECT_ENABLE
    xMemMgrHdrListInit();
    #endif

    xMemBlockListInit();

    #if XMEM_SUPERBLOCK_ENABLE
    xMemSuperBlockListInit();
    #endif

    xmem_init_flag=1;    
    return;
}

/***************************************************************************
 * FUNCTION
 * xmalloc
 * DESCRIPTION
 * allocate a memory block
 * PARAMETERS
 * size     [IN]    block size that required
 * RETURNS
 * void * memory block address
 * *************************************************************************/
void * xmalloc(size_t size)
{
    SYS_ENTER_CRITICAL_SECTION;

    if(!xmem_init_flag){
        xMemInit();
    }

    #if XMEM_BOUNDRY_CHECK_ENABLE
    xMemBlockListCheck();
    #endif

    #if XMEM_SUPERBLOCK_ENABLE
    #if XMEM_8META_ENABLE
    if(size<=XMEM_8META_BLOCK_SIZE)
    #else
    if(size<=XMEM_4META_BLOCK_SIZE)
    #endif
    {
        return xMallocMetaBlockAlloc(size);
    }else{
    #endif
        return xMemBlockAlloc(size);
    #if XMEM_SUPERBLOCK_ENABLE
    }
    #endif

    SYS_EXIT_CRITICAL_SECTION;
}

/***************************************************************************
 * FUNCTION
 * xfree
 * DESCRIPTION
 * free a memory block
 * PARAMETERS
 * void *       [IN]    memory block pointer
 * RETURNS
 * void
 * *************************************************************************/
void xfree(void *ptr)
{
    SYS_ENTER_CRITICAL_SECTION;

    #if XMEM_BOUNDRY_CHECK_ENABLE
    xMemBlockListCheck();
    #endif

    if(
        #if XMEM_SUPERBLOCK_ENABLE
        xMemMetaBlockFree(ptr)&&
        #endif
        xMemBlockFree(ptr))
    {//Meta block first, then common block, avoid super block start addr equals common block start addr

        xMemPrintf("prt:%u\n",(u32)ptr);
        xMemSuperBlockInfoDump();
    }

    SYS_EXIT_CRITICAL_SECTION;
    return;
}

/***************************************************************************
 * FUNCTION
 * xMemInfoDump
 * DESCRIPTION
 * Dump X-Memory Information, Header List information, Block List
 * information, Super Block List information
 * PARAMETERS
 * void
 * RETURNS
 * void
 * *************************************************************************/
void xMemInfoDump(void)
{
    #if XMEM_HEADER_PROTECT_ENABLE
    xMemPrintf("hdr end:%u,blk start:%u\n",xMemMgrHdrListEnd,xMemBlkPoolStart);
    xMemMgrHdrListInfoDump();
    #endif

    xMemBlockListInfoDump();

    #if XMEM_SUPERBLOCK_ENABLE
    xMemSuperBlockInfoDump();
    #endif

    xMemPrintf("\n");
}
