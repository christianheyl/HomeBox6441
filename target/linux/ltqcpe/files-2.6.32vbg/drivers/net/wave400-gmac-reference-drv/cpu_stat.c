
#include <linux/interrupt.h>
#include "cpu_stat.h"

#ifdef DO_CPU_STAT


typedef struct
{
  cpu_stat_track_id_e id;
  const char*         name;
} cpu_stat_track_info_t;

static const cpu_stat_track_info_t cpu_stat_tracks[] = 
{
  { CPU_STAT_ID_NONE,                 "DISABLED"             },
  { CPU_STAT_ID_XMIT_START,           "XMIT_START"           },
  { CPU_STAT_ID_RELEASE_TX_DATA,      "RELEASE_TX_DATA"      },
  { CPU_STAT_ID_RX_DATA_FRAME,        "RX_DATA_FRAME"        },
  { CPU_STAT_ID_ISR_LATENCY,          "ISR_LATENCY"          },
  { CPU_STAT_ID_SET_TX_QPTR,          "SET_TX_QPTR"          },
  { CPU_STAT_ID_GET_RX_QPTR,          "GET_RX_QPTR"          }, 
  { CPU_STAT_ID_NETIF_RX,             "NETIF_RX"             }, 
  { CPU_STAT_ID_ALLOC_SKB,            "ALLOC_SKB"            }, 
  { CPU_STAT_ID_MAP,                  "MAP"                  }, 
  { CPU_STAT_ID_UNMAP,                "UNMAP"                }, 
  { CPU_STAT_ID_SKB_PUT,              "SKB_PUT"              }, 
  { CPU_STAT_ID_GET_TEMP,             "TEMP"                 }, 
  { CPU_STAT_ID_LAST,                 "UNKNOWN#"             } 
};


static const cpu_stat_track_info_t *
_CPU_STAT_GET_INFO(cpu_stat_t         *obj, 
                   cpu_stat_track_id_e id)
{
  const cpu_stat_track_info_t *res = NULL;
  int                          i   = 0;

  for (; i < ARRAY_SIZE(cpu_stat_tracks); i++) {
    res = &cpu_stat_tracks[i];
    if (res->id == id)
      break;
    res = NULL;
  }

  return res;
}

const char*
_CPU_STAT_GET_NAME (cpu_stat_t         *obj, 
                    cpu_stat_track_id_e id)
{
  const cpu_stat_track_info_t *info = _CPU_STAT_GET_INFO(obj, id);

  return info?info->name:NULL;
}

void
_CPU_STAT_GET_NAME_EX (cpu_stat_t         *obj, 
                       cpu_stat_track_id_e id,
                       char               *buf,
                       uint32              size)
{
  const char* name = _CPU_STAT_GET_NAME(obj, id);
  if (name) {
    strncpy(buf, name, size);
  }
  else {
    TR0("UNKNOWN#%03d", id);
  }
}

#endif
