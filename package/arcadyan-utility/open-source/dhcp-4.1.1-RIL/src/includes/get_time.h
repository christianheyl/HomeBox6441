
#define USE_UPTIME	1
#ifdef USE_UPTIME 
#define gettimeofday(x,y) get_time(x)
int get_time(struct timeval *tv);
#endif
