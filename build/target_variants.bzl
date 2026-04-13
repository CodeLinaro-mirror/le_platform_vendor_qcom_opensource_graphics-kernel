targets = [
    # keep sorted
    "art",
    "bengal",
    "bluey",
    "canoe",
    "chora",
    "lahaina",
    "malabar",
    "monaco",
    "parrot",
    "sun",
    "vienna",
]

target_16k = [
    "art16k",
]

la_variants = [
    # keep sorted
    "consolidate",
    "perf",
]

gki_targets = [
    # keep sorted
    "anorak",
    "autoghgvm",
    "autogvm",
    "blair",
    "neo-la",
    "niobe",
    "pitti",
    "sdmsteppeauto",
]

gki_variants = [
    # keep sorted
    "consolidate",
    "gki",
]

gki_perf_targets = [
    # keep sorted
    "gen3auto",
    "hamoa",
    "pineapple",
    "seraph",
]

gki_perf_variants = [
    # keep sorted
    "consolidate",
    "gki",
    "perf",
]

le_targets = [
    # keep sorted
    "vienna-le",
]

le_variants = [
    # keep sorted
    "debug-defconfig",
    "defconfig",
]

def get_16k_tv():
    tv = [(t, v) for t in target_16k for v in la_variants]
    return tv

def get_all_variants():
    tv = [(t, v) for t in targets for v in la_variants]
    tv = tv + [(t, v) for t in gki_targets for v in gki_variants]
    tv = tv + [(t, v) for t in gki_perf_targets for v in gki_perf_variants]
    tv = tv + [(t, v) for t in le_targets for v in le_variants]
    tv = tv + [(t, v) for t in target_16k for v in la_variants]

    return tv
