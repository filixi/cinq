#pragma once

#include "enumerable-source.h"
#include "operator-category.h"
#include "operator-specialized-iterator.h"
#include "../utility.h"

#include <variant>

namespace cinq_v3::detail {
template <bool ConstVersion, class TEnumerable>
class Cinq;

template <class T>
struct is_cinq : std::false_type {};
template <bool ConstVersion, class TE>
struct is_cinq<Cinq<ConstVersion, TE>> : std::true_type {};
template <class T>
inline constexpr bool is_cinq_v = is_cinq<T>::value;

template <bool ConstVersion, EnumerableCategory Category, OperatorType Operator, class TFn, class... TSources>
class Enumerable;

template <class>
class is_enumerable : std::false_type {};
template <bool ConstVersion, EnumerableCategory Category, OperatorType Operator, class TFn, class... TSources>
class is_enumerable<Enumerable<ConstVersion, Category, Operator, TFn, TSources...>> : std::true_type {};
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
  auto &GetSource() {
    return std::get<index>(sources_);
  }

  auto &SourceFront() {
    return GetSource<0>();
  }

  template <size_t index>
  using SourceIterator = typename std::tuple_element_t<index, std::tuple<TSources...>>::ResultIterator;

  using CommonType = std::common_type_t<decltype(*std::declval<typename TSources::ResultIterator>())...>;

  using IteratorTuple = std::tuple<typename TSources::ResultIterator...>;

  std::tuple<TSources...> &GetSourceTuple() {
    return sources_;
  }

private:
  template <size_t... index>
  IteratorTuple GetBeginIteratorTuple(std::index_sequence<index...>) const {
    return std::make_tuple(std::begin(GetSource<index>())...);
  }

  template <size_t... index>
  IteratorTuple GetEndIteratorTuple(std::index_sequence<index...>) const {
    return std::make_tuple(std::end(GetSource<index>())...);
  }

  std::tuple<TSources...> sources_;
};

template <>
class MultipleSources<void> {
public:
  constexpr size_t SourcesSize() const { return 0; }
};

template <EnumerableCategory Category, OperatorType Operator, class TFn, class... TSources>
class BasicEnumerable : protected MultipleSources<TSources...> {
protected:
  friend class OperatorSpecializedIterator<true, true, BasicEnumerable>;
  friend class OperatorSpecializedIterator<true, false, BasicEnumerable>;
  friend class OperatorSpecializedIterator<false, true, BasicEnumerable>;
  friend class OperatorSpecializedIterator<false, false, BasicEnumerable>;
  friend class MultiVisitorSetIterator<true, Operator, TFn, TSources...>;
  friend class MultiVisitorSetIterator<false, Operator, TFn, TSources...>;

public:
  template <class Fn>
  BasicEnumerable(Fn &&fn, TSources&&... sources)
    : MultipleSources<TSources...>(std::move(sources)...), fn_(std::forward<Fn>(fn)) {}

  template <bool ArgConstness, bool RetConstness>
  class Iterator : public OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<Category, Operator, TFn, TSources...>> {
    using base = OperatorSpecializedIterator<ArgConstness, RetConstness, BasicEnumerable<Category, Operator, TFn, TSources...>>;
  public:
    Iterator() {}

    Iterator(BasicEnumerable *e, bool is_past_end_iterator) : base(e, is_past_end_iterator) {}

    Iterator(const Iterator &) = default;
    Iterator(Iterator &&) = default;

    ~Iterator() = default;

    Iterator &operator=(const Iterator &) = default;
    Iterator &operator=(Iterator &&) = default;

    // value_type shall never be referece
    static_assert(!std::is_reference_v<typename base::value_type>);

    using difference_type = std::ptrdiff_t;
    using reference = std::add_lvalue_reference_t<typename base::value_type>;
    using pointer = std::add_pointer_t<typename base::value_type>;
    using iterator_category = std::input_iterator_tag;
  };

private:
  mutable TFn fn_;
};

template <bool ConstVersion, EnumerableCategory Category, OperatorType Operator, class TFn, class... TSources>
class Enumerable : private BasicEnumerable<Category, Operator, TFn, TSources...> {
  using base = BasicEnumerable<Category, Operator, TFn, TSources...>;
public:
  template <class Fn>
  Enumerable(Fn &&fn, TSources&&... sources)
    : base(std::forward<Fn>(fn), std::forward<TSources>(sources)...) {}

  using ResultIterator = typename base::template Iterator<ConstVersion, ConstVersion>;
  using ConstResultIterator = typename base::template Iterator<ConstVersion, true>;

  ResultIterator begin() {
    return ResultIterator(this, false); // not the past-the-end iterator
  }

  ResultIterator end() {
    return ResultIterator(this, true); // past-the-end iterator
  }

  ConstResultIterator cbegin() {
    return ConstResultIterator(this, false); // not the past-the-end iterator
  }

  ConstResultIterator cend() {
    return ConstResultIterator(this, true); // past-the-end iterator
  }
};

} // namespace cinq_v3::detail
