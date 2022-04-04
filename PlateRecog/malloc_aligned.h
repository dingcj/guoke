#ifndef _MALLOC_ALIGNED_
#define _MALLOC_ALIGNED_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "portab.h"

#ifdef DM8127
#include <mcfw/src_bios6/utils/utils_mem.h>
//#define _TMS320C6X_L2
#endif

#ifdef WIN32
#define MALLOC_CHECK_DEBUG  0       // 内存释放检查调试
#if MALLOC_CHECK_DEBUG
typedef struct MemStr 
{
    int32_t pointer;
    int32_t len;
    int32_t cnt;
} MemStr;

#define MAX_MEM_BLOCK   (10000)
MemStr mem_str[MAX_MEM_BLOCK];
#endif
#endif

static void* checked_malloc(int size, int alignment)
{
    unsigned char *mem_ptr = NULL;

    if (!alignment)
	{
        mem_ptr = (unsigned char *)malloc(size + 1);
		/* We have not to satisfy any alignment */
        if (mem_ptr != NULL)
		{
			/* Store (mem_ptr - "real allocated memory") in *(mem_ptr-1) */
            *mem_ptr = (unsigned char)1;

            /* Return the mem_ptr pointer */
            return ((void *)(mem_ptr+1));
        }
    }
	else
	{
        unsigned char *tmp = NULL;

        tmp = (unsigned char *)malloc(size + alignment);

        /* Allocate the required size memory + alignment so we
        * can realign the data if necessary */
        if (tmp != NULL)
		{
            /* Align the tmp pointer */
            mem_ptr =
                (unsigned char *) ((unsigned int) (tmp + alignment - 1) &
                (~(unsigned int) (alignment - 1)));

            /* Special case where malloc have already satisfied the alignment
            * We must add alignment to mem_ptr because we must store
            * (mem_ptr - tmp) in *(mem_ptr-1)
            * If we do not add alignment to mem_ptr then *(mem_ptr-1) points
            * to a forbidden memory space */
            if (mem_ptr == tmp)
			{
				mem_ptr += alignment;
			}

			/* (mem_ptr - tmp) is stored in *(mem_ptr-1) so we are able to retrieve
            * the real malloc block allocated and free it in xvid_free */
            *(mem_ptr - 1) = (unsigned char) (mem_ptr - tmp);

            /* Return the aligned pointer */
            return ((void *)mem_ptr);
        }
    }

    return(NULL);
}

static void checked_free(void* p, int size)
{
    unsigned char *ptr = NULL;

    if (p == NULL)
	{
		return;
	}

	size++; // just for disable the compilce warning

	/* Aligned pointer */
    ptr = (unsigned char *)p;

    /* *(ptr - 1) holds the offset to the real allocated block
    * we sub that offset os we free the real pointer */
    ptr -= *(ptr - 1);

    /* Free the memory */
    free(ptr);
	ptr = NULL;
}

// L2空间分配
#ifdef _TMS320C6X_L2
#include <xdc/std.h>
#include <xdc/runtime/IHeap.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/ipc/SharedRegion.h>
#include <mcfw/src_bios6/utils/utils_mem.h>
#include <mcfw/interfaces/link_api/system_debug.h>
#include <mcfw/interfaces/link_api/system_common.h>
#include <mcfw/interfaces/link_api/system_tiler.h>
extern IHeap_Handle internalHeap;

#define MEM_alloc_I(var, size, alignment)                   \
{                                                           \
    Error_Block ebObj;                                      \
    Error_Block *eb = &ebObj;                               \
    Error_init(eb);                                         \
                                                            \
    var = Memory_alloc(internalHeap, size, alignment, eb);  \
}

#define MEM_free_I(var, size)               \
{                                           \
    Memory_free(internalHeap, var, size);   \
}
#endif

#ifdef DM8127
#define CHECKED_MALLOC(var, size, alignment)            \
{                                                       \
    if (size > 0)                                       \
    {                                                   \
        var = Utils_memAlloc_cached(size, alignment);   \
        if(!var)                                        \
        {                                               \
            Vps_printf("\n malloc failed !\n");	        \
            exit(0);                                    \
        }                                               \
    }                                                   \
    else                                                \
    {                                                   \
        var = NULL;                                     \
    }                                                   \
}

#define CHECKED_FREE(var, size)             \
{                                           \
    if ((var) != NULL)                      \
    {                                       \
        Utils_memFree_cached(var, size);    \
    }                                       \
}
#else
#if MALLOC_CHECK_DEBUG
#define CHECKED_MALLOC(var, size, alignment)	\
{	                                            \
    int mem_str_cnt;				                	    \
    if (size > 0)                                   \
    {                                               \
        var = checked_malloc(size, alignment);      \
        for (mem_str_cnt = 0; mem_str_cnt < MAX_MEM_BLOCK; mem_str_cnt++)         \
        {                                           \
            if (mem_str[mem_str_cnt].pointer == 0)            \
            {                                       \
                mem_str[mem_str_cnt].pointer = (int)var;      \
                mem_str[mem_str_cnt].len = size;              \
                break;                              \
            }                                       \
        }                                           \
        if(!var)                                    \
        {                                           \
            printf("\n malloc failed !\n");         \
            while(1);                                \
        }                                           \
    }                                               \
    else                                            \
    {                                               \
        var = NULL;                                 \
    }                                           \
}

#define CHECKED_FREE(var, size)	                    \
{							                        \
    int mem_str_cnt;                                          \
    for (mem_str_cnt = 0; mem_str_cnt < MAX_MEM_BLOCK; mem_str_cnt++)             \
    {                                               \
        if ((int)var == mem_str[mem_str_cnt].pointer)         \
        {                                           \
            if (mem_str[mem_str_cnt].len == (int)(size))      \
            {                                       \
                break;                              \
            }                                       \
            else                                    \
            {                                       \
                printf("malloc_free size err!\n");  \
                while(1);                           \
            }                                       \
        }                                           \
    }                                               \
                                                    \
    if (mem_str_cnt == MAX_MEM_BLOCK)                         \
    {                                               \
        printf("pointer err!, p: %d\n", var);       \
        assert(mem_str_cnt == MAX_MEM_BLOCK);                                    \
    }                                               \
    checked_free(var, size);		                \
    mem_str[mem_str_cnt].pointer = 0;                         \
    mem_str[mem_str_cnt].len = 0;                             \
}
#else
#define CHECKED_MALLOC(var, size, alignment)    \
{                                               \
    var = checked_malloc(size, alignment);      \
    if(!var)                                    \
    {                                           \
        printf("\n malloc failed !\n");         \
        exit(0);                                \
    }                                           \
}

#define CHECKED_FREE(var, size)     \
{                                   \
    if (var)                        \
    {                               \
        checked_free(var, size);    \
    }                               \
}
#endif
#endif

#endif // _MALLOC_ALIGNED_
