#include <stdio.h>
#include <stdlib.h>
#include "checkmem.h"

/*
 * 演示一种检测内存越界的办法
 * 添加上下限方式
 */
int main(int argc, char* argv[])
{

    // 实验步骤是, 是申请内存, 在操作内存
    char* as = mc_malloc(15);
    mc_check(as); //lok

    as[14] = 'a';
    mc_check(as);

    // 重新分配内存, 再次越界
    as = mc_realloc(as, 32);
    as[33] = 'b';

    mc_check(as);

    mc_free(as);

    return ;
}
