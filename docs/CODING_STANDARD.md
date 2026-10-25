# Coding standard (enforced by `.clang-format`, `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`, cppcheck, review)

1. **Language**: C++17, no compiler extensions. Everything lives in namespace `lms`.
2. **Naming**: types `PascalCase`; functions and variables `camelCase`; private data members end with `_`;
   constants `kPascalCase`; interfaces start with `I` (`IUserRepository`); enums are `enum class`.
3. **Files**: one main class per header/source pair named after the class; `#pragma once`; include own header
   first, then project headers, then system headers.
4. **Layering**: UI -> services -> repository interfaces -> SQLite. No SQL outside `src/data`; no business rules in
   `src/ui`; services depend on interfaces only (DIP).
5. **SQL**: only through `Statement` with `?N` bound parameters. String concatenation of user data into SQL is forbidden.
6. **Errors**: throw the `LmsError` subclasses in `common/Errors.h`; never return error codes or `exit()`. Only the
   UI layer and `main` catch exceptions. Messages shown to users contain no internal detail.
7. **Resources and ownership**: RAII only; no raw `new`/`delete`; `unique_ptr` for ownership, references for
   non-owning dependencies; no globals (clock, hasher and strategy are injected).
8. **Const-correctness**: methods and parameters are `const` unless they mutate; no C-style casts.
9. **Size**: functions should fit on one screen (<= ~50 lines) and keep cyclomatic complexity <= 15.
10. **Comments**: explain *why* and cite requirement/design decisions; do not restate the code.
11. **Security**: passwords are never logged, stored or compared in plain text; all permission checks happen in
    services (never only in the UI).
12. **Tests**: every new rule gets a unit test; every Must requirement has a system test.
