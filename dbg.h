/*
  The macro dbg() in C++, just like in Rust
  This provides two macros: dbg(...) and DERIVE_DEBUG(...), an analogue of
  dbg!(...) and #[derive(Debug)]

  dbg(...)
  Print the debug information in the following form:
  [<file>:<line> (<function>) <date> <time>]
  <variable>: <type> = <pretty-printed variable>
  This is repeated for each nested variable with the nice indentation

  DERIVE_DEBUG(...)
  Generate the method within class, that will called to pretty-print it.
  Can be called with fields, expressions and method calls, for instance:
  DERIVE_DEBUG(a, b + c, (Method(a, b))).
  In order to correctly split on ["a", "b + c", "Method(a, b)"],
  not ["a", "b + c", "Method(a", "b)"], the method calls should be enclosed
  in parentheses

  Brings into scope where it was included the following symbols:
  dbg, DERIVE_DEBUG, DBG_DEMANGLE_CLASS_NAMES, DBG_WRITE_TO_FILE.
  None of the #include's arrive

  By default debug information are piped into dbg.log file.
  And by default it's rewritten on each run. To append instead of rewrite define
  DBG_APPEND_TO_FILE macro before including this file:

    #define DBG_APPEND_TO_FILE
    #include "dbg.h"

  To pipe debug information into stdout define DBG_WRITE_TO_STDOUT:

    #define DBG_WRITE_TO_STDOUT
    #include "dbg.h"

  On GCC and Clang user-defined class names turn out to be correct, thanks to
  their extension abi::__cxa_demangle. On MSVC class names can be mangled a bit
*/


#pragma once

#include <array>
#include <concepts>
#include <ctime>
#include <deque>
#include <fstream>
#include <iomanip>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>


// On GCC and Clang enable the extension abi::__cxa_demangle that demangles
// user-defined class names. On MSVC class names can be mangled a bit
#if defined(__GNUC__) || defined(__clang__)
#define DBG_DEMANGLE_CLASS_NAMES
#include <cxxabi.h>
#endif


// To write to stdout if user wishes so
#if defined(DBG_WRITE_TO_STDOUT)
#include <iostream>
#endif


// Incapsulate logic within this namespace
namespace __dbg_internal {

// By default, write to clean file "dbg.log"
#if !defined(DBG_APPEND_FILE) && !defined(DBG_WRITE_TO_STDOUT)
#define DBG_WRITE_TO_FILE
#endif

#if defined(DBG_WRITE_TO_FILE)
std::ofstream out("dbg.log");
#endif

// Or append to "dbg.log" if user wishes so
#if defined(DBG_APPEND_TO_FILE)
std::ofstream out("dbg.log", std::ios_base::app);
#endif

// Or write to stdout if user wishes so
#if defined(DBG_WRITE_TO_STDOUT)
auto &out = std::cout;
#endif

// dbg() use it to add one more \n between calls
bool dbg_was_called = false;

// dbg() use it to determine if it should work at all
bool dbg_enabled = true;


// The current global indentation, PrettyPrint() functions prints it along with
// the data to make it readable
std::string indent;

// Add two spaces to indentation before the new {} block
void IncreaseIndent() {
  indent += "  ";
}

// Erase two spaces from indentation after the {} block
void DecreaseIndent() {
  indent.resize(indent.size() - 2);
}


// Tries to print type name, without template parameter types.
// On GCC and Clang user-defined class names turn out to be correct, thanks to
// their extension abi::__cxa_demangle. On MSVC class names can be mangled a bit
template <class T>
void PrintTypeName(const T &x) {
  const char *mangled = typeid(x).name();

#ifdef DBG_DEMANGLE_CLASS_NAMES
  int status = 0;
  std::unique_ptr<char, void (*)(void *)> demangled{
      abi::__cxa_demangle(mangled, nullptr, nullptr, &status), std::free};
  const char *name = (status == 0) ? demangled.get() : mangled;
#else
  const char *name = mangled;
#endif

  for (int i = 0; name[i] != '\0'; ++i) {
    if (name[i] == '<') {
      std::string_view name_view(name, name + i);
      // Make weird std names more readable
      if (name_view == "std::__cxx11::basic_string") {
        out << "std::string";
      } else if (name_view == "std::__cxx11::list") {
        out << "std::list";
      } else {
        out << name_view;
      }
      return;
    }
  }
  out << name;
}


// Prints current time in form of <dd.mm.yy HH:MM:SS>
void PrintCurrTime() {
  std::time_t timestamp = std::time(nullptr);
  std::tm *now = std::localtime(&timestamp);
  out << std::setfill('0') << std::setw(2) << now->tm_mday << "."
      << std::setfill('0') << std::setw(2) << (now->tm_mon + 1) << "."
      << std::setfill('0') << std::setw(2) << (now->tm_year % 100) << " "
      << std::setfill('0') << std::setw(2) << now->tm_hour << ":"
      << std::setfill('0') << std::setw(2) << now->tm_min << ":"
      << std::setfill('0') << std::setw(2) << now->tm_sec;
}


// Distiguish scalar types like int, float or char
template <class T>
concept is_scalar = std::is_scalar_v<T>;

// Distinguish class types, regardless from std or user-defined
template <class T>
concept is_class = std::is_class_v<T>;


// Print scalar type like int, float or char
template <class T>
  requires is_scalar<T>
void PrettyPrint(T x) {
  out << x;
}

// Print any class object, that isn't overloaded later
// Note that class must generate PrettyPrint method using DERIVE_DEBUG macro
template <class T>
  requires is_class<T>
void PrettyPrint(const T &x) {
  const_cast<T &>(x).PrettyPrint();
}


// Print what's behind std::ref
template <class T>
void PrettyPrint(std::reference_wrapper<T> x) {
  PrettyPrint(x.get());
}


// Print std::string
void PrettyPrint(const std::string &x) {
  out << "\"" << x << "\"";
}


// Print std::array of scalars
template <class T, size_t N>
  requires is_scalar<T>
void PrettyPrint(const std::array<T, N> &x) {
  out << "{";
  if (!x.empty()) {
    PrettyPrint(x[0]);
  }
  for (int i = 1; i < x.size(); ++i) {
    out << ", ";
    PrettyPrint(x[i]);
  }
  out << "}";
}

// Print std::array of class objects
template <class T, size_t N>
  requires is_class<T>
void PrettyPrint(const std::array<T, N> &x) {
  if (x.empty()) {
    out << "{}";
    return;
  }
  out << "{\n";
  IncreaseIndent();
  out << indent << "<";
  PrintTypeName(x[0]);
  out << ">\n";
  for (int i = 0; i < x.size(); ++i) {
    out << indent << "[" << i << "] = ";
    PrettyPrint(x[i]);
    out << "\n";
  }
  DecreaseIndent();
  out << indent << "}";
}


// Print std::vector of scalars
template <class T>
  requires is_scalar<T>
void PrettyPrint(const std::vector<T> &x) {
  out << "{";
  if (!x.empty()) {
    PrettyPrint(x[0]);
  }
  for (int i = 1; i < x.size(); ++i) {
    out << ", ";
    PrettyPrint(x[i]);
  }
  out << "}";
}

// Print std::vector of class objects
template <class T>
  requires is_class<T>
void PrettyPrint(const std::vector<T> &x) {
  if (x.empty()) {
    out << "{}";
    return;
  }
  out << "{\n";
  IncreaseIndent();
  out << indent << "<";
  PrintTypeName(x[0]);
  out << ">\n";
  for (int i = 0; i < x.size(); ++i) {
    out << indent << "[" << i << "] = ";
    PrettyPrint(x[i]);
    out << "\n";
  }
  DecreaseIndent();
  out << indent << "}";
}


// Print std::deque of scalars
template <class T>
  requires is_scalar<T>
void PrettyPrint(const std::deque<T> &x) {
  out << "{";
  if (!x.empty()) {
    PrettyPrint(x[0]);
  }
  for (int i = 1; i < x.size(); ++i) {
    out << ", ";
    PrettyPrint(x[i]);
  }
  out << "}";
}

// Print std::deque of class objects
template <class T>
  requires is_class<T>
void PrettyPrint(const std::deque<T> &x) {
  if (x.empty()) {
    out << "{}";
    return;
  }
  out << "{\n";
  IncreaseIndent();
  out << indent << "<";
  PrintTypeName(x[0]);
  out << ">\n";
  for (int i = 0; i < x.size(); ++i) {
    out << indent << "[" << i << "] = ";
    PrettyPrint(x[i]);
    out << "\n";
  }
  DecreaseIndent();
  out << indent << "}";
}


// Print std::queue of scalars
template <class T>
  requires is_scalar<T>
void PrettyPrint(std::queue<T> x) {
  out << "{";
  if (!x.empty()) {
    PrettyPrint(x.front());
    x.pop();
  }
  while (!x.empty()) {
    out << ", ";
    PrettyPrint(x.front());
    x.pop();
  }
  out << "}";
}

// Print std::queue of class objects
template <class T>
  requires is_class<T> && std::copy_constructible<T>
void PrettyPrint(std::queue<T> x) {
  if (x.empty()) {
    out << "{}";
    return;
  }
  out << "{\n";
  IncreaseIndent();
  out << indent << "<";
  PrintTypeName(x.front());
  out << ">\n";
  for (int i = 0; !x.empty(); ++i) {
    out << indent << "[" << i << "] = ";
    PrettyPrint(x.front());
    x.pop();
    out << "\n";
  }
  DecreaseIndent();
  out << indent << "}";
}


// Print std::stack of scalars
template <class T>
  requires is_scalar<T>
void PrettyPrint(std::stack<T> x) {
  out << "{";
  if (!x.empty()) {
    PrettyPrint(x.top());
    x.pop();
  }
  while (!x.empty()) {
    out << ", ";
    PrettyPrint(x.top());
    x.pop();
  }
  out << "}";
}

// Print std::stack of class objects
template <class T>
  requires is_class<T> && std::copy_constructible<T>
void PrettyPrint(std::stack<T> x) {
  if (x.empty()) {
    out << "{}";
    return;
  }
  out << "{\n";
  IncreaseIndent();
  out << indent << "<";
  PrintTypeName(x.top());
  out << ">\n";
  for (int i = 0; !x.empty(); ++i) {
    out << indent << "[" << i << "] = ";
    PrettyPrint(x.top());
    x.pop();
    out << "\n";
  }
  DecreaseIndent();
  out << indent << "}";
}


// Print std::list of scalars
template <class T>
  requires is_scalar<T>
void PrettyPrint(const std::list<T> &x) {
  out << "{";
  auto it = x.begin(), end = x.end();
  if (!x.empty()) {
    PrettyPrint(*it);
    ++it;
  }
  for (; it != end; ++it) {
    out << ", ";
    PrettyPrint(*it);
  }
  out << "}";
}

// Print std::list of class objects
template <class T>
  requires is_class<T>
void PrettyPrint(const std::list<T> &x) {
  if (x.empty()) {
    out << "{}";
    return;
  }
  out << "{\n";
  auto it = x.begin(), end = x.end();
  IncreaseIndent();
  out << indent << "<";
  PrintTypeName(*it);
  out << ">\n";
  for (int i = 0; it != end; ++i, ++it) {
    out << indent << "[" << i << "] = ";
    PrettyPrint(*it);
    out << "\n";
  }
  DecreaseIndent();
  out << indent << "}";
}


// Print std::set of scalars
template <class T>
  requires is_scalar<T>
void PrettyPrint(const std::set<T> &x) {
  out << "{";
  auto it = x.begin(), end = x.end();
  if (!x.empty()) {
    PrettyPrint(*it);
    ++it;
  }
  for (; it != end; ++it) {
    out << ", ";
    PrettyPrint(*it);
  }
  out << "}";
}

// Print std::set of class objects
template <class T>
  requires is_class<T>
void PrettyPrint(const std::set<T> &x) {
  if (x.empty()) {
    out << "{}";
    return;
  }
  out << "{\n";
  auto it = x.begin(), end = x.end();
  IncreaseIndent();
  out << indent << "<";
  PrintTypeName(*it);
  out << ">\n";
  for (int i = 0; it != end; ++i, ++it) {
    out << indent << "[" << i << "] = ";
    PrettyPrint(*it);
    out << "\n";
  }
  DecreaseIndent();
  out << indent << "}";
}


// Print std::unordered_set of scalars
template <class T>
  requires is_scalar<T>
void PrettyPrint(const std::unordered_set<T> &x) {
  out << "{";
  auto it = x.begin(), end = x.end();
  if (!x.empty()) {
    PrettyPrint(*it);
    ++it;
  }
  for (; it != end; ++it) {
    out << ", ";
    PrettyPrint(*it);
  }
  out << "}";
}

// Print std::unordered_set of class objects
template <class T>
  requires is_class<T>
void PrettyPrint(const std::unordered_set<T> &x) {
  if (x.empty()) {
    out << "{}";
    return;
  }
  out << "{\n";
  auto it = x.begin(), end = x.end();
  IncreaseIndent();
  out << indent << "<";
  PrintTypeName(*it);
  out << ">\n";
  for (int i = 0; it != end; ++i, ++it) {
    PrettyPrint(*it);
    out << "\n";
  }
  DecreaseIndent();
  out << indent << "}";
}


// Print std::map regargdell keys and values are scalars or class objects
template <class K, class V>
void PrettyPrint(const std::map<K, V> &x) {
  if (x.empty()) {
    out << "{}";
    return;
  }
  out << "{\n";
  auto it = x.begin(), end = x.end();
  IncreaseIndent();
  out << indent << "<";
  PrintTypeName(it->first);
  out << " -> ";
  PrintTypeName(it->second);
  out << ">\n";
  for (; it != end; ++it) {
    out << indent << "[";
    PrettyPrint(it->first);
    out << "] = ";
    PrettyPrint(it->second);
    out << "\n";
  }
  DecreaseIndent();
  out << indent << "}";
}


// Print std::unordered_map regargdell keys and values are scalars or classes
template <class K, class V>
void PrettyPrint(const std::unordered_map<K, V> &x) {
  if (x.empty()) {
    out << "{}";
    return;
  }
  out << "{\n";
  auto it = x.begin(), end = x.end();
  IncreaseIndent();
  out << indent << "<";
  PrintTypeName(it->first);
  out << " -> ";
  PrintTypeName(it->second);
  out << ">\n";
  for (; it != end; ++it) {
    out << indent << "[";
    PrettyPrint(it->first);
    out << "] = ";
    PrettyPrint(it->second);
    out << "\n";
  }
  DecreaseIndent();
  out << indent << "}";
}


// Print unique pointer to scalar
template <class T>
  requires is_scalar<T>
void PrettyPrint(const std::unique_ptr<T> &x) {
  out << "{";
  PrettyPrint(*x);
  out << "}";
}

// Print unique pointer to class
template <class T>
  requires is_class<T>
void PrettyPrint(const std::unique_ptr<T> &x) {
  IncreaseIndent();
  out << "{\n" << indent << "< -> ";
  PrintTypeName(*x);
  out << ">\n" << indent;
  PrettyPrint(*x);
  DecreaseIndent();
  out << "\n" << indent << "}";
}


// Print shared pointer to scalar
template <class T>
  requires is_scalar<T>
void PrettyPrint(const std::shared_ptr<T> &x) {
  out << "{";
  PrettyPrint(*x);
  out << "}";
}

// Print shared pointer to class
template <class T>
  requires is_class<T>
void PrettyPrint(const std::shared_ptr<T> &x) {
  IncreaseIndent();
  out << "{\n" << indent << "<";
  PrintTypeName(*x);
  out << ">\n" << indent;
  PrettyPrint(*x);
  DecreaseIndent();
  out << "\n" << indent << "}";
}


// Parse single C-string into argument names that DERIVE_DEBUG was called with
// DERIVE_DEBUG can be called with fields, expressions and method calls, for
// instance: DERIVE_DEBUG(a, b + c, (Method(a, b))). In order to correctly split
// on ["a", "b + c", "Method(a, b)"], not ["a", "b + c", "Method(a", "b)"],
// the method callings should be enclosed in parentheses.
class ArgNames {
 public:
  ArgNames(const char *args) : args_(args) {
  }

  std::string pop() {
    for (; idx_ < args_.size() and isspace(args_[idx_]); ++idx_) {
    }
    if (idx_ == args_.size()) {
      return "";
    }

    std::string name;
    if (args_[idx_] == '(') {
      int lvl = 1, end = idx_ + 1;
      for (; end < args_.size(); ++end) {
        if (args_[end] == '(') {
          ++lvl;
        } else if (args_[end] == ')') {
          --lvl;
        }

        if (lvl == 0) {
          break;
        }
      }
      name = args_.substr(idx_ + 1, end - idx_ - 1);
      for (; end < args_.size() and args_[end] != ','; ++end) {
      }
      idx_ = end + 1;

    } else {
      int end = idx_ + 1;
      for (; end < args_.size() and args_[end] != ','; ++end) {
      }
      name = args_.substr(idx_, end - idx_);
      idx_ = end + 1;
    }

    return name;
  }

 private:
  std::string args_;
  int idx_ = 0;
};


// Call the PrettyPrint on the last argument from the given variadic list
// assigning it the top name from ArgNames.
template <class T>
void MultiplexPrettyPrintOnNamedArgs(ArgNames &names, const T &last) {
  out << indent << names.pop() << ": ";
  PrintTypeName(last);
  out << " = ";
  PrettyPrint(last);
  out << "\n";
}

// Call the PrettyPrint on the first argument from the given variadic list
// assigning it the top name from ArgNames. The following call reduces argument
// list by one
template <class T, class... Args>
void MultiplexPrettyPrintOnNamedArgs(ArgNames &names, const T &first,
                                     const Args &...args) {
  out << indent << names.pop() << ": ";
  PrintTypeName(first);
  out << " = ";
  PrettyPrint(first);
  out << "\n";
  MultiplexPrettyPrintOnNamedArgs(names, args...);
}

// Parse single C-string into argument names that dbg or DERIVE_DEBUG was called
// with and start calling PrettyPrint on the arguments one by one
template <class... Args>
void MultiplexPrettyPrintOnVaArgs(const char *names, const Args &...args) {
  ArgNames arg_names(names);
  MultiplexPrettyPrintOnNamedArgs(arg_names, args...);
}

}  // namespace __dbg_internal


// Print the debug information in the following form:
// [<file>:<line> (<function>) <date> <time>]
// <variable>: <type> = <pretty-printed variable>
// This is repeated for each nested variable with the nice indentation
#define dbg(...)                                                             \
  if (__dbg_internal::dbg_enabled) {                                         \
    if (__dbg_internal::dbg_was_called)                                      \
      __dbg_internal::out << "\n";                                           \
    __dbg_internal::dbg_was_called = true;                                   \
    __dbg_internal::out << "[" << __FILE__ << ":" << __LINE__ << " ("        \
                        << __func__ << ") ";                                 \
    __dbg_internal::PrintCurrTime();                                         \
    __dbg_internal::out << "]\n";                                            \
    __dbg_internal::MultiplexPrettyPrintOnVaArgs(#__VA_ARGS__, __VA_ARGS__); \
    std::flush(__dbg_internal::out);                                         \
  }


// Generate the PrettyPrint() method within class, that will called from
// __dbg_internal::PrettyPrint function for user-defined classes.
// DERIVE_DEBUG can be called with fields, expressions and method calls, for
// instance: DERIVE_DEBUG(a, b + c, (Method(a, b))). In order to correctly split
// on ["a", "b + c", "Method(a, b)"], not ["a", "b + c", "Method(a", "b)"],
// the method callings should be enclosed in parentheses.
#define DERIVE_DEBUG(...)                                                    \
  void PrettyPrint() {                                                       \
    __dbg_internal::out << "{\n";                                            \
    __dbg_internal::IncreaseIndent();                                        \
    __dbg_internal::MultiplexPrettyPrintOnVaArgs(#__VA_ARGS__, __VA_ARGS__); \
    __dbg_internal::DecreaseIndent();                                        \
    __dbg_internal::out << __dbg_internal::indent << "}";                    \
    std::flush(__dbg_internal::out);                                         \
  }


// This provides the ability of turning off debugging on some areas of code
#define DISABLE_DEBUG __dbg_internal::dbg_enabled = false;
#define ENABLE_DEBUG __dbg_internal::dbg_enabled = true;
