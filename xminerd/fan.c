#include <stdio.h>
#include <stdlib.h>
#include <base.h>
#include <libminer.h>

static int roud10(int t) {
    return (t / 10) * 10;
    
}

int is_fan_healthy(int duty, int *in, int *out, int wait) {
    char *fanin = "fanin";  
    char *fanout = "fanout";  
    int lvl0,lvl1, rpm0, rpm1;
    int ret, ret_in = 0, ret_out = 0;

    if(duty) {
            ret = system_set_fan_speed(fanin, duty);
            if(ret != duty)
                    return 0;
            ret = system_set_fan_speed(fanout, duty);
            if(ret != duty)
                    return 0;
    }


    if(wait) {
            while(system_get_fan_state(fanin) != FAN_STABLE) {
                    usleep(500 *1000);
            }

            while(system_get_fan_state(fanout) != FAN_STABLE) {
                    usleep(500 *1000);
            }
    } else  {
            if(system_get_fan_state(fanin) != FAN_STABLE || 
                    system_get_fan_state(fanout) != FAN_STABLE)
                    return 3;
    }
    

    lvl0 = system_get_fan_speed(fanin);
    rpm0 = system_get_fan_rpm(fanin);
    lvl1 = system_get_fan_speed(fanout);
    rpm1 = system_get_fan_rpm(fanout);


    if(in == NULL || out == NULL) {
            ret_in = rpm0 > (lvl0 * 38) ? 1 : 0;
            ret_out = rpm1 > (lvl1 * 38 ) ? 1 : 0;
    } else {
            if(lvl0 < 30) {
                    if(rpm0 > 10)
                            ret_in = 1;
                    else
                            ret_in = 0;
            
            } else if(in[lvl0] == -1) {
                    ret_in = (rpm0 < (in[roud10(lvl0)]   *  6 / 10)) ? 0 : 1; 
                    in[lvl0] = rpm0;
            } else {
                    ret_in = (rpm0 < (in[lvl0]   *  6 / 10)) ? 0 : 1;
            }

            if(lvl1 < 30) {
                    if(rpm1 > 10)
                            ret_out = 1;
                    else
                            ret_out = 0;
            }else if(in[lvl1] == -1 ) {
                    ret_out = (rpm1 < (out[roud10(lvl1)]   *  6 / 10)) ? 0 : 1; 
                    out[lvl1] = rpm1;
            } else {
                    ret_out = (rpm1 < (out[lvl1]   *  6 / 10)) ? 0 : 1;
            }

    }
    applog(INFO, "[Xminerd] FANIN(%d, %d)  FANOUT(%d, %d).\n", lvl0, rpm0, lvl1, rpm1);

    return ret_in | (ret_out << 1);
}


int fan_study(int * in, int *out) {
        char *fanin = "fanin";  
        char *fanout = "fanout";  
        int duty, i;
        for(i = 1; i < 11; i++) {
                duty = i * 10;
                system_set_fan_speed(fanin, duty); 
                system_set_fan_speed(fanout, duty); 
                while(system_get_fan_state(fanin) != FAN_STABLE) {
                        usleep(100 *1000);
                }
                while(system_get_fan_state(fanout) != FAN_STABLE) {
                        usleep(100 *1000);
                }

                *(in + i *10) = system_get_fan_rpm(fanin);
                *(out + i * 10) = system_get_fan_rpm(fanout);
        }

        return 0;
}

