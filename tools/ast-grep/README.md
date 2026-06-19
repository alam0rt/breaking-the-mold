# ast-grep clarity checks

This directory contains lightweight, non-blocking C clarity rules for the decompilation cleanup workflow.

Run:

```sh
make lint-clarity
```

The rules are intentionally `hint` severity because the current codebase still has many known cleanup candidates. They are meant to point at the next readable refactors without breaking normal builds or existing `make lint` usage.

Current checks:

- `no-void-pointer-params`: prefer concrete pointer parameter types over `void *` when layouts are known.
- `no-void-pointer-return`: prefer concrete factory/initializer return types over `void *`.
- `no-byte-pointer-arithmetic`: flag byte-cast offset arithmetic that likely wants struct fields.
- `no-entity-cast-field-access`: flag `((Entity *)x)->field` style access when the value should be typed directly.
- `no-raw-address-symbols`: flag direct `D_800xxxxx` C identifiers so they can become meaningful asm-labeled names.
