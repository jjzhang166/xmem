#ifndef __XTYPES_H__
#define __XTYPES_H__

#include "xconfig.h"

typedef int s32;
typedef unsigned int u32;
typedef short s16;
typedef unsigned short u16;
typedef char s8;
typedef unsigned char u8;


#define XMEM_ATTR_PACKED __attribute((packed))
#define XMEM_ATTR_ALIGNED_4 __attribute((aligned(4)))


enum{
    XMEM_LIST_TYPE_FREE=0,
    XMEM_LIST_TYPE_BLOCK,
    XMEM_LIST_TYPE_SUPERBLOCK,
};

typedef struct XMEM_ATTR_PACKED XMEM_ATTR_ALIGNED_4  t_xMemManagementHeader{
    struct t_xMemManagementHeader * next;
    u8 reserve;
    u8 type;
    u16 size;
}xMemMgrHdr,*pxMemMgrHdr;

typedef struct XMEM_ATTR_PACKED XMEM_ATTR_ALIGNED_4 t_xMemBlock{
    struct t_xMemBlock * next;
    #if XMEM_HEADER_PROTECT_ENABLE == 0
    void * addr;
    #endif
    u32 blksize;
    u8 free;
    #if XMEM_HEADER_PROTECT_ENABLE == 0
    u8 reserve[3];
    #else
    u8 reserve;
    #endif
}xMemBlock,*pxMemBlock;

typedef struct XMEM_ATTR_PACKED XMEM_ATTR_ALIGNED_4 t_xMemSuperBlock{
    struct t_xMemSuperBlock *next;
    void * addr;
    u32 freeList;
    u8  nfree;
    u8  nblk;
    u16  blksize;
}xMemSuperBlock;


#define XMEM_HEADER_SIZE sizeof(xMemMgrHdr)
#define XMEM_BLOCK_SIZE sizeof(xMemBlock)
#define XMEM_NODE_SIZE(t) sizeof(t)

#endif // __XTYPES_H__
