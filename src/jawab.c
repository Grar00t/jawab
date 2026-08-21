#include "jawab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#if !defined(_WIN32)
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#define ISW(c) (isalnum((unsigned char)(c)) || (unsigned char)(c) > 127)

#if defined(__AVX2__) && (defined(__x86_64__) || defined(_M_X64))
  #include <immintrin.h>
  #define JAWAB_SIMD_AVX2 1
#elif defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
  #include <arm_neon.h>
  #define JAWAB_SIMD_NEON 1
#endif

static inline int jw_streq(const char *a, const char *b) {
    size_t la = strlen(a), lb = strlen(b);
    if (la != lb) return 0;
    if (la == 0) return 1;
    size_t i = 0;
#if defined(JAWAB_SIMD_AVX2)
    for (; i + 32 <= la; i += 32) {
        __m256i va = _mm256_loadu_si256((const __m256i *)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i *)(b + i));
        __m256i eq = _mm256_cmpeq_epi8(va, vb);
        if ((unsigned)_mm256_movemask_epi8(eq) != 0xFFFFFFFFu) return 0;
    }
#elif defined(JAWAB_SIMD_NEON)
    for (; i + 16 <= la; i += 16) {
        uint8x16_t va = vld1q_u8((const uint8_t *)(a + i));
        uint8x16_t vb = vld1q_u8((const uint8_t *)(b + i));
        uint8x16_t eq = vceqq_u8(va, vb);
        uint8x8_t lo = vget_low_u8(eq), hi = vget_high_u8(eq);
        if (vminv_u8(lo) != 0xFF || vminv_u8(hi) != 0xFF) return 0;
    }
#endif
    return memcmp(a + i, b + i, la - i) == 0;
}

/* FNV-1a 64-bit. Offset basis MUST be 14695981039346656037 (0xcbf29ce484222325).
 * v0.3.0 through v0.3.1 shipped a truncated constant (1469598103934665603,
 * missing a trailing digit), which silently produced a non-standard hash.
 * Fixed and covered by tests/test_jawab.c against reference vectors. */
uint64_t jw_fnv1a(const char *s){
    uint64_t h = 14695981039346656037ULL;
    while (*s){ h ^= (unsigned char)*s++; h *= 1099511628211ULL; }
    return h;
}

#define ROR(x,n) (((x)>>(n)) | ((x)<<(32-(n))))
static const uint32_t SHA_K[64] = {
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};

static void sha_block(uint32_t h[8], const uint8_t *p){
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
        w[i] = ((uint32_t)p[i*4]<<24)|((uint32_t)p[i*4+1]<<16)|
               ((uint32_t)p[i*4+2]<<8)|((uint32_t)p[i*4+3]);
    for (int i = 16; i < 64; i++){
        uint32_t s0 = ROR(w[i-15],7)^ROR(w[i-15],18)^(w[i-15]>>3);
        uint32_t s1 = ROR(w[i-2],17)^ROR(w[i-2],19)^(w[i-2]>>10);
        w[i] = w[i-16]+s0+w[i-7]+s1;
    }
    uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],z=h[7];
    for (int i = 0; i < 64; i++){
        uint32_t S1 = ROR(e,6)^ROR(e,11)^ROR(e,25);
        uint32_t ch = (e&f)^(~e&g);
        uint32_t t1 = z+S1+ch+SHA_K[i]+w[i];
        uint32_t S0 = ROR(a,2)^ROR(a,13)^ROR(a,22);
        uint32_t mj = (a&b)^(a&c)^(b&c);
        uint32_t t2 = S0+mj;
        z=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=z;
}

void jw_sha256(const void *data, size_t len, uint8_t out[32]){
    uint32_t h[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
                     0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    const uint8_t *p = data;
    size_t off = 0;
    while (len - off >= 64){ sha_block(h, p+off); off += 64; }

    uint8_t tail[128];
    size_t rem = len - off;
    memcpy(tail, p+off, rem);
    tail[rem++] = 0x80;
    size_t need = (rem <= 56) ? 56 : 120;
    while (rem < need) tail[rem++] = 0;
    uint64_t bits = (uint64_t)len * 8;
    for (int i = 0; i < 8; i++) tail[rem++] = (uint8_t)(bits >> (56 - i*8));
    for (size_t b = 0; b < rem; b += 64) sha_block(h, tail+b);

    for (int i = 0; i < 8; i++){
        out[i*4]   = (uint8_t)(h[i]>>24);
        out[i*4+1] = (uint8_t)(h[i]>>16);
        out[i*4+2] = (uint8_t)(h[i]>>8);
        out[i*4+3] = (uint8_t)(h[i]);
    }
}

void jw_sha_hex(const uint8_t in[32], char out[65]){
    static const char hx[] = "0123456789abcdef";
    for (int i = 0; i < 32; i++){ out[i*2]=hx[in[i]>>4]; out[i*2+1]=hx[in[i]&15]; }
    out[64] = 0;
}

static int tok_step(const char *s, size_t *i, char *buf, size_t cap){
    size_t n = 0;
    while (s[*i] && !ISW(s[*i])) (*i)++;
    while (s[*i] && ISW(s[*i]) && n < cap-1) buf[n++] = s[(*i)++];
    buf[n] = 0;
    return n > 0;
}

void jw_init(jw_index_t *ix){ ix->v=NULL; ix->n=0; ix->cap=0; ix->avglen=1.0; }

void jw_free(jw_index_t *ix){
    for (size_t i = 0; i < ix->n; i++){ free(ix->v[i].orig); free(ix->v[i].low); free(ix->v[i].src); }
    free(ix->v);
    jw_init(ix);
}

static char *xstrdup(const char *s){
    size_t n = strlen(s)+1;
    char *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

int jw_add_line(jw_index_t *ix, const char *line, const char *src){
    if (ix->n == ix->cap){
        size_t nc = ix->cap ? ix->cap*2 : 256;
        jw_passage_t *nv = realloc(ix->v, nc * sizeof *nv);
        if (!nv) return -1;
        ix->v = nv; ix->cap = nc;
    }
    jw_passage_t *p = &ix->v[ix->n];
    p->orig = xstrdup(line); if (!p->orig) return -1;
    p->low  = xstrdup(line); if (!p->low){ free(p->orig); return -1; }
    p->src  = xstrdup(src);  if (!p->src){ free(p->orig); free(p->low); return -1; }
    for (char *c = p->low; *c; c++) *c = (char)tolower((unsigned char)*c);
    p->ntok = 0;
    size_t i = 0; char buf[JAWAB_MAX_TERM];
    while (tok_step(p->low, &i, buf, sizeof buf)) p->ntok++;
    if (!p->ntok) p->ntok = 1;
    p->fp = jw_fnv1a(line);
    jw_sha256(line, strlen(line), p->sha);
    ix->avglen += ((double)p->ntok - ix->avglen) / (double)(ix->n + 1);
    ix->n++;
    return 0;
}

int jw_add_file(jw_index_t *ix, const char *path){
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    char line[4096];
    int added = 0;
    while (fgets(line, sizeof line, f)){
        size_t L = strlen(line);
        while (L && isspace((unsigned char)line[L-1])) line[--L] = 0;
        if (L < 3) continue;
        if (jw_add_line(ix, line, path) == 0) added++;
    }
    fclose(f);
    return added;
}

static size_t count_occ(const char *low, const char *term){
    size_t i = 0, c = 0; char buf[JAWAB_MAX_TERM];
    while (tok_step(low, &i, buf, sizeof buf))
        if (jw_streq(buf, term)) c++;
    return c;
}

size_t jw_query(const jw_index_t *ix, const char *query, jw_hit_t *out, size_t k){
    char lq[2048];
    size_t j = 0;
    for (size_t i = 0; query[i] && j < sizeof lq - 1; i++)
        lq[j++] = (char)tolower((unsigned char)query[i]);
    lq[j] = 0;

    char q[JAWAB_MAX_QUERY][JAWAB_MAX_TERM];
    size_t nq = 0, i = 0;
    char buf[JAWAB_MAX_TERM];
    while (nq < JAWAB_MAX_QUERY && tok_step(lq, &i, buf, sizeof buf))
        strcpy(q[nq++], buf);
    if (!nq || !ix->n) return 0;

    double df[JAWAB_MAX_QUERY];
    for (size_t t = 0; t < nq; t++){
        df[t] = 0;
        for (size_t d = 0; d < ix->n; d++)
            if (count_occ(ix->v[d].low, q[t])) df[t]++;
    }

    const double k1 = 1.5, b = 0.75;
    size_t found = 0;
    for (size_t d = 0; d < ix->n; d++){
        double s = 0;
        for (size_t t = 0; t < nq; t++){
            size_t tf = count_occ(ix->v[d].low, q[t]);
            if (!tf || !df[t]) continue;
            double idf = log(1.0 + ((double)ix->n - df[t] + 0.5) / (df[t] + 0.5));
            double tfn = ((double)tf*(k1+1.0)) /
                         ((double)tf + k1*(1.0 - b + b*(double)ix->v[d].ntok/ix->avglen));
            s += idf * tfn;
        }
        if (s <= 0) continue;
        size_t pos;
        if (found < k){
            pos = found;
            while (pos > 0 && out[pos-1].score < s) pos--;
            for (size_t m = found; m > pos; m--) out[m] = out[m-1];
            out[pos].p = &ix->v[d]; out[pos].score = s;
            found++;
        } else if (s > out[k-1].score){
            pos = k;
            while (pos > 0 && out[pos-1].score < s) pos--;
            for (size_t m = k-1; m > pos; m--) out[m] = out[m-1];
            out[pos].p = &ix->v[d]; out[pos].score = s;
        }
    }
    return found;
}

int jw_index_save(const jw_index_t *ix, const char *path){
    if (!ix || !path) return -1;
    uint64_t *orig_off = malloc(ix->n * sizeof *orig_off);
    uint64_t *low_off  = malloc(ix->n * sizeof *low_off);
    uint64_t *src_off  = malloc(ix->n * sizeof *src_off);
    if (!orig_off || !low_off || !src_off){
        free(orig_off); free(low_off); free(src_off);
        return -1;
    }
    uint64_t cursor = 0;
    for (size_t i = 0; i < ix->n; i++){
        orig_off[i] = cursor; cursor += strlen(ix->v[i].orig) + 1;
        low_off[i]  = cursor; cursor += strlen(ix->v[i].low)  + 1;
        src_off[i]  = cursor; cursor += strlen(ix->v[i].src)  + 1;
    }
    uint64_t blob_len = cursor;
    jw_idx_header_t hdr;
    memset(&hdr, 0, sizeof hdr);
    memcpy(hdr.magic, JW_IDX_MAGIC, 8);
    hdr.version = JW_IDX_VERSION;
    hdr.flags = 0;
    hdr.n_passages = (uint64_t)ix->n;
    hdr.avglen = ix->avglen;
    hdr.passage_table_off = sizeof hdr;
    hdr.blob_off = hdr.passage_table_off + ix->n * sizeof(jw_idx_rec_t);
    hdr.blob_len = blob_len;

    FILE *f = fopen(path, "wb");
    if (!f){ free(orig_off); free(low_off); free(src_off); return -1; }
    if (fwrite(&hdr, sizeof hdr, 1, f) != 1) goto fail;
    for (size_t i = 0; i < ix->n; i++){
        jw_idx_rec_t rec;
        memset(&rec, 0, sizeof rec);
        rec.orig_off = orig_off[i]; rec.orig_len = strlen(ix->v[i].orig);
        rec.low_off  = low_off[i];  rec.low_len  = strlen(ix->v[i].low);
        rec.src_off  = src_off[i];  rec.src_len  = strlen(ix->v[i].src);
        rec.ntok = (uint64_t)ix->v[i].ntok;
        rec.fp   = ix->v[i].fp;
        memcpy(rec.sha, ix->v[i].sha, 32);
        if (fwrite(&rec, sizeof rec, 1, f) != 1) goto fail;
    }
    for (size_t i = 0; i < ix->n; i++){
        size_t lo = strlen(ix->v[i].orig)+1;
        size_t ll = strlen(ix->v[i].low)+1;
        size_t ls = strlen(ix->v[i].src)+1;
        if (fwrite(ix->v[i].orig, 1, lo, f) != lo) goto fail;
        if (fwrite(ix->v[i].low,  1, ll, f) != ll) goto fail;
        if (fwrite(ix->v[i].src,  1, ls, f) != ls) goto fail;
    }
    free(orig_off); free(low_off); free(src_off);
    fclose(f);
    return 0;
fail:
    free(orig_off); free(low_off); free(src_off);
    fclose(f);
    return -1;
}

#if defined(_WIN32)
int jw_mmap_open(jw_mmap_index_t *mix, const char *path){
    memset(mix, 0, sizeof *mix);
    HANDLE fh = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == INVALID_HANDLE_VALUE) return -1;
    LARGE_INTEGER sz;
    if (!GetFileSizeEx(fh, &sz)){ CloseHandle(fh); return -1; }
    HANDLE mh = CreateFileMappingA(fh, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!mh){ CloseHandle(fh); return -1; }
    void *base = MapViewOfFile(mh, FILE_MAP_READ, 0, 0, 0);
    if (!base){ CloseHandle(mh); CloseHandle(fh); return -1; }
    mix->file_handle = fh; mix->map_handle = mh; mix->base = base;
    mix->size = (size_t)sz.QuadPart;
    mix->header  = (const jw_idx_header_t *)base;
    mix->records = (const jw_idx_rec_t *)((const char *)base + mix->header->passage_table_off);
    mix->blob    = (const char *)base + mix->header->blob_off;
    if (mix->size < sizeof(jw_idx_header_t) ||
        memcmp(mix->header->magic, JW_IDX_MAGIC, 8) != 0 ||
        mix->header->version != JW_IDX_VERSION){
        jw_mmap_close(mix);
        return -1;
    }
    return 0;
}
void jw_mmap_close(jw_mmap_index_t *mix){
    if (mix->base) UnmapViewOfFile(mix->base);
    if (mix->map_handle) CloseHandle(mix->map_handle);
    if (mix->file_handle) CloseHandle(mix->file_handle);
    memset(mix, 0, sizeof *mix);
}
#else
int jw_mmap_open(jw_mmap_index_t *mix, const char *path){
    memset(mix, 0, sizeof *mix);
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    struct stat st;
    if (fstat(fd, &st) != 0){ close(fd); return -1; }
    void *base = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (base == MAP_FAILED){ close(fd); return -1; }
    mix->fd = fd; mix->base = base; mix->size = (size_t)st.st_size;
    mix->header  = (const jw_idx_header_t *)base;
    mix->records = (const jw_idx_rec_t *)((const char *)base + mix->header->passage_table_off);
    mix->blob    = (const char *)base + mix->header->blob_off;
    if (mix->size < sizeof(jw_idx_header_t) ||
        memcmp(mix->header->magic, JW_IDX_MAGIC, 8) != 0 ||
        mix->header->version != JW_IDX_VERSION){
        jw_mmap_close(mix);
        return -1;
    }
    return 0;
}
void jw_mmap_close(jw_mmap_index_t *mix){
    if (mix->base) munmap(mix->base, mix->size);
    if (mix->fd) close(mix->fd);
    memset(mix, 0, sizeof *mix);
}
#endif

static size_t count_occ_ptr(const char *low, const char *term){
    return count_occ(low, term);
}

size_t jw_mmap_query(const jw_mmap_index_t *mix, const char *query,
                      jw_mmap_hit_t *out, size_t k){
    if (!mix || !mix->header || !mix->records) return 0;
    size_t n = (size_t)mix->header->n_passages;
    double avglen = mix->header->avglen;
    char lq[2048]; size_t j = 0;
    for (size_t i = 0; query[i] && j < sizeof lq - 1; i++)
        lq[j++] = (char)tolower((unsigned char)query[i]);
    lq[j] = 0;
    char q[JAWAB_MAX_QUERY][JAWAB_MAX_TERM];
    size_t nq = 0, i = 0; char buf[JAWAB_MAX_TERM];
    while (nq < JAWAB_MAX_QUERY && tok_step(lq, &i, buf, sizeof buf))
        strcpy(q[nq++], buf);
    if (!nq || !n) return 0;
    double df[JAWAB_MAX_QUERY];
    for (size_t t = 0; t < nq; t++){
        df[t] = 0;
        for (size_t d = 0; d < n; d++){
            const char *low = mix->blob + mix->records[d].low_off;
            if (count_occ_ptr(low, q[t])) df[t]++;
        }
    }
    const double k1 = 1.5, b = 0.75;
    size_t found = 0;
    for (size_t d = 0; d < n; d++){
        const jw_idx_rec_t *rec = &mix->records[d];
        const char *low = mix->blob + rec->low_off;
        double s = 0;
        for (size_t t = 0; t < nq; t++){
            size_t tf = count_occ_ptr(low, q[t]);
            if (!tf || !df[t]) continue;
            double idf = log(1.0 + ((double)n - df[t] + 0.5) / (df[t] + 0.5));
            double tfn = ((double)tf*(k1+1.0)) /
                         ((double)tf + k1*(1.0 - b + b*(double)rec->ntok/avglen));
            s += idf * tfn;
        }
        if (s <= 0) continue;
        jw_mmap_hit_t hit;
        hit.rec = rec; hit.orig = mix->blob + rec->orig_off;
        hit.low = mix->blob + rec->low_off; hit.src = mix->blob + rec->src_off;
        hit.score = s;
        size_t pos;
        if (found < k){
            pos = found;
            while (pos > 0 && out[pos-1].score < s) pos--;
            for (size_t m = found; m > pos; m--) out[m] = out[m-1];
            out[pos] = hit; found++;
        } else if (s > out[k-1].score){
            pos = k;
            while (pos > 0 && out[pos-1].score < s) pos--;
            for (size_t m = k-1; m > pos; m--) out[m] = out[m-1];
            out[pos] = hit;
        }
    }
    return found;
}
