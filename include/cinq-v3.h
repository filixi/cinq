#pragma once

#ifndef CINQV3
#define CINQV3
#endif // !CINQV3

#if defined(CINQV1) || defined(CINQV2) || defined(CINQV4) || defined(CINQV5)
#error Different versions of cinq cannot coexiste
#endif

#include "../cinq/cinq-v3/enumerable.h"
#include "../cinq/cinq-v3/enumerable-source.h"

#include <array>

namespace cinq_v3 {
namespace detail {
template <class... TEnumerables>
class Cinq {
public:
  using ResultIterator = typename std::tuple_element_t<sizeof...(TEnumerables) - 1, std::tuple<TEnumerables...>>::ResultIterator;

  template <class T>
  Cinq(T &&enumerables) : enumerables_(std::forward<T>(enumerables)) {
    ResetAllSource<sizeof...(TEnumerables) - 1>();
  }

  template <class T, size_t... indexs>
  Cinq(T &container, std::index_sequence<indexs...>)
    : enumerables_(std::tuple_element_t<0, std::tuple<TEnumerables...>>{container[indexs]...}) {
    ResetAllSource<sizeof...(TEnumerables) - 1>();
  }

  Cinq(const Cinq &) = delete;
  Cinq(Cinq &&c) : enumerables_(std::move(c.enumerables_)) {
    ResetAllSource<sizeof...(TEnumerables) - 1>();
  }

  Cinq &operator=(const Cinq &) = delete;
  Cinq &operator=(Cinq &&) = delete;

  auto begin() const {
    return std::cbegin(LastEnumerable());
  }

  auto end() const {
    return std::cend(LastEnumerable());
  }

  template <class Fn>
  auto Select(Fn fn) && {
    return Append(
      CreateEnumerable<EnumerableCategory::Producer, OperatorType::Select>(
        std::ref(LastEnumerable()), std::move(fn)), fn);
  }

  template <class Fn>
  auto SelectMany(Fn fn) && {
    return Append(
      CreateEnumerable<EnumerableCategory::Producer, OperatorType::SelectMany>(
        std::ref(LastEnumerable()), std::move(fn)), fn);
  }

  template <class Fn>
  auto Where(Fn fn) && {
    return Append(
      CreateEnumerable<EnumerableCategory::Subrange, OperatorType::Where>(
        std::ref(LastEnumerable()), std::move(fn)), fn);
  }

  template <class Inner, class OuterKeySelector, class InnerKeySelector, class ResultSelector>
  auto Join(Inner &&inner, OuterKeySelector outer_key_selector, InnerKeySelector &&inner_key_selector, ResultSelector &&result_selector) && {
    return std::move(*this).Select(outer_key_selector).Join<void>(
      std::forward<Inner>(inner),
      std::forward<InnerKeySelector>(inner_key_selector),
      std::forward<ResultSelector>(result_selector));
  }

  template <class, class Inner, class InnerKeySelector, class ResultSelector>
  auto Join(Inner &&inner, InnerKeySelector inner_key_selector, ResultSelector result_selector) && {
    auto fn = [](auto &&) {};
    return Append(
      CreateEnumerable<EnumerableCategory::Producer, OperatorType::Join>(
        std::ref(LastEnumerable()), 
        0, cinq_v3::Cinq(std::forward<Inner>(inner)).Select(std::move(inner_key_selector))),
        fn);
  }

  auto ToVector() && {
    using value_type = std::decay_t<typename std::decay_t<decltype(std::cbegin(*this))>::value_type>;
    return std::vector<value_type>(std::cbegin(*this), std::cend(*this));
  }

private:
  template <size_t current_index>
  void ResetAllSource() {
    if constexpr (current_index >= 1) {
      std::get<current_index>(enumerables_).SetSource(
          std::addressof(std::get<current_index-1>(enumerables_))
        );
      ResetAllSource<current_index-1>();
    }
  }

  template <class TEnumerable, class Fn>
  Cinq<TEnumerables..., TEnumerable> Append(TEnumerable &&enumerable, Fn &fn) {
    using CallableCheck = decltype(fn(*std::cbegin(LastEnumerable())));

    return Cinq<TEnumerables..., TEnumerable>(
      std::tuple_cat(std::move(enumerables_), std::make_tuple(std::move(enumerable))));
  }

  decltype(auto) LastEnumerable() const {
    return std::get<sizeof...(TEnumerables) - 1>(enumerables_);
  }

  std::tuple<TEnumerables...> enumerables_;
};

} // namespace detail


// User provided container:
// non reference_wrapper:
//  if array type: convert to std::array
//  else copy/move initialize
//
// reference_wrapper:
//  if array type: reference to array
//  else reference to the container.

template <class T>
struct is_cinq : std::false_type {};
template <class... TE>
struct is_cinq<detail::Cinq<TE...>> : std::true_type {};
template <class T>
constexpr bool is_cinq_v = is_cinq<T>::value;

template <class TEnumerable>
decltype(auto) Cinq(TEnumerable &&container) {
  if constexpr (is_cinq_v<std::decay_t<TEnumerable>>)
    return std::forward<TEnumerable &&>(container);
  else
    return detail::Cinq<EnumerableSource<std::decay_t<TEnumerable>>>(std::forward<TEnumerable>(container));
}

template <class T, size_t size>
auto Cinq(T (&container)[size]) {
  return detail::Cinq<EnumerableSource<std::array<T, size>>>(container, std::make_index_sequence<size>());
}

template <class T, size_t size>
auto Cinq(const T (&container)[size]) {
  return detail::Cinq<EnumerableSource<std::array<T, size>>>(container, std::make_index_sequence<size>());
}

template <class TEnumerable>
auto Cinq(std::reference_wrapper<TEnumerable> container) {
  return detail::Cinq<EnumerableSource<TEnumerable &>>(container.get());
}

} // namespace cinq_v3
