#pragma once

#include <cassert>

#include <iostream>
#include <memory>

#include "operator-category.h"

namespace cinq_v3 {
template <class TSource, class TFn, EnumerableCategory Category, OperatorType Operator, class... TRestSources>
class Enumerable;

// For each category/Operator, specialize this class to provide Iterator implementation.
template <class TEnumerable>
class OperatorSpecializedIterator;

template <class TSource, class TFn>
class OperatorSpecializedIterator<Enumerable<TSource, TFn, EnumerableCategory::Producer, OperatorType::SelectMany>> {
public:
  using Enumerable = Enumerable<TSource, TFn, EnumerableCategory::Producer, OperatorType::SelectMany>;

  using SourceIterator = typename Enumerable::SourceIterator;

  using ProducedEnumerable = decltype(std::declval<TFn>()(*std::declval<SourceIterator>()));
  using ProducedIterator = decltype(std::cbegin(std::declval<ProducedEnumerable>()));

  using value_type = std::decay_t<decltype(*std::declval<ProducedIterator>())>;

  struct ProducedEnumerableHolder {
    // need to check for copy elision
    ProducedEnumerableHolder(ProducedEnumerable produced_enumerable)
      : produced_enumerable_(produced_enumerable) {}

    auto begin() const {
      return std::cbegin(produced_enumerable_);
    }

    auto end() const {
      return std::cend(produced_enumerable_);
    }

    ProducedEnumerable produced_enumerable_;
  };

  OperatorSpecializedIterator(const Enumerable *enumerable, bool is_past_the_end_iterator)
    : enumerable_(enumerable), is_past_the_end_iterator_(is_past_the_end_iterator) {
    if (!is_past_the_end_iterator_)
      InitalizeNewProducedEnumerable(first_);
  }

  OperatorSpecializedIterator(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator(OperatorSpecializedIterator &&) = default;

  OperatorSpecializedIterator &operator=(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator &operator=(OperatorSpecializedIterator &&) = default;
  
  OperatorSpecializedIterator &operator++() {
    assert(!is_past_the_end_iterator_);
    ++produced_first_;

    while (produced_first_ == std::cend(*produced_enumerable_)) {
      ++first_;
      if (first_ == last_)
        return *this;
      InitalizeNewProducedEnumerable(first_);
    }

    return *this;
  }

  OperatorSpecializedIterator &operator++(int) {
    assert(!is_past_the_end_iterator_);
    OperatorSpecializedIterator prev(*this);
    ++*this;
    return prev;
  }

  auto &operator*() const {
    assert(!is_past_the_end_iterator_);
    return *produced_first_;
  }

  friend bool operator!=(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    return !(lhs.is_past_the_end_iterator_ && rhs.is_past_the_end_iterator_) &&
      (!rhs.is_past_the_end_iterator_ || lhs.first_ != lhs.last_ && lhs.produced_first_ != lhs.produced_last_);
  }

  friend bool operator==(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    return !(lhs != rhs);
  }

private:
  void InitalizeNewProducedEnumerable(SourceIterator &iter) {
    if (iter == std::cend(*enumerable_->source_))
      return ;

    produced_enumerable_ = std::make_shared<ProducedEnumerableHolder>(enumerable_->fn_(*iter));
    produced_first_ = std::cbegin(*produced_enumerable_);
    produced_last_ = std::cend(*produced_enumerable_);
  }

  const Enumerable *enumerable_;

  bool is_past_the_end_iterator_;

  SourceIterator first_ = std::cbegin(*enumerable_->source_);
  SourceIterator last_ = std::cend(*enumerable_->source_);

  std::shared_ptr<ProducedEnumerableHolder> produced_enumerable_;
  ProducedIterator produced_first_;
  ProducedIterator produced_last_;
};

template <class TSource, class TFn>
class OperatorSpecializedIterator<Enumerable<TSource, TFn, EnumerableCategory::Producer, OperatorType::Select>> {
public:
  using Enumerable = Enumerable<TSource, TFn, EnumerableCategory::Producer, OperatorType::Select>;

  using SourceIterator = typename Enumerable::SourceIterator;

  using value_type = std::invoke_result_t<TFn, const typename SourceIterator::value_type &>;

  OperatorSpecializedIterator(const Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable),
      iterator_(is_past_the_end_iteratorator ? std::cend(*enumerable_->source_) : std::cbegin(*enumerable_->source_)) {}

  OperatorSpecializedIterator(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator(OperatorSpecializedIterator &&) = default;

  OperatorSpecializedIterator &operator=(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator &operator=(OperatorSpecializedIterator &&) = default;

  decltype(auto) operator*() {
    return enumerable_->fn_(*iterator_);
  }

  friend bool operator!=(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    return lhs.iterator_ != rhs.iterator_;
  }

  friend bool operator==(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    return lhs.iterator_ != rhs.iterator_;
  }

  OperatorSpecializedIterator &operator++() {
    ++iterator_;
    return *this;
  }

  OperatorSpecializedIterator operator++(int) {
    OperatorSpecializedIterator previous(*this);
    ++iterator_;
    return previous;
  }

private:
  const Enumerable *enumerable_;

  SourceIterator iterator_;
};

template <class Fn, class Arg1, class Arg2, class = std::invoke_result_t<Fn, Arg>>
constexpr bool is_callable(int) { return true; }
template <class Fn, class Arg1, class Arg2>
constexpr bool is_callable(...) { return false; }

template <class T>
using add_const_on_rvalue_reference_v = std::conditional_t<std::is_rvalue_reference_v<T>, const std::remove_reference_t<T> &, T>;

template <class TSource, class TFn, class TSource2>
class OperatorSpecializedIterator<Enumerable<TSource, TFn, EnumerableCategory::Producer, OperatorType::Join, TSource2>> {
public:
  using Enumerable = Enumerable<TSource, TFn, EnumerableCategory::Producer, OperatorType::Join, TSource2>;

  using SourceIterator = typename Enumerable::SourceIterator;
  using SourceIterator2 = typename Enumerable::SourceIterator2;

  using value_type = std::tuple<
    add_const_on_rvalue_reference_v<decltype(*std::declval<SourceIterator>())>,
    add_const_on_rvalue_reference_v<decltype(*std::declval<SourceIterator>())>
  >;

  OperatorSpecializedIterator(const Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable), is_past_the_end_iteratorator_(is_past_the_end_iteratorator) {}

  OperatorSpecializedIterator(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator(OperatorSpecializedIterator &&) = default;

  OperatorSpecializedIterator &operator=(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator &operator=(OperatorSpecializedIterator &&) = default;

  value_type operator*() {
    return value_type(*first_, *first2_);
  }

  friend bool operator!=(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    return lhs.first_ != rhs.first_;
  }

  friend bool operator==(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    return lhs.first_ == rhs.first_;
  }

  OperatorSpecializedIterator &operator++() {
    assert(!is_past_the_end_iteratorator_);
    ++first2_;
    if (!(first2_ != last2_)) {
      first2_.~SourceIterator2();
      new(&first2_) SourceIterator2(std::begin(enumerable_->source2_));
      ++first_;
    }
    return *this;
  }

  OperatorSpecializedIterator operator++(int) {
    assert(!is_past_the_end_iteratorator_);
    OperatorSpecializedIterator previous(*this);
    ++*this;
    return previous;
  }

private:
  const Enumerable *enumerable_;

  bool is_past_the_end_iteratorator_;

  SourceIterator first_ = is_past_the_end_iteratorator_ ? std::cend(*enumerable_->source_) : std::cbegin(*enumerable_->source_);
  SourceIterator last_ = std::cend(*enumerable_->source_);

  SourceIterator2 first2_ = is_past_the_end_iteratorator_ ? std::cend(enumerable_->source2_) : std::cbegin(enumerable_->source2_);
  SourceIterator2 last2_ = std::cend(enumerable_->source2_);
};

template <class TSource, class TFn>
class OperatorSpecializedIterator<Enumerable<TSource, TFn, EnumerableCategory::Subrange, OperatorType::Where>> {
public:
  using Enumerable = Enumerable<TSource, TFn, EnumerableCategory::Subrange, OperatorType::Where>;

  using SourceIterator = typename Enumerable::SourceIterator;

  using value_type = typename SourceIterator::value_type;

  OperatorSpecializedIterator(const Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable), is_past_the_end_iteratorator_(is_past_the_end_iteratorator) {
    FindNextValideElement();
  }

  OperatorSpecializedIterator(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator(OperatorSpecializedIterator &&) = default;

  OperatorSpecializedIterator &operator=(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator &operator=(OperatorSpecializedIterator &&) = default;

  decltype(auto) operator*() {
    return *first_;
  }

  friend bool operator!=(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    return lhs.first_ != rhs.first_;
  }

  friend bool operator==(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    return lhs.first_ == rhs.first_;
  }

  OperatorSpecializedIterator &operator++() {
    ++first_;
    FindNextValideElement();
    return *this;
  }

  OperatorSpecializedIterator operator++(int) {
    OperatorSpecializedIterator previous(*this);
    ++first_;
    FindNextValideElement();
    return previous;
  }

private:
  void FindNextValideElement() {
    while (first_ != last_ && !enumerable_->fn_(*first_))
      ++first_;
  }

  const Enumerable * const enumerable_;

  const bool is_past_the_end_iteratorator_;

  SourceIterator first_ = is_past_the_end_iteratorator_ ? std::cend(*enumerable_->source_) : std::cbegin(*enumerable_->source_);
  SourceIterator last_ = std::cend(*enumerable_->source_);
};

} // namespace cinq_v3
