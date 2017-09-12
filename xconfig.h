#ifndef __XCONFIG_H__
#define __XCONFIG_H__

#define XMEM_POOL_OPPOSITE   0

#define XMEM_DEBUG    0

#define CPU_64_BIT    0

#define XMEM_POOL_SIZE    (1024*10)

#define XMEM_SUPERBLOCK_ENABLE    1

#define XMEM_SUPERBLOCK_BLKS_MAX      32

#if XMEM_POOL_OPPOSITE
/*
------------------------------------------------------------------------------
|Header List|  xMemHdrEnd--->|  ...   |<---xMemBlkStart  |Blocks,Super Blocks|
------------------------------------------------------------------------------
*/
#define XMEM_BOUNDRY_CHECK_ENABLE   0
#define XMEM_HEADER_PROTECT_ENABLE  1
#else
/*
--------------------------------------------------------------
|       |     prev     |     blk      |    next      |       |
|  ...  |--------------|--------------|--------------|  ...  |
|       |header|  mem  |header|  mem  |header|  mem  |       |
--------------------------------------------------------------
*/
#define XMEM_BOUNDRY_CHECK_ENABLE   1
#define XMEM_HEADER_PROTECT_ENABLE  0
#endif

#if CPU_64_BIT
#define XMEM_META_BLOCK_SIZE    ((u32)8)
#define XMEM_8META_ENABLE   0
#define XMEM_SUPERBLOCK_LIST_COUNT  3
#else
#define XMEM_META_BLOCK_SIZE    ((u32)4)
#define XMEM_8META_ENABLE   1
#if XMEM_8META_ENABLE
#define XMEM_SUPERBLOCK_LIST_COUNT  4
#else
#define XMEM_SUPERBLOCK_LIST_COUNT  3
#endif
#endif

#define XMEM_BALLANCE_SIZE    (XMEM_META_BLOCK_SIZE*4)
/******************************************************************************************
 * on 32-bit cpu, xMemMgrHdr requires 8 bytes, xMemBlock requires 16 bytes, to manage a
 * block will spend 24 bytes for a header
 *
 * on 64-bit cpu, xMemMgrHdr requires 12 bytes, xMemBlock requires 24 bytes, to manage a
 * block will spend 36 bytes for a header
 *
 * ballance size determined waste size when a free block lager than requrired
*******************************************************************************************/

#define XMEM_2META_BLOCK_SIZE    (XMEM_META_BLOCK_SIZE*2)
#define XMEM_4META_BLOCK_SIZE    (XMEM_META_BLOCK_SIZE*4)
#define XMEM_8META_BLOCK_SIZE    (XMEM_META_BLOCK_SIZE*8)

#define XMEM_SUPERBLOCK_1META_CNT        ((u32)16)
#define XMEM_SUPERBLOCK_2META_CNT        ((u32)32)
#define XMEM_SUPERBLOCK_4META_CNT        ((u32)16)
#define XMEM_SUPERBLOCK_8META_CNT        ((u32)16)

#define XMEM_SUPERBLOCK_1META_MIN_SIZE (XMEM_META_BLOCK_SIZE*XMEM_SUPERBLOCK_1META_CNT)
#define XMEM_SUPERBLOCK_2META_MIN_SIZE (XMEM_2META_BLOCK_SIZE*XMEM_SUPERBLOCK_2META_CNT)
#define XMEM_SUPERBLOCK_4META_MIN_SIZE (XMEM_4META_BLOCK_SIZE*XMEM_SUPERBLOCK_4META_CNT)
#define XMEM_SUPERBLOCK_8META_MIN_SIZE (XMEM_8META_BLOCK_SIZE*XMEM_SUPERBLOCK_8META_CNT)

#endif // __XCONFIG_H__
