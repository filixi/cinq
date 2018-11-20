#pragma once

#include <iterator>
#include <set>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include "detail/concept.h"
#include "detail/utility.h"
#include "query-category.h"
#include "query-iterator-fwd.h"

namespace cinq::detail {
template <bool ArgConstness, bool RetConstness, class TFn, class TSource>
class QueryIterator<ArgConstness, RetConstness, BasicEnumerable<QueryCategory::Distinct, std::tuple<TFn>, TSource>> {
public:
  using Enumerable = BasicEnumerable<QueryCategory::Distinct, std::tuple<TFn>, TSource>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;
  using SourceIteratorYieldType = decltype(*std::declval<SourceIterator>());

  using FunctionObjectArgumentType = cinq::utility::transform_to_function_object_argument_t<ArgConstness, SourceIteratorYieldType>;

  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, SourceIteratorYieldType, cinq::utility::SourceType::InternalStorage>;
  using value_type = std::decay_t<ResultType>;

  QueryIterator() : first_(), last_() {}

  QueryIterator(Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable), is_past_the_end_iteratorator_(is_past_the_end_iteratorator) {
    if (!is_past_the_end_iteratorator)
      FindNextValideElement();
  }

  QueryIterator(const QueryIterator &) = default;
  QueryIterator(QueryIterator &&) = default;

  QueryIterator &operator=(const QueryIterator &) = default;
  QueryIterator &operator=(QueryIterator &&) = default;

  ResultType operator*() const {
    return distinct_helper_.Get();
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
    while (first_ != last_ && !distinct_helper_(*first_)) // what if *first return a temporary
      ++first_;
  }

  Enumerable *enumerable_ = nullptr;

  bool is_past_the_end_iteratorator_ = false;

  class DistinctHelper {
  public:
    DistinctHelper() = default;

    DistinctHelper(const DistinctHelper &rhs)
      : distinct_set_(rhs.distinct_set_), current_ (rhs.current_ == rhs.distinct_set_.end() ? distinct_set_.end() : distinct_set_.find(*rhs.current_)) {}
    DistinctHelper(DistinctHelper &&rhs) {
      if (rhs.current_ == rhs.distinct_set_.end()) {
        distinct_set_ = std::move(rhs.distinct_set_);
        current_ = distinct_set_.end();
      } else {
        auto node = rhs.distinct_set_.extract(rhs.current_);
        distinct_set_ = std::move(rhs.distinct_set_);
        current_ = distinct_set_.insert(std::move(node)).position;
      }
    }

    DistinctHelper &operator=(const DistinctHelper &rhs) {
      distinct_set_ = rhs.distinct_set_;
      if (rhs.current_ == rhs.distinct_set_.end())
        current_ = distinct_set_.end();
      else
        current_ = distinct_set_.find(*rhs.current_);
      return *this;
    }

    DistinctHelper &operator=(DistinctHelper &&rhs) {
      if (rhs.current_ == rhs.distinct_set_.end()) {
        distinct_set_ = std::move(rhs.distinct_set_);
        current_ = distinct_set_.end();
      } else {
        auto node = rhs.distinct_set_.extract(rhs.current_);
        distinct_set_ = std::move(rhs.distinct_set_);
        current_ = distinct_set_.insert(std::move(node)).position;
      }
      return *this;
    }

    bool operator()(const value_type &t) const {
      auto ret = distinct_set_.insert(t);
      current_ = ret.first;
      return ret.second;
    };

    const auto &Get() const {
      return *current_;
    }

  private:
    using InternalStorageType = std::conditional_t<std::is_reference_v<SourceIteratorYieldType>,
      cinq::utility::ReferenceWrapper<value_type>, value_type>;

    using SetType = std::conditional_t<cinq::utility::ReferenceWrapper<value_type>::hash_version,
      std::unordered_set<InternalStorageType>,std::set<InternalStorageType>>;

    mutable SetType distinct_set_;
    mutable typename SetType::iterator current_ = distinct_set_.end();
  } distinct_helper_;

  static_assert(concept::PredicateCheck<DistinctHelper, FunctionObjectArgumentType>(), "(Internal error) Bad predicate");

  SourceIterator first_ = is_past_the_end_iteratorator_ ? std::end(enumerable_->SourceFront()) : std::begin(enumerable_->SourceFront());
  SourceIterator last_ = std::end(enumerable_->SourceFront());
};

} // namespace cinq::detail
