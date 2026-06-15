/* asset_hash_sieve: read names from stdin (one per line, possibly multi-word with
 * spaces/non-alnum that calcHash skips), apply Skullmonkeys asset hash, and
 * print the original line + hex id if the id is in a target set loaded from
 * argv[1].
 *
 * Target file: one decimal or 0xHEX id per line. Lines starting with '#' or
 * blank are ignored.
 *
 * Build:
 *     gcc -O3 -o /tmp/asset_hash_sieve tools/asset_hash_sieve.c
 *
 * Usage:
 *     /tmp/asset_hash_sieve docs/reference/asset-ids-targets.txt \
 *         < /nix/store/.../rockyou.txt > hits.txt
 *
 * Optional environment variables:
 *     SIEVE_PREFIX="HEAD_JOE_"   - prepend before each input line
 *     SIEVE_SUFFIX=".SPR"        - append after each input line
 *     SIEVE_UPPER=1              - uppercase the input (default: yes; calcHash does
 *                                  the same internally, this is for the output line)
 *     SIEVE_MIN_LEN=2            - skip lines whose alnum length is less than this
 *     SIEVE_MAX_LEN=24           - skip lines whose alnum length exceeds this
 *
 * Output format (per hit):
 *     0xDEADBEEF\tINPUT_LINE\tEXPANDED_NAME
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

/* Skullmonkeys / Neverhood calcHash: cumulative bit-toggle over the normalized
 * alnum content of the input string. */
static uint32_t calc_hash(const char *s) {
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
    return h;
}

static inline uint32_t asset_id(const char *s) {
    return 0x28C0E011u ^ rotl(calc_hash(s), 27);
}

#define TARGET_HASH_BITS 20
#define TARGET_HASH_SIZE (1u << TARGET_HASH_BITS)
#define TARGET_HASH_MASK (TARGET_HASH_SIZE - 1)

static uint32_t target_bitmap[TARGET_HASH_SIZE / 32];

static inline void target_set(uint32_t id) {
    uint32_t h = id & TARGET_HASH_MASK;
    target_bitmap[h >> 5] |= 1u << (h & 31);
}

static inline int target_maybe(uint32_t id) {
    uint32_t h = id & TARGET_HASH_MASK;
    return (target_bitmap[h >> 5] >> (h & 31)) & 1u;
}

/* Exact target set for confirmation after bloom hit */
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
    if (!f) {
        fprintf(stderr, "open %s: %s\n", path, strerror(errno));
        exit(2);
    }
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
        target_set(v);
    }
    fclose(f);
    qsort(targets, n_targets, sizeof *targets, cmp_u32);
    /* Deduplicate */
    size_t w = 0;
    for (size_t i = 0; i < n_targets; i++) {
        if (w == 0 || targets[w - 1] != targets[i]) targets[w++] = targets[i];
    }
    n_targets = w;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <targets-file>\n", argv[0]);
        return 2;
    }
    load_targets(argv[1]);
    fprintf(stderr, "loaded %zu unique target ids\n", n_targets);

    const char *pfx = getenv("SIEVE_PREFIX");
    const char *sfx = getenv("SIEVE_SUFFIX");
    int upper = 1;
    if (getenv("SIEVE_UPPER")) upper = atoi(getenv("SIEVE_UPPER"));
    int min_len = 2;
    int max_len = 32;
    if (getenv("SIEVE_MIN_LEN")) min_len = atoi(getenv("SIEVE_MIN_LEN"));
    if (getenv("SIEVE_MAX_LEN")) max_len = atoi(getenv("SIEVE_MAX_LEN"));

    char line[512];
    char buf[1024];
    size_t hits = 0;
    size_t scanned = 0;
    while (fgets(line, sizeof line, stdin)) {
        /* strip trailing newlines/CR */
        size_t len = strlen(line);
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (!len) continue;

        /* Build expanded name = prefix + line + suffix */
        size_t off = 0;
        if (pfx) {
            size_t l = strlen(pfx);
            if (off + l < sizeof buf) memcpy(buf + off, pfx, l), off += l;
        }
        if (off + len < sizeof buf) memcpy(buf + off, line, len), off += len;
        if (sfx) {
            size_t l = strlen(sfx);
            if (off + l < sizeof buf) memcpy(buf + off, sfx, l), off += l;
        }
        if (off >= sizeof buf) continue;
        buf[off] = '\0';

        /* Count alnum length for the min/max filter */
        int alnum = 0;
        for (char *p = buf; *p; p++) if (isalnum((unsigned char)*p)) alnum++;
        if (alnum < min_len || alnum > max_len) continue;

        if (upper) {
            for (char *p = buf; *p; p++) *p = (char)toupper((unsigned char)*p);
        }

        uint32_t id = asset_id(buf);
        scanned++;
        if (!target_maybe(id)) continue;
        if (!target_contains(id)) continue;

        printf("0x%08x\t%s\t%s\n", id, line, buf);
        hits++;
        if ((hits & 0xFFFF) == 0) fflush(stdout);
    }
    fflush(stdout);
    fprintf(stderr, "scanned %zu names, %zu hits\n", scanned, hits);
    return 0;
}
