// Project: Salis
// Author:  Paul Oliver
// Email:   contact@pauloliver.dev

static void mvec_alloc(Core *core, U32 addr);

#define PROC_MB_COUNT (3)

#define PROC_FIELDS \
    PROC_FIELD(U32, dmmy)

#define DUMMY_PROC_MB_SIZE  (8)
#define DUMMY_PROC_MB0_ADDR (8 + 16 * 0)
#define DUMMY_PROC_MB1_ADDR (8 + 16 * 1)
#define DUMMY_PROC_MB2_ADDR (8 + 16 * 2)

static void mvec_init(Core *core) {
    assert(core);

    for (int j = 0; j < DUMMY_PROC_MB_SIZE; ++j) {
        mvec_alloc(core, DUMMY_PROC_MB0_ADDR + j);
        mvec_alloc(core, DUMMY_PROC_MB1_ADDR + j);
        mvec_alloc(core, DUMMY_PROC_MB2_ADDR + j);
    }
}

static U64 proc_mb_addr(const Core *core, U64 pi, int mbi) {
    assert(core);
    assert(pi >= core->pfst && pi <= core->plst);
    assert(mbi < PROC_MB_COUNT);

    switch (mbi) {
    case 0:
        return DUMMY_PROC_MB0_ADDR;
    case 1:
        return DUMMY_PROC_MB1_ADDR;
    case 2:
        return DUMMY_PROC_MB2_ADDR;
    default:
        return 0;
    }
}

static U64 proc_mb_size(const Core *core, U64 pi, int mbi) {
    assert(core);
    assert(pi >= core->pfst && pi <= core->plst);
    assert(mbi < PROC_MB_COUNT);
    return DUMMY_PROC_MB_SIZE;
}

static U64 proc_slice(Core *core, U64 pi) {
    assert(core);
    assert(pi >= core->pfst && pi <= core->plst);
    return 1;
}

static void proc_step(Core *core, U64 pi) {
    assert(core);
    assert(pi >= core->pfst && pi <= core->plst);
    return;
}

static const wchar_t *PROC_SYMBOLS = (
    L"⠀⠁⠂⠃⠄⠅⠆⠇⡀⡁⡂⡃⡄⡅⡆⡇⠈⠉⠊⠋⠌⠍⠎⠏⡈⡉⡊⡋⡌⡍⡎⡏⠐⠑⠒⠓⠔⠕⠖⠗⡐⡑⡒⡓⡔⡕⡖⡗⠘⠙⠚⠛⠜⠝⠞⠟⡘⡙⡚⡛⡜⡝⡞⡟"
    L"⠠⠡⠢⠣⠤⠥⠦⠧⡠⡡⡢⡣⡤⡥⡦⡧⠨⠩⠪⠫⠬⠭⠮⠯⡨⡩⡪⡫⡬⡭⡮⡯⠰⠱⠲⠳⠴⠵⠶⠷⡰⡱⡲⡳⡴⡵⡶⡷⠸⠹⠺⠻⠼⠽⠾⠿⡸⡹⡺⡻⡼⡽⡾⡿"
    L"⢀⢁⢂⢃⢄⢅⢆⢇⣀⣁⣂⣃⣄⣅⣆⣇⢈⢉⢊⢋⢌⢍⢎⢏⣈⣉⣊⣋⣌⣍⣎⣏⢐⢑⢒⢓⢔⢕⢖⢗⣐⣑⣒⣓⣔⣕⣖⣗⢘⢙⢚⢛⢜⢝⢞⢟⣘⣙⣚⣛⣜⣝⣞⣟"
    L"⢠⢡⢢⢣⢤⢥⢦⢧⣠⣡⣢⣣⣤⣥⣦⣧⢨⢩⢪⢫⢬⢭⢮⢯⣨⣩⣪⣫⣬⣭⣮⣯⢰⢱⢲⢳⢴⢵⢶⢷⣰⣱⣲⣳⣴⣵⣶⣷⢸⢹⢺⢻⢼⢽⢾⢿⣸⣹⣺⣻⣼⣽⣾⣿"
);

static wchar_t gfx_inst_to_symbol(U8 inst) {
    return PROC_SYMBOLS[inst];
}

static void gfx_inst_to_mnemonic(U8 inst, char *mnem) {
    assert(mnem);
    sprintf(mnem, "dmmy %#x", inst);
}
