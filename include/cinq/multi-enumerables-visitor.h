#pragma once

#include <cstddef>
#include <exception>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace cinq::detail {
template <OperatorType Operator, class TFn, class... TSources>
class BasicEnumerable;

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

  using Enumerable = BasicEnumerable<Operator, TFn, TSources...>;

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

} // namespace cinq::detail
