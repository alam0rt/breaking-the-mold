/* asset_constraint_sieve: enumerate compound prefix candidates against
 * cluster pair-hypothesis constraints.
 *
 * Input:
 *   TARGETS  - asset ids (one per line, decimal or 0xHEX)
 *   HYPS     - hypothesis file: "id_a,id_b,SUFFIX_A,SUFFIX_B" per line, all
 *              from the same swap class (or any cluster output of
 *              generate_pair_hypotheses.py)
 *   PREPENDS, BASE1, SEP1, BASE2, SEP2, BASE3, SEP3
 *              - text files of tokens. Cartesian product:
 *                  PREPEND + BASE1 + SEP1 + BASE2 + SEP2 + BASE3 + SEP3
 *              Each file may be empty (skipped) by passing literal "-".
 *              Empty lines in a file produce the empty token "".
 *
 * Build:
 *   gcc -O3 -o /tmp/asset_constraint_sieve tools/asset_constraint_sieve.c
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

/* Hypothesis hashtable: key=(h_P << 5) | sh_P, value=list of hypothesis records */
struct hyp {
    uint32_t key;       /* (h_P << 5) | sh_P */
    uint32_t id_a, id_b;
    char *Sa, *Sb;
    struct hyp *next;
};

#define HT_BUCKETS (1u << 22)
static struct hyp **ht = NULL;
static size_t n_hyps = 0;

static inline uint32_t ht_bucket(uint32_t key) {
    uint32_t k = key;
    k ^= k >> 16; k *= 0x7feb352du;
    k ^= k >> 15; k *= 0x846ca68bu;
    k ^= k >> 16;
    return k & (HT_BUCKETS - 1);
}

static void load_hypotheses(const char *path) {
    ht = calloc(HT_BUCKETS, sizeof *ht);
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(2); }
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;
        char *fields[4] = {0};
        int nf = 0;
        char *tok = strtok(p, ",\n\r");
        while (tok && nf < 4) {
            fields[nf++] = tok;
            tok = strtok(NULL, ",\n\r");
        }
        if (nf < 4) continue;
        uint32_t id_a = (fields[0][0] == '0' && (fields[0][1] | 32) == 'x')
                         ? (uint32_t)strtoul(fields[0] + 2, NULL, 16)
                         : (uint32_t)strtoul(fields[0], NULL, 10);
        uint32_t id_b = (fields[1][0] == '0' && (fields[1][1] | 32) == 'x')
                         ? (uint32_t)strtoul(fields[1] + 2, NULL, 16)
                         : (uint32_t)strtoul(fields[1], NULL, 10);
        const char *Sa = fields[2], *Sb = fields[3];
        uint32_t sh_Sa, sh_Sb;
        uint32_t h_Sa = calc_hash_with_shift(Sa, &sh_Sa);
        uint32_t h_Sb = calc_hash_with_shift(Sb, &sh_Sb);
        uint32_t V = h_Sa ^ h_Sb;
        if (V == 0) continue;
        uint32_t delta = id_a ^ id_b;
        int sh_P = -1;
        for (int sh = 0; sh < 32; sh++) {
            if (rotl(V, sh + OUT_ROT) == delta) { sh_P = sh; break; }
        }
        if (sh_P < 0) continue;
        uint32_t h_P = rotr(id_a ^ SEED, OUT_ROT) ^ rotl(h_Sa, sh_P);
        uint32_t key = (h_P << 5) | (sh_P & 31);

        struct hyp *h = malloc(sizeof *h);
        h->key = key;
        h->id_a = id_a; h->id_b = id_b;
        h->Sa = strdup(Sa); h->Sb = strdup(Sb);
        uint32_t b = ht_bucket(key);
        h->next = ht[b];
        ht[b] = h;
        n_hyps++;
    }
    fclose(f);
    fprintf(stderr, "hypotheses indexed: %zu (h_P, sh_P) constraints\n", n_hyps);
}

static struct hyp *lookup_hyp(uint32_t h_P, uint32_t sh_P) {
    uint32_t key = (h_P << 5) | (sh_P & 31);
    struct hyp *cur = ht[ht_bucket(key)];
    while (cur) {
        if (cur->key == key) return cur;
        cur = cur->next;
    }
    return NULL;
}

/* Token list loader */
struct word {
    char *text;
    uint32_t h, sh;
};
static struct word *load_word_file(const char *path, size_t *n_out,
                                   int min_alnum, int max_alnum) {
    struct word *out = NULL;
    size_t cap = 0, n = 0;
    if (path && strcmp(path, "-") != 0) {
        FILE *f = fopen(path, "r");
        if (!f) { perror(path); exit(2); }
        char line[512];
        while (fgets(line, sizeof line, f)) {
            size_t len = strlen(line);
            while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
            int alnum = 0;
            for (size_t i = 0; i < len; i++) if (isalnum((unsigned char)line[i])) alnum++;
            if (alnum < min_alnum || alnum > max_alnum) continue;
            if (n == cap) { cap = cap ? cap * 2 : 1024; out = realloc(out, cap * sizeof *out); }
            out[n].text = strdup(line);
            out[n].h = calc_hash_with_shift(line, &out[n].sh);
            n++;
        }
        fclose(f);
    }
    if (!n) {
        cap = 1; out = realloc(out, cap * sizeof *out);
        out[0].text = strdup("");
        out[0].h = 0; out[0].sh = 0;
        n = 1;
    }
    *n_out = n;
    return out;
}

int main(int argc, char **argv) {
    if (argc < 9) {
        fprintf(stderr, "usage: %s TARGETS HYPS PREPENDS BASE1 SEP1 BASE2 SEP2 BASE3 [SEP3]\n", argv[0]);
        return 2;
    }
    (void)argv[1]; /* targets unused — hypotheses encode them */
    load_hypotheses(argv[2]);
    if (n_hyps == 0) { fprintf(stderr, "no hypotheses loaded\n"); return 1; }

    size_t n_prep, n_b1, n_s1, n_b2, n_s2, n_b3, n_s3 = 1;
    struct word *prep = load_word_file(argv[3], &n_prep, 0, 12);
    struct word *b1   = load_word_file(argv[4], &n_b1, 1, 12);
    struct word *s1   = load_word_file(argv[5], &n_s1, 0, 3);
    struct word *b2   = load_word_file(argv[6], &n_b2, 0, 12);
    struct word *s2   = load_word_file(argv[7], &n_s2, 0, 3);
    struct word *b3   = load_word_file(argv[8], &n_b3, 0, 12);
    struct word *s3   = NULL;
    if (argc >= 10) {
        s3 = load_word_file(argv[9], &n_s3, 0, 3);
    } else {
        s3 = load_word_file("-", &n_s3, 0, 0);
    }
    fprintf(stderr, "prep=%zu b1=%zu s1=%zu b2=%zu s2=%zu b3=%zu s3=%zu combos=%zu\n",
            n_prep, n_b1, n_s1, n_b2, n_s2, n_b3, n_s3,
            n_prep * n_b1 * n_s1 * n_b2 * n_s2 * n_b3 * n_s3);

    char buf[1024];
    size_t hits = 0;
    size_t combos = 0;
    for (size_t i = 0; i < n_prep; i++) {
        uint32_t hp = prep[i].h, shp = prep[i].sh;
        for (size_t j = 0; j < n_b1; j++) {
            uint32_t h2 = hp ^ rotl(b1[j].h, shp);
            uint32_t sh2 = (shp + b1[j].sh) & 31;
            for (size_t k = 0; k < n_s1; k++) {
                uint32_t h3 = h2 ^ rotl(s1[k].h, sh2);
                uint32_t sh3 = (sh2 + s1[k].sh) & 31;
                for (size_t l = 0; l < n_b2; l++) {
                    uint32_t h4 = h3 ^ rotl(b2[l].h, sh3);
                    uint32_t sh4 = (sh3 + b2[l].sh) & 31;
                    for (size_t m = 0; m < n_s2; m++) {
                        uint32_t h5 = h4 ^ rotl(s2[m].h, sh4);
                        uint32_t sh5 = (sh4 + s2[m].sh) & 31;
                        for (size_t o = 0; o < n_b3; o++) {
                            uint32_t h6 = h5 ^ rotl(b3[o].h, sh5);
                            uint32_t sh6 = (sh5 + b3[o].sh) & 31;
                            for (size_t q = 0; q < n_s3; q++) {
                                uint32_t h7 = h6 ^ rotl(s3[q].h, sh6);
                                uint32_t sh7 = (sh6 + s3[q].sh) & 31;
                                combos++;
                                struct hyp *match = lookup_hyp(h7, sh7);
                                if (!match) continue;
                                /* Found! Iterate all matching hypotheses and
                                 * VERIFY each by reconstructing both asset
                                 * IDs from the prefix + suffix. */
                                uint32_t key = (h7 << 5) | (sh7 & 31);
                                struct hyp *cur = ht[ht_bucket(key)];
                                while (cur) {
                                    if (cur->key != key) { cur = cur->next; continue; }
                                    uint32_t sh_Sa, sh_Sb;
                                    uint32_t h_Sa = calc_hash_with_shift(cur->Sa, &sh_Sa);
                                    uint32_t h_Sb = calc_hash_with_shift(cur->Sb, &sh_Sb);
                                    uint32_t id_a_calc = SEED ^ rotl(h7 ^ rotl(h_Sa, sh7), OUT_ROT);
                                    uint32_t id_b_calc = SEED ^ rotl(h7 ^ rotl(h_Sb, sh7), OUT_ROT);
                                    if (id_a_calc == cur->id_a && id_b_calc == cur->id_b) {
                                        int n = snprintf(buf, sizeof buf,
                                            "0x%08x\t%u\t%s%s%s%s%s%s%s\t%s\t%s\t0x%08x\t0x%08x\n",
                                            h7, sh7,
                                            prep[i].text, b1[j].text, s1[k].text,
                                            b2[l].text, s2[m].text, b3[o].text, s3[q].text,
                                            cur->Sa, cur->Sb,
                                            cur->id_a, cur->id_b);
                                        if (n > 0) fwrite(buf, 1, (size_t)n, stdout);
                                        hits++;
                                    }
                                    cur = cur->next;
                                }
                            }
                        }
                    }
                }
            }
        }
        if ((i & 0xF) == 0xF) {
            fflush(stdout);
            fprintf(stderr, "  prep %zu/%zu  combos=%zu  hits=%zu\n",
                    i + 1, n_prep, combos, hits);
        }
    }
    fflush(stdout);
    fprintf(stderr, "scanned %zu combos, %zu hits\n", combos, hits);
    return 0;
}
