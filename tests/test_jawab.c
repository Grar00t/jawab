#include "../src/jawab.h"
#include <stdio.h>
#include <string.h>

/* Validates jw_sha256() against official NIST SHA-256 test vectors and
 * jw_fnv1a() against the canonical FNV-1a 64-bit reference vectors
 * (isthe.com/chongo/tech/comp/fnv/). Run via `make test`. */

static int failures = 0;

static void check_sha(const char *input, const char *expected_hex, const char *label){
    uint8_t out[32];
    char hex[65];
    jw_sha256(input, strlen(input), out);
    jw_sha_hex(out, hex);
    int ok = strcmp(hex, expected_hex) == 0;
    printf("[sha256] %-20s got=%s\n", label, hex);
    printf("[sha256] %-20s exp=%s  %s\n", label, expected_hex, ok ? "OK" : "FAIL");
    if (!ok) failures++;
}

static void check_fnv(const char *input, uint64_t expected, const char *label){
    uint64_t got = jw_fnv1a(input);
    int ok = got == expected;
    printf("[fnv1a]  %-20s got=%016llx exp=%016llx  %s\n",
           label, (unsigned long long)got, (unsigned long long)expected, ok ? "OK" : "FAIL");
    if (!ok) failures++;
}

int main(void){
    check_sha("", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "empty string");
    check_sha("abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "\"abc\"");
    check_sha("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
              "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", "56-byte msg");

    check_fnv("", 0xcbf29ce484222325ULL, "empty string");
    check_fnv("a", 0xaf63dc4c8601ec8cULL, "\"a\"");
    check_fnv("foobar", 0x85944171f73967e8ULL, "\"foobar\"");

    jw_index_t ix; jw_init(&ix);
    jw_add_line(&ix, "the quick brown fox jumps over the lazy dog", "test");
    jw_add_line(&ix, "sovereignty means the query never leaves the machine", "test");
    jw_hit_t hits[JAWAB_TOPK];
    size_t n = jw_query(&ix, "sovereignty machine", hits, JAWAB_TOPK);
    int ok = (n == 1) && (strstr(hits[0].p->orig, "sovereignty") != NULL);
    printf("[bm25]   %-20s hits=%zu  %s\n", "basic relevance", n, ok ? "OK" : "FAIL");
    if (!ok) failures++;
    jw_free(&ix);

    printf("\nRESULT: %d failure(s)\n", failures);
    return failures ? 1 : 0;
}
