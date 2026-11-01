# C++ Roadmap Companion — the why behind each step

Oct 25, 2026 · @Yi

## How to use this doc

This sits next to ROADMAP.md v4. The roadmap says *what* to build; this says *why* each step exists and *what you are actually being asked to do*, in plain words. Open the exercise here before you open it there.

Three rules of use:

1. Read only the exercise you are on. The later ones will make more sense after the earlier ones, and reading ahead turns this back into a textbook.
2. Before each step, read its entry here, then write one sentence in `notes.md`: "this step exists because \_\_\_". After the step, one more: "I will meet this again when \_\_\_". Two minutes. That is the whole understanding ritual.
3. Each exercise ends with a "where this goes next" line. That is the thread that ties the pieces together; if a step ever feels random, go back to the map below and find which question it is answering.

Phase 1 is written out in full. Phases 2 onward will be added when you get near them.

## The map: four questions every topic answers

Almost everything in performance C++ is an answer to one of four questions. When a concept turns up, decide which question it belongs to; that is the "why" you have been missing.

**Q1. Where does my data live?** Stack or heap; inside the object or behind a pointer; one allocation or many; which bytes share a cache line. In C you decided this with `malloc`. In C++ the language gives you tools to decide it precisely and to hide the decision behind a clean interface.

- Phase 1: small-string optimisation (ex 1), raw storage vs constructed objects (ex 2), `MyOptional` without the heap (ex 4), small-buffer `MyFunction` (ex 5)
- Later: cache lines and layout (Phase 2), flat hash maps, arenas, pools, small vectors (Phase 3)

**Q2. Who owns my data, and when does it die?** In C the answer lived in your head and in comments. In C++ the answer is written into the type: this object owns that buffer, and when this object dies the buffer dies with it. Everything called RAII, move semantics, smart pointers and "the rule of five" is this one question.

- Phase 1: the five special members (ex 1), moving instead of copying during growth (ex 2), `unique_ptr`/`shared_ptr` (ex 3), ownership inside a union (ex 4), owning a callable you cannot name (ex 5)
- Later: intrusive lists and pool-owned objects (Phase 3), who owns a connection buffer (Phase 5)

**Q3. How does my data move?** Between functions (copy, move, reference, forwarding), between threads (atomics, queues), between machines (sockets, framing).

- Phase 1: `std::move` and `std::forward` (ex 1, 2), value categories (ex 4)
- Later: all of Phase 4 and Phase 5

**Q4. What does the CPU actually do with my code?** Which calls are direct and which go through a pointer; which decisions are made at compile time and which at run time; what the compiler can see through and what it cannot.

- Phase 1: `constexpr` (ex 0), `explicit` and overloads (ex 1), templates vs virtual vs `variant` (ex 5, 7), reading your own assembly (ex 7)
- Later: all of Phase 2, SIMD (Phase 3), memory ordering (Phase 4)

The exercises are ordered so that each one needs a tool the previous one built. `MyString` teaches ownership of one buffer; `MyVector` reuses that to own many objects; smart pointers package the ownership rule so you stop rewriting it; `MyOptional` and `MyFunction` apply the same "storage is separate from object" idea in two new shapes; the sprint and dispatch exercises fill in the remaining language; the evaluator makes you use all of it in one program. Nothing in Phase 1 is there for its own sake.

## Exercises 0 and 1 in hindsight

You have done these, so this is only the thread to carry forward.

**Exercise 0, `Histogram`** answered a practical question: how do I know whether anything I build later is fast? Every "ends with a number" rule in the roadmap depends on it. The one language idea worth keeping is `constexpr`: it is the first time you told the compiler "work this out before the program runs" (Q4), and it comes back in exercise 6.7 and in every lookup table you ever build.

**Exercise 1, `MyString`** was Q2 in its purest form. The double-free crash in step 2 was the point of the whole exercise: C++ will write a copy constructor for you, and the one it writes copies the pointer, not the buffer. So the moment a class owns memory, you must write all five special members yourself, or the compiler's versions will silently do the wrong thing. That rule is called the rule of five, and you will apply it in exercises 2, 3, 4 and 5. `std::move` turned out to be only a cast that marks an object as safe to steal from; the stealing is done by the move constructor you wrote. The small-string optimisation was Q1: the same text can live inside the object or on the heap, and the class hides which.

**Where this goes next:** `MyVector` is `MyString` generalised. One owned buffer becomes one owned buffer of *any* type, and the new difficulty is that the buffer may contain fewer objects than it has room for.

## Exercise 2: `MyVector<T>`, step by step

**Why it exists.** `std::vector` is the most used container in C++ and the one interviewers most often ask you to rebuild. But the real lesson is one idea you did not have in C: *getting memory* and *putting an object in it* are two different acts. In C, `malloc(8 * sizeof(struct))` gives you bytes and you use them. In C++, a type can have a constructor and a destructor, so "bytes exist" and "an object exists" are no longer the same thing. A vector with capacity 8 and size 3 has memory for 8 but only 3 live objects, and the other 5 slots must never be touched as objects. Everything in this exercise follows from that. Questions: Q1 (storage), Q2 (who owns the elements), Q3 (how elements move during growth).

**Step 1: the naive version with `new T[cap]`.** You are asked to write the simplest possible vector: allocate an array of `cap` elements, keep a `size`, `push_back` copies into slot `size` and bumps it. Test with `int`. Then write down the flaw: `new T[cap]` *constructs* `cap` objects immediately. For `int` that is harmless. For `MyString` it means 8 empty strings are built when you only wanted room for 8, it requires `T` to have a default constructor, and `push_back` then assigns over an existing object instead of building a new one. You are deliberately building the wrong version so that step 3 is a fix for a problem you have seen, not a rule you were told.

**Step 2: growth, then move instead of copy.** When `size == cap`, allocate a new array twice as big, bring the elements across, free the old array. First do it by copying, test with 1000 ints. Then change the "bring across" to `std::move(old[i])` and test with `MyString`. The point: during growth every element is transferred exactly once, and if that transfer copies, growing a vector of 1 KB strings copies all of them for nothing. Moving steals each buffer instead. To *prove* no copies happen, add a static counter to `MyString`'s copy constructor and assert it stays at zero. This is Q3 applied: data moving between two buffers should move, not copy. (Why double? Doubling means each element is moved at most about once on average over the vector's life; the step 7 benchmark makes you check that against 1.5×.)

**Step 3: raw memory, placement new, manual destructors.** This is the heart of the exercise. Replace `new T[cap]` with `::operator new(cap * sizeof(T))`: that is `malloc` with a C++ name, it returns bytes and constructs nothing. Now you build objects one at a time where you want them: `new (data + i) T(value)` is "placement new" and means "run `T`'s constructor at this address" with no allocation. To remove an object you call its destructor by hand, `data[i].~T()`, and to free the bytes you call `::operator delete(data)`. The rule you are learning: every constructed object must be destroyed exactly once, and only objects you constructed may be destroyed. AddressSanitizer will catch you if you destroy slot 5 when only 3 exist. Once this works, step 1's flaw is gone: capacity 8, size 3, five slots of bytes that no one treats as objects.

**Step 4: `emplace_back` and perfect forwarding.** `push_back(const T&)` takes a finished object and copies it in. `emplace_back(args...)` takes the *constructor arguments* and builds the object directly in the slot, skipping the temporary. To do that you need two things. A variadic template, `template<class... Args>`, so the function accepts any number of arguments of any types. And `std::forward<Args>(args)...`, which passes each argument on exactly as it arrived: an lvalue stays an lvalue, a temporary stays a temporary. Without `forward`, a temporary passed in would arrive at `T`'s constructor as an lvalue and be copied, defeating the purpose. Test that `v.emplace_back("hello")` on a `MyVector<MyString>` runs `MyString`'s constructor once and its copy constructor zero times. This is Q3 again: `forward` is how you pass something through a function without changing how it moves.

**Step 5: iterators.** Range-for and `std::sort` do not need anything clever; they need `begin()` and `end()` that return something you can `++`, `*` and compare. For contiguous storage a raw `T*` is a perfect iterator. Add `const` overloads so a `const MyVector` can still be read. The lesson is small but worth noticing: the standard library is built on conventions, not magic, and a plain pointer satisfies them.

**Step 6: exception safety.** Now the hard one. Suppose growth is halfway through copying elements into the new buffer and a copy constructor throws. What state is your vector in? If you freed the old buffer early, the data is gone. If you constructed 3 of 5 in the new buffer and then abandoned it, those 3 leak. The required guarantee is: if growth fails, the vector is exactly as it was before. The fix has two parts. First, build the new buffer completely before you touch the old one; if anything throws, destroy what you built in the new buffer and rethrow, and the old buffer is untouched. Second, `std::move_if_noexcept`: moving is faster, but if `T`'s move constructor can throw, a failed move halfway through has already destroyed the source elements and you cannot roll back. So the standard library moves only when the move is marked `noexcept`, and copies otherwise. This is why exercise 1 told you to mark the move constructor `noexcept`: without that mark, `std::vector<MyString>` would copy on every growth. Write the test type that throws on its third copy, push five, run under ASan, and make it pass.

**Step 7: benchmark.** Compare `push_back` against `std::vector` at 1,000 and 1,000,000 elements, and growth factor 1.5 against 2. Being within 2× is the goal; it tells you nothing structural is wrong. If you are 10× slower, something is being copied that should be moved.

**What you should be able to say afterwards.** "A vector owns raw bytes and a count of how many objects are alive in them. Growth allocates new bytes, moves the live objects across if their move cannot throw and copies them otherwise, destroys the old objects, frees the old bytes. `emplace_back` forwards constructor arguments into the slot so no temporary exists."

**Where this goes next.** Exercise 3 needs a container that can hold move-only types; your `MyVector<MyUniquePtr<int>>` test there is checking step 2 and step 4 of this exercise. Exercise 4 reuses the "bytes are not objects" idea inside a union. Phase 3's `SmallVector` is this class plus an inline buffer.

## Exercise 3: `MyUniquePtr` and `MySharedPtr`

**Why it exists.** Exercises 1 and 2 made you write the ownership rule by hand each time: allocate in the constructor, free in the destructor, steal in the move, forbid or deep-copy in the copy. That is tedious and error-prone, which is exactly why C++ packages it. A smart pointer is a tiny class whose only job is to own one heap object and apply the rule of five correctly, so that every other class can hold a smart pointer and get correct ownership for free without writing any of the five members. This is Q2 turned into a reusable tool, and it is the real meaning of the acronym RAII: tie the resource's lifetime to an object's lifetime and let the destructor do the cleanup. After this exercise you should almost never write `delete` again.

**Step 1: `MyUniquePtr`, no copying.** Hold a `T*`; delete it in the destructor; give access through `*`, `->` and `get()`. Then mark the copy constructor and copy assignment `= delete`. Why forbid copying rather than implement it? Because two owners of one pointer is the exercise 1 double free all over again, and a deep copy would be surprising for a pointer type. The `static_assert(!std::is_copy_constructible_v<...>)` test is you asking the compiler to confirm the type is uncopyable. It is the first time you treat "this compiles" and "this must not compile" as equally testable facts.

**Step 2: moving, `release`, `reset`.** Move means steal the pointer and leave the source holding `nullptr`, so the source's destructor deletes nothing. `release()` hands the raw pointer back to you and gives up ownership; `reset(p)` deletes what it holds and takes `p`. Then put a `MyUniquePtr` into your `MyVector`: this works only if exercise 2 moves elements during growth, because a unique pointer cannot be copied. If it fails to compile, that is exercise 2 telling you something.

**Step 3: the deleter, and why it costs nothing.** Not everything is freed with `delete`; a `FILE*` needs `fclose`. So the deleter becomes a template parameter with a default that calls `delete`. Now the question that makes this a performance exercise: if the pointer class stores a deleter object as a member, is the pointer bigger than a raw pointer? A deleter with no data (a struct with only an `operator()`) still has size 1 in C++, and padding makes the whole thing 16 bytes instead of 8. That doubles the memory of every container of pointers. The fix is `[[no_unique_address]]` (or inheriting from the deleter), which lets an empty member occupy no space. Check `sizeof(MyUniquePtr<int>) == sizeof(int*)`. This is Q1 and Q4 together: zero-cost abstraction means the safe version has the same size and the same instructions as the raw version.

**Step 4: `make_unique`.** A helper that takes constructor arguments, forwards them (exercise 2 step 4 again) to `new T(...)`, and wraps the result. Its purpose is that you never hold a raw `new` result even briefly.

**Step 5: `MySharedPtr` and the control block.** Sometimes several owners genuinely need one object, and it should die when the last one leaves. The pointer alone cannot know how many owners exist, so the count must live somewhere all owners can see: a separately allocated control block holding `strong` and `weak` counts. Copying increments `strong`; the destructor decrements it and deletes the object at zero. Test by making three copies and destroying them in different orders; the object must be destroyed exactly once regardless. Notice what you have paid for this: an extra allocation, an extra pointer, and a count that must be updated on every copy. That is why `shared_ptr` is slower than people assume and why `unique_ptr` is the default.

**Step 6: `MyWeakPtr`.** A weak pointer watches an object without keeping it alive. It needs the control block (so it bumps `weak`) but not `strong`. `lock()` checks `strong > 0` and, if so, hands you a shared pointer; otherwise the object is gone and you get an empty one. The control block itself is freed only when both counts reach zero, because a weak pointer may still need to read `strong` after the object has died. That is the subtle case to test.

**Step 7: `make_shared` and the benchmark.** Allocate the object and the control block in one allocation by putting both in a single struct. One allocation instead of two, and the object sits next to its count in memory (Q1: one cache line, not two). Benchmark that against `MySharedPtr(new T)`. Then check in godbolt that dereferencing a `MyUniquePtr` produces the same assembly as dereferencing a raw pointer. That check is the whole promise of this exercise made visible.

**Where this goes next.** Exercise 5's `MyFunction` holds its callable in a `MyUniquePtr`. Exercise 7 stores polymorphic objects as `MyVector<MyUniquePtr<Base>>`. Exercise 8's expression tree is built from unique pointers. From here on, raw owning pointers should not appear in your code.

## Exercise 4: `MyOptional<T>`

**Why it exists.** You need a way to say "maybe there is a value here" without using a null pointer (which needs the heap) and without a sentinel like -1 (which does not work for every type). The obvious design, `T value; bool has;`, is wrong because it constructs a `T` even when there is no value, which may be expensive or impossible. The right design is exercise 2 step 3 in miniature: reserve the *bytes* for a `T` inside the object, and only construct a `T` in them when there is a value. Q1 (the value lives inline, never on the heap) and Q2 (you manage one object's lifetime by hand), plus a first look at value categories (Q3).

**Step 1: storage with a union.** A union holds storage big enough for its largest member, and only one member is active at a time. In C you could not put a struct with a destructor in a union; in C++ you can, but the compiler then refuses to write the union's constructor and destructor for you, because it cannot know which member is active. So you write them yourself, empty, meaning "do nothing, I will handle it". The `char dummy` member exists so that the union has an active member even when there is no `T`. The `bool has_value` sits beside the union and is your record of which member is live.

**Step 2: constructing and destroying in place.** Constructing from a `T` means placement new into `&value` and setting the flag. The destructor checks the flag and, only if a value exists, calls `value.~T()` by hand. This is exactly the discipline from exercise 2: construct what you need, destroy exactly what you constructed. Test with `MyString` under ASan, which will catch a destructor call on bytes that hold no object.

**Step 3: the four cases.** Copy and move assignment each have four situations: both sides empty, only the source has a value, only the target has a value, both have one. Each needs a different action (nothing; construct; destroy; assign). Writing all four out, for both copy and move, is the point of the step, because `std::optional` has to do precisely this and most hand-written attempts miss a case. Use a type that counts constructions and destructions in its test and check the counts balance.

**Step 4: `value() &` and `value() &&`.** This is new. A member function can be overloaded on whether the object it is called on is an lvalue (`&`) or a temporary (`&&`). Why would you want that? If someone writes `std::move(opt).value()`, they are saying "I am done with this optional, take the contents", and the `&&` overload can return `T&&` so the value is moved out instead of copied. The `&` overload returns a reference and leaves the value in place. This is Q3: the same function behaves differently depending on how the data is about to move. `value_or`, `emplace` and the operators are routine once this is clear.

**Step 5: sizes and padding.** `sizeof(MyOptional<int>)` is 8, not 5: four bytes of `int`, one of `bool`, three of padding so the next object in an array is aligned. For `MyString` it is `sizeof(MyString) + 8` for the same reason. Write one sentence on why. The stretch goal, `constexpr` usability, needs C++20 features you have not met; skip it if it takes more than a session.

**What you should be able to say afterwards.** "Why a union instead of a value and a flag: the union gives me bytes without an object, so an empty optional constructs nothing, and the flag tells me which of the union's members is alive."

**Where this goes next.** Exercise 5 uses the same inline-storage trick for a callable. Exercise 6 uses `std::optional`, `std::variant` and `std::expected`, which are all this idea with more alternatives. Exercise 8's tokens and tree nodes are variants.

## Exercise 5: `MyFunction<R(Args...)>`

**Why it exists.** In C, a callback is a function pointer. In C++, a callable can also be a lambda that captured three variables, or an object with `operator()`. Each of those is a different type, and a lambda's type does not even have a name you can write. So how do you store "some callable that takes an `int` and returns an `int`" in a member variable when you do not know its type? The technique is called type erasure, and it is how `std::function`, `std::any`, and most plugin and event systems work. The second lesson is cost: calling through an erased wrapper is an indirect call, and you will measure what that costs because every hot path you ever write will have to decide whether to pay it. Q2 (owning something whose type you cannot name) and Q4 (direct vs indirect calls).

**Step 1: hide the type behind a virtual interface.** Define an abstract base `Callable` with a virtual `call(Args...)` and a virtual destructor. For each concrete callable type `F`, a template struct `Impl<F>` derives from `Callable`, holds an `F`, and implements `call` by invoking it. `MyFunction` then holds a `MyUniquePtr<Callable>`, which has a fixed type no matter what `F` was. The constructor is a template, so it learns `F` at the moment you hand it a lambda, builds an `Impl<F>`, and from then on only the base type is visible. Test with a lambda, a plain function pointer, and a capturing lambda. This is the general answer to "store a type you don't know": wrap it in something whose type you do know.

**Step 2: copying.** A `MyUniquePtr` cannot be copied, so neither can your `MyFunction` yet. Add a virtual `clone()` to the interface that returns a fresh `MyUniquePtr<Callable>` holding a copy of the `F`. Now copying a `MyFunction` copies whatever lambda is inside, captures and all. Test with a lambda that captured a `MyString`, so a copy failure would show up as a double free under ASan.

**Step 3: the same thing with function pointers instead of virtuals.** Replace the virtual interface with two plain function pointers, one that invokes and one that destroys, plus a `void*` to the stored object. The template constructor fills in the pointers with functions that cast the `void*` back to `F*`. Same tests must pass. Then look at both versions in godbolt and count the indirections for one call. You will find they are about the same, which is the lesson: a virtual call *is* a load from a table followed by an indirect call, nothing more mysterious. Exercise 7 builds on this directly.

**Step 4: small-buffer optimisation.** Most callables are tiny: a function pointer, or a lambda capturing one or two things. Allocating on the heap for those is a waste, exactly as it was for short strings in exercise 1. So keep an inline buffer of 16 bytes, aligned for a pointer, and construct the callable there when it fits and its move cannot throw; otherwise use the heap. The test overrides `operator new` to count allocations and asserts that a lambda capturing one `int` causes zero. This is the third time you have built "inline when small, heap when large": string, optional, function. The pattern will now be yours.

**Step 5: benchmark.** Direct call, plain function pointer, your `MyFunction`, and `std::function`, all on a lambda that adds two ints. The direct call will likely be inlined to nothing; the others cost a few nanoseconds each. That gap is the price of erasing the type, and knowing it is what lets you decide, in later phases, when a template is worth its compile-time cost.

**Where this goes next.** Exercise 7 compares virtual, variant and compile-time dispatch head to head, which is step 3 of this exercise made into a whole exercise. Phase 4's thread pool stores its tasks as `MyFunction<void()>`.

## Exercise 6: the modern C++ sprint

**Why it exists.** Exercises 1 to 5 taught the deep mechanics. This one fills in the vocabulary that modern codebases are written in, so that when you read a database or trading codebase in Phase 6, nothing on the page is foreign. Each item is deliberately tiny. The rule "attempt first, read the reference after" is there so the reading answers a question you actually had.

The ten items fall into four groups, and knowing the groups is more useful than knowing the list.

**Group A: values that may be one of several things (items 1, 2).** `std::expected<T, E>` holds either a result or an error, and is the modern replacement for return codes and for exceptions in hot code; writing `parse_pair` teaches you how errors propagate without a `throw` anywhere. `std::variant<A, B>` holds one of several types and `std::visit` runs the right code for whichever it is; the `overloaded` helper is just a way to pass several lambdas as one visitor. Both are exercise 4's union idea, generalised. Exercise 8 is built out of them.

**Group B: looking at memory without owning it (items 3, 8).** `std::span<const std::byte>` is a pointer plus a length over someone else's bytes; `read_be32` is a preview of parsing network and market-data protocols in Phase 5 and Project B, where messages arrive as raw bytes and you must interpret them without copying. `<bit>` gives you safe versions of the bit tricks you did with casts and shifts in C: `bit_cast` reinterprets bits without undefined behaviour, `byteswap` handles endianness, `popcount` counts bits. Item 8 asks you to rewrite item 3 with them so you see the difference.

**Group C: moving work to compile time (items 4, 5, 7).** Fold expressions and `if constexpr` let a template do different things for different types without writing overloads; structured bindings are the small convenience of unpacking a pair. Concepts put a name on "what a template requires of `T`" so that a wrong type produces a short error instead of a page of noise; constraining `MyVector` with `std::movable` is the first time you document a requirement the compiler enforces. `consteval` forces a function to run at compile time, and the CRC32 table is the classic use: compute the table once, at build time, and ship it as data. Project A's write-ahead log uses that exact table. All of this is Q4.

**Group D: the rest of the standard toolkit (items 6, 9, 10).** Ranges are composable pipelines over containers; comparing one to a raw loop in godbolt shows you whether the abstraction costs anything (usually not, at `-O2`). `std::format` is the type-safe `printf` and replaces the stream code in exercise 0. `std::jthread` with `std::stop_token` is a thread that asks itself to stop and joins itself in its destructor, which is RAII (Q2) applied to threads; it is the first thing you will write in Phase 4.

**The one-line README entries matter.** "When I'd use this instead of X" is the sentence an interviewer wants to hear, and writing it is how a feature stops being trivia.

**Where this goes next.** Exercise 7 reuses the `overloaded` helper. Exercise 8 is almost entirely built from items 1, 2 and 4. Phase 5 is items 3 and 8 at scale.

## Exercise 7: dispatch three ways

**Why it exists.** "Dispatch" means: I have a message, it is one of several kinds, run the right code for its kind. Every server, parser and trading system does this millions of times a second, so how you do it matters. C++ offers three mechanisms with different costs, and you have already met all three: virtual functions (exercise 5 step 1), `std::variant` with `std::visit` (exercise 6 item 2), and templates resolved at compile time (every exercise). This exercise builds the same thing three times so that you can measure them rather than repeat folklore. It is also the last big piece of language you have not used: inheritance. Q4 throughout.

**Step 1: the virtual version.** A `Base` with a virtual `process()` and a virtual destructor; `Add`, `Cancel`, `Trade` derive from it and `override`. Store them as `MyVector<MyUniquePtr<Base>>`: each message is its own heap object, and the vector holds pointers to them, because different derived types have different sizes and cannot sit in one array by value. The virtual destructor is required so that deleting through a `Base*` runs the derived destructor; without it, that is undefined behaviour. `final` on a class or method tells the compiler nothing derives further, which sometimes lets it skip the indirection.

**Step 2: look at the vtable.** Every object of a class with virtual functions carries a hidden pointer, the vptr, to a table of function pointers, the vtable. A virtual call is: load the vptr from the object, load the function pointer from the table, call through it. That is two dependent memory loads before the call can even start, and the CPU cannot predict the target until the second load finishes. Seeing the table in godbolt, and noticing `sizeof(Base)` is 8 even with no data members, makes the cost concrete. Write the three-step description in the README in your own words; it is a standard interview question.

**Step 3: the variant version.** `std::variant<Add, Cancel, Trade>` stores whichever message it holds *by value*, inline, so the vector is one contiguous block with no pointer chasing (Q1). `std::visit` dispatches with a switch on the variant's index, which is a jump table rather than a dependent load. The trade-off is that every element occupies the size of the largest alternative, and adding a fourth message type means touching every visitor.

**Step 4: CRTP.** The curiously recurring template pattern: `Base<Derived>` is a template, and `process()` in the base does `static_cast<Derived*>(this)->process_impl()`. Because `Derived` is known at compile time, the call is direct and can be inlined; there is no table at all. The price is that `Base<Add>` and `Base<Cancel>` are unrelated types, so you cannot put them in one vector. Write down that limitation. CRTP is for when the type is known at compile time and you want the code reuse of inheritance without the runtime cost.

**Step 5: benchmark, random and sorted.** Run all three over a million messages, once in random order and once sorted by type. The virtual version will be noticeably faster when sorted, and the reason is branch prediction: when the same message type repeats, the CPU guesses the call target correctly. Random order defeats the guess. Exercise 2.4 in Phase 2 makes this precise; for now, record the numbers and write what you think is happening.

**What you should be able to say afterwards.** "Virtual: flexible, open to extension, pays two loads and an unpredictable call. Variant: closed set of types, contiguous storage, switch-based dispatch. CRTP: zero cost, but the type must be known at compile time." And the numbers that back each sentence.

**Where this goes next.** Project B's message types and Project A's command handling are this decision made for real. Phase 2 explains the sorted-vs-random gap.

## Exercise 8: the expression evaluator

**Why it exists.** Everything so far lived in one header with one test file. Real programs span many files, and the problems that only appear then (headers, include order, duplicate symbols, the linker) are the ones a Python background has never shown you. The evaluator is a small, well-understood program whose structure forces all of that, and it is the first thing in the roadmap you could show someone as a program rather than a component. The three stages are a universal shape: tokenizer (characters to tokens), parser (tokens to a tree), evaluator (tree to a result). Compilers, config loaders, protocol decoders and query engines all have it.

**Step 1: tokenizer.** Turn `"(3 + 4) * 2"` into a list of tokens. Each token is one of a few kinds, so `std::variant<Number, Operator, LParen, RParen>` is the natural type (exercise 6, group A). The function returns `std::expected<MyVector<Token>, Error>` so that an unknown character becomes a returned error, not an exception. Tests include that error path.

**Step 2: parser.** Build a tree where a leaf is a number and an inner node is an operator with two children. The children are `MyUniquePtr<Expr>` because a node owns its subtrees and the tree has unbounded depth (exercise 3). Recursive descent is the method: one function per precedence level, each calling the next-higher level for its operands, so that `1 + 2 * 3` groups the multiplication first. Fifteen minutes of reading on the technique is allowed because the idea is small and old; the implementation is yours. Tests pin down precedence and parentheses.

**Step 3: evaluator.** Walk the tree with `std::visit`: a number evaluates to itself, an operator node evaluates both children and combines them. Division by zero returns an error through `expected` rather than crashing. Note how clean this is when the tree is a variant and the error type is a value.

**Step 4: split into files, and break the linker on purpose.** Move each stage into its own `.hpp`/`.cpp`. Then deliberately define the same non-`inline` function in two `.cpp` files and read the error. The linker is telling you the One Definition Rule has been violated: every function and variable may have exactly one definition across the whole program. The fixes are `inline` (the definition may appear in several files but means the same thing), `static` or an anonymous namespace (the definition is private to this file), or moving the definition to one `.cpp`. Two sentences on ODR in the README. This single step prevents a whole class of mysterious errors you would otherwise meet at the worst moment.

**Step 5: REPL and first profile.** A loop reading lines and printing results. Then benchmark evaluations per second on a 50-token expression and run `perf record` on it. You are not expected to optimise anything; you are expected to see, for the first time, which of your functions the time goes into. Phase 2 is entirely about that question.

**Where this goes next.** The Phase 1 checkpoint. Then Phase 2 turns the `perf` output from step 5 into a skill.

## Phase 1 checkpoint, and what comes after

The checkpoint questions are not a quiz on trivia; each one tests whether a thread from this doc held. Here is what each is really asking.

- *`v.push_back("abc")` step by step* is exercise 2's whole story in one line: a temporary `std::string` is built from the literal, storage is checked and possibly grown (moving existing elements), the temporary is moved into the slot, the temporary is destroyed. If you can narrate that, Q1, Q2 and Q3 are yours.
- *A class with correct move semantics from memory* is exercise 1, and the justification for each member is the rule of five.
- *Read a template error* is exercise 6 item 5: concepts exist because you have felt the pain.
- *Three UB cases and which sanitizer caught each* means you have been running ASan and UBSan as the rules say, and you can name a double free, a use after move or a destructor on raw bytes.
- *`const T&` vs `T&&` vs `auto&&`* is exercise 2 step 4 and exercise 4 step 4: how a parameter is declared decides whether data is copied, moved or forwarded.
- *Why exercise 7's numbers are what they are* is Q4: you can connect a mechanism to a measurement.

**What comes after.** Phase 1 answered the four questions at the level of a single object. Phase 2 asks Q1 and Q4 at the level of the machine: what a cache line is, why `vector` beats `list`, what the compiler did to your loop. Phase 3 then builds the containers that trading and database code actually use, every one of them a direct application of something from Phase 1 plus one fact from Phase 2. I will write those sections when you are about a month from starting them.
