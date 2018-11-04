#pragma once

#include <cassert>
#include <functional>
#include <utility>
#include <vector>

namespace cinq_test {
template <class T>
struct VisitTupleTree {
  template <class Node, class Visitor, class LeafVisitor>
  static void Visit(Node &&node, Visitor &&, LeafVisitor &&leaf_visitor) {
    std::invoke(std::forward<LeafVisitor>(leaf_visitor), std::forward<Node>(node));
  }
};

template <class Node, class Visitor, class LeafVisitor, size_t... indexs>
void VisitChild(Node &&node, Visitor &&visitor, LeafVisitor &&leaf_visitor, std::index_sequence<indexs...>);

template <class... Ts>
struct VisitTupleTree<std::tuple<Ts...>> {
  template <class Node, class Visitor, class LeafVisitor>
  static void Visit(Node &&node, Visitor &&visitor, LeafVisitor &&leaf_visitor) {
    std::invoke(std::forward<Visitor>(visitor), std::forward<Node>(node));
    VisitChild(std::forward<Node>(node), std::forward<Visitor>(visitor), std::forward<LeafVisitor>(leaf_visitor), std::index_sequence_for<Ts...>{});
  }
};

template <class Node, class Visitor, class LeafVisitor, size_t... indexs>
void VisitChild(Node &&node, Visitor &&visitor, LeafVisitor &&leaf_visitor, std::index_sequence<indexs...>) {
  (VisitTupleTree<std::decay_t<std::tuple_element_t<indexs, std::decay_t<Node>>>>::Visit(
    std::get<indexs>(std::forward<Node>(node)), std::forward<Visitor>(visitor), std::forward<LeafVisitor>(leaf_visitor)), ...);
}

struct LifeTimeCheckInt {
  LifeTimeCheckInt(int v) noexcept : value_(v) {}

  LifeTimeCheckInt(const LifeTimeCheckInt &v) noexcept : value_(v.value_) { assert(v.is_valid_); }
  LifeTimeCheckInt(LifeTimeCheckInt &&v) noexcept : value_(v.value_) { assert(v.is_valid_); v.is_valid_ = false; }

  ~LifeTimeCheckInt() noexcept { assert(!is_destoryed_); is_valid_ = false; is_destoryed_ = true; }

  LifeTimeCheckInt &operator=(const LifeTimeCheckInt &v) noexcept {
    assert(v.is_valid_ && !is_destoryed_);

    value_ = v.value_;
    is_valid_ = true;
    return *this;
  }
  LifeTimeCheckInt &operator=(LifeTimeCheckInt &&v) noexcept {
    assert(v.is_valid_ && !is_destoryed_);
    v.is_valid_ = false;

    value_ = v.value_;
    is_valid_ = true;
    return *this;
  }

  operator int &() noexcept {
    assert(is_valid_);
    return value_;
  }
  operator const int &() const noexcept {
    assert(is_valid_);
    return value_;
  }

  bool operator<(const int &rhs) const noexcept {
    return int(*this) < rhs;
  }
  bool operator==(const int &rhs) const noexcept {
    return int(*this) == rhs;
  }

  int value_;
  bool is_valid_ = true;
  bool is_destoryed_ = false;
};

const std::vector<LifeTimeCheckInt> empty_source;
const std::vector<LifeTimeCheckInt> one_element{ 0 };
const std::vector<LifeTimeCheckInt> five_elements{ 0, 1, 2, 3, 4 }; // elements must be unique

} // namespace cinq_test

namespace std {
template <>
struct hash<cinq_test::LifeTimeCheckInt> : hash<int> {};

} // namespace std
