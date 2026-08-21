#ifndef JAWAB_H
#define JAWAB_H

#include <stddef.h>
#include <stdint.h>

#define JAWAB_MAX_TERM   128
#define JAWAB_MAX_QUERY  64
#define JAWAB_TOPK       3

typedef struct {
    char    *orig;    /* النص الأصلي كما هو */
    char    *low;     /* نسخة مخفوضة للتقطيع */
    char    *src;     /* اسم الملف المصدر */
    size_t   ntok;
    uint64_t fp;      /* بصمة FNV-1a سريعة */
    uint8_t  sha[32]; /* SHA-256 للسطر = إيصال لا يُمحى */
} jw_passage_t;

typedef struct {
    jw_passage_t *v;
    size_t n, cap;
    double avglen;
} jw_index_t;

typedef struct {
    const jw_passage_t *p;
    double score;
} jw_hit_t;

void  jw_init(jw_index_t *ix);
void  jw_free(jw_index_t *ix);
int   jw_add_file(jw_index_t *ix, const char *path);
int   jw_add_line(jw_index_t *ix, const char *line, const char *src);
size_t jw_query(const jw_index_t *ix, const char *query, jw_hit_t *out, size_t k);

uint64_t jw_fnv1a(const char *s);
void jw_sha256(const void *data, size_t len, uint8_t out[32]);
void jw_sha_hex(const uint8_t in[32], char out[65]);

#endif
