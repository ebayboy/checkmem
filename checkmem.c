#include "checkmem.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

// 控制台打印错误信息, fmt必须是双引号括起来的宏
#define CERR(fmt, ...) \
	fprintf(stderr,"[%s:%s:%d][error %d:%s]" fmt "\r\n",\
			__FILE__, __func__, __LINE__, errno, strerror(errno), ##__VA_ARGS__)
//控制台打印错误信息并退出, t同样fmt必须是 ""括起来的字符串常量
#define CERR_EXIT(fmt,...) \
	CERR(fmt,##__VA_ARGS__),exit(EXIT_FAILURE)

// 插入字节块的个数
#define _INT_CHECK        (1<<4) //16字节  == 2 * size_t(8 byte)

//  头部 | 实际返回数据指针 |  尾部
//  头部 ： _INT_CHECK =  8字节到尾部检测块的长度 | 8字节(size_t)存储

static void _iserror(char* s, char* e);

/*
 * 对malloc进行的封装, 添加了边界检测内存块
 * sz        : 申请内存长度
 *            : 返回得到的内存首地址
 */
inline void*
mc_malloc(size_t sz)
{
    // 头和尾都加内存检测块, 默认0x00
    char* ptr = calloc(1, sz +  2  * _INT_CHECK);
    if (NULL == ptr)
    {
        CERR_EXIT("malloc sz + sizeof struct check is error!");
    }

    //前8个字节保存 最后一个内存块地址 大小
    size_t* iptr = (size_t*)ptr;
    *iptr = sz + _INT_CHECK;
    //int:4 size_t:8 _INT_CHECK: 16
    //  8B | 8B | xxxx |

    return ptr + _INT_CHECK;
}

/*
 * 对calloc进行封装, 添加边界检测内存块
 * cut        : 申请的个数
 * sz        : 每个的大小
 */
inline void*
mc_calloc(size_t cut, size_t sz)
{
    return mc_malloc(cut * sz);
}

/*
 * 对relloc进行了封装, 同样添加了边间检测内存块
 */
inline void*
mc_realloc(void* ptr, size_t sz)
{
    // 先检测一下内存
    mc_check(ptr);

    char* optr = (char*)ptr - _INT_CHECK;
    size_t* osz = (size_t*)optr;

    // 重新申请内存
    char* nptr = calloc(1, sz +  2  * _INT_CHECK);
    if (NULL == nptr)
    {
        CERR_EXIT("realloc is error:%p.", ptr);
    }
    size_t* iptr = (size_t*)nptr;

    //移动内存
    memcpy(nptr + _INT_CHECK, optr, *osz < (sz + _INT_CHECK) ? *osz : (sz + _INT_CHECK));
    *iptr = sz + _INT_CHECK;

    free(optr);

    return nptr + _INT_CHECK;
}

// 检测内存是否错误, 错误返回 true, 在控制台打印信息
static void _iserror(char* s, char* e)
{
    while (s < e)
    {
        if (*s)
        {
            CERR_EXIT("Need to debug test!!! ptr is : (%p, %p).check is %d!", s, e, *s);
        }
        ++s;
    }
}

/*
 * 对内存检测, 看是否出错, 出错直接打印错误信息
 * 只能检测, check_* 得到的内存
 */
inline void
mc_check(void* ptr)
{
    char *sptr = (char*)ptr - _INT_CHECK;

    //先检测头部
    char* s = sptr + sizeof(size_t);  // 跳过8字节存储尾指针长度
    char* e = sptr + _INT_CHECK;
    _iserror(s, e);

    //后检测尾部
    size_t sz = *(size_t*)sptr; // get tail ptr
    s = sptr + sz;
    e = s + _INT_CHECK;
    _iserror(s, e);
}

inline void mc_free(void *ptr)
{
    if (ptr)
    {
        mc_check(ptr);
        free(ptr - _INT_CHECK);
    }
}
