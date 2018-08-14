#pragma once

#include "enumerable-source.h"
#include "operator-category.h"
#include "operator-specialized-iterator.h"
#include "../utility.h"

namespace cinq_v3 {

// For each category/operator, specialize this class to provide Enumerable implementation when necessary.
template <class TSource, class TFn, EnumerableCategory Category, OperatorType Operator, class... TRestSources>
class Enumerable;

template <class TSource, class TFn, OperatorType Operator>
class Enumerable<TSource, TFn, EnumerableCategory::Producer, Operator> {
protected:
  static_assert(!cinq::utility::is_reference_wrapper_v<TSource>);
  static_assert(!std::is_rvalue_reference_v<TSource>);

  friend class OperatorSpecializedIterator<Enumerable>;

public:
  using SourceIterator = typename std::decay_t<TSource>::ResultIterator;

  template <class Source, class Fn>
  Enumerable(Source &&source, Fn &&fn)
    : source_(std::move(source)), fn_(std::forward<Fn>(fn)) {
    static_assert(std::is_rvalue_reference_v<Source &&>);
  }

  class Iterator : public OperatorSpecializedIterator<Enumerable> {
  public:
    Iterator(const Enumerable *e, bool is_past_end_iterator) : OperatorSpecializedIterator<Enumerable>(e, is_past_end_iterator) {}

    Iterator(const Iterator &) = default;
    Iterator(Iterator &&) = default;

    ~Iterator() = default;

    Iterator &operator=(const Iterator &) = default;
    Iterator &operator=(Iterator &&) = default;

    using difference_type = std::ptrdiff_t;
    using reference = std::add_lvalue_reference_t<typename OperatorSpecializedIterator<Enumerable>::value_type>;
    using pointer = std::add_pointer_t<typename OperatorSpecializedIterator<Enumerable>::value_type>;
    using iterator_category = std::input_iterator_tag;
  };

  using ResultIterator = Iterator;

  static_assert(!std::is_polymorphic_v<Iterator>, "Iterator should not be polymorphic.");
  static_assert(sizeof(Iterator) == sizeof(OperatorSpecializedIterator<Enumerable>), "Iterator should not introduce any non static member variable.");

  Iterator begin() const {
    return Iterator(this, false); // not the past end iterator
  }

  Iterator end() const {
    return Iterator(this, true); // past end iterator
  }

protected:
  TSource &GetSource() {
    return source_;
  }

  TSource source_;
  TFn fn_;
};

template <class TSource, class TFn, class TSource2>
class Enumerable<TSource, TFn, EnumerableCategory::Producer, OperatorType::Join, TSource2> {
protected:
  static_assert(!cinq::utility::is_reference_wrapper_v<TSource>);
  static_assert(!std::is_rvalue_reference_v<TSource>);

  friend class OperatorSpecializedIterator<Enumerable>;

public:
  using SourceIterator = typename std::decay_t<TSource>::ResultIterator;
  using SourceIterator2 = typename std::decay_t<TSource2>::ResultIterator;

  template <class Source, class Source2>
  Enumerable(Source &&source, Source2 &&source2, TFn &&fn)
    : source_(std::move(source)), source2_(std::move(source2)), result_selector_(std::move(fn)) {
    static_assert(std::is_rvalue_reference_v<Source &&>);
    static_assert(std::is_rvalue_reference_v<Source2 &&>);
  }

  class Iterator : public OperatorSpecializedIterator<Enumerable> {
  public:
    Iterator(const Enumerable * e, bool is_past_end_iterator) : OperatorSpecializedIterator<Enumerable>(e, is_past_end_iterator) {}

    Iterator(const Iterator &) = default;
    Iterator(Iterator &&) = default;

    ~Iterator() = default;

    Iterator &operator=(const Iterator &) = default;
    Iterator &operator=(Iterator &&) = default;

    using difference_type = std::ptrdiff_t;
    using reference = std::add_lvalue_reference_t<typename OperatorSpecializedIterator<Enumerable>::value_type>;
    using pointer = std::add_pointer_t<typename OperatorSpecializedIterator<Enumerable>::value_type>;
    using iterator_category = std::input_iterator_tag;
  };

  using ResultIterator = Iterator;

  static_assert(!std::is_polymorphic_v<Iterator>, "Iterator should not be polymorphic.");
  static_assert(sizeof(Iterator) == sizeof(OperatorSpecializedIterator<Enumerable>), "Iterator should not introduce any non static member variable.");

  Iterator begin() const {
    return Iterator(this, false); // not the past end iterator
  }

  Iterator end() const {
    return Iterator(this, true); // past end iterator
  }

protected:
  TSource source_;
  TSource2 source2_;
  TFn result_selector_;
};

template <class TSource, class TFn, OperatorType Operator>
class Enumerable<TSource, TFn, EnumerableCategory::Subrange, Operator> {
private:
  static_assert(!cinq::utility::is_reference_wrapper_v<TSource>);
  static_assert(!std::is_rvalue_reference_v<TSource>);

  friend class OperatorSpecializedIterator<Enumerable>;

public:
  using SourceIterator = typename std::decay_t<TSource>::ResultIterator;

  template <class Source, class Fn>
  Enumerable(Source &&source, Fn &&fn)
    : source_(std::move(source)), fn_(std::forward<Fn>(fn)) {
    static_assert(std::is_rvalue_reference_v<Source &&>);
  }

  Enumerable(const Enumerable &) = delete;
  Enumerable(Enumerable &&) = default;

  Enumerable &operator=(const Enumerable &) = delete;
  Enumerable &operator=(Enumerable &&) = default;

  class Iterator : public OperatorSpecializedIterator<Enumerable> {
  public:
    Iterator(const Enumerable *e, bool is_past_end_iterator) : OperatorSpecializedIterator<Enumerable>(e, is_past_end_iterator) {}

    Iterator(const Iterator &) = default;
    Iterator(Iterator &&) = default;

    ~Iterator() = default;

    Iterator &operator=(const Iterator &) = default;
    Iterator &operator=(Iterator &&) = default;

    using difference_type = std::ptrdiff_t;
    using reference = std::add_lvalue_reference_t<typename OperatorSpecializedIterator<Enumerable>::value_type>;
    using pointer = std::add_pointer_t<typename OperatorSpecializedIterator<Enumerable>::value_type>;
    using iterator_category = std::input_iterator_tag;
  };

  using ResultIterator = Iterator;
  
  static_assert(!std::is_polymorphic_v<Iterator>, "Iterator should not be polymorphic.");
  static_assert(sizeof(Iterator) == sizeof(OperatorSpecializedIterator<Enumerable>), "Iterator should not introduce any non static member variable.");

  Iterator begin() const {
    return Iterator(this, false);
  }

  Iterator end() const {
    return Iterator(this, true);
  }

  void SetSource(const std::decay_t<TSource> *source) {
    source_ = source;
  }

private:
  TSource source_;
  TFn fn_;
};

} // namespace cinq_v3
