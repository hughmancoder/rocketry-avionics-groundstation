#ifndef RTT_H
#define RTT_H

/*
 * Minimal SEGGER-RTT compatible console (single up channel).
 * Lets printf output stream over the ST-Link SWD connection via OpenOCD,
 * so no USB-UART adapter is needed. See rtt_monitor.sh.
 */

void RTT_Init(void);
/* Non-blocking: bytes that don't fit (no host reading) are dropped. */
int RTT_Write(const char *data, int len);

#endif // RTT_H
