#pragma once

#ifndef CINQV3
#define CINQV3
#endif // !CINQV3

#if defined(CINQV1) || defined(CINQV2) || defined(CINQV4) || defined(CINQV5)
#error Different versions of cinq cannot coexiste
#endif

#include "../cinq/cinq-v3/enumerable.h"
#include "../cinq/cinq-v3/enumerable-source.h"

#include <array>

namespace cinq_v3 {
namespace detail {
template <class TEnumerable>
class Cinq {
public:
  using ResultIterator = typename TEnumerable::ResultIterator;

  template <class T, size_t... indexs>
  Cinq(T &container, std::index_sequence<indexs...>)
    : root_(TEnumerable{container[indexs]...}) {}

  template <class... Args>
  Cinq(Args&&... args) : root_(std::forward<Args>(args)...) {}

  Cinq(const Cinq &) = delete;
  Cinq(Cinq &&) = default;

  Cinq &operator=(const Cinq &) = delete;
  Cinq &operator=(Cinq &&) = delete;

  auto begin() const {
    return std::cbegin(root_);
  }

  auto end() const {
    return std::cend(root_);
  }

  template <class Fn>
  auto Select(Fn fn) && {
    using SelectType = Enumerable<TEnumerable, Fn, EnumerableCategory::Producer, OperatorType::Select>;
    return Cinq<SelectType>(std::move(root_), std::move(fn));
  }

  template <class Fn>
  auto SelectMany(Fn fn) && {
    using SelectManyType = Enumerable<TEnumerable, Fn, EnumerableCategory::Producer, OperatorType::SelectMany>;
    return Cinq<SelectManyType>(std::move(root_), std::move(fn));
  }

  template <class Fn>
  auto Where(Fn fn) && {
    using WhereType = Enumerable<TEnumerable, Fn, EnumerableCategory::Subrange, OperatorType::Where>;
    return Cinq<WhereType>(std::move(root_), std::move(fn));
  }

  template <class Inner, class OuterKeySelector, class InnerKeySelector, class ResultSelector>
  auto Join(Inner &&inner, OuterKeySelector outer_key_selector, InnerKeySelector &&inner_key_selector, ResultSelector result_selector) && {
    using OuterSelectType = Enumerable<TEnumerable, OuterKeySelector, EnumerableCategory::Producer, OperatorType::Select>;
    auto inner_select = cinq_v3::Cinq(inner).Select(std::forward<InnerKeySelector>(inner_key_selector));

    using JoinType = Enumerable<OuterSelectType, ResultSelector, EnumerableCategory::Producer, OperatorType::Join, decltype(inner_select)>;
    return Cinq<JoinType>(
        OuterSelectType(std::move(root_), outer_key_selector),
        std::move(inner_select),
        std::move(result_selector)
      );
  }
 
  auto ToVector() && {
    using value_type = std::decay_t<typename std::decay_t<decltype(std::cbegin(*this))>::value_type>;
    return std::vector<value_type>(std::cbegin(*this), std::cend(*this));
  }

private:
  TEnumerable root_;
};

} // namespace detail


// User provided container:
// non reference_wrapper:
//  if array type: convert to std::array
//  else copy/move initialize
//
// reference_wrapper:
//  if array type: reference to array
//  else reference to the container.

template <class T>
struct is_cinq : std::false_type {};
template <class... TE>
struct is_cinq<detail::Cinq<TE...>> : std::true_type {};
template <class T>
constexpr bool is_cinq_v = is_cinq<T>::value;

template <class TEnumerable>
decltype(auto) Cinq(TEnumerable &&container) {
  if constexpr (is_cinq_v<std::decay_t<TEnumerable>>)
    return std::forward<TEnumerable &&>(container);
  else
    return detail::Cinq<EnumerableSource<std::decay_t<TEnumerable>>>(std::forward<TEnumerable>(container));
}

template <class T, size_t size>
auto Cinq(T (&container)[size]) {
  return detail::Cinq<EnumerableSource<std::array<T, size>>>(container, std::make_index_sequence<size>());
}

template <class T, size_t size>
auto Cinq(const T (&container)[size]) {
  return detail::Cinq<EnumerableSource<std::array<T, size>>>(container, std::make_index_sequence<size>());
}

template <class TEnumerable>
auto Cinq(std::reference_wrapper<TEnumerable> container) {
  return detail::Cinq<EnumerableSource<TEnumerable &>>(container.get());
}

} // namespace cinq_v3
