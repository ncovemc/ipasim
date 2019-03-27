// Common.hpp

#ifndef COMMON_HPP
#define COMMON_HPP

#include <llvm/Support/raw_ostream.h>

#include <cstdint>
#include <filesystem>
#include <string>

// LibType
enum class LibType { None = 0, Dylib = 0x1, DLL = 0x2, Both = 0x3 };

constexpr bool operator&(LibType Value, LibType Flag) {
  return ((uint32_t)Value & (uint32_t)Flag) == (uint32_t)Flag;
}

// Prefix and postfix operators.
#define prefix(op) &operator op()
#define postfix(op) operator op(int)

// Filesystem.
std::unique_ptr<llvm::raw_fd_ostream> createOutputFile(const std::string &Path);
std::filesystem::path createOutputDir(const char *Path);

// Strings.
// `constexpr` `strlen`. Usage: `constexpr size_t len = length(ConstExprVar);`.
size_t constexpr length(const char *S) { return *S ? 1 + length(S + 1) : 0; }
struct ConstexprString {
  constexpr ConstexprString(const char *S) : S(S), Len(length(S)) {}

  const char *S;
  size_t Len;
};
inline bool startsWith(const std::string &S, const std::string &Prefix) {
  return !S.compare(0, Prefix.length(), Prefix);
}
inline bool startsWith(const std::string &S, ConstexprString Prefix) {
  return !S.compare(0, Prefix.Len, Prefix.S);
}
inline bool endsWith(const std::string &S, const std::string &Suffix) {
  return !S.compare(S.length() - Suffix.length(), Suffix.length(), Suffix);
}
inline bool endsWith(const std::string &S, ConstexprString Suffix) {
  return !S.compare(S.length() - Suffix.Len, Suffix.Len, Suffix.S);
}

// Iterators.

template <typename T, typename FTy> class MappedContainer {
public:
  using ItTy = decltype(std::declval<T>().begin());

  MappedContainer(T &&Container, FTy &&Func)
      : Container(std::forward<T>(Container)), Func(std::forward<FTy>(Func)) {}

  auto begin() { return Func(Container.begin()); }
  auto end() { return Func(Container.end()); }

private:
  T &&Container;
  FTy Func;
};
template <typename T, typename FTy>
auto mapContainer(T &&Container, FTy &&Func) {
  return MappedContainer<T, FTy>(std::forward<T>(Container),
                                 std::forward<FTy>(Func));
}
template <typename T, typename FTy>
auto mapIterator(T &&Container, FTy &&Func) {
  return mapContainer(std::forward<T>(Container),
                      [Func(std::forward<FTy>(Func))](auto It) {
                        return llvm::map_iterator(std::move(It), Func);
                      });
}

// Should work for, e.g., `T = std::vector<std::vector<uint32_t>::iterator>`.
// Dereferences iterated values.
template <typename T> auto deref(T &&Container) {
  return mapIterator(std::forward<T>(Container),
                     [](auto Value) { return *Value; });
}

template <typename ItTy>
class WithPtrsIterator
    : public llvm::iterator_adaptor_base<WithPtrsIterator<ItTy>, ItTy> {
public:
  WithPtrsIterator(ItTy It)
      : WithPtrsIterator::iterator_adaptor_base(std::move(It)) {}

  auto getCurrent() { return this->I; }
  auto operator*() { return std::make_pair(this->I, *this->I); }
};

// Should work for, e.g., `T = std::vector<uint32_t>`. Maps iterated values to
// pairs of their iterators and themselves.
template <typename T> auto withPtrs(T &&Container) {
  return mapContainer(std::forward<T>(Container),
                      [](auto It) { return WithPtrsIterator(std::move(It)); });
}

class Counter {
public:
  template <typename T> auto operator()(T &Value) {
    return std::pair<size_t, T &>(Idx++, Value);
  }

private:
  size_t Idx = 0;
};

// Maps iterated values to pairs of their indices and themselves.
template <typename T> auto withIndices(T &&Container) {
  return mapIterator(std::forward<T>(Container), Counter());
}

// !defined(COMMON_HPP)
#endif
