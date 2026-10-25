# Git branching strategy (Phase 6)
`main` always builds and passes CI. Work happens on short-lived branches (`feature/<topic>`, `fix/<topic>`),
merged to `main` by pull request after CI is green and a self-review checklist is ticked. Commit messages:
imperative, one logical change (e.g. `Add late-penalty strategy and boundary tests`). Releases are annotated tags
(`v1.0`, `v1.1`). Suggested first commits for this code base, in order: project skeleton + CMake -> common utilities ->
domain + strategy -> security -> schema + repositories -> services -> console UI -> tests -> CI/docs.
