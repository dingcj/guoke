#ifndef _TIME_TEST_
#define _TIME_TEST_


#define TIME_TEST_OPEN  0

#if TIME_TEST_OPEN
#ifdef WIN32
#include <time.h>
#include <windows.h>
#define PC_TIME_TEST       // PC时间测试开关

#elif defined (DM8127)
#include "../../utils/utils_common.h"
#define DM8127_TIME_TEST   // DM8127时间测试开关

#elif defined (_TMS320C6X_)
#define DSP_TIME_TEST      // DSP时间测试开关

#endif
#endif

#ifdef PC_TIME_TEST     // PC时间测试
#define CLOCK_INITIALIZATION()
#define CLOCK_START(function_name)              \
    {                                           \
        LARGE_INTEGER t1, t2;                   \
        LARGE_INTEGER frequency;                \
        float run_time;                         \
        QueryPerformanceFrequency(&frequency);  \
        QueryPerformanceCounter(&t1);

#define CLOCK_END(function_name)                                                    \
        QueryPerformanceCounter(&t2);                                               \
        run_time = (1000 * (t2.QuadPart - t1.QuadPart) / (float)frequency.QuadPart);\
        printf("%35s = %.4fms\n", function_name, run_time);			                \
    }
#define CLOCK_ENTER_LINE()  \
    {                       \
        printf("\n");       \
    }

#elif defined (DSP_TIME_TEST)   // DSP时间测试
#define CLOCK_INITIALIZATION()
#define CLOCK_START(function_name)      \
    {                                   \
        unsigned long t1;               \
        unsigned long t2;               \
        unsigned long dt;               \
        t1 = CLK_gethtime();            \
        t2 = CLK_gethtime();            \
        dt = t2 - t1;                   \
        t1 = CLK_gethtime();

#define CLOCK_END(function_name)                    \
    t2 = CLK_gethtime();                        \
    t2 = t2 - t1 - dt;                          \
    printf("%35s: ", function_name);            \
    printf("%8d cycles", t2);                   \
    printf("%5d ms\n", t2 / CLK_countspms());   \
    }

#define CLOCK_ENTER_LINE()  \
    {                       \
        printf("\n");       \
    }

#elif defined (DM8127_TIME_TEST)
#define CLOCK_START(function_name)      \
    {                                   \
        uint32_t t1, t2;                \
        t1 = Utils_getCurTimeInMsec();

#define CLOCK_END(function_name)                            \
    t2 = Utils_getCurTimeInMsec();                      \
    Vps_printf("%35s: %5dms\n", function_name, t2 - t1);\
    }

#define CLOCK_ENTER_LINE()  \
    {                       \
        Vps_printf("\n");   \
    }
#else
#define CLOCK_INITIALIZATION()
#define CLOCK_START(function_name)
#define CLOCK_END(function_name)
#define CLOCK_ENTER_LINE()
#endif // 时间测试

#endif // _TIME_TEST_

