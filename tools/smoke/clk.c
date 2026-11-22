#include <stdio.h>
#include <exec/types.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/timer.h>
struct Device* TimerBase;
int main(void)
{
    struct MsgPort* port = CreateMsgPort();
    struct timerequest* req = (struct timerequest*)CreateIORequest(port, sizeof(struct timerequest));
    OpenDevice((CONST_STRPTR) "timer.device", UNIT_ECLOCK, (struct IORequest*)req, 0);
    TimerBase = req->tr_node.io_Device;
    struct EClockVal a, b;
    ULONG f = ReadEClock(&a);
    register unsigned long n = 20000000;
    __asm__ volatile("1: subq.l #1,%0\n\tbne 1b" : "+d"(n));
    ReadEClock(&b);
    unsigned long ticks = b.ev_lo - a.ev_lo;
    printf("freq %lu ticks %lu -> %lu ps per loop iteration (subq+bne)\n", f, ticks, (unsigned long)((double)ticks / f * 1e12 / 20000000.0));
    return 0;
}
