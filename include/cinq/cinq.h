#pragma once

#ifndef CINQV3
#define CINQV3
#endif // !CINQV3

#if defined(CINQV1) || defined(CINQV2) || defined(CINQV4) || defined(CINQV5)
#error Different versions of cinq cannot coexiste
#endif

#include <array>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

#include "detail/concept.h"
#include "enumerable.h"
#include "enumerable-source.h"

namespace cinq::detail {
template <bool ConstVersion, class TEnumerable>
decltype(auto) CinqImpl(TEnumerable &&container);

template <bool ConstVersion, class T, size_t size>
auto CinqImpl(T (&container)[size]);

template <bool ConstVersion, class T, size_t size>
auto CinqImpl(const T (&container)[size]);

template <bool ConstVersion, class TEnumerable>
auto CinqImpl(std::reference_wrapper<TEnumerable> container);

template <bool ConstVersion, bool SourceConstVersion, class T>
auto CinqImpl(EnumerableSource<SourceConstVersion, T> &&source);

template <bool ConstVersion, class TEnumerable>
class Cinq {
public:
  using ResultIterator = typename TEnumerable::ResultIterator;
  static_assert(!std::is_reference_v<TEnumerable>);

  template <class T, size_t... indexs>
  Cinq(T &container, std::index_sequence<indexs...>)
    : root_(TEnumerable{container[indexs]...}) {}

  template <class... Args>
  Cinq(Args&&... args) : root_(std::forward<Args>(args)...) {}

  Cinq(const Cinq &) = delete;
  Cinq(Cinq &&) = default;

  Cinq &operator=(const Cinq &) = delete;
  Cinq &operator=(Cinq &&) = delete;

  auto begin() {
    return std::begin(root_);
  }

  auto end() {
    return std::end(root_);
  }

  auto begin() const {
    return root_.cbegin();
  }

  auto end() const {
    return root_.cend();
  }

  auto cbegin() const {
    return begin();
  }

  auto cend() const {
    return end();
  }

  template <class Fn>
  auto Select(Fn fn) && {
    using SelectType = Enumerable<ConstVersion, OperatorType::Select, Fn, TEnumerable>;
    return Cinq<ConstVersion, SelectType>(std::move(fn), std::move(root_));
  }

  template <class Fn>
  auto SelectMany(Fn fn) && {
    using SelectManyType = Enumerable<ConstVersion, OperatorType::SelectMany, Fn, TEnumerable>;
    return Cinq<ConstVersion, SelectManyType>(std::move(fn), std::move(root_));
  }

  template <class Fn>
  auto Where(Fn fn) && {
    using WhereType = Enumerable<ConstVersion, OperatorType::Where, Fn, TEnumerable>;
    return Cinq<ConstVersion, WhereType>(std::move(fn), std::move(root_));
  }

  template <class Inner, class OuterKeySelector, class InnerKeySelector, class ResultSelector>
  auto Join(Inner &&inner, OuterKeySelector outer_key_selector, InnerKeySelector inner_key_selector, ResultSelector result_selector) && {
    using OuterSelectType = Enumerable<ConstVersion, OperatorType::Select, OuterKeySelector, TEnumerable>;
    
    auto inner_select = CinqImpl<ConstVersion>(std::forward<Inner>(inner)).Select(std::move(inner_key_selector));

    using JoinType = Enumerable<ConstVersion, OperatorType::Join, ResultSelector, OuterSelectType, decltype(inner_select)>;
    return Cinq<ConstVersion, JoinType>(
        std::move(result_selector),
        OuterSelectType(std::move(outer_key_selector), std::move(root_)),
        std::move(inner_select)
      );
  }

  template <class Source, class... Rest>
  auto Intersect(Source &&source, Rest&&... rest) && {
    auto self = CinqImpl<ConstVersion>(std::move(root_)).Distinct();

    using IntersectType = Enumerable<ConstVersion, OperatorType::Intersect, bool,
      decltype(self),
      std::remove_reference_t<decltype(CinqImpl<ConstVersion>(std::forward<Source>(source)).Distinct())>,
      std::remove_reference_t<decltype(CinqImpl<ConstVersion>(std::forward<Rest>(rest)).Distinct())>...>;

    return Cinq<ConstVersion, IntersectType>(
        false,
        GetEnumerable(std::move(self)),
        GetEnumerable(CinqImpl<ConstVersion>(std::forward<Source>(source)).Distinct()),
        GetEnumerable(CinqImpl<ConstVersion>(std::forward<Rest>(rest)).Distinct())...
      );
  }

  template <class Source, class... Rest>
  auto Union(Source &&source, Rest&&... rest) && {
    auto self = CinqImpl<ConstVersion>(std::move(root_)).Distinct();

    using UnionType = Enumerable<ConstVersion, OperatorType::Union, bool,
      decltype(self),
      std::remove_reference_t<decltype(CinqImpl<ConstVersion>(std::forward<Source>(source)).Distinct())>,
      std::remove_reference_t<decltype(CinqImpl<ConstVersion>(std::forward<Rest>(rest)).Distinct())>...>;

    return Cinq<ConstVersion, UnionType>(
        false,
        GetEnumerable(std::move(self)),
        GetEnumerable(CinqImpl<ConstVersion>(std::forward<Source>(source)).Distinct()),
        GetEnumerable(CinqImpl<ConstVersion>(std::forward<Rest>(rest)).Distinct())...
      );
  }

  template <class Source, class... Rest>
  auto Concat(Source &&sources, Rest&&... rest) && {
    using ConcatType = Enumerable<ConstVersion, OperatorType::Concat, bool, TEnumerable, 
      std::remove_reference_t<decltype(CinqImpl<ConstVersion>(std::forward<Source>(sources)))>,
      std::remove_reference_t<decltype(CinqImpl<ConstVersion>(std::forward<Rest>(rest)))>...>;
    return Cinq<ConstVersion, ConcatType>(
        false,
        std::move(root_),
        GetEnumerable(CinqImpl<ConstVersion>(std::forward<Source>(sources))),
        GetEnumerable(CinqImpl<ConstVersion>(std::forward<Rest>(rest)))...
      );
  }

  auto Distinct() && {
    // TODO : use ReferenceWrapper when possible
    using DistinctType = Enumerable<ConstVersion, OperatorType::Distinct, bool, TEnumerable>;
    return Cinq<ConstVersion, DistinctType>(true, std::move(root_));
  }
 
  auto ToVector() {
    using value_type = std::decay_t<typename std::decay_t<decltype(std::begin(*this))>::value_type>;
    auto vtr = std::vector<value_type>(std::begin(*this), std::end(*this));
    return vtr;
  }

  auto ToSet() {
    using value_type = std::decay_t<typename std::decay_t<decltype(std::begin(*this))>::value_type>;
    auto set = std::set<value_type>(std::begin(*this), std::end(*this));
    return set;
  }

  auto Const() && {
    return Cinq<true, TEnumerable>(std::move(root_));
  }

private:
  friend TEnumerable &&GetEnumerable(Cinq &&c) {
    return std::move(c.root_);
  }

  mutable TEnumerable root_;
};

template <bool ConstVersion, class TEnumerable>
decltype(auto) CinqImpl(TEnumerable &&container) {
  if constexpr (cinq::utility::is_smart_ptr_v<std::decay_t<TEnumerable>>) {
    static_assert(concept::ContainerCheck<true, typename std::decay_t<TEnumerable>::element_type>(), "Bad enumerable");
  } else {
    static_assert(concept::ContainerCheck<true, std::decay_t<TEnumerable>>(), "Bad enumerable");
  }

  if constexpr (is_cinq_v<std::decay_t<TEnumerable>>)
    return std::forward<TEnumerable &&>(container);
  else
    return Cinq<ConstVersion, EnumerableSource<ConstVersion, std::decay_t<TEnumerable>>>(std::forward<TEnumerable>(container));
}

template <bool ConstVersion, class T, size_t size>
auto CinqImpl(T (&container)[size]) {
  return Cinq<ConstVersion, EnumerableSource<ConstVersion, std::array<T, size>>>(container, std::make_index_sequence<size>());
}

template <bool ConstVersion, class T, size_t size>
auto CinqImpl(const T (&container)[size]) {
  return Cinq<ConstVersion, EnumerableSource<ConstVersion, std::array<T, size>>>(container, std::make_index_sequence<size>());
}

template <bool ConstVersion, class TEnumerable>
auto CinqImpl(std::reference_wrapper<TEnumerable> container) {
  if constexpr (cinq::utility::is_smart_ptr_v<std::decay_t<TEnumerable>>) {
    static_assert(concept::ContainerCheck<true, typename std::decay_t<TEnumerable>::element_type>(), "Bad enumerable");
  } else {
    static_assert(concept::ContainerCheck<true, TEnumerable &>(), "Bad enumerable");
  }

  if constexpr (std::is_const_v<TEnumerable>)
    return Cinq<ConstVersion, EnumerableSource<true, TEnumerable &>>(container.get());
  else
    return Cinq<ConstVersion, EnumerableSource<ConstVersion, TEnumerable &>>(container.get());
}

template <bool ConstVersion, bool SourceConstVersion, class T>
auto CinqImpl(EnumerableSource<SourceConstVersion, T> &&source) {
  return CinqImpl<ConstVersion>(MoveSource(std::move(source)));
}

} // namespace cinq::detail
