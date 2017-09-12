#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


#if XMEM_DEBUG

#define XMEM_LOG printf

#else

#define XMEM_LOG(...)

#endif

#define xMemAssert   assert
#define xMemPrintf  printf

#define SYS_ENTER_CRITICAL_SECTION
#define SYS_EXIT_CRITICAL_SECTION

#endif // __PLATFORM_H__
