// Project: Salis
// Author:  Paul Oliver
// Email:   contact@pauloliver.dev

#define CONFIG_ARCH          "arch/dummy.c"
#define CONFIG_CORE_COUNT    (2)
#define CONFIG_SIZE          (1ull << 20)
#define CONFIG_SYNC_INTERVAL (1ull << 20)
#define CONFIG_RUNNING_STEPS (1ull << 22)
#define CONFIG_MUTA_RANGE    (1ull << 32)
#define CONFIG_THREAD_GAP    (0x100)
#define CONFIG_UI            "ui/curses.c"

