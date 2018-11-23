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
template <bool ArgConstness, bool RetConstness, class TFn, class TSource>
class QueryIterator<ArgConstness, RetConstness, BasicEnumerable<QueryCategory::Where, std::tuple<TFn>, TSource>> {
public:
  using Enumerable = BasicEnumerable<QueryCategory::Where, std::tuple<TFn>, TSource>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;
  using SourceIteratorYieldType = decltype(*std::declval<SourceIterator>());

  using FunctionObjectArgumentType = cinq::utility::transform_to_function_object_argument_t<ArgConstness, SourceIteratorYieldType>;

  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, SourceIteratorYieldType, cinq::utility::SourceType::Iterator>;
  using value_type = std::decay_t<ResultType>;

  static_assert(concept::PredicateCheck<TFn, FunctionObjectArgumentType>(), "Bad predicate");

  QueryIterator() : first_(), last_() {}

  QueryIterator(Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable), is_past_the_end_iteratorator_(is_past_the_end_iteratorator) {
    FindNextValideElement();
  }

  QueryIterator(const QueryIterator &) = default;
  QueryIterator(QueryIterator &&) = default;

  QueryIterator &operator=(const QueryIterator &) = default;
  QueryIterator &operator=(QueryIterator &&) = default;

  ResultType operator*() const {
    return *first_;
  }

  friend bool operator!=(const QueryIterator &lhs, const QueryIterator &rhs) {
    return lhs.first_ != rhs.first_;
  }

  friend bool operator==(const QueryIterator &lhs, const QueryIterator &rhs) {
    return lhs.first_ == rhs.first_;
  }

  QueryIterator &operator++() {
    ++first_;
    FindNextValideElement();
    return *this;
  }

  QueryIterator operator++(int) {
    QueryIterator previous(*this);
    ++first_;
    FindNextValideElement();
    return previous;
  }

private:
  void FindNextValideElement() {
    while (first_ != last_ && !enumerable_->FirstFn()(static_cast<FunctionObjectArgumentType>(*first_)))
      ++first_;
  }

  Enumerable *enumerable_ = nullptr;

  bool is_past_the_end_iteratorator_ = false;

  SourceIterator first_ = is_past_the_end_iteratorator_ ? std::end(enumerable_->SourceFront()) : std::begin(enumerable_->SourceFront());
  SourceIterator last_ = std::end(enumerable_->SourceFront());
};

} // namespace cinq::detail
