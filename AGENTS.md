# AGENTS.md - libccll

## What this is

C shared library (`libcll.so`) providing common Linux utilities: ini config parser, tracked allocator, doubly-linked list, event pub/sub, RSA/KLC cipher, shell executor, timer (timerfd), threadpool, memory stats, IO helpers.

The developing env may be ubuntu server 20.04 or 24.04, but the running env is ubuntu server 24.04.

## Build

Never build it

## Test

Never test it

## Header naming convention

Each module has two header directories under `<module>/inc/`:
- `_foo.h` (underscore prefix) = **internal** header, used only within the library
- `foo.h` = **public** API header, used by consumers (and tests via `out/inc/cl_foo.h`)

When adding a new module, follow this pattern.

## TAG macro (required in every .c file)

Every `.c` file that uses logging **must** define:
```c
TAG = "my-module";
```
This expands to `const static char *cltag`. The `CL_TAG` macro references `cltag` for all `CLOGD`/`CLOGI`/`CLOGW`/`CLOGE`/`TRACE` calls. Without it, compilation fails.

The `TAG` macro is defined in `log/inc/log.h` and must be included.

## Init/deinit lifecycle

`cl_init(print_fun)` initializes all subsystems (log, evt, timer, rsa, threadpool) in order. `cl_deinit()` tears them down in reverse. Call `cl_init` before using any library function.

## Build output structure

```
out/
  lib/libcll.so    # shared library
  inc/cl_*.h       # public headers copied with cl_ prefix
  obj/             # object files (mirrors source tree)
  test/test        # test binary
  etc/             # config files (make cfgs target)
```

## Code style

K&R braces (opening `{` on same line). Tabs for indentation. See `comm/inc/def.h` for base types (`Ret`, `bool`, `container_of`, buffer structs).
