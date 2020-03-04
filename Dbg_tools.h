#ifndef __DBG_TOOLS_H__
#define __DBG_TOOLS_H__

#define DBGUART 1
extern "C"
{
#include "usart.h"
}
#define DBG_RED 	"\033[31m""%s""%s""\033[0m"
#define DBG_GRE 	"\033[32m""%s""%s""\033[0m"
#define DBG_BLACK 	"\033[30m""%s""%s""\033[0m"
#define DBG_YELLOW 	"\033[33m""%s""%s""\033[0m"
#define DBG_BLUE 	"\033[34m""%s""%s""\033[0m"
#define DBG_PINK 	"\033[35m""%s""%s""\033[0m"
#define DBG_CYAN 	"\033[36m""%s""%s""\033[0m"
#define DBG_WHITE 	"\033[37m""%s""%s""\033[0m"
#if (DBGUART)
#define DBG_Printf(x, t, ...)        \
    do                               \
    {                                \
        DPrintf(x, t,##__VA_ARGS__); \
    } while (0)
#else
#define EN_LOG 0
#define TAG ""
#define DBG_Printf(...)
#endif
#endif