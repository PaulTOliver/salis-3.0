// Project: Salis
// Author:  Paul Oliver
// Email:   contact@pauloliver.dev

int main(int argc, char *argv[]) {
    printf("hello salis\n");

    U64 seed;
    U64 steps;

    sscanf(argv[1], "%llx", &seed);
    sscanf(argv[2], "%llu", &steps);

    sl_init(seed);
    sl_step(steps);

    printf("seed         => %llx\n", seed);
    printf("g_steps      => %llu\n", g_steps);
    printf("g_syncs      => %llu\n", g_syncs);

    for (int i = 0; i < CONFIG_CORE_COUNT; ++i) {
        putchar('\n');
        printf("core %d mall  => %llx\n", i, g_cores[i].mall);
        printf("core %d mut0  => %llx\n", i, g_cores[i].muta[0]);
        printf("core %d mut1  => %llx\n", i, g_cores[i].muta[1]);
        printf("core %d mut2  => %llx\n", i, g_cores[i].muta[2]);
        printf("core %d mut3  => %llx\n", i, g_cores[i].muta[3]);
        printf("core %d pnum  => %llx\n", i, g_cores[i].pnum);
        printf("core %d pcap  => %llx\n", i, g_cores[i].pcap);
        printf("core %d pfst  => %llx\n", i, g_cores[i].pfst);
        printf("core %d plst  => %llx\n", i, g_cores[i].plst);
        printf("core %d pcur  => %llx\n", i, g_cores[i].pcur);
        printf("core %d psli  => %llu\n", i, g_cores[i].psli);
        printf("core %d ncyc  => %llu\n", i, g_cores[i].ncyc);
        putchar('\n');

        for (int j = 0; j < 32; ++j) {
            printf("%d ", g_cores[i].mvec[j]);
        }

        putchar('\n');
    }

    sl_free();
}
