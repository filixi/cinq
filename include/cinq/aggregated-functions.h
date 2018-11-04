#pragma once

#include <tuple>
#include <type_traits>

namespace cinq::detail {
template <class... TFns>
struct AggregatedFunctions {
  AggregatedFunctions(std::tuple<TFns...> &&fns) : fns_(std::move(fns)) {}

  template <class Fn>
  auto PushFn(Fn &&fn) && {
    return AggregatedFunctions<TFns..., std::decay_t<Fn>>(std::tuple_cat(fns_, std::make_tuple(std::forward<Fn>(fn))));
  }

  template <size_t index, class T>
  decltype(auto) InvokeImpl(T &&v) {
    if constexpr (index < sizeof...(TFns)-1)
      return InvokeImpl<index + 1>(std::invoke(std::get<index>(fns_), std::forward<T>(v)));
    else
      return std::invoke(std::get<index>(fns_), std::forward<T>(v));
  }

  template <class T>
  decltype(auto) operator()(T &&v) {
    return Invoke(std::forward<T>(v));
  }

  template <class T>
  decltype(auto) Invoke(T &&v) {
    return InvokeImpl<0>(std::forward<T>(v));
  }

  std::tuple<TFns...> fns_;
};

template <>
struct AggregatedFunctions<> {
  AggregatedFunctions() = default;

  template <class Fn>
  auto PushFn(Fn &&fn) && {
    return AggregatedFunctions<std::decay_t<Fn>>(std::make_tuple(std::forward<Fn>(fn)));
  }

  template <size_t index, class T>
  decltype(auto) InvokeImpl(T &&v) {
    return std::forward<T>(v);
  }

  template <class T>
  decltype(auto) operator()(T &&v) {
    return Invoke(std::forward<T>(v));
  }

  template <class T>
  decltype(auto) Invoke(T &&v) {
    return InvokeImpl<0>(std::forward<T>(v));
  }
};

template <class... TFn>
auto CreateAggregatedFunctions(TFn&&... fn) {
  return AggregatedFunctions<std::decay_t<TFn>...>(std::make_tuple(std::forward<TFn>(fn)...));
}

auto CreateAggregatedFunctions() {
  return AggregatedFunctions<>();
}

} // namespace cinq::detail
