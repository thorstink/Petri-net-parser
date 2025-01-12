#pragma once

/** @file colors.hpp */

#include <string.h>

#include <algorithm>
#include <array>  // std::array
#include <cassert>
#include <functional>
#include <string_view>
#include <type_traits>
#include <utility>  // std::index_sequence
#include <vector>   // hash

#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace sym_impl {
// https://rodusek.com/posts/2021/03/09/getting-an-unmangled-type-name-at-compile-time/

template <std::size_t... Idxs>
constexpr auto substring_as_array(std::string_view str,
                                  std::index_sequence<Idxs...>) {
  return std::array{str[Idxs]..., '\0'};
}

template <typename T>
constexpr auto type_name_array() {
#if defined(__clang__)
  constexpr auto prefix = std::string_view{"[T = symmetri::"};
  constexpr auto suffix = std::string_view{"]"};
  constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
#elif defined(__GNUC__)
  constexpr auto prefix = std::string_view{"with T = symmetri::"};
  constexpr auto suffix = std::string_view{"]"};
  constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
#elif defined(_MSC_VER)
  constexpr auto prefix = std::string_view{"type_name_array<symmetri::"};
  constexpr auto suffix = std::string_view{">(void)"};
  constexpr auto function = std::string_view{__FUNCSIG__};
#else
#error Unsupported compiler
#endif

  constexpr auto start = function.find(prefix) + prefix.size();
  constexpr auto end = function.rfind(suffix);

  static_assert(start < end);

  constexpr auto name = function.substr(start, (end - start));
  return substring_as_array(name, std::make_index_sequence<name.size()>{});
}

template <typename T>
struct type_name_holder {
  static inline constexpr auto value = type_name_array<T>();
};

template <typename T>
constexpr auto type_name() -> std::string_view {
  constexpr auto& value = type_name_holder<T>::value;
  return std::string_view{value.data(), value.size()};
}

template <auto Id>
struct counter {
  using tag = counter;

  struct generator {
    template <typename...>
    friend constexpr auto is_defined(tag) {
      return true;
    }
  };

  template <typename...>
  friend constexpr auto is_defined(tag);

  template <typename Tag = tag, auto I = (int)is_defined(Tag{})>
  static constexpr auto exists(decltype(I)) {
    return true;
  }

  static constexpr auto exists(...) { return generator(), false; }
};

template <typename T, auto Id = int{}>
constexpr auto unique_id() {
  if constexpr (counter<Id>::exists(Id)) {
    return unique_id<T, Id + 1>();
  } else {
    return Id;
  }
}

#endif /* DOXYGEN_SHOULD_SKIP_THIS */
}
namespace symmetri {

/**
 * @brief Tokens are elements that can reside in places. Tokens can have a color
 * which makes them distinguishable from other tokens. Tokens that have the same
 * color are not distinguishable. Users can create their own token-colors by
 * either using the CREATE_CUSTOM_TOKEN-macro (compile-time) or by calling
 * Token's public constructor which takes a token-name.
 *
 */
class Token {
 public:
  /**
   * @brief Get a list of all the colors
   *
   * @return std::vector<const char *>
   */
  static std::vector<const char*> getColors() {
    std::vector<const char*> _colors;
    _colors.reserve(colors.size());
    std::copy_if(colors.begin(), colors.end(), std::back_inserter(_colors),
                 [](const auto& color) { return color != nullptr; });
    return _colors;
  }

  constexpr const auto& toString() const { return colors[id]; }

  template <class T>
  constexpr bool operator==(const T& t) const {
    return id == t.id;
  }

  constexpr bool operator<(const Token& rhs) const {
    return id < rhs.toIndex();
  }

  constexpr bool operator>(const Token& rhs) const {
    return id > rhs.toIndex();
  }
  /**
   * @brief returns the unique index for this color.
   *
   * @return constexpr size_t
   */
  constexpr int toIndex() const { return id; }

  Token(const char* _id)
      : id([_id]() -> int {
          const auto it =
              std::find_if(colors.cbegin(), colors.cend(), [=](const char* s) {
                return s != nullptr && 0 == strcmp(_id, s);
              });
          if (it == colors.cend()) {
            const auto nullptr_it =
                std::find(colors.cbegin(), colors.cend(), nullptr);
            assert(nullptr_it != colors.cend() &&
                   "There can only be kMaxTokenColors different token-colors.");
            const auto idx = std::distance(colors.cbegin(), nullptr_it);
            colors[idx] = strdup(_id);
            return idx;
          } else {
            return std::distance(colors.cbegin(), it);
          }
        }()) {}

 protected:
  const static size_t kMaxTokenColors =
      100;  ///< Maximum amount of different colors
  constexpr Token(const char* id, const int idx) : id(idx) { colors[idx] = id; }
  inline static std::array<const char*, kMaxTokenColors> colors = {nullptr};
  int id;
};

}  // namespace symmetri

// Custom specialization of std::hash can be injected in namespace std.
template <>
struct std::hash<symmetri::Token> {
  constexpr std::size_t operator()(const symmetri::Token& s) const noexcept {
    return s.toIndex();
  }
};

/**
 * @brief A macro from which we can create token-colors. Colors ceated this way
 * end up in the symmetri namespace.
 *
 */
#define CREATE_CUSTOM_TOKEN(name)                                       \
  namespace symmetri {                                                  \
  struct name : public Token {                                          \
    constexpr name()                                                    \
        : Token(sym_impl::type_name<name>().data(),                     \
                sym_impl::unique_id<name>()) {                          \
      static_assert(                                                    \
          sym_impl::unique_id<name>() < kMaxTokenColors,                \
          "There can only be kMaxTokenColors different token-colors."); \
    }                                                                   \
  };                                                                    \
  static inline name name;                                              \
  }

CREATE_CUSTOM_TOKEN(Scheduled)
CREATE_CUSTOM_TOKEN(Started)
CREATE_CUSTOM_TOKEN(Success)
CREATE_CUSTOM_TOKEN(Deadlocked)
CREATE_CUSTOM_TOKEN(Canceled)
CREATE_CUSTOM_TOKEN(Paused)
CREATE_CUSTOM_TOKEN(Failed)
