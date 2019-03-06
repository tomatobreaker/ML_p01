#include <stdarg.h>
enum loglevel{
    ERR,
    WARNING,
    NOTICE,
    INFO,   
    DEBUG,
};

#define LOGBUFSIZ 0x1000

extern void _applog(int prio, const char *str);

#define applog(prio, fmt, ...) do { \
    if (prio < DEBUG) { \
            char tmp42[LOGBUFSIZ]; \
            memset(tmp42, 0 , LOGBUFSIZ);\
            snprintf(tmp42, sizeof(tmp42), fmt, ##__VA_ARGS__); \
            _applog(prio, tmp42); \
    } \
} while (0)


