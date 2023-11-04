// Project: Salis
// Author:  Paul Oliver
// Email:   contact@pauloliver.dev

#include <argp.h>
#include <curses.h>
#include <locale.h>

#include "argp_basic.c"
#include "graphics.c"

#define DATA_WIDTH (27)

enum {
    PAGE_CORE,
    PAGE_PROCESS,
    PAGE_WORLD,
    PAGE_IPC,
    PAGE_COUNT
};

enum {
    ATTR_NOUSE,
    ATTR_NORMAL,
    ATTR_HEADER,
    ATTR_LIVE_PROC,
    ATTR_SELECTED_PROC,
    ATTR_FREE_CELL,
    ATTR_ALLOC_CELL,
    ATTR_MEM_BLOCK_START,
    ATTR_MEM_BLOCK_1,
    ATTR_MEM_BLOCK_N,
    ATTR_SELECTED_X1,
    ATTR_SELECTED_X2,
};

static int      g_exit;
static int      g_running;
static unsigned g_core;
static unsigned g_page;
static int      g_proc_genes;
static U64      g_proc_scroll;
static U64      g_proc_selected;
static U64      g_wrld_pos;
static U64      g_wrld_zoom;
static int      g_wcursor_mode;
static int      g_wcursor_x;
static int      g_wcursor_y;

static const wchar_t *ZOOMED_SYMBOLS = (
    L"⠀⠁⠂⠃⠄⠅⠆⠇⡀⡁⡂⡃⡄⡅⡆⡇⠈⠉⠊⠋⠌⠍⠎⠏⡈⡉⡊⡋⡌⡍⡎⡏⠐⠑⠒⠓⠔⠕⠖⠗⡐⡑⡒⡓⡔⡕⡖⡗⠘⠙⠚⠛⠜⠝⠞⠟⡘⡙⡚⡛⡜⡝⡞⡟"
    L"⠠⠡⠢⠣⠤⠥⠦⠧⡠⡡⡢⡣⡤⡥⡦⡧⠨⠩⠪⠫⠬⠭⠮⠯⡨⡩⡪⡫⡬⡭⡮⡯⠰⠱⠲⠳⠴⠵⠶⠷⡰⡱⡲⡳⡴⡵⡶⡷⠸⠹⠺⠻⠼⠽⠾⠿⡸⡹⡺⡻⡼⡽⡾⡿"
    L"⢀⢁⢂⢃⢄⢅⢆⢇⣀⣁⣂⣃⣄⣅⣆⣇⢈⢉⢊⢋⢌⢍⢎⢏⣈⣉⣊⣋⣌⣍⣎⣏⢐⢑⢒⢓⢔⢕⢖⢗⣐⣑⣒⣓⣔⣕⣖⣗⢘⢙⢚⢛⢜⢝⢞⢟⣘⣙⣚⣛⣜⣝⣞⣟"
    L"⢠⢡⢢⢣⢤⢥⢦⢧⣠⣡⣢⣣⣤⣥⣦⣧⢨⢩⢪⢫⢬⢭⢮⢯⣨⣩⣪⣫⣬⣭⣮⣯⢰⢱⢲⢳⢴⢵⢶⢷⣰⣱⣲⣳⣴⣵⣶⣷⢸⢹⢺⢻⢼⢽⢾⢿⣸⣹⣺⣻⣼⣽⣾⣿"
);

static void ui_line(
    int         line,
    int         clear,
    int         color,
    int         attr,
    const char *format,
    ...
) {
    if (line >= LINES) {
        return;
    }

    if (clear) {
        move(line, 0);
        clrtoeol();
    }

    char    dstr[64];
    va_list args;

    attron(COLOR_PAIR(color) | attr);

    va_start(args, format);
    vsprintf(dstr, format, args);
    mvprintw(line, 1, "%.*s", COLS - 1, dstr);
    va_end(args);

    attroff(COLOR_PAIR(color) | attr);
}

static void ui_estr(int l, const char *label, const char *value) {
    ui_line(l, 0, ATTR_NORMAL, 0, "%s : %18s", label, value);
}

static void ui_eulx(int l, const char *label, U64 value) {
    ui_line(l, 0, ATTR_NORMAL, 0, "%s : %#18lx", label, value);
}

static void ui_print_core(int l) {
    ui_line(++l, 0, ATTR_HEADER, A_BOLD, "CORE [%d]", g_core);
    ui_eulx(++l, "mall", g_cores[g_core].mall);
    ui_eulx(++l, "mut0", g_cores[g_core].muta[0]);
    ui_eulx(++l, "mut1", g_cores[g_core].muta[1]);
    ui_eulx(++l, "mut2", g_cores[g_core].muta[2]);
    ui_eulx(++l, "mut3", g_cores[g_core].muta[3]);
    ui_eulx(++l, "pnum", g_cores[g_core].pnum);
    ui_eulx(++l, "pcap", g_cores[g_core].pcap);
    ui_eulx(++l, "pfst", g_cores[g_core].pfst);
    ui_eulx(++l, "plst", g_cores[g_core].plst);
    ui_eulx(++l, "pcur", g_cores[g_core].pcur);
    ui_eulx(++l, "psli", g_cores[g_core].psli);
    ui_eulx(++l, "ncyc", g_cores[g_core].ncyc);
}

static int ui_proc_attr(U64 pi) {
    int bali = (pi >= g_cores[g_core].pfst && pi <= g_cores[g_core].plst);
    int bsel = (pi == g_proc_selected);
    int aali = bali ? ATTR_LIVE_PROC : ATTR_NORMAL;
    int asel = bsel ? ATTR_SELECTED_PROC : aali;

    return asel;
}

static const char *ui_proc_state(U64 pi) {
    int         bali = (pi >= g_cores[g_core].pfst && pi <= g_cores[g_core].plst);
    const char *sali = bali ? "live" : "dead";

    return sali;
}

static void ui_print_process_genes(int l, U64 pi) {
    if (pi >= g_cores[g_core].pcap) {
        return ui_line(l, 1, ATTR_NORMAL, 0, "");
    }

    ui_line(l, 0, ui_proc_attr(pi), 0, "%s : %#18lx :", ui_proc_state(pi), pi);
}

static void ui_print_process_fields(int l, U64 pi) {
    if (pi >= g_cores[g_core].pcap) {
        return ui_line(l, 1, ATTR_NORMAL, 0, "");
    }

    ui_line(l, 0, ui_proc_attr(pi), 0, "%s : %#18lx"
        #define PROC_FIELD(type, name) " : %#18lx"
            PROC_FIELDS
        #undef PROC_FIELD
            , ui_proc_state(pi), pi
        #define PROC_FIELD(type, name) , g_cores[g_core].pvec[pi].name
            PROC_FIELDS
        #undef PROC_FIELD
    );
}

static void ui_print_process(int l) {
    ui_line(++l, 1, ATTR_HEADER, A_BOLD,
        "PROCESS [vs:%#lx | ps:%#lx | pf:%#lx | pl:%#lx]", g_proc_scroll,
        g_proc_selected, g_cores[g_core].pfst, g_cores[g_core].plst
    );

    U64 pi = g_proc_scroll;

    if (g_proc_genes) {
        ui_line(++l, 0, ATTR_NORMAL, 0, "%s : %18s : %s", "stat", "pi", "genome");

        while (l < LINES) {
            ui_print_process_genes(++l, pi++);
        }
    } else {
        ui_line(++l, 0, ATTR_NORMAL, 0, "%s : %18s"
            #define PROC_FIELD(type, name) " : %18s"
                PROC_FIELDS
            #undef PROC_FIELD
                , "stat", "pi"
            #define PROC_FIELD(type, name) , #name
                PROC_FIELDS
            #undef PROC_FIELD
        );

        while (l < LINES) {
            ui_print_process_fields(++l, pi++);
        }
    }
}

static void ui_print_cell(U64 i, U64 r, U64 x, U64 y) {
    if (g_wrld_pos + i * g_wrld_zoom >= CONFIG_SIZE) {
        mvaddch(y, x, ' ');
        return;
    }

    wchar_t inst_nstr[2] = { L'\0', L'\0' };
    U64     inst_avrg    = g_inst_gfx[g_core][i] / g_wrld_zoom;
    cchar_t cchar        = { 0 };

    if (g_wrld_zoom == 1) {
        inst_nstr[0] = gfx_inst_to_symbol((U8)inst_avrg);
    } else {
        inst_nstr[0] = ZOOMED_SYMBOLS[(U8)inst_avrg];
    }

    int cell_attr;

    if (g_wcursor_mode && r == g_wcursor_x && y == g_wcursor_y) {
        cell_attr = ATTR_NORMAL;
    } else if (g_mps0_gfx[g_core][i] != 0) {
        cell_attr = ATTR_MEM_BLOCK_1;
    } else if (g_mpsX_gfx[g_core][i] != 0) {
        cell_attr = ATTR_MEM_BLOCK_N;
    } else if (g_mbst_gfx[g_core][i] != 0) {
        cell_attr = ATTR_MEM_BLOCK_START;
    } else if (g_mall_gfx[g_core][i] != 0) {
        cell_attr = ATTR_ALLOC_CELL;
    } else {
        cell_attr = ATTR_FREE_CELL;
    }

    setcchar(&cchar, inst_nstr, 0, cell_attr, NULL);
    mvadd_wch(y, x, &cchar);
}

static void ui_get_world_rng(U64 *vwdt, U64 *vsiz) {
    *vwdt = 0;
    *vsiz = 0;

    if (COLS >= DATA_WIDTH) {
        *vwdt = COLS - DATA_WIDTH;
        *vsiz = LINES * *vwdt;
    }
}

static void ui_print_wcursor_bar() {
    U64 vwdt = 0;
    U64 vsiz = 0;
    ui_get_world_rng(&vwdt, &vsiz);

    attron(COLOR_PAIR(ATTR_NORMAL));
    move(LINES - 1, 0);
    clrtoeol();

    U64 cpos  = g_wcursor_y * (COLS - DATA_WIDTH) + g_wcursor_x;
    U64 caddr = cpos * g_wrld_zoom + g_wrld_pos;

    char cmnem[32] = "----";

    if (g_wrld_zoom == 1 && caddr < CONFIG_SIZE) {
        U8 cinst = g_cores[g_core].mvec[caddr];

        gfx_inst_to_mnemonic(cinst, cmnem);
    }

    mvprintw(
        LINES - 1,
        1,
        (
            "cursor"
            " | x:%#x"
            " | y:%#x"
            " | addr:%#lx"
            " | isum:%#lx"
            " | iavr:%#lx"
            " | mall:%#lx"
            " | mbst:%#lx"
            " | mnem:%s"
        ),
        g_wcursor_x,
        g_wcursor_y,
        caddr,
        g_inst_gfx[g_core][cpos],
        g_inst_gfx[g_core][cpos] / g_wrld_zoom,
        g_mall_gfx[g_core][cpos],
        g_mbst_gfx[g_core][cpos],
        cmnem
    );
}

static void ui_print_world(int l) {
    U64 vwdt = 0;
    U64 vsiz = 0;
    ui_get_world_rng(&vwdt, &vsiz);

    ui_line(++l, 0, ATTR_HEADER, A_BOLD, "WORLD");
    ui_eulx(++l, "wrlp", g_wrld_pos);
    ui_eulx(++l, "wrlz", g_wrld_zoom);
    ui_eulx(++l, "psel", g_proc_selected);
    ui_eulx(++l, "pabs", g_proc_selected % g_cores[g_core].pcap);
    ui_eulx(++l, "vrng", vsiz * g_wrld_zoom);
    ui_estr(++l, "curs", g_wcursor_mode ? "on" : "off");
    ++l;

    ui_line(++l, 0, ATTR_HEADER, A_BOLD, "SELECTED");

    #define PROC_FIELD(type, name) \
        ui_eulx(++l, #name, \
            proc_get(&g_cores[g_core], g_proc_selected)->name \
        );
        PROC_FIELDS
    #undef PROC_FIELD

    gfx_render(g_core, g_wrld_zoom, g_wrld_pos, vsiz, g_proc_selected);

    if (g_wcursor_mode) {
        int xmax = COLS - DATA_WIDTH - 1;
        int ymax = LINES - 2;

        g_wcursor_x = (g_wcursor_x < xmax) ? g_wcursor_x : xmax;
        g_wcursor_y = (g_wcursor_y < ymax) ? g_wcursor_y : ymax;
    }

    for (U64 i = 0; i < vsiz; ++i) {
        U64 r = i % vwdt;
        U64 x = r + DATA_WIDTH;
        U64 y = i / vwdt;

        ui_print_cell(i, r, x, y);
    }

    if (g_wcursor_mode) {
        ui_print_wcursor_bar();
    }
}

static void ui_print_ipc(int l) {
    ui_line(++l, 0, ATTR_HEADER, A_BOLD, "IPC");
}

static void ui_print() {
    int l = 0;
    ui_line(++l, 0, ATTR_HEADER, A_BOLD, "SALIS [%d:%d]", g_core,
        CONFIG_CORE_COUNT
    );
    ui_estr(++l, "name", g_name);
    ui_eulx(++l, "seed", g_seed);
    ui_estr(++l, "arch", CONFIG_ARCH);
    ui_eulx(++l, "size", CONFIG_SIZE);
    ui_eulx(++l, "syni", CONFIG_SYNC_INTERVAL);
    ui_eulx(++l, "step", g_steps);
    ui_eulx(++l, "sync", g_syncs);

    switch (g_page) {
    case PAGE_CORE:
        ui_print_core(++l);
        break;
    case PAGE_PROCESS:
        ui_print_process(++l);
        break;
    case PAGE_WORLD:
        ui_print_world(++l);
        break;
    case PAGE_IPC:
        ui_print_ipc(++l);
        break;
    default:
        break;
    }
}

static void ev_vscroll(int ev) {
    switch (g_page) {
    case PAGE_PROCESS:
        switch (ev) {
        case 'w':
            g_proc_scroll += 1;
            break;
        case 's':
            g_proc_scroll -= g_proc_scroll ? 1 : 0;
            break;
        case 'q':
            g_proc_scroll = 0;
            break;
        default:
            break;
        }

        break;
    case PAGE_WORLD: {
        U64 vwdt = 0;
        U64 vsiz = 0;
        ui_get_world_rng(&vwdt, &vsiz);

        U64 vlnr = vwdt * g_wrld_zoom;
        U64 vrng = vsiz * g_wrld_zoom;

        switch (ev) {
        case 'W':
            g_wrld_pos += (g_wrld_pos + vrng < CONFIG_SIZE) ? vrng : 0;
            break;
        case 'S':
            g_wrld_pos -= (g_wrld_pos >= vrng) ? vrng : g_wrld_pos;
            break;
        case 'w':
            g_wrld_pos += (g_wrld_pos + vlnr < CONFIG_SIZE) ? vlnr : 0;
            break;
        case 's':
            g_wrld_pos -= (g_wrld_pos >= vlnr) ? vlnr : g_wrld_pos;
            break;
        case 'q':
            g_wrld_pos = 0;
            break;
        default:
            break;
        }

        break;
    }
    default:
        break;
    }
}

static void ev_hscroll(int ev) {
    switch (g_page) {
    case PAGE_WORLD:
        switch (ev) {
        case 'a':
            g_wrld_pos -= (g_wrld_pos >= g_wrld_zoom) ? g_wrld_zoom : g_wrld_pos;
            break;
        case 'd':
            g_wrld_pos += (g_wrld_pos + g_wrld_zoom < CONFIG_SIZE) ? g_wrld_zoom : 0;
            break;
        default:
            break;
        }

        break;
    default:
        break;
    }
}

static void ev_zoom(int ev) {
    U64 vwdt = 0;
    U64 vsiz = 0;
    ui_get_world_rng(&vwdt, &vsiz);

    U64 vrng = vsiz * g_wrld_zoom;

    switch (g_page) {
    case PAGE_WORLD:
        switch (ev) {
        case 'x':
            g_wrld_zoom *= (vwdt != 0 && vrng < CONFIG_SIZE) ? 2 : 1;
            break;
        case 'z':
            g_wrld_zoom /= (g_wrld_zoom != 1) ? 2 : 1;
            break;
        default:
            break;
        }

        break;
    default:
        break;
    }
}

static void ev_move_wcursor(int ev) {
    switch (ev) {
    case KEY_UP:
        g_wcursor_y -= (g_wcursor_y != 0) ? 1 : 0;
        break;
    case KEY_DOWN:
        g_wcursor_y += (g_wcursor_y < LINES - 2) ? 1 : 0;
        break;
    case KEY_LEFT:
        g_wcursor_x -= (g_wcursor_x != 0) ? 1 : 0;
        break;
    case KEY_RIGHT:
        g_wcursor_x += (g_wcursor_x < COLS - DATA_WIDTH - 1) ? 1 : 0;
        break;
    default:
        break;
    }
}

static void ev_sel_proc(int ev) {
    if (g_page != PAGE_PROCESS && g_page != PAGE_WORLD) {
        return;
    }

    switch (ev) {
    case 'o':
        g_proc_selected -= g_proc_selected ? 1 : 0;
        break;
    case 'p':
        g_proc_selected += 1;
        break;
    case 'f':
        g_proc_selected = g_cores[g_core].pfst;
        break;
    case 'l':
        g_proc_selected = g_cores[g_core].plst;
        break;
    default:
        break;
    }
}

static void ev_handle() {
    int ev = getch();

    if (g_page == PAGE_WORLD && g_wcursor_mode) {
        switch (ev) {
        case KEY_UP:
        case KEY_DOWN:
        case KEY_LEFT:
        case KEY_RIGHT:
            ev_move_wcursor(ev);
            return;
        default:
            break;
        }
    }

    switch (ev) {
    case KEY_SLEFT:
        clear();
        g_core = (g_core - 1) % CONFIG_CORE_COUNT;
        break;
    case KEY_SRIGHT:
        clear();
        g_core = (g_core + 1) % CONFIG_CORE_COUNT;
        break;
    case KEY_LEFT:
        clear();
        g_page = (g_page - 1) % PAGE_COUNT;
        break;
    case KEY_RIGHT:
        clear();
        g_page = (g_page + 1) % PAGE_COUNT;
        break;
    case KEY_RESIZE:
        clear();
        break;
    case 'W':
    case 'S':
    case 'w':
    case 's':
    case 'q':
        ev_vscroll(ev);
        break;
    case 'a':
    case 'd':
        ev_hscroll(ev);
        break;
    case 'z':
    case 'x':
        ev_zoom(ev);
        break;
    case 'o':
    case 'p':
    case 'f':
    case 'l':
        ev_sel_proc(ev);
        break;
    case 'g':
        if (g_page == PAGE_PROCESS) {
            clear();
            g_proc_genes = !g_proc_genes;
        }

        break;
    case 'c':
        if (g_page == PAGE_WORLD) {
            clear();
            g_wcursor_mode = !g_wcursor_mode;
        }

        break;
    case ' ':
        g_running = !g_running;
        nodelay(stdscr, g_running);
        break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '0':
        if (!g_running) {
            U64 cycles = 1 << (((ev - '0') ? (ev - '0') : 10) - 1);
            sl_step(cycles);
        }

        break;
    default:
        break;
    }
}

static void init(struct Arguments *arguments) {
    setlocale(LC_ALL, "");

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    start_color();
    init_color(COLOR_BLACK, 0, 0, 0);

    init_pair(ATTR_NORMAL,          COLOR_WHITE,  COLOR_BLACK  );
    init_pair(ATTR_HEADER,          COLOR_BLUE,   COLOR_BLACK  );
    init_pair(ATTR_LIVE_PROC,       COLOR_BLUE,   COLOR_BLACK  );
    init_pair(ATTR_SELECTED_PROC,   COLOR_YELLOW, COLOR_BLACK  );
    init_pair(ATTR_FREE_CELL,       COLOR_BLACK,  COLOR_BLUE   );
    init_pair(ATTR_ALLOC_CELL,      COLOR_BLACK,  COLOR_CYAN   );
    init_pair(ATTR_MEM_BLOCK_START, COLOR_BLACK,  COLOR_WHITE  );
    init_pair(ATTR_MEM_BLOCK_1,     COLOR_BLACK,  COLOR_YELLOW );
    init_pair(ATTR_MEM_BLOCK_N,     COLOR_BLACK,  COLOR_GREEN  );
    init_pair(ATTR_SELECTED_X1,     COLOR_BLACK,  COLOR_MAGENTA);
    init_pair(ATTR_SELECTED_X2,     COLOR_BLACK,  COLOR_RED    );

    sl_init(arguments->name, arguments->seed);

    g_wrld_zoom = 1;
}

static void exec() {
    while (!g_exit) {
        if (g_running) {
            sl_step(CONFIG_RUNNING_STEPS - (g_steps % CONFIG_RUNNING_STEPS));
        }

        ui_print();
        ev_handle();
    }
}

static void quit() {
    sl_free();
    endwin();
}

int main(int argc, char *argv[]) {
    struct Arguments arguments;

    arguments.name = "def.sim";
    arguments.seed = 0;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    init(&arguments);
    exec();
    quit();
}
