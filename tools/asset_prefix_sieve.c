/* asset_prefix_sieve: cluster-driven prefix recovery for Skullmonkeys.
 *
 * Build:
 *   gcc -O3 -o /tmp/asset_prefix_sieve tools/asset_prefix_sieve.c
 *
 * Usage:
 *   /tmp/asset_prefix_sieve TARGETS.txt PAIRS.txt WORDLIST.txt > hits.txt
 *
 * TARGETS  - one asset id per line (decimal or 0xHEX). All known target ids.
 * PAIRS    - "id_a,id_b,SUFFIX_A,SUFFIX_B" per line. Each entry is a
 *            hypothesis: "If id_a came from PREFIX+SUFFIX_A and id_b from
 *            PREFIX+SUFFIX_B, what prefix would explain it?"
 * WORDLIST - one candidate word per line.
 *
 * Algorithm:
 *   For each pair hypothesis (id_a, id_b, Sa, Sb):
 *     - Compute h_Sa, sh_Sa, h_Sb, sh_Sb
 *     - V = h_Sa ^ h_Sb
 *     - delta = id_a ^ id_b
 *     - Find sh_P (0..31) such that rotl(V, sh_P + 27) == delta
 *     - If no such sh_P exists, skip
 *     - h_P = rotr(id_a ^ SEED, 27) ^ rotl(h_Sa, sh_P)
 *     - Look up (h_P, sh_P) in wordlist hash table -> any matching word is
 *       a candidate prefix
 *   For each candidate prefix found, verify by computing all known asset IDs
 *   that should be in the cluster, and report.
 *
 * Output:
 *   "0xPREFIXHASH SHIFT WORD SUFFIX_A SUFFIX_B id_a id_b"
 */
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEED 0x28C0E011u
#define OUT_ROT 27

static inline uint32_t rotl(uint32_t v, int r) {
    r &= 31;
    return (v << r) | (v >> ((32 - r) & 31));
}
static inline uint32_t rotr(uint32_t v, int r) {
    r &= 31;
    return (v >> r) | (v << ((32 - r) & 31));
}

static uint32_t calc_hash_with_shift(const char *s, uint32_t *out_sh) {
    uint32_t h = 0, sh = 0;
    for (; *s; s++) {
        int c = (unsigned char)*s;
        int orig = c;
        if (c >= 'a' && c <= 'z') c -= 32;
        else if (c >= '0' && c <= '9') c += 22;
        if ((c >= 'A' && c <= 'Z') || (orig >= '0' && orig <= '9')) {
            sh = (sh + (c - 64)) & 31;
            h ^= 1u << sh;
        }
    }
    *out_sh = sh;
    return h;
}

/* Targets bitmap for cluster membership lookup */
#define BLOOM_BITS 22
#define BLOOM_SIZE (1u << BLOOM_BITS)
#define BLOOM_MASK (BLOOM_SIZE - 1)
static uint32_t targets_bloom[BLOOM_SIZE / 32];

static uint32_t *targets_arr = NULL;
static size_t n_targets = 0, cap_targets = 0;

static int cmp_u32(const void *a, const void *b) {
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return (x > y) - (x < y);
}
static int targets_contains(uint32_t id) {
    if (!((targets_bloom[(id & BLOOM_MASK) >> 5] >> (id & 31)) & 1u)) return 0;
    size_t lo = 0, hi = n_targets;
    while (lo < hi) {
        size_t mid = (lo + hi) >> 1;
        if (targets_arr[mid] < id) lo = mid + 1;
        else hi = mid;
    }
    return lo < n_targets && targets_arr[lo] == id;
}

static void load_targets(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(2); }
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;
        uint32_t v = (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
                     ? (uint32_t)strtoul(p + 2, NULL, 16)
                     : (uint32_t)strtoul(p, NULL, 10);
        if (n_targets == cap_targets) {
            cap_targets = cap_targets ? cap_targets * 2 : 1024;
            targets_arr = realloc(targets_arr, cap_targets * sizeof *targets_arr);
        }
        targets_arr[n_targets++] = v;
        targets_bloom[(v & BLOOM_MASK) >> 5] |= 1u << (v & 31);
    }
    fclose(f);
    qsort(targets_arr, n_targets, sizeof *targets_arr, cmp_u32);
    size_t w = 0;
    for (size_t i = 0; i < n_targets; i++) {
        if (w == 0 || targets_arr[w - 1] != targets_arr[i]) targets_arr[w++] = targets_arr[i];
    }
    n_targets = w;
}

/* Wordlist hashtable: key = (h_P << 5) | sh_P, value = linked list of words */
struct wlnode {
    char *word;
    uint32_t key;       /* combined */
    struct wlnode *next;
};

#define WL_BUCKETS (1u << 24)
static struct wlnode **wl_table = NULL;
static size_t n_words = 0;

static inline uint32_t wl_bucket(uint32_t key) {
    /* simple mix */
    uint32_t k = key;
    k ^= k >> 16;
    k *= 0x7feb352du;
    k ^= k >> 15;
    k *= 0x846ca68bu;
    k ^= k >> 16;
    return k & (WL_BUCKETS - 1);
}

static void load_wordlist(const char *path) {
    wl_table = calloc(WL_BUCKETS, sizeof *wl_table);
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(2); }
    char line[512];
    while (fgets(line, sizeof line, f)) {
        size_t len = strlen(line);
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
        /* Count alnum to skip useless lines */
        int alnum = 0;
        for (size_t i = 0; i < len; i++) if (isalnum((unsigned char)line[i])) alnum++;
        if (alnum < 1 || alnum > 24) continue;
        uint32_t sh;
        uint32_t h = calc_hash_with_shift(line, &sh);
        uint32_t key = (h << 5) | (sh & 31);
        struct wlnode *n = malloc(sizeof *n);
        n->word = strdup(line);
        n->key = key;
        n->next = wl_table[wl_bucket(key)];
        wl_table[wl_bucket(key)] = n;
        n_words++;
    }
    fclose(f);
    fprintf(stderr, "wordlist: %zu candidate words loaded\n", n_words);
}

static struct wlnode *wl_lookup(uint32_t h_P, uint32_t sh_P) {
    uint32_t key = (h_P << 5) | (sh_P & 31);
    struct wlnode *head = wl_table[wl_bucket(key)];
    /* Caller must filter by exact key */
    (void)key;
    return head;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: %s TARGETS PAIRS WORDLIST\n", argv[0]);
        return 2;
    }
    load_targets(argv[1]);
    fprintf(stderr, "targets loaded: %zu unique\n", n_targets);
    load_wordlist(argv[3]);

    FILE *pf = fopen(argv[2], "r");
    if (!pf) { perror(argv[2]); return 2; }
    char line[1024];
    size_t pairs = 0;
    size_t hits = 0;
    while (fgets(line, sizeof line, pf)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;
        char *fields[4] = {0};
        int nf = 0;
        char *tok = strtok(p, ",\n\r");
        while (tok && nf < 4) {
            while (*tok == ' ' || *tok == '\t') tok++;
            char *end = tok + strlen(tok);
            while (end > tok && (end[-1] == ' ' || end[-1] == '\t')) *--end = '\0';
            fields[nf++] = tok;
            tok = strtok(NULL, ",\n\r");
        }
        if (nf < 4) continue;
        uint32_t id_a = (fields[0][0] == '0' && (fields[0][1] == 'x' || fields[0][1] == 'X'))
                         ? (uint32_t)strtoul(fields[0] + 2, NULL, 16)
                         : (uint32_t)strtoul(fields[0], NULL, 10);
        uint32_t id_b = (fields[1][0] == '0' && (fields[1][1] == 'x' || fields[1][1] == 'X'))
                         ? (uint32_t)strtoul(fields[1] + 2, NULL, 16)
                         : (uint32_t)strtoul(fields[1], NULL, 10);
        const char *Sa = fields[2];
        const char *Sb = fields[3];
        uint32_t sh_Sa, sh_Sb;
        uint32_t h_Sa = calc_hash_with_shift(Sa, &sh_Sa);
        uint32_t h_Sb = calc_hash_with_shift(Sb, &sh_Sb);
        uint32_t V = h_Sa ^ h_Sb;
        if (V == 0) continue;
        uint32_t delta = id_a ^ id_b;
        /* Find sh_P such that rotl(V, sh_P + 27) == delta */
        int found_sh = -1;
        for (int sh = 0; sh < 32; sh++) {
            if (rotl(V, sh + OUT_ROT) == delta) {
                found_sh = sh;
                break;
            }
        }
        if (found_sh < 0) continue;
        pairs++;
        uint32_t sh_P = (uint32_t)found_sh;
        uint32_t h_P_a = rotr(id_a ^ SEED, OUT_ROT) ^ rotl(h_Sa, sh_P);
        uint32_t h_P_b = rotr(id_b ^ SEED, OUT_ROT) ^ rotl(h_Sb, sh_P);
        if (h_P_a != h_P_b) continue;  /* shouldn't happen if math is right */
        uint32_t h_P = h_P_a;
        uint32_t key = (h_P << 5) | (sh_P & 31);
        struct wlnode *head = wl_lookup(h_P, sh_P);
        for (struct wlnode *n = head; n; n = n->next) {
            if (n->key != key) continue;
            /* Verify by reconstructing both IDs */
            uint32_t hh_a = n->key >> 5; /* h_P */
            (void)hh_a;
            uint32_t id_a_calc = SEED ^ rotl(h_P ^ rotl(h_Sa, sh_P), OUT_ROT);
            uint32_t id_b_calc = SEED ^ rotl(h_P ^ rotl(h_Sb, sh_P), OUT_ROT);
            if (id_a_calc != id_a || id_b_calc != id_b) continue;
            printf("0x%08x\t%u\t%s\t%s\t%s\t0x%08x\t0x%08x\n",
                   h_P, sh_P, n->word, Sa, Sb, id_a, id_b);
            hits++;
        }
    }
    fclose(pf);
    fprintf(stderr, "processed %zu valid pairs, %zu prefix hits\n", pairs, hits);
    return 0;
}
