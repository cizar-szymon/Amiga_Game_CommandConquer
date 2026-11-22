/* shapebench.c - which pixel loop is fastest on the 68060 for the ghost+fading+transparent blit
 * (BF_Ghost_Fading_Trans of keybuff.cpp, 100 calls / 140 kpixels per frame in a battle).
 * Sprite 36x36 with runs of opaque and transparent pixels; three densities of opaque pixels.
 * Build: m68k-amigaos-gcc -mcpu=68060 -O2 -noixemul -o shapebench shapebench.c
 * Time: ReadEClock (709379 Hz). Output: ns per pixel of the sprite area (36*36 per call). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/types.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <exec/execbase.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef u32 __attribute__((may_alias)) u32a;
#define LD32(p) (*(const u32a*)(p))
#define ST32(p, v) (*(u32a*)(p) = (v))

struct Device* TimerBase;
static struct MsgPort* port;
static struct timerequest* req;

static u8 ghost_lookup[256], fade_tab[256];
static u8* ghost_tab; /* 256*256 */

#define W 36
#define H 36
#define PITCH 640

static u8 src_[W * H + 8], dstA[PITCH * 64], dstB[PITCH * 64];

/* V0: the original */
static void V0(int width, int height, u8* dst, u8* src, int dst_pitch, int src_pitch, u8* gl, u8* gt, u8* ft, int count)
{
    while (height--) {
        for (int i = width; i > 0; --i) {
            u8 sbyte = *src++;
            if (sbyte) {
                u8 fbyte = gl[sbyte];
                if (fbyte != 0xFF) sbyte = gt[*dst + fbyte * 256];
                for (int j = 0; j < count; ++j) sbyte = ft[sbyte];
                *dst = sbyte;
            }
            ++dst;
        }
        src += src_pitch;
        dst += dst_pitch;
    }
}

/* V1: fold fade into res[], ghost flag in comb[], plain per-pixel loop */
static void V1(int width, int height, u8* dst, u8* src, int dst_pitch, int src_pitch, u8* gl, u8* gt, u8* ft, int count)
{
    u8 res[256];
    u16 comb[256];
    int v;
    for (v = 0; v < 256; ++v) {
        u8 b = (u8)v;
        for (int j = 0; j < count; ++j) b = ft[b];
        res[v] = b;
    }
    for (v = 0; v < 256; ++v) comb[v] = (gl[v] == 0xFF) ? res[v] : (u16)(0x100 | gl[v]);
    while (height--) {
        for (int i = width; i > 0; --i) {
            u8 s = *src++;
            if (s) {
                u16 c = comb[s];
                *dst = (c < 0x100) ? (u8)c : res[gt[*dst + ((c & 0xFF) << 8)]];
            }
            ++dst;
        }
        src += src_pitch;
        dst += dst_pitch;
    }
}

/* V2: one 256-entry table "res" for non-ghost pixels, ghost row table gr[] (0xFF = none); no 16-bit comb */
static void V2(int width, int height, u8* dst, u8* src, int dst_pitch, int src_pitch, u8* gl, u8* gt, u8* ft, int count)
{
    u8 res[256];
    for (int v = 0; v < 256; ++v) {
        u8 b = (u8)v;
        for (int j = 0; j < count; ++j) b = ft[b];
        res[v] = b;
    }
    while (height--) {
        u8* d = dst;
        u8* s = src;
        u8* e = s + width;
        while (s < e) {
            u8 p = *s++;
            if (p) {
                u8 f = gl[p];
                if (f == 0xFF) {
                    *d = res[p];
                } else {
                    *d = res[gt[*d + (f << 8)]];
                }
            }
            ++d;
        }
        src += width + src_pitch;
        dst += width + dst_pitch;
    }
}

/* V3: V2 but skipping transparent runs with a tight inner loop */
static void V3(int width, int height, u8* dst, u8* src, int dst_pitch, int src_pitch, u8* gl, u8* gt, u8* ft, int count)
{
    u8 res[256];
    for (int v = 0; v < 256; ++v) {
        u8 b = (u8)v;
        for (int j = 0; j < count; ++j) b = ft[b];
        res[v] = b;
    }
    while (height--) {
        u8* d = dst;
        u8* s = src;
        u8* e = s + width;
        while (s < e) {
            while (s < e && *s == 0) {
                ++s;
                ++d;
            }
            while (s < e && *s != 0) {
                u8 p = *s++;
                u8 f = gl[p];
                *d = (f == 0xFF) ? res[p] : res[gt[*d + (f << 8)]];
                ++d;
            }
        }
        src += width + src_pitch;
        dst += width + dst_pitch;
    }
}

/* V4: V3 with ghost rows resolved per run: first the plain pixels (most), ghost only when gl != 0xFF */
static void V4(int width, int height, u8* dst, u8* src, int dst_pitch, int src_pitch, u8* gl, u8* gt, u8* ft, int count)
{
    u8 res[256];
    u8 plain[256]; /* 1 = pixel needs the ghost table */
    for (int v = 0; v < 256; ++v) {
        u8 b = (u8)v;
        for (int j = 0; j < count; ++j) b = ft[b];
        res[v] = b;
        plain[v] = (gl[v] != 0xFF);
    }
    while (height--) {
        u8* d = dst;
        u8* s = src;
        u8* e = s + width;
        while (s < e) {
            while (s < e && *s == 0) {
                ++s;
                ++d;
            }
            while (s < e && *s != 0) {
                u8 p = *s++;
                if (!plain[p]) {
                    *d = res[p];
                } else {
                    *d = res[gt[*d + (gl[p] << 8)]];
                }
                ++d;
            }
        }
        src += width + src_pitch;
        dst += width + dst_pitch;
    }
}

/* V5: V2 plus a read of every destination cache line of the row before writing (read allocates the line) */
static void V5(int width, int height, u8* dst, u8* src, int dst_pitch, int src_pitch, u8* gl, u8* gt, u8* ft, int count)
{
    u8 res[256];
    volatile u8 sink = 0;
    for (int v = 0; v < 256; ++v) {
        u8 b = (u8)v;
        for (int j = 0; j < count; ++j) b = ft[b];
        res[v] = b;
    }
    while (height--) {
        u8* d = dst;
        u8* s = src;
        u8* e = s + width;
        {
            u8* t = (u8*)((unsigned long)d & ~15UL);
            u8* te = d + width;
            u8 acc = 0;
            while (t < te) {
                acc += *(volatile u8*)t;
                t += 16;
            }
            sink = acc;
        }
        while (s < e) {
            u8 p = *s++;
            if (p) {
                u8 f = gl[p];
                if (f == 0xFF) {
                    *d = res[p];
                } else {
                    *d = res[gt[*d + (f << 8)]];
                }
            }
            ++d;
        }
        src += width + src_pitch;
        dst += width + dst_pitch;
    }
}

/* V6: hand written 68k inner loop (ghost + fading folded into res[], index 0 transparent) */
static void V6(int width, int height, u8* dst, u8* src, int dst_pitch, int src_pitch, u8* gl, u8* gt, u8* ft, int count)
{
    u8 res[256];
    for (int v = 0; v < 256; ++v) {
        u8 b = (u8)v;
        for (int j = 0; j < count; ++j) b = ft[b];
        res[v] = b;
    }
    while (height--) {
        u8* s = src;
        u8* d = dst;
        long n = width - 1;
        __asm__ volatile(
            "	moveq	#0,%%d0			" "\n"
            "	moveq	#0,%%d1			" "\n"
            "1:	move.b	(%0)+,%%d0		" "\n"
            "	beq	3f			" "\n"
            "	cmpi.b	#-1,(%4,%%d0.l)		" "\n"
            "	beq	2f			" "\n"
            "	move.b	(%4,%%d0.l),%%d1	" "\n"
            "	lsl.w	#8,%%d1			" "\n"
            "	move.b	(%1),%%d1		" "\n"
            "	move.b	(%5,%%d1.l),%%d0	" "\n"
            "	moveq	#0,%%d1			" "\n"
            "2:	move.b	(%3,%%d0.l),(%1)+	" "\n"
            "	dbra	%2,1b			" "\n"
            "	bra	9f			" "\n"
            "3:	addq.l	#1,%1			" "\n"
            "	dbra	%2,1b			" "\n"
            "9:				" "\n"
            : "+a"(s), "+a"(d), "+d"(n)
            : "a"(res), "a"(gl), "a"(gt)
            : "d0", "d1", "cc", "memory");
        src += width + src_pitch;
        dst += width + dst_pitch;
    }
}

/* V7: four pixels at a time, one (unaligned) long read-modify-write instead of four byte stores.
 * Stores cost ~55 ns each on this machine whatever their width, loads are cheap. */
static void V7(int width, int height, u8* dst, u8* src, int dst_pitch, int src_pitch, u8* gl, u8* gt, u8* ft, int count)
{
    u8 res[256];
    for (int v = 0; v < 256; ++v) {
        u8 b = (u8)v;
        for (int j = 0; j < count; ++j) b = ft[b];
        res[v] = b;
    }
    while (height--) {
        int n = width;
        if (n >= 4) {
            /* groups of 4, the last group overlaps the previous one so no byte tail is needed */
            u8* s = src;
            u8* d = dst;
            int x = 0;
            for (;;) {
                u32 w = LD32(s);
                if (w) {
                    u32 dv = LD32(d);
                    for (int k = 0; k < 4; ++k) {
                        u8 p = s[k];
                        if (p) {
                            int sh = 24 - 8 * k;
                            u8 f = gl[p];
                            u8 v = (f == 0xFF) ? res[p] : res[gt[((dv >> sh) & 0xFF) + (f << 8)]];
                            dv = (dv & ~(0xFFUL << sh)) | ((u32)v << sh);
                        }
                    }
                    ST32(d, dv);
                }
                x += 4;
                s += 4;
                d += 4;
                if (x >= n) break;
                if (x + 4 > n) {
                    /* remaining 1..3 pixels: bytes */
                    for (; x < n; ++x) {
                        u8 p = *s++;
                        if (p) {
                            u8 f = gl[p];
                            *d = (f == 0xFF) ? res[p] : res[gt[*d + (f << 8)]];
                        }
                        ++d;
                    }
                    break;
                }
            }
        } else {
            u8* s = src;
            u8* d = dst;
            for (int x = 0; x < n; ++x) {
                u8 p = *s++;
                if (p) {
                    u8 f = gl[p];
                    *d = (f == 0xFF) ? res[p] : res[gt[*d + (f << 8)]];
                }
                ++d;
            }
        }
        src += width + src_pitch;
        dst += width + dst_pitch;
    }
}

typedef void (*Fn)(int, int, u8*, u8*, int, int, u8*, u8*, u8*, int);

static unsigned long long now(void)
{
    struct EClockVal ev;
    ReadEClock(&ev);
    return ((unsigned long long)ev.ev_hi << 32) | ev.ev_lo;
}

int main(int argc, char** argv)
{
    {
        ULONG cacr = CacheControl(0, 0);
        printf("CACR via CacheControl: 0x%08lx (bit31 EDC data cache, bit23 EBC branch cache, bit15 EIC insn cache, bit29 ESB store buffer)\n", (unsigned long)cacr);
    }
    ULONG oldcacr = CacheControl(0, 0);
    if (argc > 1 && argv[1][0] == 'w') {
        CacheControl(CACRF_WriteAllocate, CACRF_WriteAllocate);
        printf("write allocate ON, CACR now 0x%08lx\n", (unsigned long)CacheControl(0, 0));
    }
    port = CreateMsgPort();
    req = (struct timerequest*)CreateIORequest(port, sizeof(struct timerequest));
    if (!req || OpenDevice((CONST_STRPTR) "timer.device", UNIT_ECLOCK, (struct IORequest*)req, 0)) {
        printf("no timer\n");
        return 20;
    }
    TimerBase = req->tr_node.io_Device;
    ghost_tab = (u8*)AllocVec(256 * 256, MEMF_ANY);
    srand(7);
    for (int i = 0; i < 256; ++i) {
        ghost_lookup[i] = (i >= 253) ? (u8)(i & 1) : 0xFF; /* three shadow colours use two rows of the table */
        fade_tab[i] = (u8)rand();
    }
    for (int i = 0; i < 256 * 256; ++i) ghost_tab[i] = (u8)rand();

    {
        /* calibration: what do plain byte accesses cost here (ns per pixel of a 36x36 block, rows 640 apart) */
        const int iters = 2000;
        unsigned long long t0, t1;
        volatile u8* d;
        long acc = 0;
        t0 = now();
        for (int it = 0; it < iters; ++it) {
            d = dstA;
            for (int y = 0; y < H; ++y, d += PITCH - W)
                for (int x = 0; x < W; ++x) *d++ = (u8)x;
        }
        t1 = now();
        printf("calibration byte store      %ld ns/pixel\n", (long)((double)(t1 - t0) * 1e9 / 709379.0 / ((double)iters * W * H)));
        t0 = now();
        for (int it = 0; it < iters; ++it) {
            d = dstA;
            for (int y = 0; y < H; ++y, d += PITCH - W)
                for (int x = 0; x < W; ++x) acc += *d++;
        }
        t1 = now();
        printf("calibration byte load       %ld ns/pixel (acc %ld)\n", (long)((double)(t1 - t0) * 1e9 / 709379.0 / ((double)iters * W * H)), acc);
        t0 = now();
        for (int it = 0; it < iters; ++it) {
            d = dstA;
            for (int y = 0; y < H; ++y, d += PITCH - W)
                for (int x = 0; x < W; ++x) { *d = (u8)(*d + 1); d++; }
        }
        t1 = now();
        printf("calibration byte load+store %ld ns/pixel\n", (long)((double)(t1 - t0) * 1e9 / 709379.0 / ((double)iters * W * H)));
        t0 = now();
        for (int it = 0; it < iters; ++it) {
            const u8* sp = src_;
            for (int i = 0; i < W * H; ++i) acc += sp[i];
        }
        t1 = now();
        printf("calibration src sequential  %ld ns/pixel (acc %ld)\n", (long)((double)(t1 - t0) * 1e9 / 709379.0 / ((double)iters * W * H)), acc);
    }
    {
        /* store width: the same 36x36 block written with byte, word, long and 4-long (movem) stores */
        const int iters = 2000;
        unsigned long long t0, t1;
        t0 = now();
        for (int it = 0; it < iters; ++it) {
            volatile u32* d = (volatile u32*)dstA;
            for (int y = 0; y < H; ++y, d += (PITCH - W) / 4)
                for (int x = 0; x < W / 4; ++x) *d++ = 0x01020304UL;
        }
        t1 = now();
        printf("calibration long store      %ld ns/pixel\n", (long)((double)(t1 - t0) * 1e9 / 709379.0 / ((double)iters * W * H)));
        t0 = now();
        for (int it = 0; it < iters; ++it) {
            u8* d = dstA;
            for (int y = 0; y < H; ++y, d += PITCH) {
                u8* dd = d;
                for (int x = 0; x < W / 16; ++x, dd += 16) {
                    __asm__ volatile("movem.l %%d0-%%d3,(%0)" : : "a"(dd) : "memory");
                }
            }
        }
        t1 = now();
        printf("calibration movem 16B store %ld ns/pixel (32 of 36 px)\n", (long)((double)(t1 - t0) * 1e9 / 709379.0 / ((double)iters * 32 * H)));
        /* the same long stores into one 36-byte line buffer on the stack-like static (hot) */
        static u8 hot[64];
        t0 = now();
        for (int it = 0; it < iters * H; ++it) {
            volatile u32* d = (volatile u32*)hot;
            for (int x = 0; x < W / 4; ++x) *d++ = 0x01020304UL;
        }
        t1 = now();
        printf("calibration long store hot  %ld ns/pixel\n", (long)((double)(t1 - t0) * 1e9 / 709379.0 / ((double)iters * W * H)));
        t0 = now();
        for (int it = 0; it < iters * H; ++it) {
            volatile u8* d = hot;
            for (int x = 0; x < W; ++x) *d++ = (u8)x;
        }
        t1 = now();
        printf("calibration byte store hot  %ld ns/pixel\n", (long)((double)(t1 - t0) * 1e9 / 709379.0 / ((double)iters * W * H)));
    }
    {
        /* terrain icon 24x24 copied into a 640 pitch buffer: libnix memcpy per row, six long moves, movem.l */
        static u8 icon[24 * 24 + 16];
        for (int i = 0; i < 24 * 24; ++i) icon[i] = (u8)i;
        const int iters = 4000;
        unsigned long long t0, t1;
        for (int variant = 0; variant < 3; ++variant) {
            for (int off = 0; off < 2; ++off) {
                u8* base = dstA + off * 2; /* off 0: aligned destination, off 1: 2 bytes off */
                t0 = now();
                for (int it = 0; it < iters; ++it) {
                    u8* d = base;
                    const u8* sp = icon;
                    for (int y = 0; y < 24; ++y, d += PITCH, sp += 24) {
                        if (variant == 0) {
                            memcpy(d, sp, 24);
                        } else if (variant == 1) {
                            ST32(d, LD32(sp)); ST32(d + 4, LD32(sp + 4)); ST32(d + 8, LD32(sp + 8));
                            ST32(d + 12, LD32(sp + 12)); ST32(d + 16, LD32(sp + 16)); ST32(d + 20, LD32(sp + 20));
                        } else {
                            __asm__ volatile("movem.l (%0),%%d0-%%d5" "\n\t" "movem.l %%d0-%%d5,(%1)" : : "a"(sp), "a"(d) : "d0", "d1", "d2", "d3", "d4", "d5", "memory");
                        }
                    }
                }
                t1 = now();
                static const char* vn[] = {"memcpy per row", "6 long moves", "movem.l"};
                printf("icon 24x24 %-15s dst %s: %ld ns per icon\n", vn[variant], off ? "off by 2" : "aligned ", (long)((double)(t1 - t0) * 1e9 / 709379.0 / iters));
            }
        }
    }
    static const Fn fn[] = {V0, V1, V2, V3, V4, V5, V6, V7};
    static const char* name[] = {"V0 original", "V1 comb16 per pixel", "V2 res+gl per pixel", "V3 run skipping", "V4 run skipping + plain flag", "V5 V2 + touch dst lines", "V6 hand written 68k loop", "V7 4 px per long store"};
    static const int dens[] = {25, 60, 95};
    for (int di = 0; di < 3; ++di) {
        /* runs: opaque run lengths 2..12, transparent runs sized for the wanted density */
        for (int y = 0; y < H; ++y) {
            int x = 0;
            while (x < W) {
                int on = 2 + rand() % 11;
                int off = on * (100 - dens[di]) / dens[di];
                if (off < 1 && dens[di] < 100) off = 1;
                for (int k = 0; k < on && x < W; ++k, ++x) src_[y * W + x] = (u8)(1 + rand() % 255);
                for (int k = 0; k < off && x < W; ++k, ++x) src_[y * W + x] = 0;
            }
        }
        int opaque = 0;
        for (int i = 0; i < W * H; ++i) opaque += src_[i] != 0;
        printf("density %d%% (opaque %d of %d)\n", dens[di], opaque, W * H);
        for (int v = 0; v < 8; ++v) {
            memset(dstA, 0x55, sizeof(dstA));
            const int iters = 1500;
            unsigned long long t0 = now();
            for (int it = 0; it < iters; ++it) fn[v](W, H, dstA, src_, PITCH - W, 0, ghost_lookup, ghost_tab, fade_tab, 1);
            unsigned long long t1 = now();
            double ns = (double)(t1 - t0) * 1e9 / 709379.0 / ((double)iters * W * H);
            long n10 = (long)(ns * 10.0), o10 = (long)(ns * W * H / opaque * 10.0);
            printf("  %-30s %3ld.%ld ns/pixel  (%ld.%ld ns per opaque pixel)\n", name[v], n10 / 10, n10 % 10, o10 / 10, o10 % 10);
            /* equality against V0 */
            memset(dstB, 0x55, sizeof(dstB));
            V0(W, H, dstB, src_, PITCH - W, 0, ghost_lookup, ghost_tab, fade_tab, 1);
            memset(dstA, 0x55, sizeof(dstA));
            fn[v](W, H, dstA, src_, PITCH - W, 0, ghost_lookup, ghost_tab, fade_tab, 1);
            if (memcmp(dstA, dstB, sizeof(dstA))) printf("    RESULT DIFFERS from V0\n");
        }
    }
    CacheControl(oldcacr & CACRF_WriteAllocate, CACRF_WriteAllocate);
    CloseDevice((struct IORequest*)req);
    return 0;
}
