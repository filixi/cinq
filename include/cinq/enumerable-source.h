#pragma once

#include <functional>
#include <iterator>
#include <type_traits>

#include "detail/utility.h"

namespace cinq::detail {
template <bool ConstVersion, class TSource>
struct EnumerableSource;

template <class T>
struct is_enumerable_source : std::false_type {};
template <bool ConstVersion, class T>
struct is_enumerable_source<EnumerableSource<ConstVersion, T>> : std::true_type {};
template <class T>
inline constexpr bool is_enumerable_source_v = is_enumerable_source<T>::value;

// TSource can be reference
template <bool ConstVersion, class TSource>
struct EnumerableSource {
public:
  static_assert(!is_enumerable_source_v<std::decay_t<TSource>>);
  static_assert(!std::is_rvalue_reference_v<TSource>);

  using ConstResultIterator = decltype(std::cbegin(std::declval<cinq::utility::remove_smart_ptr_t<TSource> &>()));
  using ResultIterator = std::conditional_t<ConstVersion,
      ConstResultIterator,
      decltype(std::begin(std::declval<cinq::utility::remove_smart_ptr_t<TSource> &>()))
    >;

  template <class... TS>
  EnumerableSource(TS&&... source) : source_{std::forward<TS>(source)...} {}

  ResultIterator begin() {
    if constexpr (ConstVersion) {
      return std::cbegin(*this);
    } else {
      if constexpr (cinq::utility::is_smart_ptr_v<TSource>)
        return std::begin(*source_);
      else
        return std::begin(source_);
    }
  }

  ResultIterator end() {
    if constexpr (ConstVersion) {
      return std::cend(*this);
    } else {
      if constexpr (cinq::utility::is_smart_ptr_v<TSource>)
        return std::end(*source_);
      else
        return std::end(source_);
    }
  }

  ConstResultIterator begin() const {
    if constexpr (cinq::utility::is_smart_ptr_v<TSource>)
      return std::cbegin(*source_);
    else
      return std::cbegin(source_);
  }

  ConstResultIterator end() const {
    if constexpr (cinq::utility::is_smart_ptr_v<TSource>)
      return std::cend(*source_);
    else
      return std::cend(source_);
  }

  ConstResultIterator cbegin() const {
    return begin();
  }

  ConstResultIterator cend() const {
    return end();
  }

  friend decltype(auto) MoveSource(EnumerableSource &&source) {
    if constexpr (std::is_lvalue_reference_v<TSource>) {
      return std::ref(source.source_);
    } else {
      return std::move(source.source_);
    }
  }

private:
  TSource source_;
};

} // namespace cinq::detail
