#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// 控制台打印错误信息, fmt必须是双引号括起来的宏
#define CERR(fmt, ...) \
	fprintf(stderr,"[%s:%s:%d][error %d:%s]" fmt "\r\n",\
			__FILE__, __func__, __LINE__, errno, strerror(errno), ##__VA_ARGS__)
//控制台打印错误信息并退出, t同样fmt必须是 ""括起来的字符串常量
#define CERR_EXIT(fmt,...) \
	CERR(fmt,##__VA_ARGS__),exit(EXIT_FAILURE)

// 插入字节块的个数

//16字节  == 2 * size_t(8 byte)
#define _INT_CHECK        16

//  头部(16 Byte) | 实际返回数据指针 |  尾部(16Byte)
//  头部 ： _INT_CHECK =  8 Byte尾部检测块的长度 | 8字节边界

// 检测内存是否错误, 错误返回 true, 在控制台打印信息
static int _iserror(char* s,  char* e)
{
    char *p = s;
    int i = 0;
    int flag_exit = 0;

    while (p < e)
    {
        if (*p)
        {
            flag_exit = 1;
            CERR("\nNeed to debug test!!! ptr is : (%p, %p).check is %d check [%s] faied!\n", s, e, *p, (e - s == sizeof(size_t)) ? "header" : "tail");
            break;
        }
        ++p;
    }
    if (flag_exit)
    {
        fprintf(stderr, "size:[%d]\n", e - s);
        p = s;
        CERR("Coruption memory content:[");
        while (p < e)
        {
            fprintf(stderr, "[%d]:[%c]\n", i++, *p++);
        }
        fprintf(stderr, "]\n");
        return 1;
    }

    return 0;
}

/*
 * 对malloc进行的封装, 添加了边界检测内存块
 * sz        : 申请内存长度
 *            : 返回得到的内存首地址
 */

static inline void print_char(char s)
{
    if (isprint(s))
    {
        fprintf(stderr, "[%c]\n", s);
    }
    else
    {
        fprintf(stderr, "[%x]\n", s);
    }
}
/*
 * 对内存检测, 看是否出错, 出错直接打印错误信息
 * 只能检测, check_* 得到的内存
 */
void mc_check(void* ptr)
{
    int i = 0, ret = 0;
    char *sptr = (char*)ptr - _INT_CHECK;
    size_t sz = *(size_t*)sptr; // tail check start
    char *s, *s2, *e, *e2;

    //先检测头部
    s = sptr + sizeof(size_t);  // 跳过8字节存储尾指针长度
    e = sptr + _INT_CHECK;
    s2 = sptr + sz;
    e2 = s2 + _INT_CHECK;

    ret = _iserror(s, e);
    if (ret)
    {
        fprintf(stderr, "header checck ret:[%d] len:[%d]\n", ret, e - s);
        goto error;
    }

    //后检测尾部
    ret = _iserror(s2, e2);
    if (ret)
    {
        fprintf(stderr, "header checck ret:[%d] len:[%d]\n", ret, e2 - s2);
        goto error;
    }

    return ;

error:
    //打印内存
    fprintf(stderr, "===== CHECKMEM DATA OUTPUT  ... \n");
    fprintf(stderr, "data size:[%d]\n", sz + _INT_CHECK);

    //printf header_check
    fprintf(stderr, "PRE-HEADER:[%d]\n", e - s );
    for(i = 32; i > 0; i--)
    {
        fprintf(stderr, "[%d]:", i);
        print_char(*(s - i));
    }

    fprintf(stderr, "HEADER:[%d]\n", e - s );
    for(i = 0; s + i  < e; i++)
    {
        fprintf(stderr, "[%d]", i);
        print_char(*(s + i));
    }

    //printf data
    fprintf(stderr, "DATA: size:[%d]\n", sz - _INT_CHECK);
    for(i = 0; e + i < s2; i++)
    {
        fprintf(stderr, "[%d]", i);
        print_char(*( e + i));
    }

    //printf tail_check
    fprintf(stderr, "TAILER: [%d]\n", e2 - s2);
    for (i = 0; s2 + i < e2; i++)
    {
        fprintf(stderr, "[%d]", i);
        print_char(*(s2 + i));
    }

    fprintf(stderr, "AFTER-TAILER: [%d]\n", e2 - s2);
    for (i = 0; i < 32; i++)
    {
        fprintf(stderr, "[%d]", i);
        print_char(*(e2 + _INT_CHECK + i));
    }


    fprintf(stderr, "===== CHECKMEM DATA OUTPUT  done! \n");

    exit(1);
}

void* mc_malloc(size_t sz)
{
    // 头和尾都加内存检测块, 默认0x00
    char* ptr = (char *)calloc(1, sz +  2  * _INT_CHECK);
    if (NULL == ptr)
    {
        CERR_EXIT("malloc sz + sizeof struct check is error!");
    }

    //前8个字节保存 最后一个内存块地址 大小
    size_t* iptr = (size_t*)ptr;
    *iptr = sz + _INT_CHECK;
    //size_t(to_tail_len): size_t:8 _INT_CHECK: 16
    //  8B | 8B | xxxx |

    return ptr + _INT_CHECK;
}

/*
 * 对calloc进行封装, 添加边界检测内存块
 * cut        : 申请的个数
 * sz        : 每个的大小
 */
void* mc_calloc(size_t cut, size_t sz)
{
    return mc_malloc(cut * sz);
}

/*
 * 对relloc进行了封装, 同样添加了边间检测内存块
 */
void* mc_realloc(void* ptr, size_t sz)
{
    // 先检测一下内存
    mc_check(ptr);

    char* optr = (char*)ptr - _INT_CHECK;
    size_t* osz = (size_t*)optr;

    // 重新申请内存
    char* nptr = (char *)calloc(1, sz +  2  * _INT_CHECK);
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

void mc_free(void *ptr)
{
    if (ptr)
    {
        mc_check(ptr);
        free((char *)ptr - _INT_CHECK);
    }
}
