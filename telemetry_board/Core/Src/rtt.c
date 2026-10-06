#include "rtt.h"
#include <string.h>

#define RTT_UP_BUFFER_SIZE 1024
#define RTT_DOWN_BUFFER_SIZE 16

// Layout must match SEGGER RTT so OpenOCD can find and parse it
typedef struct {
  const char *name;
  char *buffer;
  unsigned size;
  volatile unsigned wr_off;
  volatile unsigned rd_off;
  unsigned flags;
} RTT_Buffer_t;

typedef struct {
  char id[16];
  int max_up_buffers;
  int max_down_buffers;
  RTT_Buffer_t up[1];
  RTT_Buffer_t down[1];
} RTT_ControlBlock_t;

static char up_buffer[RTT_UP_BUFFER_SIZE];
static char down_buffer[RTT_DOWN_BUFFER_SIZE];

// Referenced by name in rtt_monitor.sh to locate the block in RAM
RTT_ControlBlock_t _SEGGER_RTT;

void RTT_Init(void) {
  _SEGGER_RTT.max_up_buffers = 1;
  _SEGGER_RTT.max_down_buffers = 1;

  _SEGGER_RTT.up[0].name = "Terminal";
  _SEGGER_RTT.up[0].buffer = up_buffer;
  _SEGGER_RTT.up[0].size = RTT_UP_BUFFER_SIZE;

  _SEGGER_RTT.down[0].name = "Terminal";
  _SEGGER_RTT.down[0].buffer = down_buffer;
  _SEGGER_RTT.down[0].size = RTT_DOWN_BUFFER_SIZE;

  // Write the ID last so the host never sees a half-initialised block
  memcpy(_SEGGER_RTT.id, "SEGGER RTT", 11);
}

int RTT_Write(const char *data, int len) {
  RTT_Buffer_t *buf = &_SEGGER_RTT.up[0];
  unsigned wr = buf->wr_off;
  unsigned rd = buf->rd_off;
  unsigned free_space =
      (rd > wr) ? (rd - wr - 1) : (buf->size - (wr - rd) - 1);

  if ((unsigned)len > free_space) {
    len = free_space;
  }

  for (int i = 0; i < len; i++) {
    buf->buffer[wr] = data[i];
    if (++wr >= buf->size) {
      wr = 0;
    }
  }
  buf->wr_off = wr;
  return len;
}
