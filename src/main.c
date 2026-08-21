#include "jawab.h"
#include <stdio.h>
#include <string.h>

static void usage(void){
    fprintf(stderr,
        "jawab v0.3 - sovereign offline retrieval (BM25 + SHA-256 receipts + mmap)\n"
        "usage:\n"
        "  jawab ask \"query\" file1.txt [file2.txt ...]      (build in-memory, query text corpus)\n"
        "  jawab audit file1.txt [file2.txt ...]              (dump receipts for a text corpus)\n"
        "  jawab build-index out.jwidx file1.txt [file2.txt ...]  (persist a zero-copy binary index)\n"
        "  jawab ask-idx \"query\" index.jwidx                 (zero-copy mmap query)\n"
        "  jawab audit-idx index.jwidx                        (dump receipts from a binary index)\n");
}

static int cmd_ask(const char *query, char **files, int nfiles){
    jw_index_t ix; jw_init(&ix);
    int total = 0;
    for (int i = 0; i < nfiles; i++){
        int n = jw_add_file(&ix, files[i]);
        if (n < 0) fprintf(stderr, "[jawab] cannot read: %s\n", files[i]);
        else total += n;
    }
    if (!total){ fprintf(stderr, "[jawab] empty corpus\n"); jw_free(&ix); return 1; }

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
        printf("#%zu score=%.4f fp=%016llx sha256=%s\n    src : %s\n    text: %s\n\n",
               r+1, hits[r].score, (unsigned long long)hits[r].p->fp, hex,
               hits[r].p->src, hits[r].p->orig);
    }
    jw_free(&ix);
    return 0;
}

static int cmd_audit(char **files, int nfiles){
    jw_index_t ix; jw_init(&ix);
    int total = 0;
    for (int i = 0; i < nfiles; i++){
        int n = jw_add_file(&ix, files[i]);
        if (n < 0) fprintf(stderr, "[jawab] cannot read: %s\n", files[i]);
        else total += n;
    }
    if (!total){ fprintf(stderr, "[jawab] empty corpus\n"); jw_free(&ix); return 1; }
    for (size_t i = 0; i < ix.n; i++){
        char hex[65]; jw_sha_hex(ix.v[i].sha, hex);
        printf("%016llx  %s  | %s\n", (unsigned long long)ix.v[i].fp, hex, ix.v[i].orig);
    }
    jw_free(&ix);
    return 0;
}

static int cmd_build_index(const char *out_path, char **files, int nfiles){
    jw_index_t ix; jw_init(&ix);
    int total = 0;
    for (int i = 0; i < nfiles; i++){
        int n = jw_add_file(&ix, files[i]);
        if (n < 0) fprintf(stderr, "[jawab] cannot read: %s\n", files[i]);
        else total += n;
    }
    if (!total){ fprintf(stderr, "[jawab] empty corpus, nothing to index\n"); jw_free(&ix); return 1; }

    if (jw_index_save(&ix, out_path) != 0){
        fprintf(stderr, "[jawab] failed to write index: %s\n", out_path);
        jw_free(&ix);
        return 1;
    }
    printf("[jawab] wrote %d passages -> %s (zero-copy binary index, v%u)\n",
           total, out_path, JW_IDX_VERSION);
    jw_free(&ix);
    return 0;
}

static int cmd_ask_idx(const char *query, const char *idx_path){
    jw_mmap_index_t mix;
    if (jw_mmap_open(&mix, idx_path) != 0){
        fprintf(stderr, "[jawab] cannot mmap index: %s\n", idx_path);
        return 1;
    }
    printf("[jawab] mmap'd %llu passages (zero-copy) | query: %s\n\n",
           (unsigned long long)mix.header->n_passages, query);

    jw_mmap_hit_t hits[JAWAB_TOPK];
    size_t n = jw_mmap_query(&mix, query, hits, JAWAB_TOPK);
    if (!n){
        printf("NO_MATCH score=0.0000 (deterministic empty result)\n");
        jw_mmap_close(&mix);
        return 0;
    }
    for (size_t r = 0; r < n; r++){
        char hex[65]; jw_sha_hex(hits[r].rec->sha, hex);
        printf("#%zu score=%.4f fp=%016llx sha256=%s\n    src : %s\n    text: %s\n\n",
               r+1, hits[r].score, (unsigned long long)hits[r].rec->fp, hex,
               hits[r].src, hits[r].orig);
    }
    jw_mmap_close(&mix);
    return 0;
}

static int cmd_audit_idx(const char *idx_path){
    jw_mmap_index_t mix;
    if (jw_mmap_open(&mix, idx_path) != 0){
        fprintf(stderr, "[jawab] cannot mmap index: %s\n", idx_path);
        return 1;
    }
    for (uint64_t i = 0; i < mix.header->n_passages; i++){
        const jw_idx_rec_t *rec = &mix.records[i];
        const char *orig = mix.blob + rec->orig_off;
        char hex[65]; jw_sha_hex(rec->sha, hex);
        printf("%016llx  %s  | %s\n", (unsigned long long)rec->fp, hex, orig);
    }
    jw_mmap_close(&mix);
    return 0;
}

int main(int argc, char **argv){
    if (argc < 2){ usage(); return 2; }
    const char *cmd = argv[1];

    if (!strcmp(cmd, "ask")){
        if (argc < 4){ usage(); return 2; }
        return cmd_ask(argv[2], argv + 3, argc - 3);
    }
    if (!strcmp(cmd, "audit")){
        if (argc < 3){ usage(); return 2; }
        return cmd_audit(argv + 2, argc - 2);
    }
    if (!strcmp(cmd, "build-index")){
        if (argc < 4){ usage(); return 2; }
        return cmd_build_index(argv[2], argv + 3, argc - 3);
    }
    if (!strcmp(cmd, "ask-idx")){
        if (argc < 4){ usage(); return 2; }
        return cmd_ask_idx(argv[2], argv[3]);
    }
    if (!strcmp(cmd, "audit-idx")){
        if (argc < 3){ usage(); return 2; }
        return cmd_audit_idx(argv[2]);
    }

    usage();
    return 2;
}
