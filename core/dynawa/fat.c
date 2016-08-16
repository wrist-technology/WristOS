#include <stdio.h>
#include <string.h>
#include <stdint.h>
//#include <sysdefs.h>
#include <time.h>
#include <ff.h>

#define arrsizeof(arr) (sizeof(arr) / sizeof((arr)[0])) 

// MV from lpc2148_demo/monitor/monitor.c
uint32_t get_fattime ()
{
  uint32_t tmr;
  time_t now;
  struct tm tm;

  now = time (NULL);
  localtime_r (&now, &tm);

  tmr = 0
    | ((tm.tm_year - 80) << 25)
    | ((tm.tm_mon + 1)   << 21)
    | (tm.tm_mday        << 16)
    | (tm.tm_hour        << 11)
    | (tm.tm_min         << 5)
    | (tm.tm_sec         >> 1);

  return tmr;
}

const char *f_ferrorlookup (FRESULT f)
{
  unsigned int i;

  typedef struct errorStrings_s
  {
    FRESULT fresult;
    const char *string;
  }
  errorStrings_t;
/* TODO
    FR_OK = 0,
    FR_DISK_ERR,
    FR_INT_ERR,
    FR_NOT_READY,
    FR_NO_FILE,
    FR_NO_PATH,
    FR_INVALID_NAME,
    FR_DENIED,
    FR_EXIST,
    FR_INVALID_OBJECT,
    FR_WRITE_PROTECTED,
    FR_INVALID_DRIVE,
    FR_NOT_ENABLED,
    FR_NO_FILESYSTEM,
    FR_MKFS_ABORTED,
    FR_TIMEOUT
*/

  static const errorStrings_t errorStrings [] =
  {
    { FR_OK,              "OK"              },
    { FR_NOT_READY,       "NOT_READY"       },
    { FR_NO_FILE,         "NO_FILE"         },
    { FR_NO_PATH,         "NO_PATH"         },
    { FR_INVALID_NAME,    "INVALID_NAME"    },
    { FR_INVALID_DRIVE,   "INVALID_DRIVE"   },
    { FR_DENIED,          "DENIED"          },
    { FR_EXIST,           "EXIST"           },
    //{ FR_RW_ERROR,        "RW_ERROR"        },
    { FR_WRITE_PROTECTED, "WRITE_PROTECTED" },
    { FR_NOT_ENABLED,     "NOT_ENABLED"     },
    { FR_NO_FILESYSTEM,   "NO_FILESYSTEM"   },
    { FR_INVALID_OBJECT,  "INVALID_OBJECT"  },
    { FR_MKFS_ABORTED,    "MKFS_ABORTED"    },
  };

  for (i = 0; i < arrsizeof (errorStrings); i++)
    if (errorStrings [i].fresult == f)
      return errorStrings [f].string;

  return "(no err text)";
}

void f_printerror (FRESULT f)
{
  printf ("rrc=%u %s\n", f, f_ferrorlookup (f));
}

