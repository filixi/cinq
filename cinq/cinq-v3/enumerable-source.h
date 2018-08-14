#pragma once

#include <iterator>
#include <type_traits>

#include "../utility.h"

namespace cinq_v3 {

template <class TSource>
struct EnumerableSource;

template <class T>
struct is_enumerable_source : std::false_type {};
template <class T>
struct is_enumerable_source<EnumerableSource<T>> : std::true_type {};
template <class T>
constexpr bool is_enumerable_source_v = is_enumerable_source<T>::value;

template <class TSource>
struct EnumerableSource {
public:
  static_assert(!is_enumerable_source_v<std::decay_t<TSource>>);

  using ResultIterator = decltype(std::cbegin(std::declval<cinq::utility::remove_smart_ptr_t<TSource>>()));

  template <class... TS>
  EnumerableSource(TS&&... source) : source_{std::forward<TS>(source)...} {}

  ResultIterator begin() const {
    if constexpr (cinq::utility::is_smart_ptr_v<TSource>)
      return std::cbegin(*source_);
    else
      return std::cbegin(source_);
  }

  ResultIterator end() const {
    if constexpr (cinq::utility::is_smart_ptr_v<TSource>)
      return std::cend(*source_);
    else
      return std::cend(source_);
  }

private:
  TSource source_;
};

} // namespace cinq_v3
