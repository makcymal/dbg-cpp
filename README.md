# The macro `dbg()` in C++, just like in Rust

This provides two macros `dbg(...)` and `DERIVE_DEBUG(...)`, an analogue of `dbg!(...)` and `#[derive(Debug)]`


### Macro `dbg(...)`

Print the debug information in the following form:
```
[<file>:<line> (<function>) <date> <time>]
<variable>: <type> = <pretty-printed-variable>
```
This is repeated for each nested variable with the nice indentation


### Macro `DERIVE_DEBUG(...)`

Generate the method within class, that will called to pretty-print it. Can be called with fields, expressions and method calls, for instance: `DERIVE_DEBUG(a, b + c, (Method(a, b)))`. In order to correctly split on `["a", "b + c", "Method(a, b)"]`, not `["a", "b + c", "Method(a", "b)"]`, the method calls should be enclosed in parentheses

Brings into scope where it was included the following symbols:
`dbg`, `DERIVE_DEBUG`, `DBG_DEMANGLE_CLASS_NAMES`, `DBG_WRITE_TO_FILE`.
None of the `#include`s arrive

By default debug information are piped into `dbg.log` file.
And by default it's rewritten on each run. To append instead of rewrite define
`DBG_APPEND_TO_FILE` macro before including this file:
```c++
#define DBG_APPEND_TO_FILE
#include "dbg.h"
```

To pipe debug information into `stdout` define `DBG_WRITE_TO_STDOUT`:
```c++
#define DBG_WRITE_TO_STDOUT
#include "dbg.h"
```


## Known issues

On GCC and Clang user-defined class names turn out to be correct, thanks to
their extension `abi::__cxa_demangle`. On MSVC class names can be mangled a bit
