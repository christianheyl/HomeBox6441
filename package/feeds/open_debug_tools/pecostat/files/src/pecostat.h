/*
 * pecostat.h, version 0.9.0
 */

#define PECOSTAT_MINOR 241
#define PECOSTAT_MAJOR  10
/* The PECOSTAT_MAJOR is MISC_MAJOR, i.e. 10, from linux/major.h */

#define CPU34K      1
#define MIPS_34K    CPU34K

#define MIPS_34K_HPC_MAX 4

typedef struct info_struct {
    int cpu_type;
    int perf_counters_count;
    int events_count;
    unsigned flags;
} PECOSTAT_INFO;
