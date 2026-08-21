# jawab v0.2 — Deterministic Offline Retrieval

BM25 + FNV-1a + SHA-256 receipts. Zero dependencies. Zero network. Arabic-aware.

## Build
- Linux/macOS: `make`
- Windows: `.\build.ps1`

## Run
    ./jawab ask "التلمتري" corpus/seed.txt
    ./jawab audit corpus/seed.txt

## Guarantees
- Same input -> same output, forever (deterministic).
- Every hit carries: BM25 score (explainable), FNV-1a fingerprint, SHA-256 receipt.
- UTF-8/Arabic tokenization built in (bytes > 127 are word chars).

## Roadmap
- v0.3: persistent index (serialize/mmap) + NEON/AVX2 in count_occ.
- v0.4: plug into niyah.engine / Casper.DataForge behind khz_q_svd gate (>=0.85).
- v1.0: proof-chained retrieval — every hit's sha256 enters the .proof chain.
