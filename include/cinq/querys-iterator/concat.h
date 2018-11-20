#pragma once

#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

#include "detail/concept.h"
#include "detail/utility.h"
#include "multi-enumerables-visitor.h"
#include "query-category.h"
#include "query-iterator-fwd.h"

namespace cinq::detail {
template <bool ArgConstness, bool RetConstness, class TFn, class... TSources>
class QueryIterator<ArgConstness, RetConstness, BasicEnumerable<QueryCategory::Concat, std::tuple<TFn>, TSources...>>
  : public MultiVisitorSetIterator<IteratorTupleVisitor, ArgConstness, QueryCategory::Concat, std::tuple<TFn>, TSources...> {
public:
  using Base = MultiVisitorSetIterator<IteratorTupleVisitor, ArgConstness, QueryCategory::Concat, std::tuple<TFn>, TSources...>;

  using CommonType = typename Base::AdjustedCommonType;

  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, CommonType, cinq::utility::SourceType::Iterator>;
  using value_type = typename std::decay_t<ResultType>;

  QueryIterator() {}

  QueryIterator(typename Base::Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : Base(enumerable, is_past_the_end_iteratorator) {}

  ResultType operator*() const {
    return Base::visitor.Visit([](auto &&x) -> ResultType {
        if constexpr (std::is_convertible_v<decltype(x), ResultType>)
          return std::forward<decltype(x)>(x);
        else
          return static_cast<ResultType>(std::forward<decltype(x)>(x));
      });
  }

  QueryIterator &operator++() {
    FindNextValid();
    return *this;
  }

  QueryIterator operator++(int) {
    QueryIterator previous(*this);
    ++*this;
    return previous;
  }

protected:
  void FindNextValid() override {
    Base::visitor.MoveToNext(Base::enumerable_->GetSourceTuple());
  }
};

} // namespace cinq::detail
