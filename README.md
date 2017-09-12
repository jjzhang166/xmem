# xmem
一个C语言的轻量级动态内存管理，包括动态内存分配，回收，内存访问越界（写）检测。解决某些平台缺少动态内存管理的问题。

Ultralightweight dynamic memory allocation in C.

## License

Apache License

1.xmem support 2 growing method.
	a.)header and memory block are grow by opposite direction.
	-------------------------------------------------------------------------------
	| Header List|  xMemHdrEnd--->|  ...   |<---xMemBlkStart  |Blocks,Super Blocks|
	-------------------------------------------------------------------------------
	b.)header and memory block are in the same node.
	--------------------------------------------------------------
	|       |     prev     |     blk      |    next      |       |
	|  ...  |--------------|--------------|--------------|  ---> |
	|       |header|  mem  |header|  mem  |header|  mem  |       |
	--------------------------------------------------------------
2. The super block is a block that include some smaller memory blocks, to reduce memory spend on header.	
3. Set the Macro CPU_64_BIT to 1 if your CPU is 64bits. 
4. Implement the macro SYS_ENTER_CRITICAL_SECTION and SYS_SYS_CRITICAL_SECTION according to you system, to asure xmalloc and xfree are safe. 
   Make sure no interruptions occur during xmalloc or xfree are executing, otherise might cause the header link be broke.