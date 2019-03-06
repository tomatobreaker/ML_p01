#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <base.h>

#define XMINERD_FILE_MAX   0x100000
#define XMINERD_TRASH_PATH "/usr/config/xminerd/trash/"
#define XMINERD_DBG_SYSLOG "xminerdlog"
#define XMINERD_ROTATE "xminerdlog.rotate"



static int xminerd_run_systemcmd(char *cmd)
{   
    pid_t status;
    
    status = system(cmd);
    if (-1 == status) {
          printf("system error, cmd: %s\n", cmd);
    } else {
        if (WIFEXITED(status)) {
            if (0 == WEXITSTATUS(status)) {
                return 0;
            } else {
               printf("run shell script fail, script exit code: %d\n", WEXITSTATUS(status));
            }
        } else {
             printf("exit status = [%d]\n", WEXITSTATUS(status));
        }
    }
    return -1;
}


static void xminerd_check_prepare_file() {
        char cmd[64];
        if (!access(XMINERD_TRASH_PATH ,F_OK))
                return;
        sprintf(cmd, "mkdir -p %s", XMINERD_TRASH_PATH);
        xminerd_run_systemcmd(cmd);
        return;
}

 int xminerd_save_syslog(char *buf)
 {
         char path[64];
         char cmd[128];
         int filelen;
         int least = 0, total;
         FILE *file;

         sprintf(path, "%s%s", XMINERD_TRASH_PATH, XMINERD_DBG_SYSLOG);
         xminerd_check_prepare_file();
         file = fopen(path, "a+");
         if (!file) {
                 printf("Open File Error: %s(%s)\n", path, strerror(errno));
                 return -1;
         }
         fseek(file, 0, SEEK_END);
         filelen = ftell(file);

         total = strlen(buf);
         if ((XMINERD_FILE_MAX - filelen) > total)
                 fwrite(buf, 1, total, file);
         else {
                 fwrite(buf, 1, XMINERD_FILE_MAX - filelen, file);
                 least = total - (XMINERD_FILE_MAX - filelen);
         }
         fclose(file);

         if (least) {
#if 0
                 sprintf(cmd, "mv %s %s%s", path, XMINERD_TRASH_PATH, XMINERD_ROTATE);
                 xminerd_run_systemcmd(cmd);
#else
                 sprintf(cmd, "%s%s", XMINERD_TRASH_PATH, XMINERD_ROTATE);
                 rename(path, cmd);
#endif

                 xminerd_save_syslog(buf + (XMINERD_FILE_MAX - filelen));
         }

         return 0;
 }


#if 0
void applog(enum loglevel level,  char *fmt, ...)
{
        char mesg[1024];
        char mesg1[1024];
        char datetime[64];
        struct timeval tv;
        struct tm tm;

        if (level > DEBUG)
                return;

        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &tm);
        snprintf(datetime, sizeof(datetime), " [%d-%02d-%02d %02d:%02d:%02d] ",
                        tm.tm_year + 1900,
                        tm.tm_mon + 1,
                        tm.tm_mday,
                        tm.tm_hour,
                        tm.tm_min,
                        tm.tm_sec);

        sprintf(mesg, "%s%s", datetime, fmt);

        {
                va_list ap;
                va_start(ap, mesg);
                vprintf(mesg, ap);
                va_end(ap);

        }
        snpritf(mesg1, "%s%s",datetime, mesg);
        xminerd_save_syslog(mesg1);

    return;
}
#endif

void _applog(int prio, const char *str) {
        char logbuf[4096];
        char datetime[64];
        struct timeval tv;
        struct tm tm;
        memset(logbuf, 0, 4096);
        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &tm);
        snprintf(datetime, sizeof(datetime), " [%d-%02d-%02d %02d:%02d:%02d] ",
                        tm.tm_year + 1900,
                        tm.tm_mon + 1,
                        tm.tm_mday,
                        tm.tm_hour,
                        tm.tm_min,
                        tm.tm_sec);
        
        
        fprintf(stderr, " %s %s", datetime, str); 
        
        int len = strlen(str) + strlen(datetime) + 4;
        snprintf(logbuf,  (len > LOGBUFSIZ) ? LOGBUFSIZ : len, " %s %s",datetime, str);
        xminerd_save_syslog(logbuf);

}


