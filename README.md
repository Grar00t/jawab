# jawab v0.3 — Sovereign Upgrade

BM25 + FNV-1a + SHA-256 receipts, now with a **zero-copy, mmap'd persistent
index** and **SIMD-accelerated** term matching. Still zero dependencies,
zero network calls, and fully deterministic.

## Build
- Linux/macOS: `make` (auto-enables `-mavx2` on x86_64 via `uname -m`; force
  scalar with `make SIMD=off`)
- Windows: `.\build.ps1` (MSVC `cl /arch:AVX2` if available, else `gcc -mavx2`)

## Commands

    jawab ask "query" file1.txt [file2.txt ...]
    jawab audit file1.txt [file2.txt ...]
    jawab build-index out.jwidx file1.txt [file2.txt ...]
    jawab ask-idx "query" index.jwidx
    jawab audit-idx index.jwidx

`ask`/`audit` build the BM25 index fresh from plain text every run, same as
v0.2. `build-index` persists that index to a binary `.jwidx` file; `ask-idx`
and `audit-idx` then `mmap`/`MapViewOfFile` that file directly and query it
with **no heap allocation or copy of passage text** — every string the query
touches is a pointer into the mapped file.

## What's new in v0.3

**1. Portability fix.** `main.c` now prints the FNV-1a fingerprint as
`(unsigned long long)` with `%016llx` instead of `%016lx`. `unsigned long` is
32-bit on Windows/MSVC (LLP64) but 64-bit on Linux/macOS (LP64) — the old
code silently truncated fingerprints or triggered `/W4` warnings on MSVC.

**2. Zero-copy persistent index.** New on-disk format (`jw_idx_header_t` +
fixed-width `jw_idx_rec_t` records + a string blob), opened via `mmap`
(POSIX) or `CreateFileMapping`/`MapViewOfFile` (Windows). The whole file is
one contiguous read-only mapping; passage text is accessed as pointers into
that mapping, never copied into a separate buffer. This follows a
single-pool-of-memory model: one mapping owns everything, and closing it
(`jw_mmap_close`) is the only teardown step.

**3. SIMD term matching.** `count_occ`'s hot-path string comparison
(`jw_streq`) is gated on `__AVX2__`/x86_64 (32-byte `vpcmpeqb` compares) or
`__ARM_NEON__`/`aarch64` (16-byte `vceqq_u8` compares), with a portable
`memcmp` fallback when neither is available. This directly accelerates the
BM25 TF/IDF inner loop, which re-tokenizes and re-scans every passage per
query term.

**4. Forensic anchoring placeholder.** `jw_idx_header_t.root_sha[32]` is a
structural slot for the v1.0 proof-chain root hash over every passage's
SHA-256 receipt. In v0.3 it is written as all-zero and **not** verified on
load — it exists so the file format doesn't need to change again when
chaining lands.

## Guarantees (unchanged from v0.2, still hold)

- Same input -> same output, forever (deterministic).
- Every hit carries: BM25 score (explainable), FNV-1a fingerprint, SHA-256
  receipt.
- UTF-8/Arabic tokenization built in (bytes > 127 are word chars).

## Known limitations

- The `.jwidx` binary format assumes a same-endian, same-word-size host
  wrote and reads it. No cross-arch portability layer exists yet — reading
  a `.jwidx` built on one architecture from a different one is undefined.
- NEON `vminv_u8` targets ARMv8-A; it is not gated for older ARMv7 NEON.

## Roadmap

- v0.4: cross-arch `.jwidx` portability layer (explicit little-endian
  on-disk fields), plug into niyah.engine / Casper.DataForge behind
  `khz_q_svd` gate (>=0.85).
- v1.0: proof-chained retrieval — every hit's SHA-256 enters the `.proof`
  chain, and `root_sha` becomes a verified Merkle root checked on
  `jw_mmap_open`.
