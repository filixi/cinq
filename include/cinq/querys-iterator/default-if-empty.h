#pragma once

#include "../query-category.h"
#include "detail/concept.h"
#include "detail/utility.h"
#include "query-iterator-fwd.h"

namespace cinq::detail {
template <bool ArgConstness, bool RetConstness, class TFn, class... TSources>
class QueryIterator<ArgConstness, RetConstness, BasicEnumerable<QueryCategory::DefaultIfEmpty, std::tuple<TFn>, TSources...>>
  : public MultiVisitorSetIterator<IteratorTupleVisitor, ArgConstness, QueryCategory::DefaultIfEmpty, std::tuple<TFn>, TSources...> {
public:
  using Base = MultiVisitorSetIterator<IteratorTupleVisitor, ArgConstness, QueryCategory::DefaultIfEmpty, std::tuple<TFn>, TSources...>;

  using CommonType = typename Base::AdjustedCommonType;

  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, CommonType, cinq::utility::SourceType::PartialInternalStorage>;
  using value_type = typename std::decay_t<ResultType>;

  QueryIterator() {}

  QueryIterator(typename Base::Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : Base(enumerable, is_past_the_end_iteratorator) {
    if (!is_past_the_end_iteratorator && !Base::visitor.IsLast())
      enumerable->template GetSource<1>().GetSource().clear();
  }

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
