#include "jawab.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *jawab_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *out = (char *)malloc(len);
    if (out) memcpy(out, s, len);
    return out;
}

static void jawab_trim(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' ||
                        s[len - 1] == ' ' || s[len - 1] == '\t')) {
        s[--len] = '\0';
    }
    size_t start = 0;
    while (s[start] == ' ' || s[start] == '\t') start++;
    if (start > 0) memmove(s, s + start, len - start + 1);
}

int jawab_corpus_init(jawab_corpus_t *corpus, size_t initial_capacity) {
    if (!corpus) return -1;
    if (initial_capacity == 0) initial_capacity = 64;
    corpus->entries = (jawab_entry_t *)calloc(initial_capacity, sizeof(jawab_entry_t));
    if (!corpus->entries) return -1;
    corpus->count = 0;
    corpus->capacity = initial_capacity;
    return 0;
}

void jawab_corpus_free(jawab_corpus_t *corpus) {
    if (!corpus || !corpus->entries) return;
    for (size_t i = 0; i < corpus->count; i++) {
        free(corpus->entries[i].question);
        free(corpus->entries[i].answer);
    }
    free(corpus->entries);
    corpus->entries = NULL;
    corpus->count = 0;
    corpus->capacity = 0;
}

static int jawab_corpus_push(jawab_corpus_t *corpus, const char *q, const char *a) {
    if (corpus->count == corpus->capacity) {
        size_t new_cap = corpus->capacity * 2;
        jawab_entry_t *tmp = (jawab_entry_t *)realloc(corpus->entries,
                                                        new_cap * sizeof(jawab_entry_t));
        if (!tmp) return -1;
        corpus->entries = tmp;
        corpus->capacity = new_cap;
    }
    corpus->entries[corpus->count].question = jawab_strdup(q);
    corpus->entries[corpus->count].answer = jawab_strdup(a);
    if (!corpus->entries[corpus->count].question ||
        !corpus->entries[corpus->count].answer) {
        return -1;
    }
    corpus->count++;
    return 0;
}

int jawab_corpus_load(jawab_corpus_t *corpus, const char *path) {
    if (!corpus || !path) return -1;
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    if (corpus->entries == NULL) {
        if (jawab_corpus_init(corpus, 64) != 0) {
            fclose(fp);
            return -1;
        }
    }

    char line[JAWAB_MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        jawab_trim(line);
        if (line[0] == '\0' || line[0] == '#') continue;

        char *sep = strchr(line, '|');
        if (!sep) continue; /* malformed line, skip */
        *sep = '\0';
        char *question = line;
        char *answer = sep + 1;
        jawab_trim(question);
        jawab_trim(answer);
        if (question[0] == '\0' || answer[0] == '\0') continue;

        if (jawab_corpus_push(corpus, question, answer) != 0) {
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return 0;
}

/* Lowercase + tokenize into a fixed-size array of tokens. Returns token count. */
static size_t jawab_tokenize(const char *text, char tokens[][JAWAB_TOKEN_LEN],
                              size_t max_tokens) {
    size_t count = 0;
    size_t i = 0;
    size_t len = strlen(text);

    while (i < len && count < max_tokens) {
        while (i < len && !isalnum((unsigned char)text[i])) i++;
        size_t start = i;
        while (i < len && isalnum((unsigned char)text[i])) i++;
        size_t tok_len = i - start;
        if (tok_len == 0) continue;
        if (tok_len >= JAWAB_TOKEN_LEN) tok_len = JAWAB_TOKEN_LEN - 1;
        for (size_t j = 0; j < tok_len; j++) {
            tokens[count][j] = (char)tolower((unsigned char)text[start + j]);
        }
        tokens[count][tok_len] = '\0';
        count++;
    }
    return count;
}

static double jawab_score(const char *query, const char *question) {
    char q_tokens[JAWAB_MAX_TOKENS][JAWAB_TOKEN_LEN];
    char c_tokens[JAWAB_MAX_TOKENS][JAWAB_TOKEN_LEN];
    size_t q_count = jawab_tokenize(query, q_tokens, JAWAB_MAX_TOKENS);
    size_t c_count = jawab_tokenize(question, c_tokens, JAWAB_MAX_TOKENS);

    if (q_count == 0 || c_count == 0) return 0.0;

    size_t intersection = 0;
    for (size_t i = 0; i < q_count; i++) {
        for (size_t j = 0; j < c_count; j++) {
            if (strcmp(q_tokens[i], c_tokens[j]) == 0) {
                intersection++;
                break;
            }
        }
    }

    size_t union_count = q_count + c_count - intersection;
    if (union_count == 0) return 0.0;
    return (double)intersection / (double)union_count;
}

const char *jawab_answer(const jawab_corpus_t *corpus, const char *query,
                          double *out_score) {
    if (!corpus || corpus->count == 0 || !query) {
        if (out_score) *out_score = 0.0;
        return NULL;
    }

    double best_score = -1.0;
    size_t best_idx = 0;

    for (size_t i = 0; i < corpus->count; i++) {
        double score = jawab_score(query, corpus->entries[i].question);
        if (score > best_score) {
            best_score = score;
            best_idx = i;
        }
    }

    if (out_score) *out_score = best_score;
    return corpus->entries[best_idx].answer;
}
