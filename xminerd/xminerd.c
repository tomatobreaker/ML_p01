#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <ulfius.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <fan.h>
#include <base.h>
#include <libminer.h>

#define DEFAULT_MINER_PATH  "/usr/bin/bfgminer"
#define DEFAULT_MINER  "bfgminer"
#define DEFAULT_CFG_PATH "/usr/config/bfgminer/bfgminer.json"
#define DEFAULT_MINER_PIDFILE "/var/run/xminerd.pid"

#define CHN_DNS_SERVER0 "114.114.114.114"
#define US_DNS_SERVER0 "8.8.8.8"

#define BJ_BAIDU_SERVER  "180.149.132.151"
#define ZJ_BAIDU_SERVER  "115.239.210.27"
#define BJ1_BAIDU_SERVER  "220.181.57.216"
#define BJ2_BAIDU_SERVER  "123.125.115.110"
#define TAOBAO_SERVER  "www.taobao.com"
#define MAX_PING_NR 10
#define TIMEOUT_1S  1000
#define TIMEOUT_2S  2000

static pid_t xminerd_run_miner(char * cmd, char *args)
{
        char command[256];
        int ret;
        pid_t pid;

        //sprintf(command, "%s -c %s &", cmd, args);
        //printf("[xminerd] exec command \"%s\"\n", command);
        pid = vfork();

        if(pid < 0) {
                applog(ERR, "[Xminerd] vfork err\n");
                return -1;
        }

        if(pid == 0) {
                /* child */
                char pids[8];
                FILE *fp;
                setsid();

                fp = fopen(DEFAULT_MINER_PIDFILE, "w+");
                if(!fp)
                        applog(ERR, "[Xminerd] fopen %s failed, %s\n", DEFAULT_MINER_PIDFILE, strerror(errno));
                sprintf(pids,"%d", getpid());
                fputs(pids, fp);
                fclose(fp);
                sync();
                //execl(command, DEFAULT_MINER, "-c",  DEFAULT_CFG_PATH, (char *)0);
                execl("/usr/bin/bfgminer", "bfgminer", "-c",  "/usr/config/bfgminer/bfgminer.json" ,(char *)0);
                exit(0);
        } else if( pid > 0) {
                applog(DEBUG, "[Xminerd] Father process\n");
        }
        return 0;
}

static int xminerd_process_online(char *cmd)
{
    if(access(DEFAULT_MINER_PIDFILE, F_OK) == 0)
            return 1;
    return 0;
}

static int  xminerd_network_online()
{
        applog(NOTICE, "Start check net state...\n");
        return xping(BJ_BAIDU_SERVER, TIMEOUT_1S) ||
                         xping(ZJ_BAIDU_SERVER, TIMEOUT_1S) ||
                         xping(BJ1_BAIDU_SERVER, TIMEOUT_1S) ||
                         xping(BJ2_BAIDU_SERVER, TIMEOUT_1S) ||
                         xping(CHN_DNS_SERVER0, TIMEOUT_2S);
}

static int  xminerd_file_exist(char *args)
{
    if(access(args, F_OK) == 0)
            return 1;
    else
            return 0;
}

void signalhandler(int sig)
{
        applog(DEBUG, "[Xminerd] In signal handler\n");
        pid_t pid;
        pid= waitpid(-1,NULL,0);
        if (pid > 0)
                applog(DEBUG, "[Xminerd] pid=[%d] terminated\n",pid);
        else
                applog(DEBUG, "[Xminerd] waitpid error [%s]\n",strerror(errno));

        if(access(DEFAULT_MINER_PIDFILE, F_OK) == 0)
                remove(DEFAULT_MINER_PIDFILE);
        sync();
}

static int xminerd_kill_miner()
{
        pid_t pid;
        FILE *fp;
        char buf[8];
        int ret;
        fp = fopen(DEFAULT_MINER_PIDFILE, "r");
        if(!fp) {
                applog(INFO, "[xminerd] miner process not online\n");
                return -2;
        }
        if(!fgets(buf, 8, fp)) {
                applog(INFO, "[xminerd] miner process not online\n");
                return -2;
        }

        pid = (pid_t) atoi(buf);
        applog(INFO, "[Xminerd] Kill bfgminer pid %d\n", pid);
        ret = kill(pid, SIGINT);
        return ret;
}

int main(int argc, char *argv[])
{
        int ret;
        char cmd[64];
        char args[256];
        int cur_net_sta, run, cfgdone, fanstate;
        int offline = MAX_PING_NR - 1;


        int fanin[101] = {-1}, fanout[101] = {-1}; //duty = 0,10,20 .... 100

        sprintf(cmd, "%s", DEFAULT_MINER);
        sprintf(args, "%s", DEFAULT_CFG_PATH);
        applog(INFO, "[Xminerd] Start...\n");


        while ((ret = getopt(argc, argv, "s:c:")) != -1) {
                switch (ret) {
                        case 's':
                                memset(cmd, 0, 64);
                                sprintf(cmd, "%s", optarg);
                                break;
                        case 'c':
                                memset(args, 0, 256);
                                sprintf(args, "%s", optarg);
                                break;
                        case '?':
                                applog(ERR, "encountered a unrecognized option: %c, argv: %s\n", optopt, argv[optind - 1]);
                                break;
                        case ':':
                                applog(ERR, "option: %c missing argument\n", optopt);
                                break;
                        default:
                                applog(ERR,"option: %c\n", ret);
                                break;
                }
        }
        signal(SIGCHLD, signalhandler);
#if 0

        fanstate = is_fan_healthy(100, NULL, NULL);
        if(fanstate == 0) {
                miner_state_set(FAN_EXCEP, EXCEP_ON);
                printf("*******!!STOP FOR FAN SICK !!*****\n");
                return 0;
        }
#endif
fan_loop:

        fan_study(fanin, fanout);
        applog(INFO, "[Xminerd] FAN IN [%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d]\n",
                        fanin[0], fanin[10], fanin[20], fanin[30], fanin[40], fanin[50], fanin[60], fanin[70], fanin[80], fanin[90],fanin[100]);
        applog(INFO, "[Xminerd] FAN OUT [%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d]\n",
                        fanout[0], fanout[10], fanout[20], fanout[30], fanout[40], fanout[50], fanout[60], fanout[70], fanout[80], fanout[90],fanout[100]);

#if 1

        fanstate = is_fan_healthy(100, NULL, NULL, 1);
        if(fanstate != 3) {
                miner_state_set(FAN_EXCEP, EXCEP_ON);
                if(!(fanstate & 0x1))
                        miner_state_set(FANIN_EXCEP, EXCEP_ON);
                if(!(fanstate & 0x2))
                        miner_state_set(FANOUT_EXCEP, EXCEP_ON);
				sleep(3);
                goto fan_loop;
        } else {
                miner_state_set(FAN_EXCEP, EXCEP_OFF);
                miner_state_set(FANIN_EXCEP, EXCEP_OFF);
                miner_state_set(FANOUT_EXCEP, EXCEP_OFF);
        }
#endif

        system_set_fan_speed("fanin", 100);
        system_set_fan_speed("fanout", 100);


main_loop:
        run = xminerd_process_online(cmd);
        //applog(INFO, "[Xminerd] process_online return %d\n",run);
        cfgdone = xminerd_file_exist(args);
        //applog(INFO, "[Xminerd] file exist return %d\n", cfgdone);
        cur_net_sta = xminerd_network_online();
        //applog(INFO, "[Xminerd] network online return %d\n", cur_net_sta);
        fanstate = is_fan_healthy(0, fanin, fanout, 1);
        //applog(INFO, "[Xminerd] fan healty return %d\n", cur_net_sta);
        offline = cur_net_sta ? 0 : offline + 1;

        if(fanstate != 3) {
                miner_state_set(FAN_EXCEP, EXCEP_ON);
                if(!(fanstate & 0x1))
                        miner_state_set(FANIN_EXCEP, EXCEP_ON);
                if(!(fanstate & 0x2))
                        miner_state_set(FANOUT_EXCEP, EXCEP_ON);
        } else {
                miner_state_set(FAN_EXCEP, EXCEP_OFF);
                miner_state_set(FANIN_EXCEP, EXCEP_OFF);
                miner_state_set(FANOUT_EXCEP, EXCEP_OFF);
        }

        if(offline >= MAX_PING_NR)
                miner_state_set(NET_EXCEP, EXCEP_ON);
        else
                miner_state_set(NET_EXCEP, EXCEP_OFF);

        applog(INFO, "[Xminerd] NCF(%d,%d,%d)\n", offline, cfgdone, fanstate);

        if(run) {
                if(offline >= MAX_PING_NR || !cfgdone || (fanstate != 3)) {
                        applog(NOTICE, "[Xminerd] Kill Miner Reason NCF(%d,%d,%d)!!\n", offline, cfgdone, fanstate);
                        xminerd_kill_miner();
                        miner_state_set(IDLE, EXCEP_ON);
                } else {
                        miner_state_set(IDLE, EXCEP_OFF);
                }

        } else {
                if(cfgdone && (offline == 0) && (fanstate == 3) ) {
                        applog(NOTICE, "[Xminerd] Start Miner Reason NCF(%d,%d,%d)!!\n", offline, cfgdone, fanstate);
                        xminerd_run_miner(cmd, args);
                        miner_state_set(IDLE, EXCEP_OFF);
                }  else {
                        miner_cpb_powerdown();
                        system_set_fan_speed("fanin", 10);
                        system_set_fan_speed("fanout", 10);
                        miner_state_set(IDLE, EXCEP_ON);
                }
        }
        sleep(3);
        goto main_loop;
        return 0;
}
