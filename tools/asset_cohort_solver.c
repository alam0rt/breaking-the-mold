/* asset_cohort_solver: solve for a common prefix across multiple
 * (suffix, target_id) constraints inside a cohort.
 *
 * Goal: find prefix P from a wordlist such that, for some choice of suffix
 * tokens from a fixed vocab, the compound hash
 *
 *     asset_id(P || suffix) == cohort_member_id
 *
 * holds for at least K cohort members simultaneously.
 *
 * Approach:
 *   1. Pre-compute (h_S, sh_S) for each suffix token.
 *   2. For each wordlist word W, compute (h_W, sh_W).
 *   3. For each combination (W, S, suffix), compute the asset id.
 *      We maintain a per-cohort tally: how many cohort members were hit.
 *   4. Emit any (W, cohort) with hit_count >= MIN_HITS.
 *
 * We iterate over the suffix vocab in the inner loop (fast: usually 200-500
 * tokens) and check each cohort with a single hashtable lookup.
 *
 * Build:
 *   gcc -O3 -o /tmp/asset_cohort_solver tools/asset_cohort_solver.c
 *
 * Usage:
 *   /tmp/asset_cohort_solver TARGETS COHORTS SUFFIXES WORDLIST MIN_HITS [TRAIL_FILE] > hits.txt
 *
 *   TARGETS    - asset ids (one per line, decimal or 0xHEX) - this is the
 *                full set; cohort_members must be a subset.
 *   COHORTS    - "cohort_id,id,id,id,..." per line. Each cohort lists its
 *                member IDs.
 *   SUFFIXES   - text file, one suffix token per line. Always tested
 *                including the empty suffix.
 *   WORDLIST   - text file, one prefix word per line. Each word will be
 *                used as-is AND optionally followed by each trail token
 *                (see TRAIL_FILE).
 *   MIN_HITS   - integer; only print results when a single prefix hits at
 *                least this many distinct ids in one cohort.
 *   TRAIL_FILE - (optional) text file of trail tokens; if not supplied,
 *                default trails are "" and "_".
 *
 * Output (TSV):
 *   cohort_id\thits\tprefix_full\tsample_id\tsample_suffix\tall_matches
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

/* All target ids and a bitmap for quick membership */
#define BLOOM_BITS 22
#define BLOOM_SIZE (1u << BLOOM_BITS)
#define BLOOM_MASK (BLOOM_SIZE - 1)
static uint32_t targets_bloom[BLOOM_SIZE / 32];
static uint32_t *target_arr = NULL;
static size_t n_target = 0, cap_target = 0;

static int cmp_u32(const void *a, const void *b) {
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return (x > y) - (x < y);
}
static int target_contains(uint32_t id) {
    if (!((targets_bloom[(id & BLOOM_MASK) >> 5] >> (id & 31)) & 1u)) return 0;
    size_t lo = 0, hi = n_target;
    while (lo < hi) {
        size_t mid = (lo + hi) >> 1;
        if (target_arr[mid] < id) lo = mid + 1;
        else hi = mid;
    }
    return lo < n_target && target_arr[lo] == id;
}

/* Per-cohort membership: id -> bit_in_cohort + cohort_index */
#define MAX_COHORTS 1024
#define MAX_MEMBERS_PER_COHORT 32

struct cohort {
    char id_str[24];
    uint32_t members[MAX_MEMBERS_PER_COHORT];
    int n_members;
};
static struct cohort cohorts[MAX_COHORTS];
static int n_cohorts = 0;

/* id -> list of (cohort_idx, member_idx) */
struct id_membership {
    uint32_t id;
    int cohort_idx;
    int member_idx;
};
#define MAX_MEMBERSHIPS (MAX_COHORTS * MAX_MEMBERS_PER_COHORT)
static struct id_membership memberships[MAX_MEMBERSHIPS];
static int n_memberships = 0;

/* Hashtable id -> linked list of memberships (probed by id mod table_size) */
#define ID_TABLE_BITS 18
#define ID_TABLE_SIZE (1u << ID_TABLE_BITS)
#define ID_TABLE_MASK (ID_TABLE_SIZE - 1)
struct id_node {
    uint32_t id;
    int memb_idx;
    struct id_node *next;
};
static struct id_node *id_table[ID_TABLE_SIZE];

static inline uint32_t id_bucket(uint32_t id) {
    uint32_t k = id;
    k ^= k >> 16; k *= 0x7feb352du;
    k ^= k >> 15; k *= 0x846ca68bu;
    k ^= k >> 16;
    return k & ID_TABLE_MASK;
}

static void register_membership(uint32_t id, int ci, int mi) {
    if (n_memberships >= MAX_MEMBERSHIPS) {
        fprintf(stderr, "too many memberships\n"); exit(2);
    }
    int idx = n_memberships++;
    memberships[idx].id = id;
    memberships[idx].cohort_idx = ci;
    memberships[idx].member_idx = mi;
    struct id_node *n = malloc(sizeof *n);
    n->id = id;
    n->memb_idx = idx;
    n->next = id_table[id_bucket(id)];
    id_table[id_bucket(id)] = n;
}

static void load_targets(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(2); }
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;
        uint32_t v = (p[0] == '0' && (p[1] | 32) == 'x')
                     ? (uint32_t)strtoul(p + 2, NULL, 16)
                     : (uint32_t)strtoul(p, NULL, 10);
        if (n_target == cap_target) {
            cap_target = cap_target ? cap_target * 2 : 1024;
            target_arr = realloc(target_arr, cap_target * sizeof *target_arr);
        }
        target_arr[n_target++] = v;
        targets_bloom[(v & BLOOM_MASK) >> 5] |= 1u << (v & 31);
    }
    fclose(f);
    qsort(target_arr, n_target, sizeof *target_arr, cmp_u32);
    fprintf(stderr, "targets loaded: %zu\n", n_target);
}

static void load_cohorts(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(2); }
    char line[4096];
    while (fgets(line, sizeof line, f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;
        char *nl = strchr(p, '\n'); if (nl) *nl = '\0';
        char *comma = strchr(p, ',');
        if (!comma) continue;
        *comma = '\0';
        if (n_cohorts >= MAX_COHORTS) {
            fprintf(stderr, "too many cohorts (max %d)\n", MAX_COHORTS); exit(2);
        }
        struct cohort *c = &cohorts[n_cohorts];
        strncpy(c->id_str, p, sizeof c->id_str - 1);
        c->id_str[sizeof c->id_str - 1] = '\0';
        c->n_members = 0;
        char *cur = comma + 1;
        while (cur && *cur) {
            char *next = strchr(cur, ',');
            if (next) { *next = '\0'; next++; }
            while (*cur == ' ' || *cur == '\t') cur++;
            if (*cur == '\0') { cur = next; continue; }
            uint32_t v = (cur[0] == '0' && (cur[1] | 32) == 'x')
                         ? (uint32_t)strtoul(cur + 2, NULL, 16)
                         : (uint32_t)strtoul(cur, NULL, 10);
            if (c->n_members < MAX_MEMBERS_PER_COHORT) {
                c->members[c->n_members] = v;
                register_membership(v, n_cohorts, c->n_members);
                c->n_members++;
            }
            cur = next;
        }
        if (c->n_members >= 2) {
            n_cohorts++;
        }
    }
    fclose(f);
    fprintf(stderr, "cohorts loaded: %d (with >=2 members)\n", n_cohorts);
}

struct word {
    char *text;
    uint32_t h;
    uint32_t sh;
};

static struct word *load_words(const char *path, size_t *n_out, int min_alnum, int max_alnum) {
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
    *n_out = n;
    return out;
}

int main(int argc, char **argv) {
    if (argc < 6) {
        fprintf(stderr, "usage: %s TARGETS COHORTS SUFFIXES WORDLIST MIN_HITS [TRAILS]\n", argv[0]);
        return 2;
    }
    load_targets(argv[1]);
    load_cohorts(argv[2]);
    size_t n_suf, n_word, n_trail;
    struct word *suffixes = load_words(argv[3], &n_suf, 0, 16);
    struct word *words    = load_words(argv[4], &n_word, 1, 24);
    int min_hits = atoi(argv[5]);
    struct word *trails   = NULL;
    if (argc >= 7) trails = load_words(argv[6], &n_trail, 0, 4);
    if (!trails) {
        n_trail = 2;
        trails = calloc(2, sizeof *trails);
        trails[0].text = strdup(""); trails[0].h = 0; trails[0].sh = 0;
        trails[1].text = strdup("_");
        trails[1].h = calc_hash_with_shift("_", &trails[1].sh);
    }

    /* Always also test "" as a suffix */
    int have_empty_suffix = 0;
    for (size_t i = 0; i < n_suf; i++) if (suffixes[i].text[0] == '\0') { have_empty_suffix = 1; break; }
    if (!have_empty_suffix) {
        suffixes = realloc(suffixes, (n_suf + 1) * sizeof *suffixes);
        suffixes[n_suf].text = strdup("");
        suffixes[n_suf].h = 0;
        suffixes[n_suf].sh = 0;
        n_suf++;
    }

    fprintf(stderr, "vocab: words=%zu trails=%zu suffixes=%zu  min_hits=%d\n",
            n_word, n_trail, n_suf, min_hits);

    /* Per-cohort tally buffers - one byte per (cohort, member) cleared between
     * iterations. We only walk the cohorts that received hits, so use a small
     * scratch list. */
    uint8_t *hit_mask = calloc((size_t)n_cohorts * MAX_MEMBERS_PER_COHORT, 1);
    int *touched_cohorts = malloc(sizeof(int) * n_cohorts);
    int *touched_count = calloc(n_cohorts, sizeof(int));

    char hit_examples[MAX_COHORTS][MAX_MEMBERS_PER_COHORT][96];

    char buf[2048];
    size_t total_combos = 0;
    size_t out_lines = 0;

    for (size_t iw = 0; iw < n_word; iw++) {
        uint32_t hw = words[iw].h;
        uint32_t shw = words[iw].sh;
        for (size_t it = 0; it < n_trail; it++) {
            uint32_t hp = hw ^ rotl(trails[it].h, shw);
            uint32_t shp = (shw + trails[it].sh) & 31;

            /* Reset touched cohorts */
            int n_touched = 0;
            for (size_t is = 0; is < n_suf; is++) {
                uint32_t hh = hp ^ rotl(suffixes[is].h, shp);
                uint32_t id = SEED ^ rotl(hh, OUT_ROT);
                total_combos++;
                if (!((targets_bloom[(id & BLOOM_MASK) >> 5] >> (id & 31)) & 1u)) continue;
                /* Walk id_table bucket for matching memberships */
                struct id_node *nd = id_table[id_bucket(id)];
                while (nd) {
                    if (nd->id == id) {
                        struct id_membership *m = &memberships[nd->memb_idx];
                        size_t off = (size_t)m->cohort_idx * MAX_MEMBERS_PER_COHORT
                                     + m->member_idx;
                        if (!hit_mask[off]) {
                            hit_mask[off] = 1;
                            if (touched_count[m->cohort_idx] == 0) {
                                touched_cohorts[n_touched++] = m->cohort_idx;
                            }
                            touched_count[m->cohort_idx]++;
                            snprintf(hit_examples[m->cohort_idx][m->member_idx],
                                     sizeof hit_examples[0][0],
                                     "0x%08x:%s", id, suffixes[is].text[0] ? suffixes[is].text : "(empty)");
                        }
                    }
                    nd = nd->next;
                }
            }

            /* Check which cohorts reached min_hits */
            for (int t = 0; t < n_touched; t++) {
                int ci = touched_cohorts[t];
                if (touched_count[ci] >= min_hits) {
                    /* Emit */
                    int n = snprintf(buf, sizeof buf,
                        "%s\t%d/%d\t%s%s\t",
                        cohorts[ci].id_str,
                        touched_count[ci], cohorts[ci].n_members,
                        words[iw].text, trails[it].text);
                    if (n > 0) fwrite(buf, 1, (size_t)n, stdout);
                    int first = 1;
                    for (int mi = 0; mi < cohorts[ci].n_members; mi++) {
                        size_t off = (size_t)ci * MAX_MEMBERS_PER_COHORT + mi;
                        if (hit_mask[off]) {
                            if (!first) fputc(',', stdout);
                            first = 0;
                            fputs(hit_examples[ci][mi], stdout);
                        }
                    }
                    fputc('\n', stdout);
                    out_lines++;
                }
            }
            /* Cleanup */
            for (int t = 0; t < n_touched; t++) {
                int ci = touched_cohorts[t];
                touched_count[ci] = 0;
                for (int mi = 0; mi < cohorts[ci].n_members; mi++) {
                    hit_mask[(size_t)ci * MAX_MEMBERS_PER_COHORT + mi] = 0;
                }
            }
        }
        if ((iw & 0xFFFFFu) == 0xFFFFFu) {
            fflush(stdout);
            fprintf(stderr, "  word %zu/%zu  combos=%zu  out=%zu\n",
                    iw + 1, n_word, total_combos, out_lines);
        }
    }
    fflush(stdout);
    fprintf(stderr, "scanned %zu combos, %zu output rows\n", total_combos, out_lines);
    return 0;
}
