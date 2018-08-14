#pragma once

#ifndef CINQV1
#define CINQV1
#endif // !CINQV1


#if defined(CINQV2) || defined(CINQV3) || defined(CINQV4) || defined(CINQV5)
#error Different versions of cinq cannot coexiste
#endif

#include <type_traits>

namespace cinq {

template <class TContainer>
auto Cinq(TContainer &&container);

namespace detail {
template <class TEnumerable>
class Cinq {
private:
  using value_type = decltype(*std::begin(std::declval<TEnumerable>()));

  // adjusted_value_type guaranted to be equally or more const qualified than value_type
  using adjusted_value_type = std::conditional_t<
      std::is_reference_v<TEnumerable>,
      value_type &,
      const std::decay_t<value_type> &>;

public:
  template <class T>
  Cinq(T &&enumerable) : enumerable_(std::forward<T>(enumerable)) {}

  Cinq(const Cinq &) = delete;
  Cinq(Cinq &&) = default;

  Cinq &operator=(const Cinq &) = delete;
  Cinq &operator=(Cinq &&) = default;

  auto begin() {
    return std::begin(enumerable_);
  }

  auto end() {
    return std::end(enumerable_);
  }

  template <class Fn>
  auto SelectMany(Fn fn) {
    using result_type = decltype(*std::begin(fn(std::declval<adjusted_value_type>())));
    using adjusted_result_type = std::decay_t<value_type>;

    std::vector<adjusted_result_type> result;
    for (auto &v : enumerable_) {
      decltype(auto) segemnt = fn(static_cast<adjusted_value_type>(v));
      if constexpr (std::is_reference_v<decltype(segemnt)>)
        std::copy(std::begin(segment), std::end(segment), std::back_inserter(result));
      else
        std::move(std::begin(segment), std::end(segment), std::back_inserter(result));
    }

    return cinq::Cinq(std::move(result));
  }

  template <class Fn>
  auto Where(Fn fn) {
    using result_type = std::reference_wrapper<const std::decay_t<value_type>>;

    std::vector<result_type> result;
    for (auto &v : enumerable_)
      if (fn(static_cast<adjusted_value_type>(v)))
        result.emplace_back(v);

    using adjusted_result_type = std::decay_t<value_type>;

    if constexpr (std::is_reference_v<TEnumerable>)
      return cinq::Cinq(std::vector<adjusted_result_type>(result.begin(), result.end()));
    else
      return cinq::Cinq(std::vector<adjusted_result_type>(
        std::make_move_iterator(result.begin()),
        std::make_move_iterator(result.end())));
  }

private:
  TEnumerable enumerable_;
};

} // namespace detail

template <class TContainer>
auto Cinq(TContainer &&container) {
  return detail::Cinq<std::decay_t<TContainer>>(std::forward<TContainer>(container));
}

template <class TContainer>
auto Cinq(std::reference_wrapper<TContainer> container) {
  return detail::Cinq<TContainer &>(container.get());
}

} // namespace cinq
