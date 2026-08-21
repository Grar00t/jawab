#include "jawab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* يدعم العربية: أي بايت > 127 يُعامل كحرف كلمة (UTF-8 safe) */
#define ISW(c) (isalnum((unsigned char)(c)) || (unsigned char)(c) > 127)

/* ---------------- FNV-1a ---------------- */
uint64_t jw_fnv1a(const char *s){
    uint64_t h = 1469598103934665603ULL;
    while (*s){ h ^= (unsigned char)*s++; h *= 1099511628211ULL; }
    return h;
}

/* ---------------- SHA-256 ---------------- */
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

/* ---------------- Tokenizer ---------------- */
static int tok_step(const char *s, size_t *i, char *buf, size_t cap){
    size_t n = 0;
    while (s[*i] && !ISW(s[*i])) (*i)++;
    while (s[*i] && ISW(s[*i]) && n < cap-1) buf[n++] = s[(*i)++];
    buf[n] = 0;
    return n > 0;
}

/* ---------------- Index ---------------- */
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

/* ---------------- BM25 Query ---------------- */
static size_t count_occ(const char *low, const char *term){
    size_t i = 0, c = 0; char buf[JAWAB_MAX_TERM];
    while (tok_step(low, &i, buf, sizeof buf))
        if (!strcmp(buf, term)) c++;
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
