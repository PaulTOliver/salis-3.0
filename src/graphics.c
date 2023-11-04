// Project: Salis
// Author:  Paul Oliver
// Email:   contact@pauloliver.dev

static U64 g_inst_gfx[CONFIG_CORE_COUNT][CONFIG_SIZE];
static U64 g_mall_gfx[CONFIG_CORE_COUNT][CONFIG_SIZE];
static U64 g_mbst_gfx[CONFIG_CORE_COUNT][CONFIG_SIZE];
static U64 g_mps0_gfx[CONFIG_CORE_COUNT][CONFIG_SIZE];
static U64 g_mpsX_gfx[CONFIG_CORE_COUNT][CONFIG_SIZE];

static void gfx_accum_pixel(int zoom, U64 pos, U64 size, U64 mbp, U64 *arr) {
    assert(zoom > 0);
    assert(pos < CONFIG_SIZE);
    assert(mbp < CONFIG_SIZE);
    assert(arr);

    if (mbp < pos) {
        return;
    }

    U64 rel = (mbp - pos) / zoom;

    if (rel < size) {
        ++arr[rel];
    }
}

static void gfx_clear(int core, U64 size) {
    assert(core < CONFIG_CORE_COUNT);

    memset(g_inst_gfx[core], 0, sizeof(U64) * size);
    memset(g_mall_gfx[core], 0, sizeof(U64) * size);
    memset(g_mbst_gfx[core], 0, sizeof(U64) * size);
    memset(g_mps0_gfx[core], 0, sizeof(U64) * size);
    memset(g_mpsX_gfx[core], 0, sizeof(U64) * size);
}

static void gfx_render_inst_arr(int core, int zoom, U64 pos, U64 size) {
    assert(core < CONFIG_CORE_COUNT);
    assert(zoom > 0);
    assert(pos < CONFIG_SIZE);

    for (U64 i = 0; i < size && i < CONFIG_SIZE; ++i) {
        for (U64 j = 0; j < zoom; ++j) {
            U64 addr = pos + i * zoom + j;

            if (addr < CONFIG_SIZE) {
                U8 byte = g_cores[core].mvec[addr];

                g_inst_gfx[core][i] += byte;
                g_mall_gfx[core][i] += byte & MALL_MASK ? 1 : 0;
            }
        }
    }
}

static void gfx_render_mbst_arr(int core, int zoom, U64 pos, U64 size) {
    assert(core < CONFIG_CORE_COUNT);
    assert(zoom > 0);
    assert(pos < CONFIG_SIZE);

    for (U64 pi = g_cores[core].pfst; pi <= g_cores[core].plst; ++pi) {
        for (int i = 0; i < PROC_MB_COUNT; ++i) {
            U64 mba = proc_mb_addr(&g_cores[core], pi, i);

            gfx_accum_pixel(zoom, pos, size, mba, g_mbst_gfx[core]);
        }
    }
}

static void gfx_render_mem0_arr(int core, int zoom, U64 pos, U64 size, U64 psel) {
    assert(core < CONFIG_CORE_COUNT);
    assert(zoom > 0);
    assert(pos < CONFIG_SIZE);

    U64 mba = proc_mb_addr(&g_cores[core], psel, 0);
    U64 mbs = proc_mb_size(&g_cores[core], psel, 0);

    for (int i = 0; i < mbs; ++i) {
        gfx_accum_pixel(zoom, pos, size, mba + i, g_mps0_gfx[core]);
    }
}

static void gfx_render_memN_arr(int core, int zoom, U64 pos, U64 size, U64 psel) {
    assert(core < CONFIG_CORE_COUNT);
    assert(zoom > 0);
    assert(pos < CONFIG_SIZE);

    for (int i = 1; i < PROC_MB_COUNT; ++i) {
        U64 mba = proc_mb_addr(&g_cores[core], psel, i);
        U64 mbs = proc_mb_size(&g_cores[core], psel, i);

        for (int j = 0; j < mbs; ++j) {
            gfx_accum_pixel(zoom, pos, size, mba + j, g_mpsX_gfx[core]);
        }
    }
}

static void gfx_render(int core, int zoom, U64 pos, U64 size, U64 psel) {
    assert(core < CONFIG_CORE_COUNT);
    assert(zoom > 0);
    assert(pos < CONFIG_SIZE);

    gfx_clear(core, size);
    gfx_render_inst_arr(core, zoom, pos, size);
    gfx_render_mbst_arr(core, zoom, pos, size);
    gfx_render_mem0_arr(core, zoom, pos, size, psel);
    gfx_render_memN_arr(core, zoom, pos, size, psel);
}
