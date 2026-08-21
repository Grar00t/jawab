#include "jawab.h"
#include <stdio.h>
#include <string.h>

static void usage(void){
    fprintf(stderr,
        "jawab v0.2 - deterministic offline retrieval (BM25 + SHA-256 receipts)\n"
        "usage:\n"
        "  jawab ask \"query\" file1.txt [file2.txt ...]\n"
        "  jawab audit file1.txt [file2.txt ...]\n");
}

int main(int argc, char **argv){
    if (argc < 3){ usage(); return 2; }
    const char *cmd = argv[1];
    const char *query = NULL;
    int first_file;

    if (!strcmp(cmd, "ask")){
        if (argc < 4){ usage(); return 2; }
        query = argv[2];
        first_file = 3;
    } else if (!strcmp(cmd, "audit")){
        first_file = 2;
    } else { usage(); return 2; }

    jw_index_t ix; jw_init(&ix);
    int total = 0;
    for (int i = first_file; i < argc; i++){
        int n = jw_add_file(&ix, argv[i]);
        if (n < 0) fprintf(stderr, "[jawab] cannot read: %s\n", argv[i]);
        else total += n;
    }
    if (!total){ fprintf(stderr, "[jawab] empty corpus\n"); jw_free(&ix); return 1; }

    if (!query){ /* audit mode */
        for (size_t i = 0; i < ix.n; i++){
            char hex[65]; jw_sha_hex(ix.v[i].sha, hex);
            printf("%016lx  %s  | %s\n", ix.v[i].fp, hex, ix.v[i].orig);
        }
        jw_free(&ix);
        return 0;
    }

    jw_hit_t hits[JAWAB_TOPK];
    size_t n = jw_query(&ix, query, hits, JAWAB_TOPK);
    printf("[jawab] %d passages | query: %s\n\n", total, query);
    if (!n){
        printf("NO_MATCH score=0.0000 (deterministic empty result)\n");
        jw_free(&ix);
        return 0;
    }
    for (size_t r = 0; r < n; r++){
        char hex[65]; jw_sha_hex(hits[r].p->sha, hex);
        printf("#%zu score=%.4f fp=%016lx sha256=%s\n    src : %s\n    text: %s\n\n",
               r+1, hits[r].score, hits[r].p->fp, hex,
               hits[r].p->src, hits[r].p->orig);
    }
    jw_free(&ix);
    return 0;
}
