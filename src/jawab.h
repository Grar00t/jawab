#ifndef JAWAB_H
#define JAWAB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JAWAB_MAX_LINE    2048
#define JAWAB_MAX_TOKENS  64
#define JAWAB_TOKEN_LEN   64

/* A single question/answer pair loaded from the corpus. */
typedef struct {
    char *question;
    char *answer;
} jawab_entry_t;

/* Dynamic array of entries loaded from a corpus file. */
typedef struct {
    jawab_entry_t *entries;
    size_t count;
    size_t capacity;
} jawab_corpus_t;

/* Initialize an empty corpus with the given initial capacity (0 = default). */
int jawab_corpus_init(jawab_corpus_t *corpus, size_t initial_capacity);

/* Free all memory owned by the corpus. Safe to call on a zeroed struct. */
void jawab_corpus_free(jawab_corpus_t *corpus);

/*
 * Load a corpus file. Each non-empty, non-comment line must be in the form:
 *   question|answer
 * Lines starting with '#' are treated as comments and skipped.
 * Returns 0 on success, -1 on failure (I/O error or malformed line).
 */
int jawab_corpus_load(jawab_corpus_t *corpus, const char *path);

/*
 * Find the best-matching answer for `query` inside `corpus` using a
 * token-overlap (Jaccard-like) similarity score. Returns a pointer to the
 * stored answer string (owned by the corpus, do not free) or NULL if the
 * corpus is empty. If `out_score` is non-NULL, the best match score
 * (0.0 - 1.0) is written to it.
 */
const char *jawab_answer(const jawab_corpus_t *corpus, const char *query,
                          double *out_score);

#ifdef __cplusplus
}
#endif

#endif /* JAWAB_H */
