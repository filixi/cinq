#pragma once

#include <type_traits>
#include <utility>

#include "cinq/cinq.h"

namespace cinq {
// Following function/function template overload set is the front barrier to maintain inner type consistency from user provided types.
// Such consistency will greatly reduce both compile-time and run-time errors by simplifing the inner type design.
// All user provided container type must be wrapped by class template EnumerableSource.
// Apart from EnumerableSource, every enumerable type which requires sources holds the owner-ship of the sources (of which the life-time is bound with the owner)
//   and no reference type or reference wrapper is allowed.
//
// The return type of following overload set is always a specialization of class template Cinq,
//   where the template argument will be a type of user-provieded container type after adjustment as described in following:
// Given a Cinq call Cinq(expr),
// if expr is of type array or reference to array, it is adjusted to std::array and then get wrapped into EnumerableSource, otherwise
// if expr is of type reference_wrapper<T>, it is adjusted to T & and then get wrapped into EnumerableSource, i.e. EnumerableSource holds the same reference which is held by expr, otherwise
// if expr is of type Cinq<T>, then expr will be returned with perfect forwarding, otherwise
// it is adjusted by std::decay

template <class TEnumerable>
decltype(auto) Cinq(TEnumerable &&container) {
  return detail::CinqImpl<false>(std::forward<TEnumerable>(container));
}

template <class T, size_t size>
auto Cinq(T (&container)[size]) {
  return detail::CinqImpl<false>(container);
}

template <class T, size_t size>
auto Cinq(const T (&container)[size]) {
  return detail::CinqImpl<false>(container);
}

template <class T>
auto Cinq() {
  return detail::CinqImpl<false>(std::vector<std::decay_t<T>>{});
}

} // namespace detail
