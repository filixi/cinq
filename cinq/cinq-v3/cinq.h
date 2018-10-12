#pragma once

#ifndef CINQV3
#define CINQV3
#endif // !CINQV3

#if defined(CINQV1) || defined(CINQV2) || defined(CINQV4) || defined(CINQV5)
#error Different versions of cinq cannot coexiste
#endif

#include <array>
#include <vector>

#include "enumerable.h"
#include "enumerable-source.h"

namespace cinq_v3 {
namespace detail {
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
    using SelectType = Enumerable<ConstVersion, EnumerableCategory::Producer, OperatorType::Select, Fn, TEnumerable>;
    return Cinq<ConstVersion, SelectType>(std::move(fn), std::move(root_));
  }

  template <class Fn>
  auto SelectMany(Fn fn) && {
    using SelectManyType = Enumerable<ConstVersion, EnumerableCategory::Producer, OperatorType::SelectMany, Fn, TEnumerable>;
    return Cinq<ConstVersion, SelectManyType>(std::move(fn), std::move(root_));
  }

  template <class Fn>
  auto Where(Fn fn) && {
    using WhereType = Enumerable<ConstVersion, EnumerableCategory::Subrange, OperatorType::Where, Fn, TEnumerable>;
    return Cinq<ConstVersion, WhereType>(std::move(fn), std::move(root_));
  }

  template <class Inner, class OuterKeySelector, class InnerKeySelector, class ResultSelector>
  auto Join(Inner &&inner, OuterKeySelector outer_key_selector, InnerKeySelector inner_key_selector, ResultSelector result_selector) && {
    using OuterSelectType = Enumerable<ConstVersion, EnumerableCategory::Producer, OperatorType::Select, OuterKeySelector, TEnumerable>;
    
    auto inner_select = ImplDetail::CinqImpl<ConstVersion>(std::forward<Inner>(inner)).Select(std::move(inner_key_selector));

    using JoinType = Enumerable<ConstVersion, EnumerableCategory::Producer, OperatorType::Join, ResultSelector, OuterSelectType, decltype(inner_select)>;
    return Cinq<ConstVersion, JoinType>(
        std::move(result_selector),
        OuterSelectType(std::move(outer_key_selector), std::move(root_)),
        std::move(inner_select)
      );
  }

  template <class Source, class... Rest>
  auto Intersect(Source &&source, Rest&&... rest) && {
    auto self = ImplDetail::CinqImpl<ConstVersion>(std::move(root_)).Distinct();

    using IntersectType = Enumerable<ConstVersion, EnumerableCategory::SetOperation, OperatorType::Intersect, bool,
      decltype(self),
      std::remove_reference_t<decltype(ImplDetail::CinqImpl<ConstVersion>(std::forward<Source>(source)).Distinct())>,
      std::remove_reference_t<decltype(ImplDetail::CinqImpl<ConstVersion>(std::forward<Rest>(rest)).Distinct())>...>;

    return Cinq<ConstVersion, IntersectType>(
        false,
        GetEnumerable(std::move(self)),
        GetEnumerable(ImplDetail::CinqImpl<ConstVersion>(std::forward<Source>(source)).Distinct()),
        GetEnumerable(ImplDetail::CinqImpl<ConstVersion>(std::forward<Rest>(rest)).Distinct())...
      );
  }

  template <class Source, class... Rest>
  auto Union(Source &&source, Rest&&... rest) && {
    auto self = ImplDetail::CinqImpl<ConstVersion>(std::move(root_)).Distinct();

    using UnionType = Enumerable<ConstVersion, EnumerableCategory::SetOperation, OperatorType::Union, bool,
      decltype(self),
      std::remove_reference_t<decltype(ImplDetail::CinqImpl<ConstVersion>(std::forward<Source>(source)).Distinct())>,
      std::remove_reference_t<decltype(ImplDetail::CinqImpl<ConstVersion>(std::forward<Rest>(rest)).Distinct())>...>;

    return Cinq<ConstVersion, UnionType>(
        false,
        GetEnumerable(std::move(self)),
        GetEnumerable(ImplDetail::CinqImpl<ConstVersion>(std::forward<Source>(source)).Distinct()),
        GetEnumerable(ImplDetail::CinqImpl<ConstVersion>(std::forward<Rest>(rest)).Distinct())...
      );
  }

  template <class Source, class... Rest>
  auto Concat(Source &&sources, Rest&&... rest) && {
    using ConcatType = Enumerable<ConstVersion, EnumerableCategory::SetOperation, OperatorType::Concat, bool, TEnumerable, 
      std::remove_reference_t<decltype(ImplDetail::CinqImpl<ConstVersion>(std::forward<Source>(sources)))>,
      std::remove_reference_t<decltype(ImplDetail::CinqImpl<ConstVersion>(std::forward<Rest>(rest)))>...>;
    return Cinq<ConstVersion, ConcatType>(
        false,
        std::move(root_),
        GetEnumerable(ImplDetail::CinqImpl<ConstVersion>(std::forward<Source>(sources))),
        GetEnumerable(ImplDetail::CinqImpl<ConstVersion>(std::forward<Rest>(rest)))...
      );
  }

  auto Distinct() && {
    // TODO : use ReferenceWrapper when possible
    using ResultType = decltype(*std::declval<ResultIterator>());
    using value_type = std::decay_t<ResultType>;
    class DistinctHelper {
      using InternalStorageType = std::conditional_t<std::is_reference_v<ResultType>,
        cinq::utility::ReferenceWrapper<value_type>,
        value_type>;

    public:
      bool operator()(const InternalStorageType &t) const { return distinct_set_.insert(t).second; };

    private:
      using SetType = std::conditional_t<cinq::utility::ReferenceWrapper<value_type>::hash_version,
        std::unordered_set<InternalStorageType>,std::set<InternalStorageType>>;

      mutable SetType distinct_set_;
    };

    using WhereType = Enumerable<ConstVersion, EnumerableCategory::Subrange, OperatorType::Where, DistinctHelper, TEnumerable>;
    return Cinq<ConstVersion, WhereType>(DistinctHelper(), std::move(root_));
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

} // namespace detail

namespace ImplDetail {
template <bool ConstVersion, class TEnumerable>
decltype(auto) CinqImpl(TEnumerable &&container) {
  if constexpr (detail::is_cinq_v<std::decay_t<TEnumerable>>)
    return std::forward<TEnumerable &&>(container);
  else
    return detail::Cinq<ConstVersion, detail::EnumerableSource<ConstVersion, std::decay_t<TEnumerable>>>(std::forward<TEnumerable>(container));
}

template <bool ConstVersion, class T, size_t size>
auto CinqImpl(T (&container)[size]) {
  return detail::Cinq<ConstVersion, detail::EnumerableSource<ConstVersion, std::array<T, size>>>(container, std::make_index_sequence<size>());
}

template <bool ConstVersion, class T, size_t size>
auto CinqImpl(const T (&container)[size]) {
  return detail::Cinq<ConstVersion, detail::EnumerableSource<ConstVersion, std::array<T, size>>>(container, std::make_index_sequence<size>());
}

template <bool ConstVersion, class TEnumerable>
auto CinqImpl(std::reference_wrapper<TEnumerable> container) {
  if constexpr (std::is_const_v<TEnumerable>)
    return detail::Cinq<ConstVersion, detail::EnumerableSource<true, TEnumerable &>>(container.get());
  else
    return detail::Cinq<ConstVersion, detail::EnumerableSource<ConstVersion, TEnumerable &>>(container.get());
}

template <bool ConstVersion, bool SourceConstVersion, class T>
auto CinqImpl(detail::EnumerableSource<SourceConstVersion, T> &&source) {
  return CinqImpl<ConstVersion>(MoveSource(std::move(source)));
}

} // namespace ImplDetail

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
  return ImplDetail::CinqImpl<false>(std::forward<TEnumerable>(container));
}

template <class T, size_t size>
auto Cinq(T (&container)[size]) {
  return ImplDetail::CinqImpl<false>(container);
}

template <class T, size_t size>
auto Cinq(const T (&container)[size]) {
  return ImplDetail::CinqImpl<false>(container);
}

template <class T>
auto Cinq() {
  return ImplDetail::CinqImpl<false>(std::vector<std::decay_t<T>>{});
}

} // namespace cinq_v3
