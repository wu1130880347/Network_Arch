#ifndef __DBG_TOOLS_H__
#define __DBG_TOOLS_H__

#define DBGUART 0

#define DBG_RED 	"\033[31m""%s""%s""\033[0m"
#define DBG_GRE 	"\033[32m""%s""%s""\033[0m"
#define DBG_BLACK 	"\033[30m""%s""%s""\033[0m"
#define DBG_YELLOW 	"\033[33m""%s""%s""\033[0m"
#define DBG_BLUE 	"\033[34m""%s""%s""\033[0m"
#define DBG_PINK 	"\033[35m""%s""%s""\033[0m"
#define DBG_CYAN 	"\033[36m""%s""%s""\033[0m"
#define DBG_WHITE 	"\033[37m""%s""%s""\033[0m"
#if (DBGUART)
#define DBG_Printf(x, t, ...) \
    do                        \
    {                         \
    } while (0)
#else
#define DBG_Printf(...)
#endif

//debug info output
#if DBGUART
extern "C"
{
#define DBG_USE 0

#if DBG_USE
    //是否打开该文件内的调试LOG
    static const char *EN_LOG = DBG_YELLOW;
    //LOG输出文件标记
    static const char TAG[] = "DBG: ";
#else
#ifdef DBG_Printf
#undef DBG_Printf
#define DBG_Printf(...)
#else
#endif
#endif
}

#endif
#endif