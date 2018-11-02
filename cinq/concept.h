#pragma once

#include <type_traits>
#include <iterator>

#include "../cinq/utility.h"

namespace cinq_concept {
template <class T>
constexpr bool ContainerConstructibleCheck() {
  return std::is_constructible_v<std::decay_t<T>, T>;
}

struct Begin {
  template <class T>
  auto operator()(T &&x) const -> std::decay_t<decltype(std::begin(std::forward<T>(x)))> * {
    return nullptr;
  }
};
struct End {
  template <class T>
  auto operator()(T &&x) const -> std::decay_t<decltype(std::end(std::forward<T>(x)))> * {
    return nullptr;
  }
};

template <class T>
constexpr bool ContainerIteratorCheck() {
  constexpr bool has_begin_end = std::is_invocable_v<Begin, T &> && std::is_invocable_v<End, T &>;
  if constexpr (has_begin_end)
    return std::is_same_v<std::invoke_result_t<Begin, T &>, std::invoke_result_t<End, T &>>;
  else
    return false;
}

template <class T>
constexpr bool IteratorCheck() {
  auto equality_compariable = [](auto x) -> decltype(x == x) { return true; };
  auto inequality_compariable = [](auto x) -> decltype(x != x) { return true; };
  auto dereferenceable = [](auto x) -> std::decay_t<decltype(*x)> * { return nullptr; };
  auto pre_increseable = [](auto x) -> std::decay_t<decltype(++x)> * { return nullptr; };

  using IteratorType = std::decay_t<T>;

  return std::is_default_constructible_v<IteratorType> &&
      std::is_copy_assignable_v<IteratorType> &&
      std::is_copy_constructible_v<IteratorType> &&
      std::is_destructible_v<IteratorType> &&
      std::is_invocable_v<decltype(equality_compariable), IteratorType> &&
      std::is_invocable_v<decltype(inequality_compariable), IteratorType> &&
      std::is_invocable_v<decltype(dereferenceable), IteratorType> &&
      std::is_invocable_v<decltype(pre_increseable), IteratorType>;
}

// can only be check in operator specialized iterator, where the constness of the argument is konwn.
template <class Pred, class... Args>
constexpr bool PredicateCheck() {
  constexpr bool is_invocable = std::is_invocable_v<Pred, Args...>;

  if constexpr (is_invocable)
    return std::is_convertible_v<std::invoke_result_t<Pred, Args...>, bool>;
  else
    return false;
}

// can only be check in operator specialized iterator, where the constness of the argument is konwn.
template <class Fn, class... Args>
constexpr bool SelectorCheck() {
  constexpr bool is_invocable = std::is_invocable_v<Fn, Args...>;

  if constexpr (is_invocable)
    return !std::is_same_v<std::invoke_result_t<Fn, Args...>, void>;
  else
    return false;
}

template <bool HardCheck, class T>
constexpr bool ContainerCheck() {
  if constexpr(ContainerConstructibleCheck<T>()) {
    if constexpr (ContainerIteratorCheck<T>()) {
      using IteratorType = decltype(std::begin(std::declval<T &>()));
      if constexpr (IteratorCheck<IteratorType>()) {
        return true;
      } else {
        static_assert(!HardCheck, "Bad cantainer's iterator");
      }
    } else {
      static_assert(!HardCheck, "Cannot get proper iterator from container");
    }
  } else {
    static_assert(!HardCheck, "Container not constructible");
  }

  return false;
}

// can only be check in operator specialized iterator, where the constness of the argument is konwn.
template <bool HardCheck, class Fn, class Arg>
constexpr bool SelectManySelectorCheck() {
  if constexpr (SelectorCheck<Fn, Arg>()) {
    using ResultType = std::invoke_result_t<Fn, Arg>;
    if constexpr (ContainerCheck<HardCheck, ResultType>()) {
      return true;
    } else {
      static_assert(!HardCheck, "Bad generated container");
    }
  } else {
    static_assert(!HardCheck, "Bad selector");
  }

  return false;
}

template <bool HardCheck, class T, class Fn>
constexpr bool SelectConceptCheck() {
  if constexpr (ContainerCheck<HardCheck, T>()) {
    return true;
  } else {
    static_assert(!HardCheck, "Bad container");
  }

  return false;
}

template <bool HardCheck, class T, class Fn>
constexpr bool SelectManyConceptCheck() {
  if constexpr (ContainerCheck<HardCheck, T>()) {
    return true;
  } else {
    static_assert(!HardCheck, "Bad container");
  }

  return false;
}

} // namespace cinq_concept
