#pragma once

#include <iterator>
#include <map>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "../multi-enumerables-visitor.h"
#include "../query-category.h"
#include "detail/concept.h"
#include "detail/utility.h"
#include "query-iterator-fwd.h"

namespace cinq::detail {
template <bool ArgConstness, bool RetConstness, class TFn, class... TSources>
class QueryIterator<ArgConstness, RetConstness, BasicEnumerable<QueryCategory::Intersect, std::tuple<TFn>, TSources...>>
  : public MultiVisitorSetIterator<RoundRobinIteratorTupleVisitor, ArgConstness, QueryCategory::Intersect, std::tuple<TFn>, TSources...> {
public:
  using Base = MultiVisitorSetIterator<RoundRobinIteratorTupleVisitor, ArgConstness, QueryCategory::Intersect, std::tuple<TFn>, TSources...>;

  using CommonType = typename Base::AdjustedCommonType;
  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, CommonType, cinq::utility::SourceType::InternalStorage>;
  using value_type = typename std::decay_t<ResultType>;

  QueryIterator() {}

  QueryIterator(typename Base::Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : Base(enumerable, is_past_the_end_iteratorator) {
    if (!is_past_the_end_iteratorator && Base::visitor.IsValid()) {
      // push the first element
      Base::visitor.Visit([this](auto &&x) {
        if constexpr (std::is_convertible_v<decltype(x), value_type>) {
          if constexpr (Base::is_all_reference_to_same)
            ++values_[x];
          else
            ++values_[std::forward<decltype(x)>(x)];
        } else {
          ++values_[static_cast<const value_type &>(std::forward<decltype(x)>(x))];
        }
      });

      QueryIterator::FindNextValid();
    }
  }

  QueryIterator(const QueryIterator &rhs)
    : Base(rhs), values_(rhs.values_), current_(rhs.current_ == rhs.values_.end() ? values_.end() : values_.find(rhs.current_->first)) {}

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
      current_ = values_.find(rhs.current_->first);

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
    return current_->first;
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
            succeed = ++values_[x] == sizeof...(TSources);
          else
            succeed = ++values_[std::forward<decltype(x)>(x)] == sizeof...(TSources);
        } else {
          succeed = ++values_[static_cast<const value_type &>(std::forward<decltype(x)>(x))] == sizeof...(TSources);
        }

        if (succeed)
          current_ = values_.find(x);
      });

      if (succeed)
        break;
    }
  }

  using InternalStorageType = std::conditional_t<Base::is_all_reference_to_same,
    cinq::utility::ReferenceWrapper<value_type>, value_type>;

  using MapType = std::conditional_t<cinq::utility::ReferenceWrapper<value_type>::hash_version,
    std::unordered_map<InternalStorageType, size_t>, std::map<InternalStorageType, size_t>>;

  MapType values_;
  typename MapType::iterator current_ = values_.end();
};

} // namespace cinq::detail
