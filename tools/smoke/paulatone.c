/* paulatone MODE [seconds]: plays a tone on Paula to check what is audible.
 *   dev   audio.device like the game: stereo pair (channels {3,5,10,12}), left and right requests, 8 bit
 *   dev1  audio.device, one channel (mask {1,2,4,8}), mono
 *   hw    channels locked through audio.device, then the Paula registers written directly (like HippoPlayer)
 * A 440 Hz square wave (loud) on both sides for the given seconds (default 4). */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <devices/audio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

extern struct ExecBase *SysBase;

#define N 2000 /* samples per buffer: 22030 Hz / 2000 = 11 Hz base, the wave below has 50 samples per period */

static signed char *tone(void)
{
	signed char *b = (signed char *)AllocMem(N, MEMF_CHIP | MEMF_CLEAR);
	int i;
	if (b)
		for (i = 0; i < N; i++)
			b[i] = ((i / 25) & 1) ? 100 : -100; /* 50 samples per period = 440 Hz at 22 kHz */
	return b;
}

static int run_dev(int single, int seconds)
{
	static UBYTE stereo[] = {3, 5, 10, 12};
	static UBYTE mono[] = {1, 2, 4, 8};
	struct MsgPort *port = CreateMsgPort();
	struct IOAudio *a = (struct IOAudio *)CreateIORequest(port, sizeof(struct IOAudio));
	struct IOAudio *l, *r = NULL;
	signed char *b0 = tone(), *b1 = tone();
	ULONG clock = (SysBase->VBlankFrequency == 50) ? 3546895UL : 3579545UL;
	UWORD period = (UWORD)((clock + 22050 - 1) / 22050);
	ULONG units;
	int cycles = seconds * 22030 / N;

	if (!a || !b0 || !b1)
		return 20;
	a->ioa_Request.io_Message.mn_Node.ln_Pri = 1;
	a->ioa_Request.io_Flags = ADIOF_NOWAIT;
	a->ioa_Data = single ? mono : stereo;
	a->ioa_Length = 4;
	if (OpenDevice((CONST_STRPTR)AUDIONAME, 0, (struct IORequest *)a, 0)) {
		PutStr("audio.device open failed\n");
		return 20;
	}
	units = (ULONG)a->ioa_Request.io_Unit;
	Printf("allocated units %lx period %ld\n", units, (LONG)period);
	l = (struct IOAudio *)AllocMem(sizeof(struct IOAudio), MEMF_PUBLIC);
	*l = *a;
	l->ioa_Request.io_Command = CMD_WRITE;
	l->ioa_Request.io_Flags = ADIOF_PERVOL;
	l->ioa_Data = (UBYTE *)b0;
	l->ioa_Length = N;
	l->ioa_Period = period;
	l->ioa_Volume = 64;
	l->ioa_Cycles = cycles;
	if (!single) {
		r = (struct IOAudio *)AllocMem(sizeof(struct IOAudio), MEMF_PUBLIC);
		*r = *l;
		l->ioa_Request.io_Unit = (struct Unit *)(units & 0x09);
		r->ioa_Request.io_Unit = (struct Unit *)(units & 0x06);
		r->ioa_Data = (UBYTE *)b1;
		SendIO((struct IORequest *)r);
	}
	SendIO((struct IORequest *)l);
	{
		volatile UWORD *cu = (volatile UWORD *)0xDFF000;
		Delay(10);
		Printf("after 0.2s: DMACONR %lx INTREQR %lx INTENAR %lx ADKCONR %lx l.err %ld l.chk %ld", (LONG)cu[2 / 2], (LONG)cu[0x1E / 2], (LONG)cu[0x1C / 2], (LONG)cu[0x10 / 2], (LONG)l->ioa_Request.io_Error, (LONG)CheckIO((struct IORequest *)l));
		if (r)
			Printf(" r.err %ld r.chk %ld", (LONG)r->ioa_Request.io_Error, (LONG)CheckIO((struct IORequest *)r));
		PutStr("\n");
	}
	Delay(50 * (seconds + 1));
	AbortIO((struct IORequest *)l);
	WaitIO((struct IORequest *)l);
	if (r) {
		AbortIO((struct IORequest *)r);
		WaitIO((struct IORequest *)r);
	}
	CloseDevice((struct IORequest *)a);
	return 0;
}

static int run_hw(int seconds)
{
	static UBYTE stereo[] = {3, 5, 10, 12};
	struct MsgPort *port = CreateMsgPort();
	struct IOAudio *a = (struct IOAudio *)CreateIORequest(port, sizeof(struct IOAudio));
	signed char *b0 = tone(), *b1 = tone();
	ULONG clock = (SysBase->VBlankFrequency == 50) ? 3546895UL : 3579545UL;
	UWORD period = (UWORD)((clock + 22050 - 1) / 22050);
	volatile UWORD *custom = (volatile UWORD *)0xDFF000;
	ULONG units;
	UWORD dmabits = 0;

	if (!a || !b0 || !b1)
		return 20;
	a->ioa_Request.io_Message.mn_Node.ln_Pri = 127; /* lock the channels */
	a->ioa_Request.io_Flags = ADIOF_NOWAIT;
	a->ioa_Data = stereo;
	a->ioa_Length = 4;
	if (OpenDevice((CONST_STRPTR)AUDIONAME, 0, (struct IORequest *)a, 0)) {
		PutStr("audio.device open failed\n");
		return 20;
	}
	units = (ULONG)a->ioa_Request.io_Unit;
	Printf("allocated units %lx, hardware access\n", units);
	/* left on the first channel of the left pair (0 or 3), right on the first of the right pair (1 or 2) */
	{
		int lch = (units & 1) ? 0 : 3, rch = (units & 2) ? 1 : 2;
		volatile ULONG *lc;
		Forbid();
		custom[0x96 / 2] = (UWORD)(1 << lch | 1 << rch); /* DMA off for these */
		lc = (volatile ULONG *)(0xDFF0A0 + lch * 0x10);
		lc[0] = (ULONG)b0;
		custom[(0xA4 + lch * 0x10) / 2] = N / 2;
		custom[(0xA6 + lch * 0x10) / 2] = period;
		custom[(0xA8 + lch * 0x10) / 2] = 64;
		lc = (volatile ULONG *)(0xDFF0A0 + rch * 0x10);
		lc[0] = (ULONG)b1;
		custom[(0xA4 + rch * 0x10) / 2] = N / 2;
		custom[(0xA6 + rch * 0x10) / 2] = period;
		custom[(0xA8 + rch * 0x10) / 2] = 64;
		dmabits = (UWORD)(1 << lch | 1 << rch);
		custom[0x96 / 2] = 0x8200 | dmabits; /* DMAEN + audio channels */
		Permit();
	}
	Delay(50 * seconds);
	custom[0x96 / 2] = dmabits; /* off */
	custom[(0xA8) / 2] = 0;
	CloseDevice((struct IORequest *)a);
	return 0;
}

int main(int argc, char **argv)
{
	int seconds = 4;
	if (argc < 2) {
		PutStr("paulatone dev|dev1|hw [seconds]\n");
		return 10;
	}
	if (argc > 2) {
		seconds = 0;
		const char *p = argv[2];
		while (*p >= '0' && *p <= '9')
			seconds = seconds * 10 + (*p++ - '0');
		if (seconds < 1)
			seconds = 4;
	}
	if (!strcmp(argv[1], "dev"))
		return run_dev(0, seconds);
	if (!strcmp(argv[1], "dev1"))
		return run_dev(1, seconds);
	if (!strcmp(argv[1], "hw"))
		return run_hw(seconds);
	PutStr("unknown mode\n");
	return 10;
}
