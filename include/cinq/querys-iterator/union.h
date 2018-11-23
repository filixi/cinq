#pragma once

#include <iterator>
#include <set>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include "../multi-enumerables-visitor.h"
#include "../query-category.h"
#include "detail/concept.h"
#include "detail/utility.h"
#include "query-iterator-fwd.h"

namespace cinq::detail {
template <bool ArgConstness, bool RetConstness, class TFn, class... TSources>
class QueryIterator<ArgConstness, RetConstness, BasicEnumerable<QueryCategory::Union, std::tuple<TFn>, TSources...>>
  : public MultiVisitorSetIterator<RoundRobinIteratorTupleVisitor, ArgConstness, QueryCategory::Union, std::tuple<TFn>, TSources...> {
public:
  using Base = MultiVisitorSetIterator<RoundRobinIteratorTupleVisitor, ArgConstness, QueryCategory::Union, std::tuple<TFn>, TSources...>;

  using CommonType = typename Base::AdjustedCommonType;
  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, CommonType, cinq::utility::SourceType::InternalStorage>;
  using value_type = typename std::decay_t<ResultType>;

  QueryIterator() {}

  QueryIterator(typename Base::Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : Base(enumerable, is_past_the_end_iteratorator) {
    if (!is_past_the_end_iteratorator && Base::visitor.IsValid()) {
      // push the first element
      Base::visitor.Visit([this](auto &&x) {
        bool succeed = false;
        if constexpr (std::is_convertible_v<decltype(x), value_type>) {
          if constexpr (Base::is_all_reference_to_same)
            std::tie(current_, succeed) = values_.insert(x);
          else
            std::tie(current_, succeed) = values_.insert(std::forward<decltype(x)>(x));
        } else {
          std::tie(current_, succeed) = values_.insert(static_cast<const value_type &>(std::forward<decltype(x)>(x)));
        }
      });
    }
  }

  QueryIterator(const QueryIterator &rhs)
    : Base(rhs), values_(rhs.values_), current_(rhs.current_ == rhs.values_.end() ? values_.end() : values_.find(*rhs.current_)) {}

  QueryIterator(QueryIterator &&rhs) : Base(std::move(rhs)) {
    if (rhs.current_ == rhs.values_.end()) {
      values_ = std::move(rhs.values_);
      current_ = values_.end();
    } else {
      auto node = rhs.values_.extract(rhs.current_);
      values_ = std::move(rhs.values_);
      current_ = values_.insert(std::move(node)).position;
    }
  }

  QueryIterator &operator=(const QueryIterator &rhs) {
    Base::operator=(rhs);
    values_ = rhs.values_;
    if (rhs.current_ == rhs.values_.end())
      current_ = values_.end();
    else
      current_ = values_.find(*rhs.current_);
    return *this;
  }

  QueryIterator &operator=(QueryIterator &&rhs) {
    Base::operator=(std::move(rhs));
    if (rhs.current_ == rhs.values_.end()) {
      values_ = std::move(rhs.values_);
      current_ = values_.end();
    } else {
      auto node = rhs.values_.extract(rhs.current_);
      values_ = std::move(rhs.values_);
      current_ = values_.insert(std::move(node)).position;
    }
    return *this;
  }

  ResultType operator*() const {
    return *current_;
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
    cinq::utility::CinqAssert(!Base::is_past_the_end_iteratorator_);

    bool succeed = false;
    for (;;) {
      if (!Base::visitor.MoveToNext(Base::enumerable_->GetSourceTuple()))
        break;

      Base::visitor.Visit([this, &succeed](auto &&x) {
        if constexpr (std::is_convertible_v<decltype(x), value_type>) {
          if constexpr (Base::is_all_reference_to_same)
            std::tie(current_, succeed) = values_.insert(x);
          else
            std::tie(current_, succeed) = values_.insert(std::forward<decltype(x)>(x));
        } else {
          std::tie(current_, succeed) = values_.insert(static_cast<const value_type &>(std::forward<decltype(x)>(x)));
        }
      });

      if (succeed)
        break;
    }
  }

  using InternalStorageType = std::conditional_t<Base::is_all_reference_to_same,
    cinq::utility::ReferenceWrapper<value_type>, value_type>;

  using SetType = std::conditional_t< cinq::utility::ReferenceWrapper<value_type>::hash_version,
    std::unordered_set<InternalStorageType>, std::set<InternalStorageType>>;

  SetType values_;
  typename SetType::iterator current_ = values_.end();
};

} // namespace cinq::detail
