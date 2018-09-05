#pragma once

#ifndef CINQV3
#define CINQV3
#endif // !CINQV3

#if defined(CINQV1) || defined(CINQV2) || defined(CINQV4) || defined(CINQV5)
#error Different versions of cinq cannot coexiste
#endif

#include <array>

#include "enumerable.h"
#include "enumerable-source.h"

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
    using SelectType = Enumerable<EnumerableCategory::Producer, OperatorType::Select, Fn, TEnumerable>;
    return Cinq<SelectType>(std::move(fn), std::move(root_));
  }

  template <class Fn>
  auto SelectMany(Fn fn) && {
    using SelectManyType = Enumerable<EnumerableCategory::Producer, OperatorType::SelectMany, Fn, TEnumerable>;
    return Cinq<SelectManyType>(std::move(fn), std::move(root_));
  }

  template <class Fn>
  auto Where(Fn fn) && {
    using WhereType = Enumerable<EnumerableCategory::Subrange, OperatorType::Where, Fn, TEnumerable>;
    return Cinq<WhereType>(std::move(fn), std::move(root_));
  }

  template <class Inner, class OuterKeySelector, class InnerKeySelector, class ResultSelector>
  auto Join(Inner &&inner, OuterKeySelector outer_key_selector, InnerKeySelector inner_key_selector, ResultSelector result_selector) && {
    using OuterSelectType = Enumerable<EnumerableCategory::Producer, OperatorType::Select, OuterKeySelector, TEnumerable>;
    auto inner_select = cinq_v3::Cinq(std::forward<Inner>(inner)).Select(std::move(inner_key_selector));

    using JoinType = Enumerable<EnumerableCategory::Producer, OperatorType::Join, ResultSelector, OuterSelectType, decltype(inner_select)>;
    return Cinq<JoinType>(
        std::move(result_selector),
        OuterSelectType(std::move(outer_key_selector), std::move(root_)),
        std::move(inner_select)
      );
  }

  template <class Source, class... Rest>
  auto Intersect(Source &&source, Rest&&... rest) && {
    auto self = cinq_v3::Cinq(std::move(root_)).Distinct();

    using IntersectType = Enumerable<EnumerableCategory::SetOperation, OperatorType::Intersect, bool,
      decltype(self),
      decltype(cinq_v3::Cinq(std::forward<Source>(source)).Distinct()),
      decltype(cinq_v3::Cinq(std::forward<Rest>(rest)).Distinct())...>;

    return Cinq<IntersectType>(
        false,
        GetEnumerable(std::move(self)),
        GetEnumerable(cinq_v3::Cinq(std::forward<Source>(source)).Distinct()),
        GetEnumerable(cinq_v3::Cinq(std::forward<Rest>(rest)).Distinct())...
      );
  }

  template <class Source, class... Rest>
  auto Union(Source &&source, Rest&&... rest) && {
    auto self = cinq_v3::Cinq(std::move(root_)).Distinct();

    using UnionType = Enumerable<EnumerableCategory::SetOperation, OperatorType::Union, bool,
      decltype(self),
      decltype(cinq_v3::Cinq(std::forward<Source>(source)).Distinct()),
      decltype(cinq_v3::Cinq(std::forward<Rest>(rest)).Distinct())...>;

    return Cinq<UnionType>(
        false,
        GetEnumerable(std::move(self)),
        GetEnumerable(cinq_v3::Cinq(std::forward<Source>(source)).Distinct()),
        GetEnumerable(cinq_v3::Cinq(std::forward<Rest>(rest)).Distinct())...
      );
  }

  template <class Source, class... Rest>
  auto Concat(Source &&sources, Rest&&... rest) && {
    using ConcatType = Enumerable<EnumerableCategory::SetOperation, OperatorType::Concat, bool, TEnumerable, 
      decltype(cinq_v3::Cinq(std::forward<Source>(sources))), decltype(cinq_v3::Cinq(std::forward<Rest>(rest)))...>;
    return Cinq<ConcatType>(
        false,
        std::move(root_),
        GetEnumerable(cinq_v3::Cinq(std::forward<Source>(sources))),
        GetEnumerable(cinq_v3::Cinq(std::forward<Rest>(rest)))...
      );
  }

  auto Distinct() && {
    using value_type = std::decay_t<decltype(*std::declval<ResultIterator>())>;
    class DistinctHelper {
    public:
      bool operator()(const value_type &t) const { return distinct_set_.insert(t).second; };
    private:
      mutable std::set<value_type> distinct_set_;
    };

    using WhereType = Enumerable<EnumerableCategory::Subrange, OperatorType::Where, DistinctHelper, TEnumerable>;
    return Cinq<WhereType>(DistinctHelper(), std::move(root_));
  }
 
  auto ToVector() {
    using value_type = std::decay_t<typename std::decay_t<decltype(std::cbegin(*this))>::value_type>;
    auto vtr = std::vector<value_type>(std::cbegin(*this), std::cend(*this));
    return vtr;
  }

private:
  friend TEnumerable &&GetEnumerable(Cinq &&c) {
    return std::move(c.root_);
  }

  TEnumerable root_;
};

} // namespace detail

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
  if constexpr (detail::is_cinq_v<std::decay_t<TEnumerable>>)
    return std::forward<TEnumerable &&>(container);
  else
    return detail::Cinq<detail::EnumerableSource<std::decay_t<TEnumerable>>>(std::forward<TEnumerable>(container));
}

template <class T, size_t size>
auto Cinq(T (&container)[size]) {
  return detail::Cinq<detail::EnumerableSource<std::array<T, size>>>(container, std::make_index_sequence<size>());
}

template <class T, size_t size>
auto Cinq(const T (&container)[size]) {
  return detail::Cinq<detail::EnumerableSource<std::array<T, size>>>(container, std::make_index_sequence<size>());
}

template <class TEnumerable>
auto Cinq(std::reference_wrapper<TEnumerable> container) {
  return detail::Cinq<detail::EnumerableSource<TEnumerable &>>(container.get());
}

template <class T>
auto Cinq(detail::EnumerableSource<T> &&source) {
  return Cinq(MoveSource(std::move(source)));
}

} // namespace cinq_v3
