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
#include <numeric>
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
    return std::begin(root_);
  }

  auto end() const {
    return std::end(root_);
  }

  auto cbegin() const {
    return root_.cbegin();
  }

  auto cend() const {
    return root_.cend();
  }

  template <class Fn>
  auto Select(Fn fn) && {
    using SelectType = Enumerable<ConstVersion, QueryCategory::Select, std::tuple<Fn>, TEnumerable>;
    return Cinq<ConstVersion, SelectType>(std::make_tuple(std::move(fn)), std::move(root_));
  }

  template <class Fn>
  auto SelectMany(Fn fn) && {
    using SelectManyType = Enumerable<ConstVersion, QueryCategory::SelectMany, std::tuple<Fn>, TEnumerable>;
    return Cinq<ConstVersion, SelectManyType>(std::make_tuple(std::move(fn)), std::move(root_));
  }

  template <class Fn>
  auto Where(Fn fn) && {
    using WhereType = Enumerable<ConstVersion, QueryCategory::Where, std::tuple<Fn>, TEnumerable>;
    return Cinq<ConstVersion, WhereType>(std::make_tuple(std::move(fn)), std::move(root_));
  }

  template <class Inner, class OuterKeySelector, class InnerKeySelector, class ResultSelector>
  auto Join(Inner &&inner, OuterKeySelector outer_key_selector, InnerKeySelector inner_key_selector, ResultSelector result_selector) && {
    auto self = std::move(*this);

    using JoinType = Enumerable<ConstVersion, QueryCategory::Join,
      std::tuple<OuterKeySelector, InnerKeySelector, ResultSelector>,
      decltype(self),
      std::remove_reference_t<
        decltype(
          CinqImpl<ConstVersion>(
            std::forward<Inner>(inner)
          )
        )>
    >;

    return Cinq<ConstVersion, JoinType>(
        std::make_tuple(std::move(outer_key_selector), std::move(inner_key_selector), std::move(result_selector)),
        GetEnumerable(std::move(self)),
        GetEnumerable(CinqImpl<ConstVersion>(std::forward<Inner>(inner)))
      );
  }

  template <class Source, class... Rest>
  auto Intersect(Source &&source, Rest&&... rest) && {
    auto self = std::move(*this).Distinct();

    using IntersectType = Enumerable<ConstVersion, QueryCategory::Intersect, std::tuple<int>,
      decltype(self),
      std::remove_reference_t<decltype(CinqImpl<ConstVersion>(std::forward<Source>(source)).Distinct())>,
      std::remove_reference_t<decltype(CinqImpl<ConstVersion>(std::forward<Rest>(rest)).Distinct())>...>;

    return Cinq<ConstVersion, IntersectType>(
        NoFunctionTag{},
        GetEnumerable(std::move(self)),
        GetEnumerable(CinqImpl<ConstVersion>(std::forward<Source>(source)).Distinct()),
        GetEnumerable(CinqImpl<ConstVersion>(std::forward<Rest>(rest)).Distinct())...
      );
  }

  template <class Source, class... Rest>
  auto Union(Source &&source, Rest&&... rest) && {
    auto self = std::move(*this).Distinct();

    using UnionType = Enumerable<ConstVersion, QueryCategory::Union, std::tuple<int>,
      decltype(self),
      std::remove_reference_t<decltype(CinqImpl<ConstVersion>(std::forward<Source>(source)).Distinct())>,
      std::remove_reference_t<decltype(CinqImpl<ConstVersion>(std::forward<Rest>(rest)).Distinct())>...>;

    return Cinq<ConstVersion, UnionType>(
        NoFunctionTag{},
        GetEnumerable(std::move(self)),
        GetEnumerable(CinqImpl<ConstVersion>(std::forward<Source>(source)).Distinct()),
        GetEnumerable(CinqImpl<ConstVersion>(std::forward<Rest>(rest)).Distinct())...
      );
  }

  template <class Source, class... Rest>
  auto Concat(Source &&sources, Rest&&... rest) && {
    using ConcatType = Enumerable<ConstVersion, QueryCategory::Concat, std::tuple<int>, TEnumerable, 
      std::remove_reference_t<decltype(GetEnumerable(CinqImpl<ConstVersion>(std::forward<Source>(sources))))>,
      std::remove_reference_t<decltype(GetEnumerable(CinqImpl<ConstVersion>(std::forward<Rest>(rest))))>...>;
    return Cinq<ConstVersion, ConcatType>(
        NoFunctionTag{},
        std::move(root_),
        GetEnumerable(CinqImpl<ConstVersion>(std::forward<Source>(sources))),
        GetEnumerable(CinqImpl<ConstVersion>(std::forward<Rest>(rest)))...
      );
  }

  auto Distinct() && {
    using DistinctType = Enumerable<ConstVersion, QueryCategory::Distinct, std::tuple<int>, TEnumerable>;
    return Cinq<ConstVersion, DistinctType>(NoFunctionTag{}, std::move(root_));
  }

  template <class Fn>
  auto Aggregate(Fn &&fn) const {
    auto first = begin();
    auto last = end();
    if (first == last)
      return std::decay_t<decltype(*first)>{};

    auto front = *first;
    ++first;
    return std::accumulate(first, last, std::move(front), std::forward<Fn>(fn));
  }

  template <class TInit, class Fn>
  auto Aggregate(TInit &&init, Fn &&fn) const {
    return std::accumulate(begin(), end(), std::forward<TInit>(init), std::forward<Fn>(fn));
  }

  template <class TInit, class Fn, class ResultSelector>
  auto Aggregate(TInit &&init, Fn &&fn, ResultSelector &&selector) const {
    return std::invoke(std::forward<ResultSelector>(selector),
      std::accumulate(begin(), end(), std::forward<TInit>(init), std::forward<Fn>(fn)));
  }

  bool Empty() const {
    return begin() == end();
  }

  bool Any() const {
    return Empty();
  }

  template <class Pred>
  auto Any(Pred &&pred) const {
    return std::any_of(begin(), end(), std::forward<Pred>(pred));
  }

  auto Average() const {
    auto first = begin();
    auto last = end();
    if (first == last)
      return decltype(std::declval<std::decay_t<decltype(*first)>>() / size_t{}){};

    size_t size = 1;
    auto front = *first;
    ++first;
    for (; first != last; ++first)
      front += *first, ++size;
    return front / size;
  }

  template <class Fn>
  auto Average(Fn &&fn) const {
    auto first = begin();
    auto last = end();
    if (first == last)
      return decltype(std::declval<std::decay_t<decltype(*first)>>() / size_t{}){};

    size_t size = 1;
    auto front = std::invoke(std::forward<Fn>(fn), *first);
    ++first;
    for (; first != last; ++first)
      front += std::invoke(std::forward<Fn>(fn), *first), ++size;
    return front / size;
  }

  template <class Result>
  auto StaticCast() && {
    return std::move(*this).Select([](auto &&e) { return static_cast<Result>(std::forward<decltype(e)>(e)); });
  }

  template <class Result>
  auto DynamicCast() && {
    return std::move(*this).Select([](auto &&e) { return dynamic_cast<Result>(std::forward<decltype(e)>(e)); });
  }

  template <class Result>
  auto ConstCast() && {
    return std::move(*this).Select([](auto &&e) { return const_cast<Result>(std::forward<decltype(e)>(e)); });
  }

  template <class Result>
  auto ReinterpretCast() && {
    return std::move(*this).Select([](auto &&e) { return reinterpret_cast<Result>(std::forward<decltype(e)>(e)); });
  }

  // TODO : Add test case
  template <class Pred>
  bool All(Pred &&p) const {
    static_assert(concept::PredicateCheck<Pred &&, decltype(*begin())>(), "Bad predicate");
    return std::all_of(begin(), end(), std::forward<Pred>(p));
  }

  template <class T>
  auto Append(T &&t) && {
    using element_type = typename ResultIterator::value_type;
    return std::move(*this).Concat(std::vector<element_type>{std::forward<T>(t)});
  }

  template <class T>
  bool Contain(const T &t) const {
    auto last = end();
    return std::find(begin(), last, t) != last;
  }

  size_t Count() const {
    return std::distance(begin(), end());
  }

  size_t Size() const {
    return Count();
  }

  size_t LongCount() const {
    return Count();
  }

  template <class Pred>
  size_t Count(Pred &&p) const {
    return std::count_if(begin(), end(), std::forward<Pred>(p));
  }

  template <class T>
  auto DefaultIfEmpty(T &&t) && {
    std::vector<std::decay_t<decltype(*begin())>> vtr;
    vtr.emplace_back(std::forward<T>(t)); // If compiler complains about conversion, consider using a proper cast at call site.
    using DefaultIfEmptyType = Enumerable<ConstVersion, QueryCategory::DefaultIfEmpty, std::tuple<int>, TEnumerable,
        std::remove_reference_t<decltype(GetEnumerable(CinqImpl<ConstVersion>(vtr)))>>;

    return Cinq<ConstVersion, DefaultIfEmptyType>(
        NoFunctionTag{},
        std::move(root_),
        GetEnumerable(CinqImpl<ConstVersion>(vtr))
      );
  }

  using ResultType = decltype(*std::declval<ResultIterator>());

  auto DefaultIfEmpty() && {
    return std::move(*this).DefaultIfEmpty(std::decay_t<ResultType>{});
  }

  decltype(auto) ElementAt(size_t pos) const {
    auto first = begin();
    auto last = end();
    while (first != last && pos > 0) {
      ++first;
      --pos;
    }

    if (first == last)
      throw std::invalid_argument("pos out of range");

    return *first;
  }

  decltype(auto) At(size_t pos) const {
    return ElementAt(pos);
  }

  template <class Source>
  auto Except(Source &&source) && {
    auto self = std::move(*this).Distinct();

    using ExceptType = Enumerable<ConstVersion, QueryCategory::Except, std::tuple<int>,
      decltype(self),
      std::remove_reference_t<decltype(CinqImpl<ConstVersion>(std::forward<Source>(source)))>>;

    return Cinq<ConstVersion, ExceptType>(
        NoFunctionTag{},
        GetEnumerable(std::move(self)),
        GetEnumerable(CinqImpl<ConstVersion>(std::forward<Source>(source)))
      );
  }

  decltype(auto) First() const {
    auto first = begin();
    if (first != end())
      throw std::runtime_error("Retireve first from empty query");
    return *first;
  }

  template <class Fn>
  decltype(auto) First(Fn &&fn) const {
    auto last = end();
    for (auto first = begin() ; first!=last; ++first) {
      if (std::invoke(std::forward<Fn>(fn), *first))
        return *first;
    }
    throw std::runtime_error("Retireve first from empty query");
  }

  template <class KeySelector>
  auto GroupBy(KeySelector &&key_selector) const {
    using KeyType = std::invoke_result_t<KeySelector &&, ResultType>;

    using value_type = std::decay_t<ResultType>;

    using InternalStorageType = std::conditional_t<std::is_reference_v<ResultType>,
      cinq::utility::ReferenceWrapper<value_type>, value_type>;

    using MapType = std::conditional_t< cinq::utility::ReferenceWrapper<value_type>::hash_version,
      std::unordered_multimap<KeyType, InternalStorageType>, std::multimap<KeyType, InternalStorageType>>;
  
    MapType result;
    for (auto &x : *this)
        result.emplace(std::invoke(std::forward<KeySelector>(key_selector), x), x);

    return result;
  }

  template <class KeySelector, class SourceSelector>
  auto GroupBy(KeySelector &&key_selector, SourceSelector &&source_selector) const {
    using SelectedType = std::invoke_result_t<SourceSelector &&, decltype(*std::declval<ResultIterator>())>;
    using KeyType = std::invoke_result_t<KeySelector &&, SelectedType>;

    using value_type = std::decay_t<SelectedType>;

    using InternalStorageType = std::conditional_t<std::is_reference_v<SelectedType>,
      cinq::utility::ReferenceWrapper<value_type>, value_type>;

    using MapType = std::conditional_t< cinq::utility::ReferenceWrapper<value_type>::hash_version,
      std::unordered_multimap<KeyType, InternalStorageType>, std::multimap<KeyType, InternalStorageType>>;
  
    MapType result;
    for (auto &x : *this)
        result.emplace(std::invoke(std::forward<KeySelector>(key_selector), x),
          std::invoke(std::forward<SourceSelector>(source_selector), x));

    return result;
  }

  ResultType Last() const {
    auto first = begin();
    auto last = end();

    if (first == last)
      throw std::runtime_error("Retireve last from an empty query");
    
    if constexpr (std::is_reference_v<ResultType>) {
      auto *p = &*first;

      while (++first != last)
        p = &*first;

      return *p;
    } else {
      ResultType x = *first;

      while (++first != last)
        x = *first;

      return x;
    }
  }

  decltype(auto) Back() const {
    return Last();
  }

  template <class Pred>
  ResultType Last(Pred pred) const {
    auto first = begin();
    auto last = end();

    if (first == last)
      throw std::runtime_error("Retireve last from an empty query");
    
    bool found = false;
    if constexpr (std::is_reference_v<ResultType>) {
      auto *p = &*first;

      while (++first != last) {
        if (pred(*first)) {
          p = &*first;
          found = true;
        }
      }

      if (!found)
        throw std::runtime_error("");

      return *p;
    } else {
      ResultType x = *first;

      while (++first != last) {
        if (pred(*first)) {
          x = *first;
          found = true;
        }
      }

      if (!found)
        throw std::runtime_error("");

      return x;
    }
  }

  decltype(auto) Max() const {
    auto first = begin();
    auto last = end();

    if (first == last)
      throw std::runtime_error("Max on empty query.");

    if constexpr (std::is_reference_v<ResultType>) {
      auto *max = &*first;

      ++first;
      while (first != last) {
        if (*max < *first)
          max = &*first;
        ++first;
      }

      return *max;

    } else {
      auto max = *first;

      ++first;
      while (first != last) {
        if (max < *first)
          max = *first;
        ++first;
      }

      return max;
    }
  }

  template <class ResultSelector>
  decltype(auto) Max(ResultSelector result_selector) const {
    auto first = begin();
    auto last = end();

    if (first == last)
      throw std::runtime_error("Max on empty query.");

    using SelectedType = std::invoke_result_t<ResultSelector, ResultType>;
    if (std::is_reference_v<SelectedType>) {
      auto *max = &std::invoke(result_selector, *first);

      ++first;
      while (first != last) {
        auto *curr = &std::invoke(result_selector, *first);
        if (*max < *curr)
          max = curr;
        ++first;
      }

      return *max;
    } else {
      auto max = std::invoke(result_selector, *first);

      ++first;
      while (first != last) {
        auto curr = std::invoke(result_selector, *first);
        if (max < curr)
          max = std::move(curr);
        ++first;
      }

      return max;
    }
  }

  decltype(auto) Min() const {
    auto first = begin();
    auto last = end();

    if (first == last)
      throw std::runtime_error("Min on empty query.");

    if constexpr (std::is_reference_v<ResultType>) {
      auto *min = &*first;

      ++first;
      while (first != last) {
        if (*first < *min)
          min = &*first;
        ++first;
      }

      return *min;

    } else {
      auto min = *first;

      ++first;
      while (first != last) {
        if (*first < min)
          min = *first;
        ++first;
      }

      return min;
    }
  }

  template <class ResultSelector>
  decltype(auto) Min(ResultSelector result_selector) const {
    auto first = begin();
    auto last = end();

    if (first == last)
      throw std::runtime_error("Min on empty query.");

    using SelectedType = std::invoke_result_t<ResultSelector, ResultType>;
    if (std::is_reference_v<SelectedType>) {
      auto *min = &std::invoke(result_selector, *first);

      ++first;
      while (first != last) {
        auto *curr = &std::invoke(result_selector, *first);
        if (*curr < *min)
          min = curr;
        ++first;
      }

      return *min;
    } else {
      auto min = std::invoke(result_selector, *first);

      ++first;
      while (first != last) {
        auto curr = std::invoke(result_selector, *first);
        if (curr < min)
          min = std::move(curr);
        ++first;
      }

      return min;
    }
  }

  template <class Selector>
  auto OrderBy(Selector selector) const {
    using SelectedType = std::invoke_result_t<Selector, ResultType>;
    using Wrapper = cinq::utility::KVWrapper<SelectedType, ResultType>;

    std::vector<Wrapper> paired_result;
    for (auto &e : *this)
      paired_result.emplace_back(Wrapper{std::invoke(selector, e), e});
    std::sort(paired_result.begin(), paired_result.end());

    std::vector<std::decay_t<ResultType>> sorted;
    sorted.reserve(paired_result.size());
    for (auto &e : paired_result) {
      if constexpr (std::is_reference_v<ResultType>)
        sorted.push_back(*e.value_);
      else
        sorted.push_back(e.value_);
    }

    return cinq::Cinq(sorted);
  }

  template <class Selector, class LessThan>
  auto OrderBy(Selector selector, LessThan less_than) const {
    using SelectedType = std::invoke_result_t<Selector, ResultType>;
    using Wrapper = cinq::utility::KVWrapper<SelectedType, ResultType>;

    std::vector<Wrapper> paired_result;
    for (auto &e : *this)
      paired_result.emplace_back(Wrapper{std::invoke(selector, e), e});
    std::sort(paired_result.begin(), paired_result.end(), less_than);

    std::vector<std::decay_t<ResultType>> sorted;
    sorted.reserve(paired_result.size());
    for (auto &e : paired_result) {
      if constexpr (std::is_reference_v<ResultType>)
        sorted.push_back(*e.value_);
      else
        sorted.push_back(e.value_);
    }

    return cinq::Cinq(sorted);
  }

  auto OrderBy() const {
    std::vector<std::decay_t<ResultType>> sorted(begin(), end());
    std::sort(sorted.begin(), sorted.end());
    return cinq::Cinq(sorted);
  }

  template <class T>
  auto Prepend(T &&t) && {
    using element_type = typename ResultIterator::value_type;
    return cinq::Cinq(std::vector<element_type>{std::forward<T>(t)}).Concat(std::move(*this));
  }

  auto Reverse() && {
    std::vector<std::decay_t<ResultType>> result(begin(), end());
    result.reverse();
    return cinq::Cinq(std::move(result));
  }

  auto ToVector() {
    using value_type = std::decay_t<typename std::decay_t<decltype(begin())>::value_type>;
    auto vtr = std::vector<value_type>(begin(), end());
    return vtr;
  }

  auto ToSet() {
    using value_type = std::decay_t<typename std::decay_t<decltype(begin())>::value_type>;
    auto set = std::set<value_type>(begin(), end());
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
