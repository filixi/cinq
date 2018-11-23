#pragma once

#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

#include "../query-category.h"
#include "detail/concept.h"
#include "detail/utility.h"
#include "query-iterator-fwd.h"

namespace cinq::detail {
template <bool ArgConstness, bool RetConstness, class TInnerKeySelector, class TOuterKeySelector, class TResultSelector, class TSource, class TSource2>
class QueryIterator<ArgConstness, RetConstness, BasicEnumerable<QueryCategory::Join, std::tuple<TInnerKeySelector, TOuterKeySelector, TResultSelector>, TSource, TSource2>> {
public:
  using Enumerable = BasicEnumerable<QueryCategory::Join, std::tuple<TInnerKeySelector, TOuterKeySelector, TResultSelector>, TSource, TSource2>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;
  using SourceIterator2 = typename Enumerable::template SourceIterator<1>;

  using SourceIteratorYieldType = decltype(*std::declval<SourceIterator>());
  using SourceIteratorYieldType2 = decltype(*std::declval<SourceIterator2>());

  using FunctionObjectArgumentTupleElementType = cinq::utility::transform_to_function_object_argument_t<ArgConstness, SourceIteratorYieldType>;
  using FunctionObjectArgumentTupleElementType2 = cinq::utility::transform_to_function_object_argument_t<ArgConstness, SourceIteratorYieldType2>;

  using FunctionObjectArgumentTupleType = std::tuple<FunctionObjectArgumentTupleElementType, FunctionObjectArgumentTupleElementType2>;

  using FunctionObjectYieldType = std::invoke_result_t<TResultSelector, FunctionObjectArgumentTupleElementType, FunctionObjectArgumentTupleElementType2>;
  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, FunctionObjectYieldType, cinq::utility::SourceType::FunctionObject>;

  using value_type = std::decay_t<ResultType>;

  static_assert(concept::SelectorCheck<TResultSelector, FunctionObjectArgumentTupleElementType, FunctionObjectArgumentTupleElementType2>(), "Bad selector");

  QueryIterator() : first_(), last_(), first2_(), last2_() {}

  QueryIterator(Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable), is_past_the_end_iteratorator_(is_past_the_end_iteratorator) {
    if (first2_ != last2_)
      FindNextValid();
  }

  QueryIterator(const QueryIterator &) = default;
  QueryIterator(QueryIterator &&) = default;

  QueryIterator &operator=(const QueryIterator &) = default;
  QueryIterator &operator=(QueryIterator &&) = default;

  ResultType operator*() const {
    return enumerable_->template GetFn<2>()(
        static_cast<FunctionObjectArgumentTupleElementType>(*first_),
        static_cast<FunctionObjectArgumentTupleElementType2>(*first2_)
      );
  }

  friend bool operator!=(const QueryIterator &lhs, const QueryIterator &rhs) {
    if (rhs.is_past_the_end_iteratorator_)
      return lhs.first_ != rhs.first_ && lhs.first2_ != rhs.last2_;
    return lhs.first_ != rhs.first_ || lhs.first2_ != rhs.first2_;
  }

  friend bool operator==(const QueryIterator &lhs, const QueryIterator &rhs) {
    if (rhs.is_past_the_end_iteratorator_)
      return lhs.first_ == rhs.first_ && lhs.first2_ == rhs.last2_;
    return lhs.first_ == rhs.first_ && lhs.first2_ == rhs.first2_;
  }

  QueryIterator &operator++() {
    cinq::utility::CinqAssert(!is_past_the_end_iteratorator_);
    if (first2_ != last2_)
      ++first2_;
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
    if (first_ == last_)
      return ;

    for (;;) {
      for (;first2_ != last2_; ++first2_) {
        if (enumerable_->template GetFn<0>()(static_cast<FunctionObjectArgumentTupleElementType>(*first_)) ==
            enumerable_->template GetFn<1>()(static_cast<FunctionObjectArgumentTupleElementType2>(*first2_)))
          return ;
      }

      if (++first_ == last_)
        break;
      first2_ = SourceIterator2(std::begin(enumerable_->template GetSource<1>()));
    }
  }

  Enumerable *enumerable_ = nullptr;

  bool is_past_the_end_iteratorator_ = false;

  SourceIterator first_ = is_past_the_end_iteratorator_ ? std::end(enumerable_->template GetSource<0>()) : std::begin(enumerable_->template GetSource<0>());
  SourceIterator last_ = std::end(enumerable_->template GetSource<0>());

  SourceIterator2 first2_ = is_past_the_end_iteratorator_ ? std::end(enumerable_->template GetSource<1>()) : std::begin(enumerable_->template GetSource<1>());
  SourceIterator2 last2_ = std::end(enumerable_->template GetSource<1>());
};

} // namespace cinq::detail
