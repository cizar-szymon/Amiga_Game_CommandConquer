// Test of the faster LCW_Comp (common/lcw.cpp) against the plain one of the original (lcw_old.cpp, the same code under
// other names): the compressed data must unpack to the original bytes, and it must not be much bigger. Run on the Amiga
// (WinUAE is enough): lcwtest [file]   - the file (e.g. a program) is compressed in blocks of 4096 as well.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

int LCW_Uncompress(void const* source, void* dest, unsigned length);
int LCW_Comp(void const* source, void* dest, unsigned length);
int LCW_Uncompress_Old(void const* source, void* dest, unsigned length);
int LCW_Comp_Old(void const* source, void* dest, unsigned length);

static unsigned long seed = 12345;
static unsigned Rand()
{
    seed = seed * 1103515245UL + 12345UL;
    return (unsigned)(seed >> 16) & 0x7fff;
}

static unsigned char src[70000], out[70000], c_new[140000], c_old[140000];
static long tot_new, tot_old, tot_raw, bad, runs;
static long t_new, t_old; // clock() ticks

static void Run(const char* name, const unsigned char* data, unsigned size)
{
    clock_t t0 = clock();
    int cn = LCW_Comp(data, c_new, size);
    clock_t t1 = clock();
    int co = LCW_Comp_Old(data, c_old, size);
    clock_t t2 = clock();
    t_new += (long)(t1 - t0);
    t_old += (long)(t2 - t1);
    std::memset(out, 0xA5, sizeof(out));
    int un = LCW_Uncompress(c_new, out, size);
    bool ok = (un == (int)size) && std::memcmp(out, data, size) == 0;
    // the old decoder as a second opinion (the same format)
    std::memset(out, 0x5A, sizeof(out));
    int uo = LCW_Uncompress_Old(c_new, out, size);
    ok = ok && (uo == (int)size) && std::memcmp(out, data, size) == 0;
    tot_new += cn;
    tot_old += co;
    tot_raw += size;
    runs++;
    if (!ok) {
        bad++;
        std::printf("BAD %s size %u: new %d old %d unpacked %d / %d\n", name, size, cn, co, un, uo);
    }
}

int main(int argc, char** argv)
{
    unsigned sizes[] = {1, 2, 3, 4, 7, 10, 64, 65, 100, 1000, 4096, 5000, 12000};
    for (unsigned s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++) {
        unsigned n = sizes[s];
        for (int kind = 0; kind < 7; kind++) {
            for (unsigned i = 0; i < n; i++) {
                switch (kind) {
                case 0: src[i] = (unsigned char)Rand(); break;                   // random
                case 1: src[i] = 0; break;                                       // zeros
                case 2: src[i] = (unsigned char)(i % 7); break;                  // short period
                case 3: src[i] = (unsigned char)((i / 32 == 0 ? 0 : (i % 32 < 4 ? Rand() & 3 : 0))); break; // records
                case 4: src[i] = (unsigned char)("the quick brown fox jumps over the lazy dog "[i % 44] + (Rand() % 50 == 0)); break;
                case 5: src[i] = (unsigned char)((i * i) >> 5); break;           // slowly changing
                default: src[i] = (unsigned char)(Rand() % 4); break;            // 2 bits of noise
                }
            }
            char name[32];
            std::snprintf(name, sizeof(name), "kind%d", kind);
            Run(name, src, n);
        }
    }
    if (argc > 1) {
        FILE* f = std::fopen(argv[1], "rb");
        if (f) {
            int blocks = 0;
            while (blocks < 300 && std::fread(src, 1, 4096, f) == 4096) {
                Run("file", src, 4096);
                blocks++;
            }
            std::fclose(f);
            std::printf("file blocks: %d\n", blocks);
        }
    }
    std::printf("runs %ld, bad %ld\n", runs, bad);
    std::printf("raw %ld bytes: new %ld, old %ld (new is %ld per mille of old)\n", tot_raw, tot_new, tot_old,
                tot_old ? tot_new * 1000 / tot_old : 0L);
    std::printf("time: new %ld ticks, old %ld ticks (%ld ticks per second)\n", t_new, t_old, (long)CLOCKS_PER_SEC);
    return bad ? 10 : 0;
}
