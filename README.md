# jawab

**jawab** (جواب, Arabic for "answer") is a minimal, dependency-free question-answering
engine written in C11. It loads a plain-text corpus of question/answer pairs and
answers free-form queries by scoring token overlap between the query and every
stored question, then returning the best match above a confidence threshold.

No external libraries, no ML runtime — just a small, readable C11 engine you can
read end-to-end in a few minutes.

## Features

- Zero dependencies — pure C11, standard library only.
- Simple corpus format: `question|answer`, one pair per line.
- Jaccard-like token-overlap scoring for approximate matching.
- Works on Linux/macOS (`make`) and Windows (`build.ps1`).
- Bilingual out of the box (Arabic + English sample corpus).

## Project layout

```
jawab/
├── Makefile          # Linux/macOS build (gcc/clang)
├── build.ps1          # Windows build (PowerShell)
├── README.md
├── corpus/
│   └── seed.txt       # sample question|answer corpus
└── src/
    ├── jawab.h         # public API: corpus loading + answer matching
    ├── jawab.c         # implementation
    └── main.c          # REPL entry point
```

## Building

### Linux / macOS

```sh
make
./build/jawab corpus/seed.txt
```

### Windows (PowerShell)

```powershell
.\build.ps1
.\build\jawab.exe corpus\seed.txt
```

Requires `gcc` or `clang` on `PATH` (e.g. via MinGW-w64 or LLVM). Use
`.\build.ps1 -Clean` to remove build artifacts, or `.\build.ps1 -Run` to build
and immediately launch the REPL.

## Usage

```
$ ./build/jawab corpus/seed.txt
jawab: loaded 16 entries from 'corpus/seed.txt'
jawab - minimal C11 question-answering engine
Type a question, or ':q' to quit.

> what is jawab
jawab is a minimal C11 question-answering engine that matches queries against a corpus using token overlap scoring.  [score=0.83]
> :q
```

Pass a different corpus file as the first argument to use your own dataset:

```sh
./build/jawab path/to/my_corpus.txt
```

## Corpus format

Each non-comment line is a `question|answer` pair:

```
question text here|answer text here
# comments start with '#'
```

## How matching works

For each query, `jawab_answer()` tokenizes the input and every stored question
(lowercasing and stripping punctuation), then computes:

\[ \text{score} = \frac{|Q \cap A|}{|Q \cup A|} \]

where `Q` and `A` are the token sets of the query and the candidate question.
The highest-scoring entry is returned if its score clears
`JAWAB_MATCH_THRESHOLD` (default `0.15`) in `src/main.c`.

## API

See `src/jawab.h` for the full documented API:

- `jawab_corpus_init` / `jawab_corpus_free`
- `jawab_corpus_load(corpus, path)`
- `jawab_answer(corpus, query, &score)`

## License

MIT.
