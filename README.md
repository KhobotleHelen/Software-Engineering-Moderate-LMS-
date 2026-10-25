# Moderated Learning Management System (CS4433, NUL)

C++17 console application, SQLite storage, works fully offline. Roles: Administrator, Lecturer,
Head of Department, Student. Marks stay hidden from students until the HoD approves and releases them.

Quick start: see `docs/BUILD.md`. Design differences from the Phase 3 document: `docs/DESIGN_DEVIATIONS.md`.
Requirement-to-code-to-test mapping: `docs/TRACEABILITY.md`. QA evidence: `docs/qa/`.

```
src/common     DateTime, Clock, Validation, Errors        (business-logic layer support)
src/domain     Types, Entities, User + 4 roles, UserFactory
src/strategy   ILatePenaltyStrategy, StandardLatePenalty   (Strategy pattern)
src/security   IPasswordHasher, SodiumPasswordHasher       (Argon2id via libsodium)
src/data       Database, Schema, Repositories (interfaces), SqliteRepositories (Repository/DAO)
src/services   Auth, User, Academic, Assignment, Submission, Assessment, AccessControl, AppContext
src/ui         ConsoleApp (presentation layer, no business rules, no SQL)
tests          GoogleTest: unit, integration, scripted system tests, performance
tools          lms_seed (demo / 800-student data set)
```
