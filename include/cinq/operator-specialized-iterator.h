#pragma once

#include <cassert>
#include <exception>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <iostream> // development build only

#include "detail/concept.h"
#include "detail/utility.h"
#include "multi-enumerables-visitor.h"
#include "operator-category.h"

namespace cinq::detail {
template <OperatorType Operator, class TFn, class... TSources>
class BasicEnumerable;

// For each Operator, specialize this class to provide Iterator implementation.
template <bool ArgConstness, bool RetConstness, class TEnumerable>
class OperatorSpecializedIterator;

template <bool ArgConstness, bool RetConstness, class TFn, class TSource>
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<OperatorType::SelectMany, TFn, TSource>> {
public:
  using Enumerable = BasicEnumerable<OperatorType::SelectMany, TFn, TSource>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;
  using SourceYieldType = decltype(*std::declval<SourceIterator>());

  using FunctionObjectArgumentType = cinq::utility::transform_to_function_object_argument_t<ArgConstness, SourceYieldType>;

  using ProducedEnumerable = decltype(std::declval<TFn>()(std::declval<FunctionObjectArgumentType>()));
  using ProducedIterator = decltype(std::begin(std::declval<ProducedEnumerable &>()));
  using ProducedType = decltype(*std::declval<ProducedIterator>());

  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, ProducedType, cinq::utility::SourceType::Iterator>;
  using value_type = std::decay_t<ResultType>;

  static_assert(concept::SelectorCheck<TFn, FunctionObjectArgumentType>() &&
    concept::SelectManySelectorCheck<true, TFn, FunctionObjectArgumentType>(), "Bad selector");

  class BaseIterator {
  public:
    using value_type = std::decay_t<ResultType>;

    virtual ~BaseIterator() {}

    virtual bool IsPastTheEnd() const = 0;

    virtual ResultType operator*() const = 0;

    virtual BaseIterator &operator++() = 0;

    virtual bool operator!=(const BaseIterator &rhs) const = 0;
    virtual bool operator==(const BaseIterator &rhs) final {
      return !this->operator!=(rhs);
    }

    virtual std::unique_ptr<BaseIterator> Clone() const = 0;
  };

  class BeginIterator : public BaseIterator {
  public:
    struct ProducedEnumerableHolder {
      // need to check for copy elision
      ProducedEnumerableHolder(ProducedEnumerable produced_enumerable)
        : produced_enumerable_(produced_enumerable) {}

      auto begin() {
        return std::begin(produced_enumerable_);
      }

      auto end() {
        return std::end(produced_enumerable_);
      }

      ProducedEnumerable produced_enumerable_;
    };

    BeginIterator(Enumerable *enumerable)
      : enumerable_(enumerable) {
      InitalizeNewProducedEnumerable(first_);
    }

    BeginIterator(const BeginIterator &) = default;
    BeginIterator(BeginIterator &&) = default;

    BeginIterator &operator=(const BeginIterator &) = default;
    BeginIterator &operator=(BeginIterator &&) = default;

    BeginIterator &operator++() override {
      ++produced_first_;

      while (produced_first_ == std::end(*produced_enumerable_)) {
        ++first_;
        if (first_ == last_)
          return *this;
        InitalizeNewProducedEnumerable(first_);
      }

      return *this;
    }

    ResultType operator*() const override {
      return *produced_first_;
    }

    bool operator!=(const BaseIterator &rhs) const override {
      const BeginIterator &lhs = *this;
      return !(lhs.IsPastTheEnd() && rhs.IsPastTheEnd()) &&
        (!rhs.IsPastTheEnd() || lhs.first_ != lhs.last_ && lhs.produced_first_ != lhs.produced_last_);
    }

    std::unique_ptr<BaseIterator> Clone() const override {
      return std::make_unique<BeginIterator>(*this);
    }

    bool IsPastTheEnd() const override { return false; }

  private:
    void InitalizeNewProducedEnumerable(SourceIterator &iter) {
      if (iter == std::end(enumerable_->SourceFront()))
        return ;

      produced_enumerable_ = std::make_shared<ProducedEnumerableHolder>(enumerable_->fn_(static_cast<FunctionObjectArgumentType>(*iter)));
      produced_first_ = std::begin(*produced_enumerable_);
      produced_last_ = std::end(*produced_enumerable_);
    }

    Enumerable *enumerable_;

    SourceIterator first_ = std::begin(enumerable_->SourceFront());
    SourceIterator last_ = std::end(enumerable_->SourceFront());

    std::shared_ptr<ProducedEnumerableHolder> produced_enumerable_;
    ProducedIterator produced_first_;
    ProducedIterator produced_last_;
  };

  class EndIterator : public BaseIterator {
  public:
    EndIterator(Enumerable *) {}

    bool IsPastTheEnd() const override { return true; }

    ResultType operator*() const override { throw std::runtime_error("Can not dereference past-the-end iterator."); }

    EndIterator &operator++() override { return *this; }

    bool operator!=(const BaseIterator &rhs) const override {
      const EndIterator &lhs = *this;
      return !(lhs.IsPastTheEnd() && rhs.IsPastTheEnd()) &&
        (!rhs.IsPastTheEnd() || rhs.operator!=(*this));
    }

    std::unique_ptr<BaseIterator> Clone() const override {
      return std::make_unique<EndIterator>(nullptr);
    }
  };

  OperatorSpecializedIterator() {}

  OperatorSpecializedIterator(Enumerable *enumerable, bool is_past_the_end_iterator) {
    if (is_past_the_end_iterator)
      iterator_ = std::make_unique<EndIterator>(enumerable);
    else
      iterator_ = std::make_unique<BeginIterator>(enumerable);
  }

  OperatorSpecializedIterator(const OperatorSpecializedIterator &rhs) : iterator_(rhs.iterator_->Clone()) {}
  OperatorSpecializedIterator(OperatorSpecializedIterator &&) = default;

  OperatorSpecializedIterator &operator=(const OperatorSpecializedIterator &rhs) {
    iterator_ = rhs.iterator_->Clone();
  }
  OperatorSpecializedIterator &operator=(OperatorSpecializedIterator &&) = default;

  ResultType operator*() const {
    return iterator_->operator*();
  }

  OperatorSpecializedIterator &operator++() {
    ++*iterator_;
    return *this;
  }
  OperatorSpecializedIterator operator++(int) {
    OperatorSpecializedIterator prev(*this);
    ++*iterator_;
    return prev;
  }

  friend bool operator!=(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    return *lhs.iterator_ != *rhs.iterator_;
  }

  friend bool operator==(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    return *lhs.iterator_ == *rhs.iterator_;
  }

private:
  std::unique_ptr<BaseIterator> iterator_;
};

template <bool ArgConstness, bool RetConstness, class TFn, class TSource>
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<OperatorType::Select, TFn, TSource>> {
public:
  using Enumerable = BasicEnumerable<OperatorType::Select, TFn, TSource>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;

  using SourceIteratorYieldType = decltype(*std::declval<SourceIterator>());
  using FunctionObjectArgumentType = cinq::utility::transform_to_function_object_argument_t<ArgConstness, SourceIteratorYieldType>;

  using FunctionObjectYieldType = std::invoke_result_t<TFn, FunctionObjectArgumentType>;
  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, FunctionObjectYieldType, cinq::utility::SourceType::FunctionObject>;
  using value_type = std::decay_t<ResultType>;

  static_assert(concept::SelectorCheck<TFn, FunctionObjectArgumentType>(), "Bad selector");

  OperatorSpecializedIterator() {}

  OperatorSpecializedIterator(Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable),
      iterator_(is_past_the_end_iteratorator ? std::end(enumerable_->SourceFront()) : std::begin(enumerable_->SourceFront())) {}

  OperatorSpecializedIterator(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator(OperatorSpecializedIterator &&) = default;

  OperatorSpecializedIterator &operator=(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator &operator=(OperatorSpecializedIterator &&) = default;

  ResultType operator*() const {
    return enumerable_->fn_(static_cast<FunctionObjectArgumentType>(*iterator_));
  }

  friend bool operator!=(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    return lhs.iterator_ != rhs.iterator_;
  }

  friend bool operator==(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    return lhs.iterator_ == rhs.iterator_;
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
  Enumerable *enumerable_ = nullptr;

  SourceIterator iterator_;
};

template <bool ArgConstness, bool RetConstness, class TFn, class TSource, class TSource2>
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<OperatorType::Join, TFn, TSource, TSource2>> {
public:
  using Enumerable = BasicEnumerable<OperatorType::Join, TFn, TSource, TSource2>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;
  using SourceIterator2 = typename Enumerable::template SourceIterator<1>;

  using SourceIteratorYieldType = decltype(*std::declval<SourceIterator>());
  using SourceIteratorYieldType2 = decltype(*std::declval<SourceIterator2>());

  using FunctionObjectArgumentTupleElementType = cinq::utility::transform_to_function_object_argument_t<ArgConstness, SourceIteratorYieldType>;
  using FunctionObjectArgumentTupleElementType2 = cinq::utility::transform_to_function_object_argument_t<ArgConstness, SourceIteratorYieldType2>;

  using FunctionObjectArgumentTupleType = std::tuple<FunctionObjectArgumentTupleElementType, FunctionObjectArgumentTupleElementType2>;

  using FunctionObjectYieldType = std::invoke_result_t<TFn, FunctionObjectArgumentTupleElementType, FunctionObjectArgumentTupleElementType2>;
  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, FunctionObjectYieldType, cinq::utility::SourceType::FunctionObject>;

  using value_type = std::decay_t<ResultType>;

  static_assert(concept::SelectorCheck<TFn, FunctionObjectArgumentTupleElementType, FunctionObjectArgumentTupleElementType2>(), "Bad selector");

  OperatorSpecializedIterator() : first_(), first2_(), last2_() {}

  OperatorSpecializedIterator(Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable), is_past_the_end_iteratorator_(is_past_the_end_iteratorator) {}

  OperatorSpecializedIterator(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator(OperatorSpecializedIterator &&) = default;

  OperatorSpecializedIterator &operator=(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator &operator=(OperatorSpecializedIterator &&) = default;

  ResultType operator*() const {
    return enumerable_->fn_(
        static_cast<FunctionObjectArgumentTupleElementType>(*first_),
        static_cast<FunctionObjectArgumentTupleElementType2>(*first2_)
      );
  }

  friend bool operator!=(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    if (rhs.is_past_the_end_iteratorator_)
      return lhs.first_ != rhs.first_ && lhs.first2_ != rhs.last2_;
    return lhs.first_ != rhs.first_ || lhs.first2_ != rhs.first2_;
  }

  friend bool operator==(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    return lhs.first_ == rhs.first_ && lhs.first2_ == rhs.first2_;
  }

  OperatorSpecializedIterator &operator++() {
    assert(!is_past_the_end_iteratorator_);
    ++first2_;
    if (!(first2_ != last2_)) {
      first2_ = SourceIterator2(std::begin(enumerable_->template GetSource<1>()));
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
  Enumerable *enumerable_ = nullptr;

  bool is_past_the_end_iteratorator_ = false;

  SourceIterator first_ = is_past_the_end_iteratorator_ ? std::end(enumerable_->template GetSource<0>()) : std::begin(enumerable_->template GetSource<0>());

  SourceIterator2 first2_ = std::begin(enumerable_->template GetSource<1>());
  SourceIterator2 last2_ = std::end(enumerable_->template GetSource<1>());
};

template <bool ArgConstness, bool RetConstness, class TFn, class TSource>
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<OperatorType::Where, TFn, TSource>> {
public:
  using Enumerable = BasicEnumerable<OperatorType::Where, TFn, TSource>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;
  using SourceIteratorYieldType = decltype(*std::declval<SourceIterator>());

  using FunctionObjectArgumentType = cinq::utility::transform_to_function_object_argument_t<ArgConstness, SourceIteratorYieldType>;

  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, SourceIteratorYieldType, cinq::utility::SourceType::Iterator>;
  using value_type = std::decay_t<ResultType>;

  static_assert(concept::PredicateCheck<TFn, FunctionObjectArgumentType>(), "Bad predicate");

  OperatorSpecializedIterator() : first_(), last_() {}

  OperatorSpecializedIterator(Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable), is_past_the_end_iteratorator_(is_past_the_end_iteratorator) {
    FindNextValideElement();
  }

  OperatorSpecializedIterator(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator(OperatorSpecializedIterator &&) = default;

  OperatorSpecializedIterator &operator=(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator &operator=(OperatorSpecializedIterator &&) = default;

  ResultType operator*() const {
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
    while (first_ != last_ && !enumerable_->fn_(static_cast<FunctionObjectArgumentType>(*first_)))
      ++first_;
  }

  Enumerable *enumerable_ = nullptr;

  bool is_past_the_end_iteratorator_ = false;

  SourceIterator first_ = is_past_the_end_iteratorator_ ? std::end(enumerable_->SourceFront()) : std::begin(enumerable_->SourceFront());
  SourceIterator last_ = std::end(enumerable_->SourceFront());
};

template <bool ArgConstness, bool RetConstness, class TFn, class... TSources>
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<OperatorType::Union, TFn, TSources...>>
  : public MultiVisitorSetIterator<RoundRobinIteratorTupleVisitor, ArgConstness, OperatorType::Union, TFn, TSources...> {
public:
  using Base = MultiVisitorSetIterator<RoundRobinIteratorTupleVisitor, ArgConstness, OperatorType::Union, TFn, TSources...>;

  using CommonType = typename Base::AdjustedCommonType;
  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, CommonType, cinq::utility::SourceType::InternalStorage>;
  using value_type = typename std::decay_t<ResultType>;

  OperatorSpecializedIterator() {}

  OperatorSpecializedIterator(typename Base::Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : Base(enumerable, is_past_the_end_iteratorator) {
    if (!is_past_the_end_iteratorator && Base::visitor.IsValid()) {
      // push the first element
      Base::visitor.Visit([this](auto &&x) {
        bool succeed = false;
        if constexpr (std::is_convertible_v<decltype(x), value_type>)
          std::tie(current_, succeed) = values_.insert(std::forward<decltype(x)>(x));
        else
          std::tie(current_, succeed) = values_.insert(static_cast<const value_type &>(std::forward<decltype(x)>(x)));
      });
    }
  }

  OperatorSpecializedIterator(const OperatorSpecializedIterator &rhs)
    : Base(rhs), values_(rhs.values_), current_(rhs.current_ == rhs.values_.end() ? values_.end() : values_.find(*rhs.current_)) {}

  OperatorSpecializedIterator(OperatorSpecializedIterator &&rhs) : Base(std::move(rhs)) {
    if (rhs.current_ == rhs.values_.end()) {
      values_ = std::move(rhs.values_);
      current_ = values_.end();
    } else {
      auto node = rhs.values_.extract(rhs.current_);
      values_ = std::move(rhs.values_);
      current_ = values_.insert(std::move(node)).position;
    }
  }

  OperatorSpecializedIterator &operator=(const OperatorSpecializedIterator &rhs) {
    Base::operator=(rhs);
    values_ = rhs.values_;
    if (rhs.current_ == rhs.values_.end())
      current_ = values_.end();
    else
      current_ = values_.find(*rhs.current_);
    return *this;
  }

  OperatorSpecializedIterator &operator=(OperatorSpecializedIterator &&rhs) {
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

  OperatorSpecializedIterator &operator++() {
    FindNextValid();
    return *this;
  }

  OperatorSpecializedIterator operator++(int) {
    OperatorSpecializedIterator previous(*this);
    ++*this;
    return previous;
  }

protected:
  void FindNextValid() override {
    assert(!Base::is_past_the_end_iteratorator_);

    bool succeed = false;
    for (;;) {
      if (!Base::visitor.MoveToNext(Base::enumerable_->GetSourceTuple()))
        break;

      Base::visitor.Visit([this, &succeed](auto &&x) {
        if constexpr (std::is_convertible_v<decltype(x), value_type>)
          std::tie(current_, succeed) = values_.insert(std::forward<decltype(x)>(x));
        else
          std::tie(current_, succeed) = values_.insert(static_cast<const value_type &>(std::forward<decltype(x)>(x)));
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

template <bool ArgConstness, bool RetConstness, class TFn, class... TSources>
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<OperatorType::Intersect, TFn, TSources...>>
  : public MultiVisitorSetIterator<RoundRobinIteratorTupleVisitor, ArgConstness, OperatorType::Intersect, TFn, TSources...> {
public:
  using Base = MultiVisitorSetIterator<RoundRobinIteratorTupleVisitor, ArgConstness, OperatorType::Intersect, TFn, TSources...>;

  using CommonType = typename Base::AdjustedCommonType;
  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, CommonType, cinq::utility::SourceType::InternalStorage>;
  using value_type = typename std::decay_t<ResultType>;

  OperatorSpecializedIterator() {}

  OperatorSpecializedIterator(typename Base::Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : Base(enumerable, is_past_the_end_iteratorator) {
    if (!is_past_the_end_iteratorator && Base::visitor.IsValid()) {
      // push the first element
      Base::visitor.Visit([this](auto &&x) {
        if constexpr (std::is_convertible_v<decltype(x), value_type>)
          ++values_[std::forward<decltype(x)>(x)];
        else
          ++values_[static_cast<const value_type &>(std::forward<decltype(x)>(x))];
      });

      OperatorSpecializedIterator::FindNextValid();
    }
  }

  OperatorSpecializedIterator(const OperatorSpecializedIterator &rhs)
    : Base(rhs), values_(rhs.values_), current_(rhs.current_ == rhs.values_.end() ? values_.end() : values_.find(rhs.current_->first)) {}

  OperatorSpecializedIterator(OperatorSpecializedIterator &&rhs) : Base(std::move(rhs)) {
    if (rhs.current_ == rhs.values_.end()) {
      values_ = std::move(rhs.values_);
      current_ = values_.end();
    } else {
      auto node = rhs.values_.extract(rhs.current_);
      values_ = std::move(rhs.values_);
      current_ = values_.insert(std::move(node)).position;
    }
  }

  OperatorSpecializedIterator &operator=(const OperatorSpecializedIterator &rhs) {
    Base::operator=(rhs);
    values_ = rhs.values_;
    if (rhs.current_ == rhs.values_.end())
      current_ = values_.end();
    else
      current_ = values_.find(rhs.current_->first);

    return *this;
  }

  OperatorSpecializedIterator &operator=(OperatorSpecializedIterator &&rhs) {
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

  OperatorSpecializedIterator &operator++() {
    FindNextValid();
    return *this;
  }

  OperatorSpecializedIterator operator++(int) {
    OperatorSpecializedIterator previous(*this);
    ++*this;
    return previous;
  }

protected:
  void FindNextValid() override {
    assert(!Base::is_past_the_end_iteratorator_);

    bool succeed = false;
    for (;;) {
      if (!Base::visitor.MoveToNext(Base::enumerable_->GetSourceTuple()))
        break;

      Base::visitor.Visit([this, &succeed](auto &&x) {
        if constexpr (std::is_convertible_v<decltype(x), value_type>)
          succeed = ++values_[std::forward<decltype(x)>(x)] == sizeof...(TSources);
        else
          succeed = ++values_[static_cast<const value_type &>(std::forward<decltype(x)>(x))] == sizeof...(TSources);

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

template <bool ArgConstness, bool RetConstness, class TFn, class... TSources>
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<OperatorType::Concat, TFn, TSources...>>
  : public MultiVisitorSetIterator<IteratorTupleVisitor, ArgConstness, OperatorType::Concat, TFn, TSources...> {
public:
  using Base = MultiVisitorSetIterator<IteratorTupleVisitor, ArgConstness, OperatorType::Concat, TFn, TSources...>;

  using CommonType = typename Base::AdjustedCommonType;

  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, CommonType, cinq::utility::SourceType::Iterator>;
  using value_type = typename std::decay_t<ResultType>;

  OperatorSpecializedIterator() {}

  OperatorSpecializedIterator(typename Base::Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : Base(enumerable, is_past_the_end_iteratorator) {}

  ResultType operator*() const {
    return Base::visitor.Visit([](auto &&x) -> ResultType {
        if constexpr (std::is_convertible_v<decltype(x), ResultType>)
          return std::forward<decltype(x)>(x);
        else
          return static_cast<ResultType>(std::forward<decltype(x)>(x));
      });
  }

  OperatorSpecializedIterator &operator++() {
    FindNextValid();
    return *this;
  }

  OperatorSpecializedIterator operator++(int) {
    OperatorSpecializedIterator previous(*this);
    ++*this;
    return previous;
  }

protected:
  void FindNextValid() override {
    Base::visitor.MoveToNext(Base::enumerable_->GetSourceTuple());
  }
};

template <bool ArgConstness, bool RetConstness, class TFn, class TSource>
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<OperatorType::Distinct, TFn, TSource>> {
public:
  using Enumerable = BasicEnumerable<OperatorType::Distinct, TFn, TSource>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;
  using SourceIteratorYieldType = decltype(*std::declval<SourceIterator>());

  using FunctionObjectArgumentType = cinq::utility::transform_to_function_object_argument_t<ArgConstness, SourceIteratorYieldType>;

  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, SourceIteratorYieldType, cinq::utility::SourceType::Iterator>;
  using value_type = std::decay_t<ResultType>;

  OperatorSpecializedIterator() : first_(), last_() {}

  OperatorSpecializedIterator(Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable), is_past_the_end_iteratorator_(is_past_the_end_iteratorator) {
    FindNextValideElement();
  }

  OperatorSpecializedIterator(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator(OperatorSpecializedIterator &&) = default;

  OperatorSpecializedIterator &operator=(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator &operator=(OperatorSpecializedIterator &&) = default;

  ResultType operator*() const {
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
    while (first_ != last_ && !distinct_helper_(static_cast<FunctionObjectArgumentType>(*first_)))
      ++first_;
  }

  Enumerable *enumerable_ = nullptr;

  bool is_past_the_end_iteratorator_ = false;

  class DistinctHelper {
    using InternalStorageType = std::conditional_t<std::is_reference_v<ResultType>,
      cinq::utility::ReferenceWrapper<value_type>,
      value_type>;

  public:
    bool operator()(const InternalStorageType &t) const { return distinct_set_.insert(t).second; };

  private:
    using SetType = std::conditional_t<cinq::utility::ReferenceWrapper<value_type>::hash_version,
      std::unordered_set<InternalStorageType>,std::set<InternalStorageType>>;

    mutable SetType distinct_set_;
  } distinct_helper_;

  static_assert(concept::PredicateCheck<DistinctHelper, FunctionObjectArgumentType>(), "(Internal error) Bad predicate");

  SourceIterator first_ = is_past_the_end_iteratorator_ ? std::end(enumerable_->SourceFront()) : std::begin(enumerable_->SourceFront());
  SourceIterator last_ = std::end(enumerable_->SourceFront());
};

} // namespace cinq::detail
