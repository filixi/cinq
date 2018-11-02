#pragma once

#include <cassert>

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "../cinq/concept.h"
#include "operator-category.h"

namespace cinq_v3::detail {
template <EnumerableCategory Category, OperatorType Operator, class TFn, class... TSources>
class BasicEnumerable;

// For each category/Operator, specialize this class to provide Iterator implementation.
template <bool ArgConstness, bool RetConstness, class TEnumerable>
class OperatorSpecializedIterator;

template <bool ArgConstness, bool RetConstness, class TFn, class TSource>
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<EnumerableCategory::Producer, OperatorType::SelectMany, TFn, TSource>> {
public:
  using Enumerable = BasicEnumerable<EnumerableCategory::Producer, OperatorType::SelectMany, TFn, TSource>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;
  using SourceYieldType = decltype(*std::declval<SourceIterator>());

  using FunctionObjectArgumentType = cinq::utility::transform_to_function_object_argument_t<ArgConstness, SourceYieldType>;

  using ProducedEnumerable = decltype(std::declval<TFn>()(std::declval<FunctionObjectArgumentType>()));
  using ProducedIterator = decltype(std::begin(std::declval<ProducedEnumerable &>()));
  using ProducedType = decltype(*std::declval<ProducedIterator>());

  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, ProducedType, cinq::utility::SourceType::Iterator>;
  using value_type = std::decay_t<ResultType>;

  static_assert(cinq_concept::SelectorCheck<TFn, FunctionObjectArgumentType>() &&
    cinq_concept::SelectManySelectorCheck<true, TFn, FunctionObjectArgumentType>(), "Bad selector");

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
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<EnumerableCategory::Producer, OperatorType::Select, TFn, TSource>> {
public:
  using Enumerable = BasicEnumerable<EnumerableCategory::Producer, OperatorType::Select, TFn, TSource>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;

  using SourceIteratorYieldType = decltype(*std::declval<SourceIterator>());
  using FunctionObjectArgumentType = cinq::utility::transform_to_function_object_argument_t<ArgConstness, SourceIteratorYieldType>;

  using FunctionObjectYieldType = std::invoke_result_t<TFn, FunctionObjectArgumentType>;
  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, FunctionObjectYieldType, cinq::utility::SourceType::FunctionObject>;
  using value_type = std::decay_t<ResultType>;

  static_assert(cinq_concept::SelectorCheck<TFn, FunctionObjectArgumentType>(), "Bad selector");

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
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<EnumerableCategory::Producer, OperatorType::Join, TFn, TSource, TSource2>> {
public:
  using Enumerable = BasicEnumerable<EnumerableCategory::Producer, OperatorType::Join, TFn, TSource, TSource2>;

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

  static_assert(cinq_concept::SelectorCheck<TFn, FunctionObjectArgumentTupleElementType, FunctionObjectArgumentTupleElementType2>(), "Bad selector");

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

  SourceIterator first_ = is_past_the_end_iteratorator_ ? std::end(enumerable_->GetSource<0>()) : std::begin(enumerable_->GetSource<0>());

  SourceIterator2 first2_ = std::begin(enumerable_->GetSource<1>());
  SourceIterator2 last2_ = std::end(enumerable_->GetSource<1>());
};

template <bool ArgConstness, bool RetConstness, class TFn, class TSource>
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<EnumerableCategory::Subrange, OperatorType::Where, TFn, TSource>> {
public:
  using Enumerable = BasicEnumerable<EnumerableCategory::Subrange, OperatorType::Where, TFn, TSource>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;
  using SourceIteratorYieldType = decltype(*std::declval<SourceIterator>());

  using FunctionObjectArgumentType = cinq::utility::transform_to_function_object_argument_t<ArgConstness, SourceIteratorYieldType>;

  using ResultType = cinq::utility::transform_to_result_type_t<RetConstness, SourceIteratorYieldType, cinq::utility::SourceType::Iterator>;
  using value_type = std::decay_t<ResultType>;

  static_assert(cinq_concept::PredicateCheck<TFn, FunctionObjectArgumentType>(), "Bad predicate");

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

template <bool ArgConstness, class... TSources>
class RoundRobinIteratorTupleVisitor {
  template <size_t... index>
  static auto GetFirst(std::tuple<TSources...> &enumerable, std::index_sequence<index...>) {
    return std::make_tuple(std::begin(std::get<index>(enumerable))...);
  }

  template <size_t... index>
  static auto GetLast(std::tuple<TSources...> &enumerable, std::index_sequence<index...>) {
    return std::make_tuple(std::end(std::get<index>(enumerable))...);
  }

public:
  RoundRobinIteratorTupleVisitor() {}

  RoundRobinIteratorTupleVisitor(std::tuple<TSources...> &enumerable)
    : first_(GetFirst(enumerable, std::index_sequence_for<TSources...>())),
      last_(GetLast(enumerable, std::index_sequence_for<TSources...>())){
    MoveToNext(enumerable);
  }

  template <class Fn>
  decltype(auto) Visit(Fn &&fn) {
    using Ret = std::invoke_result_t<Fn &&, decltype(*std::get<0>(first_))>;
    return Visitor<sizeof...(TSources)>::template Visit<Ret>([&fn](auto &first, auto &) -> decltype(auto) {
        return std::invoke(std::forward<Fn>(fn), *first);
      }, current_enumerable_, first_, last_);
  }

  bool MoveToNext() noexcept {
    const size_t start = current_enumerable_ = (current_enumerable_ + 1) % sizeof...(TSources);
    while (!Visitor<sizeof...(TSources)>::template Visit<bool>([this](auto &first, auto &last) -> bool {
        return first != last && (!is_touched_[current_enumerable_] || ++first != last);
      }, current_enumerable_, first_, last_)) {
      current_enumerable_ = (current_enumerable_ + 1) % sizeof...(TSources);
      if (current_enumerable_ == start)
        return is_dereferenceable_ = false;
    }

    is_dereferenceable_ = true;
    is_touched_[current_enumerable_] = true;
    return true;
  }

  bool MoveToNext(std::tuple<TSources...> &) noexcept {
    return MoveToNext();
  }

  bool IsValid() const {
    return is_dereferenceable_;
  }

  friend bool operator==(const RoundRobinIteratorTupleVisitor &lhs, const RoundRobinIteratorTupleVisitor &rhs) {
    return lhs.current_enumerable_ == rhs.current_enumerable_ &&
      lhs.is_dereferenceable_ == rhs.is_dereferenceable_ &&
      lhs.first_ == rhs.first_;
  }

private:
  template <size_t depth>
  struct Visitor {
    template <class Ret, class Fn>
    static Ret Visit(Fn &&fn, size_t target, std::tuple<typename TSources::ResultIterator...> &first, std::tuple<typename TSources::ResultIterator...> &last)  {
      if (depth - 1 == target)
        return std::invoke(std::forward<Fn>(fn), std::get<depth - 1>(first), std::get<depth - 1>(last));
      return Visitor<depth - 1>::template Visit<Ret>(std::forward<Fn>(fn), target, first, last);
    }
  };

  template <>
  struct Visitor<0> {
    template <class Ret, class Fn>
    static Ret Visit(Fn &&, size_t, std::tuple<typename TSources::ResultIterator...> &, std::tuple<typename TSources::ResultIterator...> &) {
      throw std::runtime_error("Round robin visitor internal failure");
      // return std::invoke(std::forward<Fn>(fn), std::get<0>(first), std::get<0>(last));
    }
  };

  std::tuple<typename TSources::ResultIterator...> first_;
  std::tuple<typename TSources::ResultIterator...> last_;
  bool is_touched_[sizeof...(TSources)] = {0};
  size_t current_enumerable_ = 0;

  bool is_dereferenceable_ = false;
};

template <bool ArgConstness, class... TSources>
class IteratorTupleVisitor {
  template <size_t depth = sizeof...(TSources)>
  struct EmplaceIteratorWrapper {
    static bool EmplaceIteratorAt(std::tuple<TSources...> &enumerables, size_t index, IteratorTupleVisitor *p) {
      if (index + 1 == depth) {
        p->first_.template emplace<depth-1>(std::begin(std::get<depth-1>(enumerables)));
        p->last_.template emplace<depth-1>(std::end(std::get<depth-1>(enumerables)));
        return true;
      }
      return EmplaceIteratorWrapper<depth-1>::EmplaceIteratorAt(enumerables, index, p);
    }
  };

  template <>
  struct EmplaceIteratorWrapper<0> {
    static bool EmplaceIteratorAt(const std::tuple<TSources...> &, size_t, IteratorTupleVisitor *) {
      return false;
    }
  };

public:
  IteratorTupleVisitor() {}

  IteratorTupleVisitor(std::tuple<TSources...> &enumerable) {
    bool has_value = EmplaceIteratorWrapper<>::EmplaceIteratorAt(enumerable, current_enumerable_++, this);

    auto is_end = [](auto &lhs, auto &rhs) -> bool {
      if constexpr (std::is_same_v<decltype(lhs), decltype(rhs)>)
        return lhs == rhs;
      else
        throw std::runtime_error("IteratorTupleVisitor test end failure.");
    };

    while (has_value && std::visit(is_end, first_, last_))
      has_value = EmplaceIteratorWrapper<>::EmplaceIteratorAt(enumerable, current_enumerable_++, this);
    is_dereferenceable_ = has_value;
  }

  template <class Fn>
  decltype(auto) Visit(Fn &&fn) const {
    return std::visit([&fn](auto &ite) -> decltype(auto) {
        return std::invoke(std::forward<Fn>(fn), *ite);
      },
      first_);
  }

  bool MoveToNext(std::tuple<TSources...> &enumerable) noexcept {
    std::visit([](auto &ite) {++ite;}, first_);

    auto is_end = [](auto &lhs, auto &rhs) -> bool {
      // first and last must be of the identical type
      // User-provided container's iterator is either isolated or checked to be identical.
      if constexpr (std::is_same_v<decltype(lhs), decltype(rhs)>)
        return lhs == rhs;
      else
        throw std::runtime_error("IteratorTupleVisitor's is_end internal error.");
    };

    bool has_value = true;
    while (has_value && std::visit(is_end, first_, last_))
      has_value = EmplaceIteratorWrapper<>::EmplaceIteratorAt(enumerable, current_enumerable_++, this);

    return is_dereferenceable_ = has_value;
  }

  bool IsValid() const {
    return is_dereferenceable_;
  }

  friend bool operator==(const IteratorTupleVisitor &lhs, const IteratorTupleVisitor &rhs) {
    return lhs.current_enumerable_ == rhs.current_enumerable_ &&
      lhs.is_dereferenceable_ == rhs.is_dereferenceable_ &&
      lhs.first_ == rhs.first_;
  }

private:
  std::variant<typename TSources::ResultIterator...> first_;
  std::variant<typename TSources::ResultIterator...> last_;
  size_t current_enumerable_ = 0;

  bool is_dereferenceable_ = false;
};

template <template <bool, class...> class InternalVisitor, bool ArgConstness, OperatorType Operator, class TFn, class... TSources>
class MultiVisitorSetIterator {
public:
  static_assert(sizeof...(TSources) > 1);

  using Enumerable = BasicEnumerable<EnumerableCategory::SetOperation, Operator, TFn, TSources...>;

  using IteratorYieldType = decltype(*std::declval<typename Enumerable::template SourceIterator<0>>());
  using CommonType = typename Enumerable::CommonType;
  using AdjustedCommonType = typename Enumerable::AdjustedCommonType;
  static constexpr bool is_all_reference_to_same = Enumerable::is_all_reference_to_same;
  using value_type = std::decay_t<CommonType>;

  MultiVisitorSetIterator() {}

  MultiVisitorSetIterator(const MultiVisitorSetIterator &) = default;
  MultiVisitorSetIterator(MultiVisitorSetIterator &&) = default;

  virtual ~MultiVisitorSetIterator() = default;

  MultiVisitorSetIterator &operator=(const MultiVisitorSetIterator &) = default;
  MultiVisitorSetIterator &operator=(MultiVisitorSetIterator &&) = default;

  friend bool operator!=(const MultiVisitorSetIterator &lhs, const MultiVisitorSetIterator &rhs) {
    return !(lhs == rhs);
  }

  friend bool operator==(const MultiVisitorSetIterator &lhs, const MultiVisitorSetIterator &rhs) {
    return lhs.is_past_the_end_iteratorator_ && rhs.is_past_the_end_iteratorator_ ||
      rhs.is_past_the_end_iteratorator_ && !lhs.visitor.IsValid() ||
      lhs.is_past_the_end_iteratorator_ && !rhs.visitor.IsValid() ||
      lhs.visitor == rhs.visitor;
  }

protected:
  MultiVisitorSetIterator(Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable), is_past_the_end_iteratorator_(is_past_the_end_iteratorator) {}

  virtual void FindNextValid() = 0;

  Enumerable *enumerable_ = nullptr;

  bool is_past_the_end_iteratorator_ = true;

  InternalVisitor<ArgConstness, TSources...> visitor = is_past_the_end_iteratorator_ ? InternalVisitor<ArgConstness, TSources...>{} : enumerable_->GetSourceTuple();
};

template <bool ArgConstness, bool RetConstness, class TFn, class... TSources>
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<EnumerableCategory::SetOperation, OperatorType::Union, TFn, TSources...>>
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
  typename SetType::iterator current_;
};

template <bool ArgConstness, bool RetConstness, class TFn, class... TSources>
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<EnumerableCategory::SetOperation, OperatorType::Intersect, TFn, TSources...>>
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
  typename MapType::iterator current_;
};

template <bool ArgConstness, bool RetConstness, class TFn, class... TSources>
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<EnumerableCategory::SetOperation, OperatorType::Concat, TFn, TSources...>>
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
class OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<EnumerableCategory::Subrange, OperatorType::Distinct, TFn, TSource>> {
public:
  using Enumerable = BasicEnumerable<EnumerableCategory::Subrange, OperatorType::Distinct, TFn, TSource>;

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

  static_assert(cinq_concept::PredicateCheck<DistinctHelper, FunctionObjectArgumentType>(), "(Internal error) Bad predicate");

  SourceIterator first_ = is_past_the_end_iteratorator_ ? std::end(enumerable_->SourceFront()) : std::begin(enumerable_->SourceFront());
  SourceIterator last_ = std::end(enumerable_->SourceFront());
};

} // namespace cinq_v3::detail
