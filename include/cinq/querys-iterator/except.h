#pragma once

#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include <set>
#include <unordered_set>
#include <memory>

#include "../query-category.h"
#include "detail/concept.h"
#include "detail/utility.h"
#include "query-iterator-fwd.h"

namespace cinq::detail {
template <bool ArgConstness, bool RetConstness, class TInnerKeySelector, class TOuterKeySelector, class TResultSelector, class TSource, class TSource2>
class QueryIterator<ArgConstness, RetConstness, BasicEnumerable<QueryCategory::Except, std::tuple<TInnerKeySelector, TOuterKeySelector, TResultSelector>, TSource, TSource2>> {
public:
  using Enumerable = BasicEnumerable<QueryCategory::Except, std::tuple<TInnerKeySelector, TOuterKeySelector, TResultSelector>, TSource, TSource2>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;
  using SourceIterator2 = typename Enumerable::template SourceIterator<1>;

  using SourceIteratorYieldType = decltype(*std::declval<SourceIterator>());
  using SourceIteratorYieldType2 = decltype(*std::declval<SourceIterator2>());
  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, SourceIteratorYieldType, cinq::utility::SourceType::Iterator>;

  using value_type = std::decay_t<ResultType>;

  static_assert(concept::SelectorCheck<TResultSelector, FunctionObjectArgumentTupleElementType, FunctionObjectArgumentTupleElementType2>(), "Bad selector");

  QueryIterator() : first_(), last_() {}

  QueryIterator(Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable), is_past_the_end_iteratorator_(is_past_the_end_iteratorator) {
    if (!is_past_the_end_iteratorator_)
      FindNextValid();
  }

  QueryIterator(const QueryIterator &) = default;
  QueryIterator(QueryIterator &&) = default;

  QueryIterator &operator=(const QueryIterator &) = default;
  QueryIterator &operator=(QueryIterator &&) = default;

  ResultType operator*() const {
    return *first_;
  }

  friend bool operator!=(const QueryIterator &lhs, const QueryIterator &rhs) {
    if (rhs.is_past_the_end_iteratorator_)
      return lhs.first_ != rhs.first_;
    return lhs.first_ != rhs.first_;
  }

  friend bool operator==(const QueryIterator &lhs, const QueryIterator &rhs) {
    if (rhs.is_past_the_end_iteratorator_)
      return lhs.first_ == rhs.first_;
    return lhs.first_ == rhs.first_;
  }

  QueryIterator &operator++() {
    cinq::utility::CinqAssert(!is_past_the_end_iteratorator_);
    ++first_;
    FindNextValid();
    return *this;
  }

  QueryIterator operator++(int) {
    cinq::utility::CinqAssert(!is_past_the_end_iteratorator_);
    QueryIterator previous(*this);
    ++*this;
    return previous;
  }

private:
  void FindNextValid() {
    while (first_ != last_) {
      if (!values_.find(*first_))
        break;
      ++first_;
    } 
  }

  using InternalStorageType = std::conditional_t<
    std::is_reference_v<SourceIteratorYieldType2>,
    cinq::utility::ReferenceWrapper<SourceIteratorYieldType2>, SourceIteratorYieldType2>;

  using SetType = std::conditional_t<cinq::utility::ReferenceWrapper<SourceIteratorYieldType2>::hash_version,
    std::unordered_set<InternalStorageType>, std::set<InternalStorageType>>;

  std::shared_ptr<SetType> values_ = is_past_the_end_iteratorator_ ?
    nullptr :
    std::make_shared<SetType>(std::begin(enumerable_->template GetSource<1>()), std::end(enumerable_->template GetSource<1>()));

  Enumerable *enumerable_ = nullptr;

  bool is_past_the_end_iteratorator_ = false;

  SourceIterator first_ = is_past_the_end_iteratorator_ ? std::end(enumerable_->template GetSource<0>()) : std::begin(enumerable_->template GetSource<0>());
  SourceIterator last_ = std::end(enumerable_->template GetSource<0>());
};

} // namespace cinq::detail
