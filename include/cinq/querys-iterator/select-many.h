#pragma once

#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "../query-category.h"
#include "detail/concept.h"
#include "detail/utility.h"
#include "query-iterator-fwd.h"

namespace cinq::detail {
template <bool ArgConstness, bool RetConstness, class TFn, class TSource>
class QueryIterator<ArgConstness, RetConstness, BasicEnumerable<QueryCategory::SelectMany, std::tuple<TFn>, TSource>> {
public:
  using Enumerable = BasicEnumerable<QueryCategory::SelectMany, std::tuple<TFn>, TSource>;

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

      while (produced_first_ == std::end(*produced_enumerable_vtr_.back())) {
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

      produced_enumerable_vtr_.push_back(std::make_shared<ProducedEnumerableHolder>(enumerable_->FirstFn()(static_cast<FunctionObjectArgumentType>(*iter))));
      produced_first_ = std::begin(*produced_enumerable_vtr_.back());
      produced_last_ = std::end(*produced_enumerable_vtr_.back());
    }

    Enumerable *enumerable_;

    SourceIterator first_ = std::begin(enumerable_->SourceFront());
    SourceIterator last_ = std::end(enumerable_->SourceFront());

    std::vector<std::shared_ptr<ProducedEnumerableHolder>> produced_enumerable_vtr_;
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

  QueryIterator() {}

  QueryIterator(Enumerable *enumerable, bool is_past_the_end_iterator) {
    if (is_past_the_end_iterator)
      iterator_ = std::make_unique<EndIterator>(enumerable);
    else
      iterator_ = std::make_unique<BeginIterator>(enumerable);
  }

  QueryIterator(const QueryIterator &rhs) : iterator_(rhs.iterator_->Clone()) {}
  QueryIterator(QueryIterator &&) = default;

  QueryIterator &operator=(const QueryIterator &rhs) {
    iterator_ = rhs.iterator_->Clone();
  }
  QueryIterator &operator=(QueryIterator &&) = default;

  ResultType operator*() const {
    return iterator_->operator*();
  }

  QueryIterator &operator++() {
    ++*iterator_;
    return *this;
  }
  QueryIterator operator++(int) {
    QueryIterator prev(*this);
    ++*iterator_;
    return prev;
  }

  friend bool operator!=(const QueryIterator &lhs, const QueryIterator &rhs) {
    return *lhs.iterator_ != *rhs.iterator_;
  }

  friend bool operator==(const QueryIterator &lhs, const QueryIterator &rhs) {
    return *lhs.iterator_ == *rhs.iterator_;
  }

private:
  std::unique_ptr<BaseIterator> iterator_;
};

} // namespace cinq::detail
