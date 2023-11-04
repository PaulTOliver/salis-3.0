// Project: Salis
// Author:  Paul Oliver
// Email:   contact@pauloliver.dev

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "config.h"

#define INST_MASK (0b01111111)
#define MALL_MASK (0b10000000)

typedef uint8_t  U8;
typedef uint32_t U32;
typedef uint64_t U64;

typedef struct Core Core;
typedef struct Proc Proc;

typedef thrd_t Thrd;

struct Core {
    U32 mall;

    U64 muta[4];
    U64 pnum;
    U64 pcap;
    U64 pfst;
    U64 plst;
    U64 pcur;
    U64 psli;

    U64 ncyc;

    Thrd thrd;
    U64  thri;

    Proc *pvec;
    U8    mvec[CONFIG_SIZE];
    U8    tgap[CONFIG_THREAD_GAP];
};

#include CONFIG_ARCH

struct Proc {
    #define PROC_FIELD(type, name) type name;
        PROC_FIELDS
    #undef PROC_FIELD
};

static const char *g_name;
static U64         g_seed;

static U64  g_steps;
static U64  g_syncs;
static Core g_cores[CONFIG_CORE_COUNT];

static int mvec_is_alloc(const Core *core, U32 addr) {
    assert(core);
    assert(addr < CONFIG_SIZE);
    return (core->mvec[addr] & MALL_MASK) ? 1 : 0;
}

static void mvec_alloc(Core *core, U32 addr) {
    assert(core);
    assert(addr < CONFIG_SIZE);
    assert(!mvec_is_alloc(core, addr));
    core->mvec[addr] |= MALL_MASK;
}

static void mvec_free(Core *core, U32 addr) {
    assert(core);
    assert(addr < CONFIG_SIZE);
    assert(mvec_is_alloc(core, addr));
    core->mvec[addr] ^= MALL_MASK;
}

static void mvec_flip_bit(Core *core, U32 addr, int nb) {
    assert(core);
    assert(addr < CONFIG_SIZE);
    assert(nb < 8);
    core->mvec[addr] ^= (1 << nb) & INST_MASK;
}

static U64 muta_smix(U64 *seed) {
    assert(seed);

    U64 next = (*seed += 0x9e3779b97f4a7c15);
    next     = (next ^ (next >> 30)) * 0xbf58476d1ce4e5b9;
    next     = (next ^ (next >> 27)) * 0x94d049bb133111eb;

    return next ^ (next >> 31);
}

static U64 muta_ro64(U64 x, int k) {
    return (x << k) | (x >> (64 - k));
}

static U64 muta_next(Core *core) {
    assert(core);

    U64 r = muta_ro64(core->muta[1] * 5, 7) * 9;
    U64 t = core->muta[1] << 17;

    core->muta[2] ^= core->muta[0];
    core->muta[3] ^= core->muta[1];
    core->muta[1] ^= core->muta[2];
    core->muta[0] ^= core->muta[3];

    core->muta[2] ^= t;
    core->muta[3]  = muta_ro64(core->muta[3], 45);

    return r;
}

static void muta_cosmic_ray(Core *core) {
    assert(core);

    U64 a = muta_next(core) % CONFIG_MUTA_RANGE;
    U64 b = muta_next(core) % 8;

    if (a < CONFIG_SIZE) {
        mvec_flip_bit(core, a, (int)b);
    }
}

static void proc_new(Core *core) {
    assert(core);
}

static void proc_kill(Core *core) {
    assert(core);
}

static Proc *proc_get(Core *core, U64 pi) {
    assert(core);
    assert(pi >= core->pfst && pi <= core->plst);
    return &core->pvec[pi % core->pcap];
}

static void core_init(Core *core, U64 *seed) {
    assert(core);
    assert(seed);

    if (*seed) {
        core->muta[0] = muta_smix(seed);
        core->muta[1] = muta_smix(seed);
        core->muta[2] = muta_smix(seed);
        core->muta[3] = muta_smix(seed);
    }

    core->pnum = 1;
    core->pcap = 1;

    core->pvec = calloc(1, sizeof(Proc));
    assert(core->pvec);

    mvec_init(core);
}

static void core_step(Core *core) {
    assert(core);

    if (core->psli != 0) {
        --core->psli;
        return proc_step(core, core->pcur);
    }

    if (core->pcur != core->plst) {
        core->psli = proc_slice(core, ++core->pcur);
        return core_step(core);
    }

    core->pcur = core->pfst;
    core->psli = proc_slice(core, core->pcur);

    ++core->ncyc;
    muta_cosmic_ray(core);

    return core_step(core);
}

static void sl_init(const char *name, U64 seed) {
    assert(name);

    g_name = name;
    g_seed = seed;

    for (int i = 0; i < CONFIG_CORE_COUNT; ++i) {
        core_init(&g_cores[i], &seed);
    }
}

static void sl_core_thread(Core *core) {
    assert(core);

    for (U64 i = 0; i < core->thri; ++i) {
        core_step(core);
    }
}

static void sl_core_step(U64 ns) {
    for (int i = 0; i < CONFIG_CORE_COUNT; ++i) {
        g_cores[i].thri = ns;

        thrd_create(&g_cores[i].thrd, (thrd_start_t)sl_core_thread, &g_cores[i]);
    }

    for (int i = 0; i < CONFIG_CORE_COUNT; ++i) {
        thrd_join(g_cores[i].thrd, NULL);
    }

    g_steps += ns;
}

static void sl_core_sync() {
    ++g_syncs;
}

static void sl_core_loop(U64 ns, U64 dt) {
    assert(dt);

    if (ns < dt) {
        return sl_core_step(ns);
    }

    sl_core_step(dt);
    sl_core_sync();
    sl_core_loop(ns - dt, CONFIG_SYNC_INTERVAL);
}

static void sl_step(U64 ns) {
    assert(ns);

    U64 dt = CONFIG_SYNC_INTERVAL - (g_steps % CONFIG_SYNC_INTERVAL);

    sl_core_loop(ns, dt);
}

static void sl_free() {
    for (int i = 0; i < CONFIG_CORE_COUNT; ++i) {
        assert(g_cores[i].pvec);
        free(g_cores[i].pvec);
    }
}

#include CONFIG_UI
