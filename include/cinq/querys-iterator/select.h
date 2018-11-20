#pragma once

#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

#include "detail/concept.h"
#include "detail/utility.h"
#include "query-category.h"
#include "query-iterator-fwd.h"

namespace cinq::detail {
template <bool ArgConstness, bool RetConstness, class TFn, class TSource>
class QueryIterator<ArgConstness, RetConstness, BasicEnumerable<QueryCategory::Select, std::tuple<TFn>, TSource>> {
public:
  using Enumerable = BasicEnumerable<QueryCategory::Select, std::tuple<TFn>, TSource>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;

  using SourceIteratorYieldType = decltype(*std::declval<SourceIterator>());
  using FunctionObjectArgumentType = cinq::utility::transform_to_function_object_argument_t<ArgConstness, SourceIteratorYieldType>;

  using FunctionObjectYieldType = std::invoke_result_t<TFn, FunctionObjectArgumentType>;
  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, FunctionObjectYieldType, cinq::utility::SourceType::FunctionObject>;
  using value_type = std::decay_t<ResultType>;

  static_assert(concept::SelectorCheck<TFn, FunctionObjectArgumentType>(), "Bad selector");

  QueryIterator() {}

  QueryIterator(Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable),
      iterator_(is_past_the_end_iteratorator ? std::end(enumerable_->SourceFront()) : std::begin(enumerable_->SourceFront())) {}

  QueryIterator(const QueryIterator &) = default;
  QueryIterator(QueryIterator &&) = default;

  QueryIterator &operator=(const QueryIterator &) = default;
  QueryIterator &operator=(QueryIterator &&) = default;

  ResultType operator*() const {
    return enumerable_->FirstFn()(static_cast<FunctionObjectArgumentType>(*iterator_));
  }

  friend bool operator!=(const QueryIterator &lhs, const QueryIterator &rhs) {
    return lhs.iterator_ != rhs.iterator_;
  }

  friend bool operator==(const QueryIterator &lhs, const QueryIterator &rhs) {
    return lhs.iterator_ == rhs.iterator_;
  }

  QueryIterator &operator++() {
    ++iterator_;
    return *this;
  }

  QueryIterator operator++(int) {
    QueryIterator previous(*this);
    ++iterator_;
    return previous;
  }

private:
  Enumerable *enumerable_ = nullptr;

  SourceIterator iterator_;
};

} // namespace cinq::detail
