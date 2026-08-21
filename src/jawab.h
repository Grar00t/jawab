#ifndef JAWAB_H
#define JAWAB_H

#include <stddef.h>
#include <stdint.h>

#define JAWAB_MAX_TERM   128
#define JAWAB_MAX_QUERY  64
#define JAWAB_TOPK       3

/* ============================================================
 * In-memory (build-time) representation. Used when indexing
 * raw text files fresh, before optionally persisting to disk.
 * ============================================================ */
typedef struct {
    char    *orig;
    char    *low;
    char    *src;
    size_t   ntok;
    uint64_t fp;
    uint8_t  sha[32];
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

void   jw_init(jw_index_t *ix);
void   jw_free(jw_index_t *ix);
int    jw_add_file(jw_index_t *ix, const char *path);
int    jw_add_line(jw_index_t *ix, const char *line, const char *src);
size_t jw_query(const jw_index_t *ix, const char *query, jw_hit_t *out, size_t k);

uint64_t jw_fnv1a(const char *s);
void jw_sha256(const void *data, size_t len, uint8_t out[32]);
void jw_sha_hex(const uint8_t in[32], char out[65]);

/* ============================================================
 * v0.3 Sovereign Upgrade: persistent, zero-copy, mmap'd index.
 *
 * On-disk layout (fixed-width, no embedded pointers, so the
 * whole file can be mapped and read directly with no parse
 * pass and no malloc/copy of passage text):
 *
 *   [ jw_idx_header_t ]
 *   [ jw_idx_rec_t    ] * n_passages
 *   [ string blob: orig\0 low\0 src\0 ... ]
 *
 * `root_sha` in the header is a structural placeholder for the
 * v1.0 proof-chained retrieval work: it will hold the Merkle/
 * chain root over every passage's SHA-256 receipt once chaining
 * lands. In v0.3 it is written as all-zero and unverified.
 *
 * Endianness / word-size: this format assumes a same-endian,
 * same-bitness host wrote and reads it (no cross-arch portability
 * layer). That is an explicit, documented limitation, not an
 * oversight -- fixing it is v0.4 scope.
 * ============================================================ */

#define JW_IDX_MAGIC    "JAWABIDX"
#define JW_IDX_VERSION  3u

typedef struct {
    char     magic[8];        /* "JAWABIDX" */
    uint32_t version;         /* JW_IDX_VERSION */
    uint32_t flags;           /* reserved, must be 0 in v0.3 */
    uint64_t n_passages;
    double   avglen;
    uint8_t  root_sha[32];    /* v1.0 proof-chain anchor (placeholder) */
    uint64_t passage_table_off;
    uint64_t blob_off;
    uint64_t blob_len;
    uint64_t reserved[2];
} jw_idx_header_t;

typedef struct {
    uint64_t orig_off, orig_len;  /* NUL-terminated, len excludes NUL */
    uint64_t low_off,  low_len;
    uint64_t src_off,  src_len;
    uint64_t ntok;
    uint64_t fp;
    uint8_t  sha[32];
} jw_idx_rec_t;

#if defined(_WIN32)
#include <windows.h>
typedef struct {
    HANDLE file_handle;
    HANDLE map_handle;
    void  *base;
    size_t size;
    const jw_idx_header_t *header;
    const jw_idx_rec_t    *records;
    const char            *blob;
} jw_mmap_index_t;
#else
typedef struct {
    int    fd;
    void  *base;
    size_t size;
    const jw_idx_header_t *header;
    const jw_idx_rec_t    *records;
    const char            *blob;
} jw_mmap_index_t;
#endif

typedef struct {
    const jw_idx_rec_t *rec;
    const char *orig;   /* points directly into the mapped file */
    const char *low;
    const char *src;
    double score;
} jw_mmap_hit_t;

/* Serialize an in-memory index built via jw_add_file/jw_add_line
 * to the on-disk zero-copy format described above. */
int    jw_index_save(const jw_index_t *ix, const char *path);

/* mmap (POSIX) / MapViewOfFile (Windows) the file at `path` in
 * read-only, zero-copy mode. Returns 0 on success. */
int    jw_mmap_open(jw_mmap_index_t *mix, const char *path);
void   jw_mmap_close(jw_mmap_index_t *mix);

/* BM25 query directly over the mapped index. No heap allocation
 * for passage text -- `out[i].orig/low/src` alias the mapping. */
size_t jw_mmap_query(const jw_mmap_index_t *mix, const char *query,
                      jw_mmap_hit_t *out, size_t k);

#endif /* JAWAB_H */
