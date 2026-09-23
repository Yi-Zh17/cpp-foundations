# C++ Low-Latency Developer Roadmap — v3 (build-first, plain language)

**Revised:** 23 September 2026.
**Where you are:** strong C (wrote an in-memory filesystem), Rust-style intuition for ownership, ~100 LeetCode in C++, learncpp.com chapters 1–21 covered, CS:APP ch. 2–3 read, an AI-guided C++ KV store built, tooling set up.
**Gaps:** C++11-and-later idioms (moves, smart pointers, templates in anger, C++17/20/23 library), exceptions/`expected`, virtual functions, threads, Linux systems programming, networking, measuring performance properly.
**Target:** competitive for junior/mid C++ roles in HFT / quant trading, with C++ performance roles elsewhere as stepping stones.
**Timeline:** none fixed. Milestones are ordered, not dated.

---

## Ground rules

1. **Write every line yourself.** AI is in *hint mode*: you may ask "what concept am I missing?", "which standard function should I look at?", "is this undefined behaviour?", "review this after I wrote it". You may not ask for code. If it gives code anyway, close it and write your own from the idea only.
2. **Every session ends green.** A session isn't over until something you wrote compiles with `-Wall -Wextra -Werror` and at least one test passes — even one function. Then write one line in `notes.md`: what you did, and the exact first thing to do next time.
3. **Reading is a tool.** Max 15 minutes per session, only to unblock what you're building. If you're reading and not blocked, stop and build.
4. **No catch-up.** A missed session is skipped, never doubled.
5. **Every exercise ends with a number** in its README (nanoseconds per operation, a percentile, a size in bytes).
6. **Checkpoints gate phases.** Say the answers out loud without notes. If you can't, that gap is the next session's task.
7. **Sanitizers:** build every exercise once with `-fsanitize=address,undefined`; anything with threads also with `-fsanitize=thread`.

## Weekly template (committed)

| Slot | Content | Duration |
|---|---|---|
| Any 3 weekday mornings | One step from the current exercise. Ends green + one line in `notes.md`. | 45–60 min |
| Saturday | The current exercise's big step, its benchmark, or its write-up. | 2–3 h |

Bonus (never owed): extra mornings, Sunday, a talk while cooking.
Minimum viable session for bad-sleep days: 20 minutes, one function, one test, one line in `notes.md`. That counts.
Monthly: run the current checkpoint out loud; C++ Edinburgh meetup.

## How to read the exercises

Each exercise has: **Build** (what it is, in plain words), **Why** (what it forces you to learn), **Steps** (in order; each is roughly one morning, the bigger ones a Saturday), **Done when**, and **If stuck** (exactly where to look). Do the steps in order. Don't read ahead past the current step.

Repo layout for every exercise: `include/<name>.hpp`, `test/<name>_test.cpp` (Google Test), `bench/<name>_bench.cpp` (Google Benchmark), `README.md` with the numbers.

---

## Phase 1 — Learn the language by building it

### 0. `Histogram` — a latency histogram (do this first)

**Build:** A small class that you feed timing measurements into (in nanoseconds), and which can then tell you "half the measurements were under X ns, 99% were under Y ns, the worst was Z ns". Every later exercise reports its numbers through this class, so building it first means everything after it produces a visible result.

**Why:** It's a gentle warm-up on a C++ class with a clean interface, `std::chrono` (the standard timing library), `std::array`, `constexpr`, and getting a library + test + benchmark building under CMake. It also makes you confront that "how long did this take?" is harder than it looks.

**Steps:**
1. Write a function `uint64_t now_ns()` that returns the current time in nanoseconds using `std::chrono::steady_clock`. Write a test that calls it twice and checks the second value is ≥ the first. Get this building and passing. (That's a full session if CMake fights you — fine.)
2. Design the buckets. Simplest scheme: bucket `i` holds counts of values in `[2^i, 2^(i+1))`. So `record(700)` increments bucket 9 (since 512 ≤ 700 < 1024). Use `std::array<uint64_t, 64>` for the counts. Write `record(uint64_t ns)` and a test: record 1, 2, 3, 700; check the right buckets went up. Hint: the bucket index is the position of the highest set bit — `std::bit_width(ns) - 1` from `<bit>`.
3. Write `percentile(double p)` (e.g. `p = 0.99`): walk the buckets from 0 upward, summing counts, until the running sum reaches `p × total`; return that bucket's lower bound. Also track `min`, `max`, `count` exactly. Tests: record 1..1000, check `percentile(0.5)` is in the bucket containing 500.
4. Write `print(std::ostream&)` that prints a table: p50, p90, p99, p99.9, max, count. Wire it to a tiny `main` that records 1 million calls of `now_ns()` and prints the table. Look at the numbers — that's the cost of reading the clock.
5. Bonus (Saturday): add a second timer using the CPU's cycle counter (`__rdtsc()` from `<x86intrin.h>`). Record the same thing with both and put both tables in the README. Write two sentences about which you'd trust and why.

**Done when:** tests pass, README shows the cost of one `now_ns()` call and the cost of one `record()` call (benchmark it with Google Benchmark).

**If stuck:** cppreference `std::chrono::steady_clock`, `std::bit_width`; learncpp 8.x on `constexpr` if the buckets won't be compile-time.

---

### 1. `MyString` — your own string class

**Build:** A class that owns a heap-allocated character buffer, like `std::string`, with: construct from a C string, copy, move, assign, destroy, `size()`, `c_str()`, `operator==`, `operator+`. Then add the "small string optimisation": strings under ~15 characters live inside the object itself, with no heap allocation.

**Why:** This is *the* move-semantics exercise. In C you'd write `malloc`/`free` and copy manually; here you learn the five "special member functions" C++ will generate for you (copy constructor, copy assignment, move constructor, move assignment, destructor), why you have to write all five when you own memory, and what `std::move` really does (it's just a cast that says "you may steal from this").

**Steps:**
1. Constructor from `const char*` + destructor only. `new char[len+1]`, `strcpy`, `delete[]` in the destructor. Test: construct, check `size()` and `c_str()`. Run under AddressSanitizer — it should be clean.
2. Now write a test that copies one `MyString` into another and lets both go out of scope. Run under ASan. **It should crash with a double free.** Understand why: the compiler-generated copy just copied the pointer, so both objects `delete[]` the same buffer. This crash is the whole reason for the next steps.
3. Write the copy constructor (allocate a new buffer, copy the characters) and copy assignment (`operator=`). Handle `s = s;` (self-assignment) — test it. ASan clean again.
4. Write the move constructor: take the other object's pointer and size, then set the other's pointer to `nullptr` and size to 0. Mark it `noexcept`. Write move assignment the same way (free your own buffer first). Test: `MyString b = std::move(a);` then check `a.size() == 0` and `b` has the text. Read learncpp 22.1–22.4 *now* if what `std::move` does isn't clear — 15 minutes, then back.
5. Add `operator==`, `operator+`, and a constructor from `std::string_view` so it interoperates with standard code. Mark single-argument constructors `explicit` and test that `MyString s = "abc";` fails to compile without it (then decide which you want).
6. (Saturday) Small-string optimisation: put a `union` in the class holding either `{char* ptr; size_t cap;}` or `char inline_buf[16]`; a flag or the size tells you which is active. Every function now has two paths. Existing tests must still pass. This step is where you'll find out whether your special members were really right.
7. Benchmark: copy vs move of a 1 KB string; construct a 5-char string with and without SSO. Put the ns/op in the README.

**Done when:** all tests pass under ASan; README has the four numbers; you can explain out loud why each of the five special members is written the way it is.

**If stuck:** learncpp 14.x (copy constructors), 21.12 (copy assignment), 22.1–22.4 (move). "Rule of five" on cppreference.

---

### 2. `MyVector<T>` — your own growable array

**Build:** A class template like `std::vector<T>`: `push_back`, `emplace_back`, `size`, `capacity`, `operator[]`, `begin/end` so range-for works, `reserve`, and growth (double capacity when full). It must work for `T = int` *and* `T = MyString` (something with a destructor and moves) and must not leak or double-destroy if a copy throws part-way through growing.

**Why:** Templates for real (not just `template<typename T>` on a function), and the hardest idea in this phase: *allocating memory is separate from constructing objects in it*. `std::vector` allocates raw bytes for 8 elements but only 3 exist; the other 5 slots are uninitialised. You'll use `placement new` to build an object into raw memory and call destructors by hand. Also `std::forward` (passing constructor arguments through `emplace_back` without copying) and exception safety (what if construction throws?).

**Steps:**
1. Version 1 with `T* data = new T[cap]`. `push_back(const T&)`, `size`, `operator[]`. Test with `int`. Note the flaw: `new T[cap]` *constructs* `cap` objects, which is wrong for `T = MyString` (it constructs empty strings you don't want and requires a default constructor). Write that down; it's what step 3 fixes.
2. Growth: when `size == cap`, allocate double, copy elements over, free old. Test pushing 1000 ints. Then make it move elements instead of copying (`std::move` each one) — test with `MyString` and count that no copies happen (add a static copy counter to `MyString` for the test).
3. Switch to raw memory: allocate with `static_cast<T*>(::operator new(cap * sizeof(T)))` (bytes only, no objects), construct with `new (data + i) T(value)` (placement new — builds an object at that address), destroy with `data[i].~T()`, free with `::operator delete(data)`. All tests must still pass. ASan will catch you if you destroy something you never constructed.
4. `emplace_back(Args&&... args)` — variadic template that forwards its arguments straight to `T`'s constructor: `new (data + size) T(std::forward<Args>(args)...)`. Test: `v.emplace_back("hello")` for `MyVector<MyString>` constructs exactly one string, no temporary. Read learncpp 22.x on `std::forward` / perfect forwarding if this is fuzzy.
5. Iterators: `T* begin()` and `T* end()` are enough for range-for and `std::sort`. Test both. Add `const` overloads.
6. Exception safety (Saturday): make a test type whose copy constructor throws on the 3rd copy. Push 5 of them so growth happens mid-copy. Under ASan, your vector must leave the *old* contents intact and leak nothing. Fix it: build the new buffer completely before touching the old one; use `std::move_if_noexcept` so types with `noexcept` moves get moved and others get copied. Read learncpp 27.x (exceptions) *now*, ~15 min.
7. Benchmark: `push_back` ns/op vs `std::vector` at 1,000 and 1,000,000 elements; growth factor 1.5 vs 2. README.

**Done when:** works for `int` and `MyString`, the throwing test passes under ASan, benchmark numbers are within ~2× of `std::vector`.

**If stuck:** cppreference "placement new", `std::move_if_noexcept`, `std::uninitialized_move`; learncpp 26.x (class templates), 27.x (exceptions).

---

### 3. `MyUniquePtr<T, Deleter>` and `MySharedPtr<T>`

**Build:** Two smart pointers. `MyUniquePtr` owns one heap object, cannot be copied, can be moved, deletes the object in its destructor, and accepts a custom deleter (e.g. `fclose` for a `FILE*`). `MySharedPtr` lets several owners share one object and deletes it when the last owner goes away, using a reference count. Also `make_unique`/`make_shared` helpers and a `MyWeakPtr` that observes without owning.

**Why:** RAII (resource cleanup tied to object lifetime) is the C++ answer to every `free` you forgot in C. You'll see *why* `unique_ptr` can't be copied (two owners would double-delete — you already met that in exercise 1), how a stateless deleter can cost zero bytes, and what a `shared_ptr` control block is and why `shared_ptr` is slower than people think.

**Steps:**
1. `MyUniquePtr<T>`: constructor from `T*`, destructor calls `delete`, `operator*`, `operator->`, `get()`. Copy constructor and copy assignment `= delete`. Test that copying fails to compile (`static_assert(!std::is_copy_constructible_v<MyUniquePtr<int>>)`).
2. Move constructor/assignment (steal the pointer, null the source). `release()` and `reset()`. Test moving into a `MyVector<MyUniquePtr<int>>` — this is a nice check that exercise 2 handles move-only types.
3. Deleter template parameter with a default `struct DefaultDelete { void operator()(T* p) const { delete p; } }`. Test with a custom deleter that increments a counter. Then check `sizeof(MyUniquePtr<int>) == sizeof(int*)` — if it's bigger, the deleter is taking space; fix with `[[no_unique_address]]` (C++20) or by inheriting from it.
4. `make_unique<T>(args...)` — forwards args to `new T(...)`. Test.
5. `MySharedPtr<T>`: a pointer to `T` plus a pointer to a heap `ControlBlock { long strong; long weak; }`. Copy increments `strong`; destructor decrements and deletes the object at zero. Test: three copies, destroy them in different orders, object destroyed exactly once (count with a destructor-counting test type).
6. `MyWeakPtr<T>`: copies the control block pointer, increments `weak`; `lock()` returns a `MySharedPtr` if `strong > 0`, otherwise empty. Control block is freed when both counts hit zero. Test the "object gone, weak pointer still alive" case.
7. `make_shared` that allocates the object and control block in *one* allocation (a struct holding both). Benchmark: `make_shared` vs `MySharedPtr(new T)`; copying a shared vs unique pointer; `unique_ptr` deref vs raw deref (should be identical in assembly — check in godbolt). README.

**Done when:** all tests pass under ASan, `sizeof(MyUniquePtr<int>) == 8`, README has the numbers and the size of your control block.

**If stuck:** learncpp 22.5–22.7; cppreference `unique_ptr`, `shared_ptr` ("Implementation notes" section).

---

### 4. `MyOptional<T>` — a maybe-value, without the heap

**Build:** A class that either holds a `T` or holds nothing, like `std::optional<T>`. Nothing on the heap: the `T` lives inside the `MyOptional` object, in storage that's only constructed when there's a value.

**Why:** Same "storage vs object" idea as exercise 2 but in a tiny package, plus unions with non-trivial members (a C union can't hold a `MyString`; a C++ one can, but *you* manage its lifetime), and overloads by value category (`value() &` vs `value() &&` — calling on a temporary should let you move out).

**Steps:**
1. Storage: `union { char dummy; T value; }` plus a `bool has_value`. Because `T` might have a constructor/destructor, you must write the union's constructor/destructor yourself (empty, to do nothing). Default `MyOptional` = no value. Test `has_value() == false`.
2. Constructor from `const T&` and from `T&&`: `new (&value) T(...)`, set the flag. Destructor: if `has_value`, `value.~T()`. Test with `MyString` under ASan.
3. Copy/move constructors and assignments — four cases each time (both empty, only source has value, only target has value, both). Test every case with a destructor-counting type.
4. `value()` overloaded for `&`, `const&`, `&&` (the `&&` version returns `T&&` so `std::move(opt).value()` moves out). `operator*`, `operator->`, `value_or(default)`. `emplace(args...)`. Tests.
5. `sizeof` check: `sizeof(MyOptional<int>)` should be 8 (4 + bool + padding), `sizeof(MyOptional<MyString>)` should be `sizeof(MyString) + 8`. Put both in the README with one sentence explaining the padding. (Stretch: make it `constexpr`-usable for trivial `T` — you'll need C++20's conditionally-trivial special members; skip if it eats more than one session.)

**Done when:** tests pass under ASan; you can explain out loud why `union` is used instead of `T value; bool has;`.

**If stuck:** cppreference `std::optional` (the "Notes" section), learncpp 13.x on unions if they're new.

---

### 5. `MyFunction<R(Args...)>` — a box that holds any callable

**Build:** Like `std::function`: you can store a lambda, a function pointer, or a functor object in it, and call it later without knowing which it is. Small callables (≤ 2 pointers) are stored inside the object; big ones go on the heap.

**Why:** Lambdas in depth (captures, `mutable`, generic lambdas), *type erasure* (hiding a type behind a fixed interface — the technique behind `std::function`, `std::any`, and most plugin systems), and the cost of an indirect call, which matters in every hot path you'll ever write.

**Steps:**
1. Version 1, heap-only, using a virtual interface: `struct Callable { virtual R call(Args...) = 0; virtual ~Callable() = default; }` and `template<class F> struct Impl : Callable { F f; ... }`. `MyFunction` holds a `MyUniquePtr<Callable>`. Template constructor `template<class F> MyFunction(F f)`. Test with a lambda, a function pointer, and a lambda with captures. This is the "how do you store a type you don't know?" answer: you wrap it.
2. Copy support: add `virtual MyUniquePtr<Callable> clone()`. Test copying a `MyFunction` holding a lambda that captures a `MyString`.
3. Replace the virtual interface with two function pointers (`R (*invoke)(void*, Args...)` and `void (*destroy)(void*)`) plus a `void* storage`. Same tests. Look at both versions in godbolt and note how many indirections a call takes.
4. Small-buffer optimisation: `alignas(void*) char buffer[16]`; if `sizeof(F) <= 16` and `F` is nothrow-movable, construct it in the buffer instead of the heap. Test: a lambda capturing one `int` causes zero heap allocations (override `operator new` in the test to count).
5. Benchmark: call overhead of a direct call, `MyFunction`, `std::function`, and a plain function pointer, on a lambda that adds two ints. README.

**Done when:** tests pass, small lambdas allocate nothing, README has the call costs.

**If stuck:** learncpp 20.x (lambdas); cppreference `std::invoke`, `std::function` implementation notes; search "type erasure C++ explained".

---

### 6. Modern C++ sprint — ten tiny builds, one per session

Each is ≤ 40 lines plus a test. Read the cppreference page for the feature *after* your first attempt, then fix what you got wrong. Put all ten in one folder `modern/`.

1. **`std::expected` (C++23, or `tl::expected` if your compiler lacks it):** write `parse_int(std::string_view) -> std::expected<int, ParseError>`, then `parse_pair` that calls it twice and propagates the first error. No exceptions anywhere. Test both paths.
2. **`std::variant` + `std::visit`:** `using Shape = std::variant<Circle, Square>`; write `area(const Shape&)` with `std::visit` and an `overloaded{}` helper struct (search "overloaded lambda idiom"). Test.
3. **`std::span<const std::byte>`:** function `read_be32(std::span<const std::byte> buf, size_t offset) -> uint32_t` that reads a big-endian 32-bit integer. Test with the bytes `01 02 03 04` → `0x01020304`. (Preview of Phase 6's ITCH parser.)
4. **Structured bindings, `if constexpr`, fold expressions:** a `sum(args...)` that folds `+`; a `print_all(args...)` that uses `if constexpr` to handle strings differently; unpack a `std::pair` with `auto [a, b]`.
5. **Concepts:** write `template<class T> concept Hashable = requires(T t) { { std::hash<T>{}(t) } -> std::convertible_to<size_t>; };` and constrain `MyVector<T>`'s `T` with `std::movable`. Test that a bad type gives a *short* error.
6. **Ranges:** three pipelines (`views::filter | views::transform | views::take`) over a vector; then write the same as a raw loop and compare the two in godbolt at `-O2`.
7. **`constexpr`/`consteval`:** a compile-time CRC32 lookup table (`constexpr std::array<uint32_t,256>` built by a `consteval` function). `static_assert` one known value.
8. **`<bit>`:** `std::bit_cast<uint32_t>(1.0f)`, `std::byteswap`, `std::endian::native`, `std::popcount`. Rewrite build 3 with these.
9. **`std::format` / `std::print`:** reformat exercise 0's histogram table with `std::format`.
10. **`std::jthread` + `std::stop_token`:** a thread that counts up until asked to stop, joined automatically. (Preview of Phase 4.)

**Done when:** ten green tests. Write one line per feature in the README: "when I'd use this instead of X".

---

### 7. Dispatch three ways — virtual, `variant`, CRTP

**Build:** A tiny "message" type family (say `Add`, `Cancel`, `Trade`, each with a `process()` that does a little arithmetic) implemented three times: (a) a base class with `virtual process()`; (b) `std::variant<Add,Cancel,Trade>` with `std::visit`; (c) CRTP (the base is a template on the derived class, so the call is resolved at compile time). One shared test suite runs against all three.

**Why:** Inheritance and virtual functions are the last big language topic you haven't used, and in low-latency code the *cost* of a virtual call — and the alternatives — is a standard interview question. You'll measure it rather than take it on faith.

**Steps:**
1. Version (a): base class, three derived, `override`, `final`, virtual destructor. Store them as `MyVector<MyUniquePtr<Base>>`. Test. Read learncpp 24.1–24.5 and 25.1–25.4 while doing this if inheritance syntax is unfamiliar — that's the only reading.
2. Look at the vtable: compile with `-fdump-lang-class` (GCC) or view in godbolt; find the vtable and `sizeof(Base)`. Write in the README what a virtual call does step by step (load vptr, load function pointer, call).
3. Version (b) with `std::variant` — reuse build 6.2's `overloaded` helper. Objects stored *by value* in a `MyVector<Message>`. Same tests.
4. Version (c) CRTP: `template<class Derived> struct Base { void process() { static_cast<Derived*>(this)->process_impl(); } };`. Note you can't put different `Derived` types in one vector — write down *why* (that's the trade-off).
5. (Saturday) Benchmark all three on a 1M-element array, once with types in random order and once sorted by type. Explain the difference between random and sorted in the README (branch prediction, exercise 2.4 will make this concrete).

**Done when:** three versions pass the same tests; README has six numbers and a paragraph on when you'd pick each.

**If stuck:** learncpp 24–25; search "CRTP explained"; Godbolt talk from Phase 2 for reading the assembly.

---

### 8. Expression evaluator — the integration project

**Build:** A program that takes a string like `(3 + 4) * 2 - 10 / 5` and returns `4`. Three stages: tokenizer (string → list of tokens), parser (tokens → tree), evaluator (tree → number). Errors (`3 + * 4`) are returned as `std::expected`, never thrown. Uses your `MyVector`, `MyString`, `MyUniquePtr` and `std::variant` where they fit.

**Why:** Everything in this phase under one build, across multiple files — which forces headers, `#include` discipline, `inline`, and "why does the linker say duplicate symbol" (ODR). It's also a real program you can show.

**Steps:**
1. `Token` as a `std::variant<Number, Operator, LParen, RParen>`; `tokenize(std::string_view) -> std::expected<MyVector<Token>, Error>`. Tests including an unknown character error.
2. Expression tree: `Expr` nodes as a `variant<Number, BinaryOp>` where `BinaryOp` holds two `MyUniquePtr<Expr>`. Write the recursive-descent parser for `+ - * /` with correct precedence and parentheses (search "recursive descent parser precedence" — 15 min). Tests: `1+2*3` → 7, `(1+2)*3` → 9.
3. Evaluator with `std::visit`. Division by zero → error via `expected`.
4. Split into `tokenizer.hpp/.cpp`, `parser.hpp/.cpp`, `eval.hpp/.cpp`, `main.cpp`. Deliberately define the same non-`inline` function in two `.cpp` files, read the linker error, then fix it with `inline`/`static`/an anonymous namespace. Write two sentences in the README about what ODR means.
5. (Saturday) A REPL `main` that reads lines and prints results or errors. Benchmark: evaluations per second of a 50-token expression; `perf record` it and look at where the time goes (first taste of Phase 2).

**Done when:** the REPL works, tests cover errors, README has the number and the ODR paragraph.

---

### Ongoing (optional): 2 LeetCode mediums a week in C++, STL only. Upkeep, not learning.

### Phase 1 checkpoint — say out loud, no notes:
- What happens, step by step, when `std::vector<std::string> v; v.push_back("abc");` runs (allocation, construction of a temporary string, move into storage, destruction of the temporary).
- Write a class with correct move semantics from memory and justify every special member.
- Read a template error and find the actual problem.
- Name three UB cases from your own exercises and which sanitizer caught each.
- The difference between `const T&`, `T&&`, `auto&&` parameters and when to use each.
- Why exercise 7's three versions have the numbers they have.

---

## Phase 2 — How the machine runs your code

You've read CS:APP 2–3 and know some RISC-V, so this phase is mostly benchmarks that make the remaining chapters necessary. Repo: `perf-lab`. Every benchmark reports through your `Histogram`.

**Reading, only when the step says so:** CS:APP ch. 6 (memory hierarchy) before exercise 2; ch. 5 (optimising) before exercise 3. Agner Fog's *Optimizing software in C++* is a reference to open when a number surprises you.
**Talks (listen while doing chores, then reproduce one claim from each as a benchmark):** Godbolt "What Has My Compiler Done for Me Lately?"; Carruth "Efficiency with Algorithms, Performance with Data Structures"; Doumler "Want Fast C++? Know Your Hardware"; Meyers "CPU Caches and Why You Care".

### 2.1 Godbolt prediction game

**Build:** Nothing to commit — a habit. Open godbolt.org, write a 5–10-line function, *predict on paper* what the x86-64 assembly at `-O2` will be, then compile and compare.

**Why:** "Read assembly fluently" is what lets you check whether the compiler did what you meant. You know RISC-V; x86-64 differs in calling convention (`rdi, rsi, rdx, rcx` for the first args, `rax` for the return), `lea` doing arithmetic, and SSE registers (`xmm0`) for floats.

**Steps:** Ten functions over ten sessions, 15 minutes each, at the start of a session before other work: (1) `int add(int a,int b)`; (2) a `for` loop summing an array; (3) the same with `float`; (4) a function that calls another non-inlined function; (5) a `switch` on 4 cases; (6) `std::vector<int>::size()`; (7) a virtual call from exercise 7; (8) `std::sort` on 8 ints; (9) a `constexpr` function called with a constant; (10) a struct passed by value vs by `const&`. Keep a `predictions.md`: your guess, what happened, what you learned. Repeat any you got badly wrong the following week.

**Done when:** predictions for simple loops and calls are mostly right.

### 2.2 `vector` vs `list` vs `vector<Node*>` traversal

**Build:** Fill each of `std::vector<int>`, `std::list<int>`, and `std::vector<Node*>` (Nodes allocated individually) with 10 million values. Time summing all elements of each. Then run each under `perf stat -e cache-misses,cache-references,instructions,cycles`.

**Why:** The single most important performance fact for HFT: memory access, not arithmetic, dominates. Contiguous data is fast because the CPU loads 64-byte cache lines and prefetches sequential ones; pointer chasing defeats both. Read CS:APP ch. 6.2–6.4 *before* this exercise (it's the one chapter worth reading in full) so the numbers mean something.

**Steps:** (1) Write the three fills and sums, with `benchmark::DoNotOptimize(sum)` so the compiler can't remove the loop. (2) Get ns per element for each. (3) `perf stat` each; put the cache-miss counts next to the timings. (4) Repeat at 10k elements (fits in cache) and see the gap shrink. (5) README: a table and a paragraph explaining the gap using "cache line" and "prefetcher".

**Done when:** you can predict the ratio before running it at a new size.

### 2.3 AoS vs SoA particle update

**Build:** 1M particles, each with `x, y, z, vx, vy, vz`. Version A: `std::vector<Particle>` (array of structs). Version B: six `std::vector<float>` (struct of arrays). The update is `x += vx * dt` for all three axes. Benchmark both. Then get the compiler to auto-vectorise B (use SIMD instructions that do 8 floats at once) and prove it in the assembly.

**Why:** Data layout is a design decision, and SoA is often 4–8× faster because it vectorises and doesn't load fields you don't touch. Read CS:APP ch. 5.7–5.9 before this.

**Steps:** (1) Both versions, same result check. (2) Benchmark at `-O2`, then `-O3 -march=native`. (3) Compile B with `-fopt-info-vec` (GCC) or `-Rpass=loop-vectorize` (Clang) to see whether the loop vectorised; look for `vmulps`/`vaddps` on `ymm` registers in godbolt. (4) Try to make A vectorise too and see why it's harder. (5) README with the four numbers and a sentence on what `-march=native` enabled.

### 2.4 Sorted vs unsorted branchy loop, then branchless

**Build:** An array of 1M random bytes; count how many are ≥ 128, using `if (x >= 128) count++`. Time it on random data, then on the same data sorted. Then rewrite as `count += (x >= 128)` and time both again.

**Why:** Branch prediction. The sorted version is much faster because the branch becomes predictable; the branchless version doesn't care. In a trading hot path unpredictable branches cost ~15–20 cycles each.

**Steps:** (1) Four timings. (2) `perf stat -e branches,branch-misses` for each. (3) Check in godbolt that the branchless version compiled to `cmov` or `setge`. (4) README. (5) Go back to exercise 7's sorted-vs-random result and add a sentence now that you know why.

### 2.5 Matrix multiply, four ways

**Build:** 1024×1024 float matrix multiply: (1) naive `i,j,k` loops; (2) loop order `i,k,j`; (3) blocked/tiled (do 64×64 sub-blocks so they stay in L1); (4) blocked + `-O3 -march=native`. Measure GFLOP/s and `perf stat` L1/L2 misses for each.

**Why:** The classic cache-optimisation exercise; every step is explained by cache lines and the memory hierarchy, and it's a common interview whiteboard.

**Steps:** One version per session, a table in the README, and one sentence per row explaining the change. Compare the final to a BLAS call (`cblas_sgemm`) so you know how far off hand-written is.

### 2.6 Flamegraph your own code

**Build:** `perf record -g` on the exercise 2 (`MyVector`) benchmark, generate a flamegraph (Brendan Gregg's `flamegraph.pl`, or `perf report` in the terminal), find the widest box, change one thing, re-record.

**Why:** Being able to answer "where does the time go?" in ten minutes is a basic professional skill.

**Steps:** (1) Build with `-g -O2 -fno-omit-frame-pointer`. (2) `perf record -g ./bench` then `perf script | stackcollapse-perf.pl | flamegraph.pl > out.svg`. (3) Open the SVG, identify the hot function. (4) Make one change. (5) README with before/after SVGs and numbers.

### 2.7 Benchmark hygiene write-up

**Build:** Re-run exercise 2.2 *properly*: pin to one core (`taskset -c 2`), warm up before measuring, run 10 repetitions and report the histogram (p50/p99/max, not the mean), check whether CPU frequency scaling is on (`/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor`), and use `benchmark::DoNotOptimize` / `ClobberMemory` correctly.

**Why:** A number without this is a number a senior engineer will dismiss. This write-up becomes a template you copy into every later README.

**Steps:** (1) Do each item and note what changed in the numbers. (2) Write `BENCHMARKING.md` in the repo: a checklist you'll reuse. (3) Blog post.

### Phase 2 checkpoint — out loud:
- Look at a hot loop and say which memory accesses will miss cache and why.
- Why `std::vector` beats `std::list` even for inserts in the middle at small N.
- Produce and read a flamegraph in under ten minutes.
- What `-march=native` changes; show it in assembly.
- Write a benchmark a sceptical senior engineer would trust.

---

## Phase 3 — Data structures and memory management

Start right after the Phase 1 checkpoint; Phase 2 exercises can interleave. Repo: `fast-containers`. Each container is benchmarked against its `std` equivalent using your `Histogram` and your `BENCHMARKING.md` checklist.

**Reading, only when a step says so:** CS:APP ch. 9.9 (dynamic memory allocation) before 3.2.
**Talks — watch each *after* your first attempt, then improve your version:** Skarupke "You Can Do Better than std::unordered_map"; Kulukundis "Designing a Fast, Efficient, Cache-friendly Hash Table"; Carruth "Hybrid Data Structures".

### 3.1 `FlatHashMap<K, V>` — an open-addressing hash table

**Build:** A hash map where all entries live in one flat array (no linked lists, no per-node allocation). Lookup: hash the key, take `hash & (capacity-1)` as the starting slot, and walk forward until you find the key or an empty slot ("linear probing"). Keys and values are stored inline. Deletion uses "tombstone" markers. When more than ~7/8 of slots are used, double the capacity and re-insert everything.

**Why:** `std::unordered_map` allocates a node per entry and chases pointers — exactly what exercise 2.2 showed is slow. A flat table is the standard fast replacement, and "implement one in an hour" is a real interview task.

**Steps:**
1. Storage: `capacity` always a power of two; a `std::vector<Slot>` where `Slot { K key; V value; }` plus a separate `std::vector<uint8_t> meta` where each byte is `EMPTY`, `FULL`, or `TOMBSTONE`. `insert`, `find`, `size`. Only `insert` and `find` — no growth, no erase. Test with 100 ints in a capacity-256 table.
2. Growth: when `size > capacity * 7/8`, allocate a table of double capacity and re-insert every `FULL` slot. Test inserting 1M ints.
3. `erase`: mark the slot `TOMBSTONE` (you can't just mark it empty — it would break probe chains for keys that were stored past it; write a test that proves this by erasing the middle of a chain). Count tombstones toward the growth threshold.
4. Templates: make `K`, `V`, and the hash function template parameters; test with `std::string` keys and with `MyString` keys (needs a hash — write one, or specialise `std::hash`).
5. Iteration (`begin/end` over `FULL` slots) and `operator[]`.
6. (Saturday) Benchmark insert/find/erase against `std::unordered_map` at 10k, 1M, 10M entries with random 64-bit keys. Expect 2–5× faster on find. If not, `perf stat` cache misses and find out why.
7. Watch Skarupke's talk. Implement one improvement he describes (e.g. Robin Hood probing or a better hash for integer keys). Re-benchmark.

**Done when:** all tests pass under ASan; benchmark table in README; you can write steps 1–3 again from memory in under an hour.

**If stuck:** search "open addressing linear probing explained"; Skarupke's blog post "I Wrote The Fastest Hashtable".

### 3.2 `ArenaAllocator` and `PoolAllocator<T>`

**Build:** Two custom allocators. **Arena**: grab one big block up front; `allocate(n)` just bumps a pointer forward; you never free individual objects, you reset the whole arena at once. **Pool**: for objects of one fixed size, keep a free list of slots; `allocate` pops one, `deallocate` pushes it back. Both must be usable as the `Allocator` template parameter of `std::vector`, and later as a `std::pmr::memory_resource`.

**Why:** `malloc` costs 20–100 ns and can lock; in a hot path you pre-allocate everything and hand out memory in a few instructions. Every trading system has these two. Read CS:APP 9.9 first (30 min, one session — it's the exception to the 15-minute rule, and you wrote a filesystem so it'll be familiar).

**Steps:**
1. `Arena`: constructor takes a byte count, `void* allocate(size_t bytes, size_t align)`, `reset()`. Handle alignment (round the bump pointer up to a multiple of `align`). Test: allocate 3 things, check addresses are non-overlapping and aligned.
2. `Pool<T>`: one block of `N * sizeof(T)` bytes; a free list threaded *through the free slots themselves* (each free slot holds a pointer to the next free slot — same trick as a filesystem free block list). `T* allocate()`, `void deallocate(T*)`. Test allocate-all, free-all, allocate-all again.
3. Make `Pool` fit the STL allocator interface (`value_type`, `allocate(n)`, `deallocate(p, n)`, equality, rebind). Test `std::vector<int, PoolAllocator<int>>` compiles and works. cppreference "Allocator requirements" is the reference here.
4. Implement both as `std::pmr::memory_resource` (override `do_allocate`, `do_deallocate`, `do_is_equal`). Test `std::pmr::vector<int> v(&my_arena)`.
5. Benchmark: 1M `push_back` into `std::vector<int>` with default vs pool allocator; 1M small allocations via `malloc` vs arena vs pool. README.

**Done when:** tests pass under ASan (note: ASan can't see overflows *inside* your arena — write a debug mode that poisons freed slots).

### 3.3 `IntrusiveList<T>`

**Build:** A doubly-linked list where the `next`/`prev` pointers live *inside* the user's object (as a member `ListHook`), rather than in a separately allocated node. Given a pointer to an object, you can unlink it in O(1) with no lookup and no allocation.

**Why:** This is how the order book in Phase 6 keeps orders at each price level: cancel by order ID → pointer → unlink, no search, no `malloc`. Also how the Linux kernel does every list.

**Steps:** (1) `struct ListHook { ListHook* next; ListHook* prev; }`; `IntrusiveList<T, &T::hook>` with `push_back(T&)`, `erase(T&)`, `begin/end`. Getting from a `ListHook*` back to the `T*` needs `offsetof` or a member pointer — same as the kernel's `container_of`. (2) Tests: insert 3, erase middle via pointer, iterate. (3) Benchmark unlink vs `std::list::erase` (which needs an iterator you have to have kept).

### 3.4 `SmallVector<T, N>`

**Build:** A vector that stores up to `N` elements inline (inside the object, no heap) and only allocates when it grows past `N`. Same interface as `MyVector`.

**Why:** Most vectors in real code hold 1–4 things; skipping the heap for those is a common, large win. It's also a direct reuse of exercise 1's SSO trick at a bigger scale.

**Steps:** (1) Start from `MyVector`; add `alignas(T) char inline_buf[N * sizeof(T)]` and a pointer that either points into it or to the heap. (2) All `MyVector` tests must pass, plus tests crossing the N boundary in both directions. (3) Benchmark `push_back` of 4 elements vs `std::vector`; count allocations.

### 3.5 `RingBuffer<T, N>`

**Build:** A fixed-capacity circular queue: `push` writes at `head`, `pop` reads at `tail`, both indices wrap using `& (N-1)` (N is a power of two). Single-threaded for now.

**Why:** The Phase 4 lock-free queue and the Phase 6 logger are this structure plus atomics; get the single-threaded version right first.

**Steps:** (1) `std::array<T, N>` storage, `head`, `tail` as `size_t` that count up forever (never reset); `full()` is `head - tail == N`. `push`, `pop`, `size`. Tests including wrap-around. (2) `static_assert` N is a power of two. (3) Benchmark push+pop pair in ns.

### 3.6 `FlatMap<K, V>`

**Build:** An ordered map stored as a sorted `std::vector<std::pair<K,V>>` with binary search for lookup and insertion. Find the number of elements at which `std::map` (a red-black tree) becomes faster.

**Why:** For small N, sorted-array beats tree because it's contiguous; knowing the crossover, and that it's much larger than people expect, is the kind of thing you'll be asked.

**Steps:** (1) `insert`, `find`, `erase` via `std::lower_bound`. Tests. (2) Benchmark find and insert vs `std::map` at N = 8, 64, 512, 4096, 65536. (3) README: the crossover, and why.

### 3.7 SSE2 group probing for `FlatHashMap`

**Build:** Change 3.1's metadata bytes to hold 7 bits of the key's hash (plus a bit for empty/tombstone). Lookup now loads 16 metadata bytes at once with `_mm_loadu_si128`, compares all 16 against the wanted 7 bits with `_mm_cmpeq_epi8`, and gets a 16-bit mask with `_mm_movemask_epi8` telling you which slots are worth checking. This is the "Swiss table" design behind `absl::flat_hash_map`.

**Why:** Your first SIMD intrinsics, used for something that isn't arithmetic. Watch Kulukundis's talk *before* this one — it's the only talk that's better watched first.

**Steps:** (1) Change the meta layout; all 3.1 tests still pass (no SIMD yet). (2) Add the 16-byte group lookup. (3) Benchmark vs 3.1 and vs `absl::flat_hash_map` (FetchContent it). (4) README with the three-way comparison.

### Phase 3 checkpoint — out loud:
- Implement an open-addressing hash table from scratch in under an hour, no reference.
- Why `std::unordered_map` is slow and what a fast replacement changes.
- Write a custom allocator and plug it into a std container.
- Choose a container for a given access pattern and justify it with a cache-line argument.

---

## Phase 4 — Concurrency

May start once 3.5 (`RingBuffer`) is done. Repo: `concurrency-lab`. Every exercise runs under ThreadSanitizer (`-fsanitize=thread`) and reports through `Histogram`.

**Reading, only when a step says so:** *C++ Concurrency in Action* ch. 2–4 before 4.1 (one chapter per session); ch. 5 before 4.3, read *with your SPSC queue open in the editor*; ch. 7 before 4.4. Preshing's memory-ordering posts (preshing.com, "Memory Ordering at Compile Time" onwards) alongside ch. 5 — each has a small example; reproduce it and break it under TSan.
**Talks — after your first attempt at the related exercise:** Sutter "atomic<> Weapons" 1 & 2; Pikus "C++ atomics, from basic to advanced" and "The Speed of Concurrency"; Vyukov's queue notes on 1024cores.net.

### 4.1 Thread pool with mutex + condition variable

**Build:** N worker threads pull tasks (`MyFunction<void()>`) from a shared queue. `submit(task)` locks a mutex, pushes, and notifies a condition variable; workers wait on the condvar, pop, run. `shutdown()` stops them cleanly. Measure the delay between `submit()` and the task starting to run.

**Why:** The basic building blocks (`std::thread`/`jthread`, `std::mutex`, `std::lock_guard`, `std::condition_variable`) and their real cost: a condvar wake-up costs several microseconds, which is why hot paths avoid it. You'll measure that.

**Steps:**
1. One worker, no pool: a `std::jthread` that loops on a `std::atomic<bool> running`. Test it starts and stops.
2. Shared `std::deque<Task>` + `std::mutex` + `std::condition_variable`. `submit` and worker loop. The worker's wait must use a predicate: `cv.wait(lock, [&]{ return !queue.empty() || stopping; })` — a wake-up without the predicate being true is a "spurious wakeup" and is legal. Test 1000 tasks all run exactly once (atomic counter).
3. N workers. Shutdown: set `stopping`, `notify_all`, join. Test under TSan — it should be clean; if not, you've found a real race.
4. Latency: each task records `now_ns()` at submit and at start; feed the difference to `Histogram`. Report p50/p99/max with 1 worker and with 8.
5. README: the histogram and one paragraph on what the microseconds are (futex syscall, scheduler wake-up, cache migration).

**If stuck:** Williams ch. 2–4; cppreference `condition_variable` (the example is exactly this).

### 4.2 Spinlock vs `std::mutex`

**Build:** A lock built on `std::atomic<bool>`: `lock()` spins until it can flip `false → true`; `unlock()` sets `false`. Improve it to "test-and-test-and-set" (spin reading, only try the atomic exchange when it looks free) with `_mm_pause()` in the loop and exponential backoff. Compare to `std::mutex` with 2 threads and with 16 threads hammering a shared counter.

**Why:** Your first atomics and your first `memory_order` decision (`acquire` on lock, `release` on unlock — and *why* those). Also the lesson that spinlocks win under low contention and lose badly under high contention.

**Steps:** (1) Naive spinlock with `exchange`. Test with 4 threads incrementing 1M times each → exact total. TSan clean. (2) TTAS + `_mm_pause` + backoff. (3) Benchmark all three locks at 2 and 16 threads, ns per lock/unlock pair. (4) Change the memory orders to `relaxed` and see if TSan (or a test on a weakly-ordered machine, if you have an ARM box) catches it; write down why `acquire`/`release` are the right ones.

### 4.3 `SPSCQueue<T, N>` — lock-free single-producer single-consumer queue

**Build:** Your 3.5 `RingBuffer` where one thread only pushes and one thread only pops, and there is no lock. `head` and `tail` become `std::atomic<size_t>`, each on its own cache line (`alignas(64)`), and the only memory orders are `acquire` when reading the other side's index and `release` when publishing your own.

**Why:** This is *the* HFT data structure — every logger, every feed-handler-to-strategy handoff. It's also the cleanest possible example of the C++ memory model: exactly two atomics, and you have to be able to justify every `memory_order` argument.

**Steps:**
1. Read Williams ch. 5 (memory model) across two sessions, with the 3.5 code open. Then Preshing's "An Introduction to Lock-Free Programming" and "Acquire and Release Semantics".
2. Convert `head`/`tail` to atomics with `seq_cst` everywhere (the safe default). Producer thread pushes 10M ints, consumer pops and sums. Test the sum. TSan clean.
3. Weaken to `acquire`/`release`: producer does `tail.load(acquire)` to check space and `head.store(release)` to publish; consumer the mirror. For each of the four operations, write a comment saying what it synchronises with. TSan clean.
4. Cache-line padding: `alignas(64)` on each atomic. Benchmark before/after — you should see a large jump (this is false sharing, exercise 4.6 makes it explicit).
5. Cached indices: each side keeps a local copy of the other side's index and only re-reads the atomic when the cached one says "full/empty". Benchmark again.
6. (Saturday) Pin producer and consumer to two specific cores (`pthread_setaffinity_np`) and measure round-trip latency of a single item through two queues (A→B→A). Report p50/p99/p99.9 in ns. Try two cores on the same physical core (hyperthreads) vs different cores vs different NUMA nodes if you have them.

**Done when:** you can write it from memory and defend every memory order out loud. Watch Pikus's atomics talk now and check your justifications against his.

### 4.4 `MPSCQueue<T>` — many producers, one consumer

**Build:** Multiple threads push, one pops. Use Vyukov's intrusive MPSC design (a linked list where `push` is a single `atomic exchange` on the tail pointer) or a bounded ring with a per-slot sequence number.

**Why:** The first structure where you need a compare-and-swap loop or an exchange, and where the ABA problem becomes real. Read Williams ch. 7 first and Vyukov's 1024cores page on this queue.

**Steps:** (1) Implement one design. (2) Test 8 producers × 1M items each, consumer counts all. TSan clean. (3) Benchmark vs a mutex-protected `std::deque`. (4) README: one paragraph on where ABA could bite and why your design avoids it.

### 4.5 `Seqlock<T>`

**Build:** One writer, many readers of a `T` (e.g. a market-data snapshot). Writer increments a sequence counter (making it odd), writes, increments again (even). Readers read the counter, copy `T`, re-read the counter; if it changed or was odd, retry. Readers never block the writer.

**Why:** This is how shared-memory market data is published in real systems. It's also a case where the C++ memory model is awkward (the reader's copy is technically a data race on `T`) and you'll learn what `std::atomic_ref` and `memcpy` tricks are for.

**Steps:** (1) Implement for a trivially-copyable `T`. (2) Test: writer updates a struct of 8 ints 1M times so all 8 equal the same value; 4 readers assert they never see a torn (mixed) struct. (3) Benchmark reader latency vs a `std::shared_mutex`. (4) README on why TSan complains and whether it's right.

### 4.6 False sharing demo

**Build:** Two threads, each incrementing its own counter 100M times. Version A: the two counters are adjacent in memory (`struct { int a; int b; }`). Version B: padded to separate cache lines (`alignas(64)`).

**Why:** Adjacent counters share a cache line, so the two cores fight over it; the padded version is often 5–10× faster. `perf stat -e cache-misses` shows why. It's the most common performance bug in multithreaded code and a standard interview question.

**Steps:** (1) Both versions, timed. (2) `perf stat` both. (3) Check `std::hardware_destructive_interference_size`. (4) README with the ratio.

### 4.7 Thread-per-core vs shared map

**Build:** N threads each own a private `FlatHashMap` shard, keys routed by `hash % N` (no sharing, no locks) vs. one `FlatHashMap` behind a `std::shared_mutex`. Throughput from 1 to N threads.

**Why:** "Shared-nothing" is the architecture of every serious low-latency system; the scaling plot is the argument for it.

**Steps:** (1) Both designs behind the same interface. (2) Throughput at 1, 2, 4, 8, 16 threads, 90% reads. (3) Plot (a table is fine). (4) README.

### 4.8 Thread pool, redone

**Build:** 4.1's pool with one `SPSCQueue` per worker instead of the shared mutex/condvar; `submit` round-robins; workers spin (with `_mm_pause`) instead of sleeping.

**Why:** Closing the loop: the latency histogram should drop from microseconds to tens of nanoseconds. That before/after is a strong blog post.

**Steps:** (1) Implement. (2) Same latency test as 4.1. (3) README with both histograms side by side and a paragraph on the trade-off (CPU burn vs latency).

### Phase 4 checkpoint — out loud:
- Explain acquire/release with a concrete example of what reorderings are and aren't allowed; draw the happens-before arrows.
- Write an SPSC queue from memory and justify every `memory_order`.
- Find a data race in someone else's code by reading it, then confirm with TSan.
- Why a condvar handoff costs microseconds and what to do instead.
- Explain and fix false sharing.

---

## Phase 5 — Linux systems and networking

You wrote a filesystem in C, so file descriptors, `read`/`write`, and the syscall model are known. The unknowns are sockets, "wait for many connections at once" (`epoll`), and what the kernel does between the network card and your `read()`. Repo: `net-lab`. Servers and their load-test clients are separate binaries.

**Reading:** Beej's Guide to Network Programming (free online) — read the socket-basics sections *before* 5.1; it's short and written for exactly this. Kerrisk's *The Linux Programming Interface* is a reference: ch. 56–61 during 5.1–5.4, ch. 63 before 5.2, ch. 49 (`mmap`) and 35 (priorities) when you touch them. Man pages for each syscall as you use it. Axboe's "Efficient IO with io_uring" PDF before 5.7.
**Tools to learn as you go:** `strace -c ./server` (count syscalls), `perf trace`, `ss -tlnp`, `tcpdump`, `nc`.

### 5.1 Blocking echo server + your own load tester

**Build:** A TCP server that accepts connections and, for each one, starts a thread that reads bytes and writes the same bytes back. A client that opens M connections, sends a message on each, waits for the echo, and reports round-trip latency through `Histogram`.

**Why:** The socket API end to end (`socket`, `bind`, `listen`, `accept`, `read`, `write`, `close`), and a baseline to beat. Also the lesson that TCP is a *byte stream*: a 100-byte `write` can arrive as two `read`s of 60 and 40, and you must handle that from day one.

**Steps:**
1. Server: `socket` → `setsockopt(SO_REUSEADDR)` → `bind` port 9000 → `listen` → loop `accept`. For now, echo in the main thread, one connection at a time. Test with `nc localhost 9000` by hand.
2. Thread per connection (`std::jthread` per accepted fd). Handle the client closing (`read` returns 0). Handle `EINTR`.
3. Client: opens M connections, sends a 64-byte message, reads until it has 64 bytes back (loop — partial reads!), records the RTT. M = 1, 10, 100.
4. `strace -c` the server during a run; count syscalls per message. README: p50/p99 RTT and syscalls/message.

**If stuck:** Beej's sections on `socket`, `bind`, `listen`, `accept`, `send`/`recv`; man 2 `read` ("partial reads").

### 5.2 Non-blocking echo: `poll` → `epoll` level-triggered → `epoll` edge-triggered

**Build:** The same echo server three more times, single-threaded, handling all connections in one loop by asking the kernel "which fds are ready?": first with `poll`, then `epoll` in level-triggered mode, then edge-triggered. Sockets are set non-blocking (`fcntl(O_NONBLOCK)`), so `read`/`write` return `EAGAIN` instead of waiting, and you keep per-connection buffers for bytes you couldn't yet write.

**Why:** This is the core of every network server you'll write, and the level/edge distinction is a classic interview question because each mode has a specific bug: level-triggered spins if you register for write readiness and forget to unregister; edge-triggered silently loses data if you don't drain the socket until `EAGAIN`.

**Steps:**
1. `poll` version: array of `pollfd`, loop `poll` → for each ready fd `read` → try `write` → if `write` returns short or `EAGAIN`, save the rest in that connection's outgoing buffer and set `POLLOUT` for it. Test with 5.1's client at M = 100. Correctness test: client sends 1 MB in one `write` and checks it gets 1 MB back byte-for-byte.
2. `epoll` level-triggered: `epoll_create1`, `epoll_ctl(ADD)` per fd, `epoll_wait`. Same tests. Add `EPOLLRDHUP` handling for disconnects.
3. `epoll` edge-triggered (`EPOLLET`): now you must loop `read` until `EAGAIN` on every readable event, and likewise `write`. Write a test that sends two messages back-to-back faster than the server reads and confirm neither is lost. Read Kerrisk ch. 63 now — ~30 min, exception to the rule.
4. Benchmark all four servers (5.1 and these three) with the client at M = 1, 100, 1000: p50/p99 RTT, messages/s, syscalls/message via `strace -c`. README with the table and one paragraph per mode on its characteristic bug.

**If stuck:** man 7 `epoll` (the "Questions and answers" section is essential); Beej on `poll`.

### 5.3 Length-prefixed message framer

**Build:** A class that takes an arbitrary chunk of bytes from `read()` and emits complete messages, where each message on the wire is `[4-byte length][payload]`. It must work when a chunk contains half a length field, or three messages plus a bit of a fourth. Fuzz it by taking a known byte stream and feeding it in random-sized pieces.

**Why:** TCP has no message boundaries; every protocol on top of it needs this, and it's where most real-world bugs live. The fuzz test is the habit to take away.

**Steps:** (1) `Framer::feed(std::span<const std::byte>)` appends to an internal buffer and calls a callback for each complete message. (2) Tests: one message in one chunk; one message in 1-byte chunks; three messages in one chunk. (3) Fuzz: generate 1000 random messages, serialise, split at random points, feed, assert you get the same 1000 back. Run 10,000 iterations under ASan. (4) Plug it into the 5.2 edge-triggered server so it echoes *messages* rather than bytes.

### 5.4 UDP multicast sender/receiver with sequence numbers

**Build:** A sender that blasts packets to a multicast group (e.g. `239.1.1.1:5000`), each carrying a sequence number; a receiver that joins the group, reads with `recvmmsg` (many packets per syscall), and logs every gap in sequence numbers.

**Why:** Exchanges send market data over UDP multicast — one packet reaches all subscribers, no connection, no retransmission — so gap detection and recovery is *your* job. This is Phase 6B's transport.

**Steps:** (1) Sender: `socket(AF_INET, SOCK_DGRAM)`, set `IP_MULTICAST_TTL`, `sendto` in a loop with an incrementing `uint64_t`. (2) Receiver: bind, `setsockopt(IP_ADD_MEMBERSHIP)`, `recvfrom`, check sequence, print gaps. Run both on localhost. (3) Switch to `recvmmsg` with a batch of 32; measure packets/s. (4) Make the sender go faster than the receiver can handle (small `SO_RCVBUF`) and watch the gaps appear; then raise `SO_RCVBUF` and see them vanish. README with both.

**If stuck:** Beej's multicast section; man 7 `ip` for the socket options.

### 5.5 Thread-per-core `SO_REUSEPORT` server

**Build:** N copies of the 5.2 edge-triggered server, each in its own thread, each with its own listening socket bound to the same port using `SO_REUSEPORT` (the kernel spreads incoming connections across them), each with its own `epoll`, each pinned to one core. No shared state at all.

**Why:** This is the shape of a production low-latency server: shared-nothing, one thread per core, no locks. Comparing its p99.9 to 4.1's thread pool is a strong demonstration.

**Steps:** (1) Wrap 5.2's server as a function `run_on_core(int core, int port)`. (2) N threads, each `SO_REUSEPORT` + `pthread_setaffinity_np`. (3) Client at M = 1000 against N = 1, 2, 4, 8. (4) README: p50/p99/p99.9 vs N, plus the comparison to the 4.1-style design (a shared accept thread handing fds to a pool).

### 5.6 The KV store, rewritten from a blank file

**Build:** A Redis-compatible in-memory key-value server, from scratch, by hand: thread-per-core with `SO_REUSEPORT` (5.5); each core owns a `FlatHashMap` shard (3.1) allocated from a `PoolAllocator` (3.2); a streaming parser for the RESP protocol (Redis's wire format — `*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n`) built like the 5.3 framer, supporting pipelining (many commands per read); responses batched with `writev`; `GET`, `SET`, `DEL`, `PING`. Latency reported with `Histogram`. Benchmark with `redis-benchmark` and `memtier_benchmark`, including pipelined mode.

**Why:** You've built this once with AI guidance and it didn't stick. Building it again from nothing, with your own containers, is the proof — and the line-by-line comparison against the old version is the best blog post in this whole plan.

**Steps:** (1) RESP parser with the 5.3 fuzz approach; unit tests only. (2) Single-threaded server: `PING` only, tested with `redis-cli`. (3) `GET`/`SET`/`DEL` on one `FlatHashMap`. (4) Pipelining: parse as many commands as the buffer holds, queue the responses, one `writev`. (5) Thread-per-core sharding. (6) (Two Saturdays) `redis-benchmark -t get,set -P 16` and `memtier_benchmark`; compare to real Redis on the same box. (7) Blog post: your numbers, Redis's numbers, and three things your old AI-guided version did that you now know were wrong.

### 5.7 Port 5.6 to `io_uring`

**Build:** Replace `epoll` + `read` + `writev` with `io_uring` submission/completion queues (`liburing`): submit reads and writes in batches, harvest completions, far fewer syscalls.

**Why:** `io_uring` is the modern Linux I/O interface; knowing when it helps (many small ops, batched) and when it doesn't (kernel-bypass is still faster) is current interview material.

**Steps:** (1) Read Axboe's PDF (~45 min, one session). (2) Echo server with `liburing` (`io_uring_prep_recv`, `_send`, `_submit`, `_wait_cqe`). (3) Port 5.6. (4) Same benchmarks; README on whether it helped and why. Also read up on kernel bypass (DPDK, Solarflare Onload) — one session, concept only — and write a paragraph on what they remove.

### Phase 5 checkpoint — out loud:
- Write an epoll server from a blank file with correct partial-I/O handling, no reference.
- Level- vs edge-triggered epoll and the bug each causes.
- What happens, kernel and user space, from a packet arriving at the NIC to your `read()` returning (interrupt → driver → socket buffer → wake-up → copy).
- Why market data uses UDP multicast and how gap recovery works.
- Read `strace` output and spot wasted syscalls.

---

## Phase 6 — HFT portfolio

Each project is its own repo with a benchmark, a README with numbers, and a blog post. Reading here is unavoidable but is paired with a build: read the spec section for the message you're parsing *right now*, not the whole document.

**Domain reading (15 min/session, alongside the projects):** Harris, *Trading and Exchanges* ch. 1–6, 10–12 (market structure, order types, order-driven markets). NASDAQ TotalView-ITCH 5.0 spec and OUCH 4.2 spec (public PDFs). FIX overview on fixtrading.org — what a tag=value message looks like, why binary replaced it.
**Talks:** Cook "When a Microsecond Is an Eternity"; Gross "Trading at Light Speed"; Sapir "HFT and Ultra Low Latency Development Techniques"; McGuiness "Low Latency C++ for Fun and Profit".
**Vocabulary to be fluent in:** bid/ask, spread, tick size, lot, limit/market/IOC/FOK orders, price-time priority vs pro-rata, hidden orders, opening/closing auctions, tick-to-trade, market making vs arbitrage vs stat-arb, co-location, FPGA vs software.

### 6A — Order book and matching engine

**Build:** For one instrument, a limit order book: a set of *price levels* (all resting orders at one price, first-in-first-out), bids on one side and asks on the other. Operations: `add` (new resting order), `cancel`, `modify`, `execute` (an incoming order that matches against the other side, possibly partially, generating trade reports). Everything O(1): price levels in a flat array indexed by price tick (e.g. 1,000,000 ticks) with a fallback map for prices outside the range; orders in each level as an `IntrusiveList` (3.3); order-ID → order via a `FlatHashMap` (3.1) into a `PoolAllocator` (3.2); no `malloc` after start-up. Same inputs must always give the same outputs (deterministic replay).

**Why:** This is *the* HFT interview project. Every design choice — flat arrays, intrusive lists, pools — is something you built in Phase 3, now with a reason.

**Steps:**
1. Read Harris ch. 1–3 and 10 across four sessions to learn what an order book *is* (price-time priority, best bid/ask, spread). Then design on paper: draw the data structures.
2. `Order { id, side, price, qty, ListHook }`; `PriceLevel { IntrusiveList<Order> orders; total_qty; }`; `Book { std::array<PriceLevel, MAX_TICKS> bids, asks; FlatHashMap<OrderId, Order*>; Pool<Order>; best_bid_idx; best_ask_idx; }`. `add` and `cancel` only. Tests.
3. `best_bid()` / `best_ask()` maintenance when a level empties (scan toward the middle; measure how far it usually scans).
4. Matching: an incoming buy at price P walks ask levels from best up to P, filling FIFO within each level, emitting `Trade{buy_id, sell_id, price, qty}`; the remainder rests. Partial fills. Tests for every case you can think of, including "cancel an order that was just partially filled".
5. `modify` (price change = cancel + add and you lose priority; qty decrease keeps priority — Harris ch. 4). Out-of-range price fallback (`std::map`). Tests.
6. Determinism: a replay test — record 1M random operations to a file, run twice, `diff` the trade output.
7. (Two Saturdays) Benchmark: ns per `add`/`cancel`/`execute` as a `Histogram` each, with 1M resting orders. `perf record` and fix the top hotspot. README + blog post.

### 6B — ITCH market data feed handler

**Build:** A parser for NASDAQ's ITCH 5.0 binary protocol (each message is a 1-byte type code followed by fixed-layout big-endian fields; e.g. "Add Order" = type `A`, 36 bytes) that reads directly from the raw bytes with no copying, feeds every add/cancel/execute into a 6A book per instrument, and produces a top-of-book (best bid/ask) stream. Input: (1) a real sample ITCH day file from NASDAQ's FTP replayed from disk; (2) UDP multicast packets you generate from that file with a replay tool you write (5.4), with gaps and reordering handled. Verify against a slow Python reference implementation you also write.

**Why:** Parsing a binary protocol from a spec, zero-copy, at millions of messages per second, is the daily job. The Python reference is how you know you're right.

**Steps:**
1. Download a sample ITCH 5.0 file (NASDAQ publishes them; ~5 GB compressed). Open the spec to the "Add Order" message *only*. Parse just that message type from the file with `std::span<const std::byte>` reads (build 6.3) and count them. Compare the count to a 20-line Python script.
2. Add message types one per session: System Event, Stock Directory, Add Order (with MPID), Order Executed, Order Cancel, Order Delete, Order Replace, Trade. Each with a test against a hand-constructed byte array from the spec.
3. Wire to 6A: one `Book` per stock locate code (a `std::vector<Book>` indexed by locate). Run the full day; print the final best bid/ask for 10 well-known symbols and compare to the Python reference (which now needs to maintain a simple book too).
4. Top-of-book output: whenever best bid/ask changes, emit a record. Verify against Python for one symbol.
5. (Saturday) Replay tool: read the file, wrap messages in MoldUDP64 framing (the spec's UDP wrapper, with sequence numbers), send over multicast at full speed. Receiver from 5.4 → framer → parser → books. Handle gaps (request nothing yet — just detect and count) and out-of-order packets (buffer and reorder).
6. Benchmark: messages/s over the full day, ns/message `Histogram`, `perf record` and fix the top hotspot. README + blog post.

### 6C — Low-latency logger

**Build:** A `log(fmt, args...)` that, on the calling thread, only copies the raw arguments into a per-thread `SPSCQueue` (4.3) and returns in under 100 ns. A background thread pops, formats with `std::format`, and writes to disk. The format string is parsed at compile time (so the hot path knows the argument sizes with no runtime work), and nothing on the hot path allocates.

**Why:** Every trading system needs to log without disturbing the hot path; "how would you build a logger?" is a standard design question, and this one gives you a number to quote.

**Steps:**
1. Naive version: `log` formats and `fprintf`s synchronously. Benchmark it — that's the baseline to destroy.
2. Async: `log` pushes a `std::string` (already formatted) into an `SPSCQueue`; background thread writes. Benchmark: better, but the formatting and the `std::string` allocation are still on the hot path.
3. Defer formatting: push a small struct `{ const char* fmt; timestamp; args as a fixed-size byte buffer }` — copy the raw `int`/`double`/`const char*` values with `memcpy`. Background thread formats. Benchmark.
4. Compile-time format parsing: a `consteval` function that checks the format string against the argument types (build 6.7's technique) so mismatches fail to compile. Per-thread queue via `thread_local`.
5. Benchmark vs `spdlog` async mode and vs the naive version; `Histogram` of `log()` call latency. README + blog post.

### 6D (optional) — Gateway / strategy skeleton

**Build:** Wire 6B → a trivial strategy (e.g. quote both sides one tick inside the spread, or buy when best-ask drops below a moving average) → orders in OUCH 4.2 format over TCP to a simulated exchange you write (a tiny 6A behind a socket) → fills back. Measure tick-to-trade: time from the market-data packet arriving to the order leaving. Add pre-trade risk checks (position limit, price collar) on the hot path and measure their cost.

**Steps:** (1) Simulated exchange: 6A behind 5.2's server, speaking OUCH. (2) Strategy loop reading 6B's top-of-book. (3) Timestamps at each stage, `Histogram` of the total. (4) Risk checks; measure delta. README + blog post.

### Phase 6 checkpoint — out loud:
- Whiteboard an O(1) order book and justify every memory-layout choice.
- Parse a binary protocol from a spec without help.
- State your tick-to-trade latency and where each microsecond goes.
- Talk about market microstructure for 20 minutes with someone who works in it.

---

## Phase 7 — Interview preparation (in parallel with Phase 6)

### Reading (now that every item maps to code you wrote)
- *Effective Modern C++* — full read, one item per session, one flashcard per item.
- Zhou, *A Practical Guide to Quantitative Finance Interviews* ch. 2–4 (brainteasers, probability, stochastic basics) — one problem per session.
- Re-read your own 4.3 comments on memory ordering.

### Flashcard deck (Anki; review 10 min every morning before the build)
vtable layout and the cost of a virtual call · `std::move` doesn't move; copy elision and NRVO · each `memory_order` with one canonical example · false sharing, cache line size, `alignas` · `unordered_map` internals vs open addressing · `unique_ptr` vs `shared_ptr` overhead and control block layout · exception cost model; `noexcept` and move · templates vs virtual; CRTP · `constexpr`/`consteval`/`constinit` · strict aliasing, `std::bit_cast` · latency numbers (L1 ~1 ns, L2 ~4 ns, L3 ~10–40 ns, DRAM ~80–100 ns, syscall ~100s of ns–µs, context switch ~µs) · epoll vs io_uring vs busy-polling · TCP vs UDP multicast for market data.

### Practice
- LeetCode: 100 mediums in C++ across arrays, hashing, two pointers, heaps, intervals, DP, graphs; 30-minute time-box each.
- Systems design out loud, 45 min each, recorded on your phone: "design a feed handler", "design an order gateway with risk checks", "design a low-latency logger", "design a matching engine for 10k instruments". Listen back once.
- Weekly: explain one project to a non-expert in 5 minutes, then to an expert in 15.
- At least four mock interviews (C++ Edinburgh meetup, or paid).
- Read your own six-month-old code and refactor it as an interviewer would demand.

### Applications
- CV: one page; projects first, each with a number ("p99.9 of 2.1 µs at 4.2M msg/s"); links to blog posts.
- Target firms (London / Amsterdam): Optiver, IMC, XTX, Maven, Qube, G-Research, Citadel Securities, Jump, Tower, HRT, Man AHL, Squarepoint, Flow Traders, Da Vinci, Mako. Also C++ performance roles outside trading (databases, HPC, ad-tech bidders, telecom, game engines) as stepping stones.
- Apply in batches of 5–8; treat each rejection as a data point about which checkpoint needs work.

---

## Accountability (solo-proof)

- The blog is public from exercise 0. A 200-word post with one table counts. Post each one to r/cpp or the C++ Edinburgh community; one comment from a stranger beats a private note.
- `notes.md` is the only tracking. No streaks, no percentages.

## Progress log

| Phase | Started | Checkpoint passed | Blog post |
|---|---|---|---|
| 1 | | | |
| 2 | | | |
| 3 | | | |
| 4 | | | |
| 5 | | | |
| 6A | | | |
| 6B | | | |
| 6C | | | |
| 7 | | | |
