#pragma once

#ifndef CINQV3
#define CINQV3
#endif // !CINQV3

#if defined(CINQV1) || defined(CINQV2) || defined(CINQV4) || defined(CINQV5)
#error Different versions of cinq cannot coexiste
#endif

#include "../cinq-v3/aggregated-functions.h"
#include "../cinq-v3/enumerable.h"

namespace detail {
template <class... TEnumerables>
class Cinq {
public:
  template <class T>
  Cinq(T &&enumerables) : enumerable_(std::forward<T>(enumerables)) {}

  Cinq(const Cinq &) = delete;
  Cinq(Cinq &&) = default;

  Cinq &operator=(const Cinq &) = delete;
  Cinq &operator=(Cinq &&) = default;

  auto begin() {
  
  }

  auto end() {

  }

  template <class Fn>
  auto SelectMany(Fn fn) {
    const auto forward = [](auto &&x) -> decltype(auto) { return std::forward<decltype(x)>(x); };
    return Append(
      CreateEnumerable<EnumerableCategory::Producer, OperatorType::SelectMany>(
        std::ref(LastEnumerable()), std::move(fn), CreateAggregatedFunctions()));
  }

private:
  template <class TEnumerable>
  Cinq<TEnumerables..., TEnumerable> Append(TEnumerable &&enumerable) {
    return Cinq<TEnumerables..., TEnumerable>(
      std::tuple_cat(std::move(enumerables_), std::move(enumerable)));
  }

  decltype(auto) LastEnumerable() {
    return std::get<sizeof...(TEnumerables) - 1>(enumerables_);
  }

  std::tuple<TEnumerables...> enumerables_;
};

} // namespace detail

template <class TEnumerable>
auto Cinq(TEnumerable &&container) {
  return detail::Cinq<std::decay_t<TEnumerable>>(std::forward<TEnumerable>(container));
}

template <class TEnumerable>
auto Cinq(std::reference_wrapper<TEnumerable> container) {
  return detail::Cinq<TEnumerable &>(container.get());
}