// Project: Salis
// Author:  Paul Oliver
// Email:   contact@pauloliver.dev

const char *argp_program_version     = "Salis-3.0";
const char *argp_program_bug_address = "<contact@pauloliver.dev>";

static char argp_desc[] =
    "Salis - artificial life simulator based on Tom Ray's Tierra.";

static struct argp_option options[] = {
    { "name", 'n', "NAME", 0, "File name for new or saved simulation"},
    { "seed", 's', "SEED", 0, "Seed for PRNG (0 disables mutations)"},
    { 0 }
};

struct Arguments {
    U64   seed;
    char *name;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct Arguments *arguments = state->input;

    switch (key) {
    case 's':
        sscanf(arg, "%lx", &arguments->seed);
        return 0;
    case 'n':
        arguments->name = arg;
        return 0;
    case ARGP_KEY_ARG:
        return 0;
    default:
        return ARGP_ERR_UNKNOWN;
    }
}

static struct argp argp = { options, parse_opt, NULL, argp_desc, 0, 0, 0 };
