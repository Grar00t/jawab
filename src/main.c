#include "jawab.h"

#include <stdio.h>
#include <string.h>

#define JAWAB_MATCH_THRESHOLD 0.15

static void print_banner(void) {
    printf("jawab - minimal C11 question-answering engine\n");
    printf("Type a question, or ':q' to quit.\n\n");
}

int main(int argc, char **argv) {
    const char *corpus_path = (argc > 1) ? argv[1] : "corpus/seed.txt";

    jawab_corpus_t corpus;
    if (jawab_corpus_init(&corpus, 128) != 0) {
        fprintf(stderr, "jawab: failed to allocate corpus\n");
        return 1;
    }

    if (jawab_corpus_load(&corpus, corpus_path) != 0) {
        fprintf(stderr, "jawab: could not load corpus from '%s'\n", corpus_path);
        jawab_corpus_free(&corpus);
        return 1;
    }

    fprintf(stderr, "jawab: loaded %zu entries from '%s'\n", corpus.count, corpus_path);

    print_banner();

    char line[JAWAB_MAX_LINE];
    while (1) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) break;

        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0) continue;
        if (strcmp(line, ":q") == 0 || strcmp(line, ":quit") == 0) break;

        double score = 0.0;
        const char *answer = jawab_answer(&corpus, line, &score);

        if (!answer || score < JAWAB_MATCH_THRESHOLD) {
            printf("لا أعرف الإجابة على هذا. (no confident match, score=%.2f)\n", score);
        } else {
            printf("%s  [score=%.2f]\n", answer, score);
        }
    }

    jawab_corpus_free(&corpus);
    return 0;
}
