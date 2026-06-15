/* asset_grammar_attack: Cartesian product grammar attack on Skullmonkeys asset
 * hash. Reads a prefix wordlist + suffix wordlist + (optional) prepend file,
 * and prints every combination prepend + prefix + sep + suffix whose
 * Skullmonkeys asset_id is in a target set.
 *
 * Uses the cumulative calcHash decomposition for speed:
 *
 *     calcHash(A || B) = calcHash(A) XOR rotl(calcHash(B), shift_A)
 *
 * so we precompute (h, sh) for each word and combine with rotl + xor instead of
 * re-running the per-character loop.
 *
 * Build:
 *     gcc -O3 -march=native -o /tmp/asset_grammar tools/asset_grammar_attack.c
 *
 * Usage:
 *     /tmp/asset_grammar TARGETS PREFIXES SUFFIXES [PREPENDS] [SEPS] > hits.txt
 *
 * TARGETS: text file with one id per line (decimal or 0xHEX), '#' comments ok.
 * PREFIXES, SUFFIXES, PREPENDS, SEPS: text files, one entry per line. Empty
 *   line counts as "" (e.g. include a blank line to test the no-prepend case).
 *
 * Defaults if missing:
 *   PREPENDS: just "" (no prepend)
 *   SEPS:     "" and "_"
 *
 * Output: one TSV line per hit:
 *     0xDEADBEEF\tFULL_NAME\tPREPEND\tPREFIX\tSEP\tSUFFIX
 *
 * Output is buffered; not safe to interleave with stderr in pipelines other
 * than newline-flushed. Performance: 100-300 million combinations per second
 * on a modern x86.
 */
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline uint32_t rotl(uint32_t v, int r) {
    r &= 31;
    return (v << r) | (v >> ((32 - r) & 31));
}

static uint32_t calc_hash_with_shift(const char *s, uint32_t *out_sh) {
    uint32_t h = 0;
    uint32_t sh = 0;
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

#define BLOOM_BITS 22
#define BLOOM_SIZE (1u << BLOOM_BITS)
#define BLOOM_MASK (BLOOM_SIZE - 1)
static uint32_t bloom[BLOOM_SIZE / 32];

static inline void bloom_set(uint32_t id) {
    uint32_t h = id & BLOOM_MASK;
    bloom[h >> 5] |= 1u << (h & 31);
}
static inline int bloom_maybe(uint32_t id) {
    uint32_t h = id & BLOOM_MASK;
    return (bloom[h >> 5] >> (h & 31)) & 1u;
}

static uint32_t *targets = NULL;
static size_t n_targets = 0;
static size_t cap_targets = 0;

static int cmp_u32(const void *a, const void *b) {
    uint32_t x = *(const uint32_t *)a;
    uint32_t y = *(const uint32_t *)b;
    return (x > y) - (x < y);
}

static int target_contains(uint32_t id) {
    size_t lo = 0, hi = n_targets;
    while (lo < hi) {
        size_t mid = (lo + hi) >> 1;
        if (targets[mid] < id) lo = mid + 1;
        else hi = mid;
    }
    return lo < n_targets && targets[lo] == id;
}

static void load_targets(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "open %s: %s\n", path, strerror(errno)); exit(2); }
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;
        uint32_t v;
        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
            v = (uint32_t)strtoul(p + 2, NULL, 16);
        } else {
            v = (uint32_t)strtoul(p, NULL, 10);
        }
        if (n_targets == cap_targets) {
            cap_targets = cap_targets ? cap_targets * 2 : 1024;
            targets = realloc(targets, cap_targets * sizeof *targets);
        }
        targets[n_targets++] = v;
        bloom_set(v);
    }
    fclose(f);
    qsort(targets, n_targets, sizeof *targets, cmp_u32);
    size_t w = 0;
    for (size_t i = 0; i < n_targets; i++) {
        if (w == 0 || targets[w - 1] != targets[i]) targets[w++] = targets[i];
    }
    n_targets = w;
}

struct word {
    char *text;
    uint32_t h;
    uint32_t sh;
};

static struct word *load_word_file(const char *path, size_t *n_out,
                                    int min_alnum, int max_alnum) {
    FILE *f = path ? fopen(path, "r") : NULL;
    struct word *out = NULL;
    size_t cap = 0;
    size_t n = 0;

    /* The optional file path may be NULL or "-" (skip), or "-empty" to add ""
     * automatically. We always include at least one empty entry if the file
     * doesn't supply one explicitly and the caller passes NULL. */
    char line[512];
    if (f) {
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
        cap = 1;
        out = realloc(out, cap * sizeof *out);
        out[0].text = strdup("");
        out[0].h = 0;
        out[0].sh = 0;
        n = 1;
    }
    *n_out = n;
    return out;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr,
                "usage: %s TARGETS PREFIXES SUFFIXES [PREPENDS] [SEPS]\n",
                argv[0]);
        return 2;
    }
    load_targets(argv[1]);
    fprintf(stderr, "loaded %zu unique target ids\n", n_targets);

    size_t n_pref = 0, n_suf = 0, n_prep = 0, n_sep = 0;
    struct word *prefixes = load_word_file(argv[2], &n_pref, 1, 18);
    struct word *suffixes = load_word_file(argv[3], &n_suf, 0, 16);
    struct word *prepends = (argc > 4 ? load_word_file(argv[4], &n_prep, 0, 12) : load_word_file(NULL, &n_prep, 0, 12));
    struct word *seps    = (argc > 5 ? load_word_file(argv[5], &n_sep, 0, 4) : NULL);
    if (!seps) {
        /* default: empty + underscore */
        n_sep = 2;
        seps = calloc(2, sizeof *seps);
        seps[0].text = strdup("");
        seps[1].text = strdup("_");
        for (size_t i = 0; i < n_sep; i++) {
            seps[i].h = calc_hash_with_shift(seps[i].text, &seps[i].sh);
        }
    }
    fprintf(stderr, "prefixes=%zu suffixes=%zu prepends=%zu seps=%zu => combos=%zu\n",
            n_pref, n_suf, n_prep, n_sep,
            n_pref * n_suf * n_prep * n_sep);

    char buf[512];
    size_t hits = 0;
    size_t combos = 0;
    for (size_t i = 0; i < n_prep; i++) {
        uint32_t h_p = prepends[i].h;
        uint32_t sh_p = prepends[i].sh;
        for (size_t j = 0; j < n_pref; j++) {
            uint32_t h_pp = h_p ^ rotl(prefixes[j].h, sh_p);
            uint32_t sh_pp = (sh_p + prefixes[j].sh) & 31;
            for (size_t k = 0; k < n_sep; k++) {
                uint32_t h_ppp = h_pp ^ rotl(seps[k].h, sh_pp);
                uint32_t sh_ppp = (sh_pp + seps[k].sh) & 31;
                for (size_t l = 0; l < n_suf; l++) {
                    uint32_t full_h = h_ppp ^ rotl(suffixes[l].h, sh_ppp);
                    uint32_t asset = 0x28C0E011u ^ rotl(full_h, 27);
                    combos++;
                    if (!bloom_maybe(asset)) continue;
                    if (!target_contains(asset)) continue;
                    int n = snprintf(buf, sizeof buf, "0x%08x\t%s%s%s%s\t%s\t%s\t%s\t%s\n",
                                     asset,
                                     prepends[i].text, prefixes[j].text,
                                     seps[k].text, suffixes[l].text,
                                     prepends[i].text, prefixes[j].text,
                                     seps[k].text, suffixes[l].text);
                    if (n > 0) fwrite(buf, 1, (size_t)n, stdout);
                    hits++;
                }
            }
        }
        if ((i & 0xF) == 0xF) {
            fflush(stdout);
            fprintf(stderr, "  ...prepend %zu/%zu, combos=%zu hits=%zu\n",
                    i + 1, n_prep, combos, hits);
        }
    }
    fflush(stdout);
    fprintf(stderr, "scanned %zu combos, %zu hits\n", combos, hits);
    return 0;
}
