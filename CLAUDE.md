# Claude Code Guidelines — BDE Standard

## Coding Standard: BDE (Bloomberg Development Environment)

All C++ code in this repository **must** follow BDE conventions strictly.
Deviating from any rule below is an error — do not proceed without correcting it.

---

## 1. Package / Component Layout

```
<group>/               # 3-letter package group (e.g. myp)
  <group>.mem          # lists packages in this group
  <group>.dep          # group-level dependencies
  <package>/           # package = group prefix + descriptor (e.g. mypalg)
    <package>.mem      # lists components in this package, one per line
    <package>.dep      # package-level dependencies
    <component>.h      # component header
    <component>.cpp    # component implementation
    <component>.t.cpp  # BDE test driver
```

Every component name is `<package>_<noun>` (lowercase, underscores).
Example: `mypalg_twosum`.

---

## 2. Includes — use `bsl::` not `std::`

| Wrong               | Correct                   |
|---------------------|---------------------------|
| `#include <vector>` | `#include <bsl_vector.h>` |
| `#include <unordered_map>` | `#include <bsl_unordered_map.h>` |
| `#include <string>` | `#include <bsl_string.h>` |
| `std::vector`       | `bsl::vector`             |
| `std::unordered_map`| `bsl::unordered_map`      |
| `std::string`       | `bsl::string`             |

Only use raw `std::` for items not wrapped by BSL (e.g. `std::invalid_argument`
from `<stdexcept>` is acceptable).

---

## 3. Namespaces

```cpp
namespace BloombergLP {
namespace mypalg {
    // ... declarations ...
}  // close package namespace
}  // close enterprise namespace
```

---

## 4. Header Guard Format

```cpp
#ifndef INCLUDED_MYPALG_TWOSUM
#define INCLUDED_MYPALG_TWOSUM
// ...
#endif  // INCLUDED_MYPALG_TWOSUM
```

---

## 5. Copyright Header (every file)

```cpp
// <component_name>.h                                                -*-C++-*-

// ----------------------------------------------------------------------------
// Copyright [year] Bloomberg Finance L.P.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------------------------------------------------------
```

---

## 6. Component Documentation (header)

```cpp
//@PURPOSE: One-line description ending with a period.
//
//@CLASSES:
//  pkg::ClassName: short description
//
//@DESCRIPTION: ...
//
///Usage
///-----
// ...
//..
//  code example
//..
```

---

## 7. Allocator Support

Classes that allocate memory must:
- Accept `bslma::Allocator *basicAllocator = 0` as the last constructor param.
- Store it via `bslma::Default::allocator(basicAllocator)`.
- Propagate it to all allocating members.
- Include `#include <bslma_allocator.h>` and `#include <bslma_default.h>`.

Pure utility structs with only `static` methods do not need an allocator.

---

## 8. BDE Metadata Files

Every package needs:

**`<package>.mem`** — one component name per line:
```
mypalg_twosum
```

**`<package>.dep`** — one dependency per line (BSL packages this pkg uses):
```
bsl
```

**`<group>.mem`** — one package name per line:
```
mypalg
```

**`<group>.dep`** — group dependencies:
```
bsl
```

---

## 9. Test Driver Format (`<component>.t.cpp`)

- Use the standard BDE `ASSERT` / `LOOP_ASSERT` macros (see existing files).
- Number test cases from 1 upward; case 0 falls through to the highest.
- Last test case is always the **Usage Example**.
- Each test case has: Concerns, Plan, Testing sections in the comment block.
- `main` dispatches on `argv[1]`; `argv[2]` enables verbose output.

---

## 10. Build System

Use **BDE waf tools** (`bde-tools`) when available, or BDE-compatible CMake
(`cmake-adaptor` from `bde-tools`). Do **not** use plain CMake without the
BDE cmake adaptor — it will not handle `.mem`/`.dep` files or BDE conventions.

---

## Summary Checklist (before any commit)

- [ ] All includes use `bsl_*.h` form; all types use `bsl::` prefix
- [ ] Copyright header present in every file
- [ ] `.mem` and `.dep` files exist for every package and group
- [ ] Namespace closes are annotated: `// close package namespace`
- [ ] Header guard matches `INCLUDED_<COMPONENT>` exactly
- [ ] Test driver follows numbered-case BDE format
- [ ] No raw `new`/`delete` — use `bslma` allocators or value semantics
