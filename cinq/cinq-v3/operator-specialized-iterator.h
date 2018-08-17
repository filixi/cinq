#pragma once

#include <cassert>

#include <iostream>
#include <memory>
#include <set>
#include <unordered_set>
#include <variant>

#include "operator-category.h"

namespace cinq_v3::detail {
template <EnumerableCategory Category, OperatorType Operator, class TFn, class... TSources>
class Enumerable;

template <class ResultType>
class BasicIterator {
public:
  using value_type = std::decay_t<ResultType>;

  virtual ~BasicIterator() = default;

  virtual bool IsPastTheEnd() const = 0;

  virtual ResultType operator*() const = 0;
  virtual const value_type *operator->() const = 0;

  virtual BasicIterator &operator++() = 0;

  virtual bool operator!=(const BasicIterator &rhs) const = 0;
  virtual bool operator==(const BasicIterator &rhs) final {
    return !this->operator!=(rhs);
  }

  virtual std::unique_ptr<BasicIterator> Clone() const = 0;
};

// For each category/Operator, specialize this class to provide Iterator implementation.
template <class TEnumerable>
class OperatorSpecializedIterator;

template <class TFn, class TSource>
class OperatorSpecializedIterator<Enumerable<EnumerableCategory::Producer, OperatorType::SelectMany, TFn, TSource>> {
public:
  using Enumerable = Enumerable<EnumerableCategory::Producer, OperatorType::SelectMany, TFn, TSource>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;

  using ProducedEnumerable = decltype(std::declval<TFn>()(*std::declval<SourceIterator>()));
  using ProducedIterator = decltype(std::cbegin(std::declval<ProducedEnumerable>()));

  using ResultType = decltype(*std::declval<ProducedIterator>());
  using value_type = std::decay_t<ResultType>;

  using BaseIterator = BasicIterator<ResultType>;

  class BeginIterator : public BaseIterator {
  public:
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

    BeginIterator(const Enumerable *enumerable)
      : enumerable_(enumerable) {
      InitalizeNewProducedEnumerable(first_);
    }

    BeginIterator(const BeginIterator &) = default;
    BeginIterator(BeginIterator &&) = default;

    BeginIterator &operator=(const BeginIterator &) = default;
    BeginIterator &operator=(BeginIterator &&) = default;
  
    BeginIterator &operator++() override {
      ++produced_first_;

      while (produced_first_ == std::cend(*produced_enumerable_)) {
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

    const value_type *operator->() const override {
      return std::addressof(*produced_first_);
    }

    bool operator!=(const BasicIterator<ResultType> &rhs) const override {
      const BeginIterator &lhs = *this;
      return !(lhs.IsPastTheEnd() && rhs.IsPastTheEnd()) &&
        (!rhs.IsPastTheEnd() || lhs.first_ != lhs.last_ && lhs.produced_first_ != lhs.produced_last_);
    }

    std::unique_ptr<BasicIterator<ResultType>> Clone() const override {
      return std::make_unique<BeginIterator>(*this);
    }

    bool IsPastTheEnd() const override { return false; }

  private:
    void InitalizeNewProducedEnumerable(SourceIterator &iter) {
      if (iter == std::cend(enumerable_->SourceFront()))
        return ;

      produced_enumerable_ = std::make_shared<ProducedEnumerableHolder>(enumerable_->fn_(*iter));
      produced_first_ = std::cbegin(*produced_enumerable_);
      produced_last_ = std::cend(*produced_enumerable_);
    }

    const Enumerable *enumerable_;

    SourceIterator first_ = std::cbegin(enumerable_->SourceFront());
    SourceIterator last_ = std::cend(enumerable_->SourceFront());

    std::shared_ptr<ProducedEnumerableHolder> produced_enumerable_;
    ProducedIterator produced_first_;
    ProducedIterator produced_last_;
  };

  class EndIterator : public BaseIterator {
  public:
    EndIterator(const Enumerable *) {}

    bool IsPastTheEnd() const override { return true; }

    ResultType operator*() const override { throw std::runtime_error("Dereferencing past-the-end iterator."); }
    const value_type *operator->() const override { throw std::runtime_error("Dereferencing past-the-end iterator."); }

    EndIterator &operator++() override { return *this; }

    bool operator!=(const BasicIterator<ResultType> &rhs) const override {
      const EndIterator &lhs = *this;
      return !(lhs.IsPastTheEnd() && rhs.IsPastTheEnd()) &&
        (!rhs.IsPastTheEnd() || rhs.operator!=(*this));
    }

    std::unique_ptr<BasicIterator<ResultType>> Clone() const override {
      return std::make_unique<EndIterator>(nullptr);
    }
  };

  OperatorSpecializedIterator(const Enumerable *enumerable, bool is_past_the_end_iterator) {
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

  decltype(auto) operator*() {
    return iterator_->operator*();
  }
  auto operator->() {
    return iterator_->operator->();
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

template <class TFn, class TSource>
class OperatorSpecializedIterator<Enumerable<EnumerableCategory::Producer, OperatorType::Select, TFn, TSource>> {
public:
  using Enumerable = Enumerable<EnumerableCategory::Producer, OperatorType::Select, TFn, TSource>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;

  using value_type = std::invoke_result_t<TFn, const typename SourceIterator::value_type &>;

  OperatorSpecializedIterator(const Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable),
      iterator_(is_past_the_end_iteratorator ? std::cend(enumerable_->SourceFront()) : std::cbegin(enumerable_->SourceFront())) {}

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

template <class TFn, class TSource, class TSource2>
class OperatorSpecializedIterator<Enumerable<EnumerableCategory::Producer, OperatorType::Join, TFn, TSource, TSource2>> {
public:
  using Enumerable = Enumerable<EnumerableCategory::Producer, OperatorType::Join, TFn, TSource, TSource2>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;
  using SourceIterator2 = typename Enumerable::template SourceIterator<1>;

  using value_type = std::tuple<
    cinq::utility::add_const_on_rvalue_reference_v<decltype(*std::declval<SourceIterator>())>,
    cinq::utility::add_const_on_rvalue_reference_v<decltype(*std::declval<SourceIterator>())>
  >;

  OperatorSpecializedIterator(const Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable), is_past_the_end_iteratorator_(is_past_the_end_iteratorator) {}

  OperatorSpecializedIterator(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator(OperatorSpecializedIterator &&) = default;

  OperatorSpecializedIterator &operator=(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator &operator=(OperatorSpecializedIterator &&) = default;

  value_type operator*() {
    return enumerable_->fn_(value_type(*first_, *first2_));
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
      new(&first2_) SourceIterator2(std::begin(enumerable_->template GetSource<1>()));
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

  SourceIterator first_ = is_past_the_end_iteratorator_ ? std::cend(enumerable_->GetSource<0>()) : std::cbegin(enumerable_->GetSource<0>());
  SourceIterator last_ = std::cend(enumerable_->GetSource<0>());

  SourceIterator2 first2_ = is_past_the_end_iteratorator_ ? std::cend(enumerable_->GetSource<1>()) : std::cbegin(enumerable_->GetSource<1>());
  SourceIterator2 last2_ = std::cend(enumerable_->GetSource<1>());
};

template <class TFn, class TSource>
class OperatorSpecializedIterator<Enumerable<EnumerableCategory::Subrange, OperatorType::Where, TFn, TSource>> {
public:
  using Enumerable = Enumerable<EnumerableCategory::Subrange, OperatorType::Where, TFn, TSource>;

  using SourceIterator = typename Enumerable::template SourceIterator<0>;

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

  const Enumerable *enumerable_;

  bool is_past_the_end_iteratorator_;

  SourceIterator first_ = is_past_the_end_iteratorator_ ? std::cend(enumerable_->SourceFront()) : std::cbegin(enumerable_->SourceFront());
  SourceIterator last_ = std::cend(enumerable_->SourceFront());
};

template <class... TSources>
class IteratorTupleVisitor {
  template <size_t depth = sizeof...(TSources)>
  struct EmplaceIteratorWrapper {
    static bool EmplaceIteratorAt(const std::tuple<TSources...> &enumerables, size_t index, IteratorTupleVisitor *p) {
      if (index + 1 == depth) {
        p->first_.template emplace<depth-1>(std::cbegin(std::get<depth-1>(enumerables)));
        p->last_.template emplace<depth-1>(std::cend(std::get<depth-1>(enumerables)));
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

  IteratorTupleVisitor(const std::tuple<TSources...> &enumerable) {
    bool has_value = EmplaceIteratorWrapper<>::EmplaceIteratorAt(enumerable, current_enumerable_++, this);

    auto is_end = [](auto &lhs, auto &rhs) -> bool {
      if constexpr (std::is_same_v<decltype(lhs), decltype(rhs)>)
        return lhs == rhs;
      throw std::runtime_error("IteratorTupleVisitor test end failure.");
    };

    while (has_value && std::visit(is_end, first_, last_))
      has_value = EmplaceIteratorWrapper<>::EmplaceIteratorAt(enumerable, current_enumerable_++, this);
    is_dereferenceable_ = has_value;
  }

  template <class Fn>
  decltype(auto) Visit(Fn &&fn) {
    return std::visit([&fn](auto &ite) {std::invoke(std::forward<Fn>(fn), *ite);}, first_);
  }

  bool MoveToNext(const std::tuple<TSources...> &enumerable) {
    std::visit([](auto &ite) {++ite;}, first_);

    auto is_end = [](auto &lhs, auto &rhs) -> bool {
      if constexpr (std::is_same_v<decltype(lhs), decltype(rhs)>)
        return lhs == rhs;
      throw std::runtime_error("IteratorTupleVisitor test end failure.");
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

template <class TFn, class... TSources>
class OperatorSpecializedIterator<Enumerable<EnumerableCategory::SetOperation, OperatorType::Intersect, TFn, TSources...>> {
public:
  static_assert(sizeof...(TSources) > 1);

  using Enumerable = Enumerable<EnumerableCategory::SetOperation, OperatorType::Intersect, TFn, TSources...>;
  
  using IteratorTuple = typename Enumerable::IteratorTuple;

  using value_type = typename Enumerable::template SourceIterator<0>::value_type;

  OperatorSpecializedIterator(const Enumerable *enumerable, bool is_past_the_end_iteratorator)
    : enumerable_(enumerable), is_past_the_end_iteratorator_(is_past_the_end_iteratorator) {
    if (!is_past_the_end_iteratorator)
      FindNextValid();  
  }

  OperatorSpecializedIterator(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator(OperatorSpecializedIterator &&) = default;

  OperatorSpecializedIterator &operator=(const OperatorSpecializedIterator &) = default;
  OperatorSpecializedIterator &operator=(OperatorSpecializedIterator &&) = default;

  const value_type &operator*() {
    return *current_;
  }

  friend bool operator!=(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    auto b = !(lhs == rhs);
    return b;
  }

  friend bool operator==(const OperatorSpecializedIterator &lhs, const OperatorSpecializedIterator &rhs) {
    return lhs.is_past_the_end_iteratorator_ && rhs.is_past_the_end_iteratorator_ ||
      rhs.is_past_the_end_iteratorator_ && !lhs.visitor.IsValid() ||
      lhs.is_past_the_end_iteratorator_ && !rhs.visitor.IsValid() ||
      lhs.visitor == rhs.visitor;
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

private:
  void FindNextValid() {
    assert(!is_past_the_end_iteratorator_);
    bool succeed = false;
    for (;;) {
      visitor.Visit([this, &succeed](auto &x) {
        if constexpr (std::is_convertible_v<decltype(x), value_type>)
          std::tie(current_, succeed) = values_.insert(x);
        else
          std::tie(current_, succeed) = values_.insert(static_cast<value_type>(x));
      });

      if (succeed)
        break;
      if (!visitor.MoveToNext(enumerable_->GetSourceTuple()))
        break;
    }
  }

  const Enumerable *enumerable_;

  bool is_past_the_end_iteratorator_;

  IteratorTupleVisitor<TSources...> visitor = is_past_the_end_iteratorator_ ? IteratorTupleVisitor<TSources...>{} : enumerable_->GetSourceTuple();

  using SetType = std::conditional_t<cinq::utility::is_hashable_v<value_type>,
    std::unordered_set<value_type>, std::set<value_type>>;

  SetType values_;
  typename SetType::iterator current_;
};

} // namespace cinq_v3::detail
