#pragma once

#include "enumerable-source.h"
#include "operator-category.h"
#include "operator-specialized-iterator.h"
#include "../utility.h"

namespace cinq_v3::detail {

template <class TEnumerable>
class Cinq;

template <class T>
struct is_cinq : std::false_type {};
template <class... TE>
struct is_cinq<Cinq<TE...>> : std::true_type {};
template <class T>
inline constexpr bool is_cinq_v = is_cinq<T>::value;

template <EnumerableCategory Category, OperatorType Operator, class TFn, class... TSources>
class Enumerable;

template <class>
class is_enumerable : std::false_type {};
template <EnumerableCategory Category, OperatorType Operator, class TFn, class... TSources>
class is_enumerable<Enumerable<Category, Operator, TFn, TSources...>> : std::true_type {};
template <class T>
inline constexpr bool is_enumerable_v = is_enumerable<T>::value;

template <class... TSources>
class MultipleSources {
public:
  static_assert(cinq::utility::right_fold_and_v<(!cinq::utility::is_reference_wrapper_v<TSources>)...>);
  static_assert(cinq::utility::right_fold_and_v<(!std::is_reference_v<TSources>)...>);
  static_assert(cinq::utility::right_fold_and_v<(is_enumerable_source_v<TSources> || is_cinq_v<TSources> || is_enumerable_v<TSources>)...>);

  MultipleSources(std::decay_t<TSources>&&... sources)
    : sources_(std::move(sources)...) {}

  constexpr size_t SourcesSize() const {
    return sizeof...(TSources);
  }

  template <size_t index>
  auto &GetSource() const {
    return std::get<index>(sources_);
  }

  auto &SourceFront() const {
    return GetSource<0>();
  }

  template <size_t index>
  using SourceIterator = typename std::tuple_element_t<index, std::tuple<TSources...>>::ResultIterator;

private:
  std::tuple<TSources...> sources_;
};

template <>
class MultipleSources<void> {
public:
  constexpr size_t SourcesSize() const { return 0; }
};

template <EnumerableCategory Category, OperatorType Operator, class TFn, class... TSources>
class Enumerable : private MultipleSources<TSources...> {
protected:
  friend class OperatorSpecializedIterator<Enumerable>;

public:
  template <class Fn>
  Enumerable(Fn &&fn, TSources&&... sources)
    : MultipleSources<TSources...>(std::move(sources)...), fn_(std::forward<Fn>(fn)) {}

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

  static_assert(sizeof(Iterator) == sizeof(OperatorSpecializedIterator<Enumerable>), "Iterator should not introduce any non static member variable.");

  Iterator begin() const {
    return Iterator(this, false); // not the past end iterator
  }

  Iterator end() const {
    return Iterator(this, true); // past end iterator
  }

private:
  TFn fn_;
};

} // namespace cinq_v3::detail
