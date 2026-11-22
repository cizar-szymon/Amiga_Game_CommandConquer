// smoke.cpp - F0 smoke test: C++ (libnix) program that opens a 640x400x8 RTG
// screen, loads a gradient palette, writes pixels with WritePixelArray8-style
// chunky copy and exits after ~3 s or Ctrl-C. Writes a log to RAM:smoke.txt.
#include <stdio.h>
#include <string.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/modeid.h>
#include <dos/dos.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/cybergraphics.h>

struct Library *CyberGfxBase;
extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *GfxBase;

static const int W = 640, H = 400;

struct Counter {
    int n;
    Counter() : n(42) {}   // static constructor must run (C++ startup check)
};
static Counter g_counter;

int main()
{
    FILE *log = fopen("RAM:smoke.txt", "w");
    if (log) fprintf(log, "smoke start, ctor n=%d\n", g_counter.n);

    CyberGfxBase = OpenLibrary((CONST_STRPTR)"cybergraphics.library", 0);
    ULONG id = BestModeID(BIDTAG_NominalWidth, W, BIDTAG_NominalHeight, H, BIDTAG_Depth, 8, TAG_DONE);
    if (log) fprintf(log, "cgx=%p modeid=0x%08lx\n", (void *)CyberGfxBase, (unsigned long)id);

    struct Screen *scr = OpenScreenTags(NULL, SA_Width, W, SA_Height, H, SA_Depth, 8,
                                        SA_DisplayID, id, SA_Quiet, TRUE, SA_Title, (ULONG)"smoke", TAG_DONE);
    if (!scr) {
        if (log) { fprintf(log, "OpenScreen failed\n"); fclose(log); }
        return 10;
    }
    struct Window *win = OpenWindowTags(NULL, WA_CustomScreen, (ULONG)scr, WA_Left, 0, WA_Top, 0,
                                        WA_Width, W, WA_Height, H, WA_Borderless, TRUE,
                                        WA_Backdrop, TRUE, WA_Activate, TRUE, WA_RMBTrap, TRUE, TAG_DONE);
    if (win) {
        ULONG pal[2 + 256 * 3];
        pal[0] = (256UL << 16) | 0;
        for (int i = 0; i < 256; i++) {
            ULONG v = (ULONG)i * 0x01010101UL;
            pal[1 + i * 3 + 0] = v;
            pal[1 + i * 3 + 1] = (ULONG)((255 - i) * 0x01010101UL);
            pal[1 + i * 3 + 2] = (ULONG)((i * 3 & 255) * 0x01010101UL);
        }
        pal[1 + 256 * 3] = 0;
        LoadRGB32(&scr->ViewPort, pal);

        UBYTE *buf = (UBYTE *)AllocVec(W * H, MEMF_ANY);
        if (buf) {
            for (int y = 0; y < H; y++)
                for (int x = 0; x < W; x++)
                    buf[y * W + x] = (UBYTE)(x * 256 / W);
            WritePixelArray(buf, 0, 0, W, win->RPort, 0, 0, W, H, RECTFMT_LUT8);
            FreeVec(buf);
        }
        if (log) fprintf(log, "screen %dx%d depth %d drawn\n", W, H, 8);
        for (int t = 0; t < 150; t++) {
            Delay(1);
            if (SetSignal(0, 0) & SIGBREAKF_CTRL_C) break;
        }
        CloseWindow(win);
    }
    CloseScreen(scr);
    if (CyberGfxBase) CloseLibrary(CyberGfxBase);
    if (log) { fprintf(log, "smoke done\n"); fclose(log); }
    return 0;
}
