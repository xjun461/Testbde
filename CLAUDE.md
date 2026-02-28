# Claude Code Guidelines — BDE Standard

All C++ code must follow BDE (Bloomberg Development Environment) conventions.

## Key Rules

- Use `bsl::vector`, `bsl::unordered_map`, `bsl::string`, etc. — never raw `std::` containers.
  Include as `#include <bsl_vector.h>`, not `#include <vector>`. Only use raw `std::` for items
  not wrapped by BSL (e.g. `std::invalid_argument` from `<stdexcept>` is acceptable).
- Every package needs `<pkg>.mem` and `<pkg>.dep` metadata files.
- Every group needs `<group>.mem` and `<group>.dep` metadata files.
- Copyright header (Bloomberg LP / Apache 2.0) in every file.
- Namespace: `BloombergLP::<package>`. Close comments required (`// close package namespace`).
- Header guard: `INCLUDED_<COMPONENT>` (e.g. `INCLUDED_MYPALG_TWOSUM`).
- Test driver: numbered cases, BDE `ASSERT`/`LOOP_ASSERT` macros, usage example last.
- Build: use bde-tools waf or BDE cmake adaptor — not plain CMake.
- Classes that allocate memory must accept `bslma::Allocator *basicAllocator = 0` as the last
  constructor param and store it via `bslma::Default::allocator(basicAllocator)`.
