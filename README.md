# The macro `dbg()` in C++, just like in Rust

This provides two macros `dbg(...)` and `DERIVE_DEBUG(...)`, an analogue of `dbg!(...)` and `#[derive(Debug)]`


## Example

For example, the following code with nested classes and variables:
```c++
#include "dbg.h"

class Foo {
  class Bar {
    float pi = 3.14, e = 2.71;
    std::shared_ptr<float> count{std::make_shared<float>(9.81)};
   public:
    DERIVE_DEBUG(pi * e, count);
  };

  std::map<std::string, float> map{{"light speed", 2.9e+8},
                                   {"electron mass", 9.1e-31}};
  std::vector<Bar> bars{Bar()};
 public:
  DERIVE_DEBUG(map, bars);
};

int main() {
  std::string msg = "dbg is fun!";
  dbg(msg);
  std::list<Foo> foo_list{Foo()};
  dbg(foo_list, (msg.substr(7, 3)));
}
```

... gives the following `dbg.log`:
```
[main.cc:26 (main) 25.01.25 12:34:56]
msg: std::string = "dbg is fun!"

[main.cc:28 (main) 25.01.25 12:34:56]
foo_list: std::list = {
  <Foo>
  [0] = {
    map: std::map = {
      <std::string -> float>
      ["electron mass"] = 9.1e-31
      ["light speed"] = 2.9e+08
    }
    bars: std::vector = {
      <Foo::Bar>
      [0] = {
        pi * e: float = 8.5094
        count: std::shared_ptr = {9.81}
      }
    }
  }
}
msg.substr(7, 3): std::string = "fun"
```

## Macro `dbg(...)`

Print the debug information in the following form:
```
[<file>:<line> (<function>) <date> <time>]
<variable>: <type> = <pretty-printed-variable>
```
This is repeated for each nested variable with the nice indentation. Can be called with fields, expressions and method calls, for instance: `dbg(a, b + c, (Method(a, b)))`. In order to correctly split on `["a", "b + c", "Method(a, b)"]`, not `["a", "b + c", "Method(a", "b)"]`, the method calls should be enclosed in parentheses

> Enclose the expessions containing commas in parentheses


## Macro `DERIVE_DEBUG(...)`

Generate the method within class, that will called to pretty-print it. Can be called with fields, expressions and method calls, for instance: `DERIVE_DEBUG(a, b + c, (Method(a, b)))`.

> Ensure `DERIVE_DEBUG()` is placed in public block

In order to correctly split on `["a", "b + c", "Method(a, b)"]`, not `["a", "b + c", "Method(a", "b)"]`, the method calls should be enclosed in parentheses

> Enclose the expessions containing commas in parentheses


## Enabling or disable debug

One can disable debugging without having to remove all `dbg()`'s with `DISABLE_DEBUG`. To enable it back do `ENABLE_DEBUG`. By default it's enabled, of course. Ensure the order of your `DISABLE_DEBUG`'s and `ENABLE_DEBUG`'s is what you think it is


## Customization

By default debug messages are piped into `dbg.log` file (e.g., it's a pain to look for them among test reports, isn't it?).
And by default it's rewritten on each run. To append instead of rewrite define
`DBG_APPEND_TO_FILE` macro before including this file:
```c++
#define DBG_APPEND_TO_FILE
#include "dbg.h"
```

To pipe debug messages into `stdout` define `DBG_WRITE_TO_STDOUT`:
```c++
#define DBG_WRITE_TO_STDOUT
#include "dbg.h"
```


## Known issues

- On GCC and Clang user-defined class names turn out to be correct, thanks to
their extension `abi::__cxa_demangle`. On MSVC class names can be mangled a bit

- Besides `dbg` and `DERIVE_DEBUG` brings into scope where it was included the symbols `DBG_DEMANGLE_CLASS_NAMES`, `DBG_WRITE_TO_FILE`
