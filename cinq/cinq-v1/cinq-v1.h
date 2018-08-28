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

  template <class Fn> 
  auto Skip(Fn fn) {
    using result_type = std::reference_wrapper<const std::decay_t<value_type>>;

    std::vector<result_type> result;

    Fn index = 0;
    for (auto &v : enumerable_) {
      if (index < fn) {
        index++;
      } else {
        result.emplace_back(v);
      }
    }

    using adjusted_result_type = std::decay_t<value_type>;

    if constexpr (std::is_reference_v<TEnumerable>) //constexpr compile
      return cinq::Cinq(std::vector<adjusted_result_type>(result.begin(), result.end())); // copy
    else
      return cinq::Cinq(std::vector<adjusted_result_type>( // move
        std::make_move_iterator(result.begin()),
        std::make_move_iterator(result.end())));
  }

  template <class Fn>
  auto Take(Fn fn) {
    using result_type = std::reference_wrapper<const std::decay_t<value_type>>;

    std::vector<result_type> result;
    Fn index = 0;
    for (auto &v : enumerable_) {
        if (index < fn) {
            result.emplace_back(v);
            index++;
        }
    }

    using adjusted_result_type = std::decay_t<value_type>;

    if constexpr (std::is_reference_v<TEnumerable>) //constexpr compile
      return cinq::Cinq(std::vector<adjusted_result_type>(result.begin(), result.end())); // copy
    else
      return cinq::Cinq(std::vector<adjusted_result_type>( // move
        std::make_move_iterator(result.begin()),
        std::make_move_iterator(result.end())));

  }

  template <class Fn>
  auto TakeWhile(Fn fn) {
    using result_type = std::reference_wrapper<const std::decay_t<value_type>>;

    std::vector<result_type> result;

    for (auto &v : enumerable_) {
      if (fn(static_cast<adjusted_value_type>(std::forward<value_type>(v))))
        result.emplace_back(v);
      else break;
    }

    using adjusted_result_type = std::decay_t<value_type>;

    if constexpr (std::is_reference_v<TEnumerable>) //constexpr compile
      return cinq::Cinq(std::vector<adjusted_result_type>(result.begin(), result.end())); // copy
    else
      return cinq::Cinq(std::vector<adjusted_result_type>( // move
        std::make_move_iterator(result.begin()),
        std::make_move_iterator(result.end())));
  }

  template <class Fn>
  auto SkipWhile(Fn fn) {
    auto index = 0;
    for (auto &v : enumerable_) {
      if (fn(static_cast<adjusted_value_type>(std::forward<value_type>(v))))
        index++;
      else break;
    }

    using adjusted_result_type = std::decay_t<value_type>;

    if constexpr (std::is_reference_v<TEnumerable>) //constexpr compile
      return cinq::Cinq(std::vector<adjusted_result_type>(enumerable_.begin() + index, enumerable_.end())); // copy
    else
      return cinq::Cinq(std::vector<adjusted_result_type>( // move
        std::make_move_iterator(enumerable_.begin() + index),
        std::make_move_iterator(enumerable_.end())));
  }

  auto Distinct() {
    using adjusted_result_type = std::decay_t<value_type>;
    using result_type = std::reference_wrapper<const adjusted_result_type>;
    
    std::unordered_set<adjusted_result_type> temp_set;
    std::vector<result_type> result;

    for (auto &v : enumerable_) {
      if (temp_set.find(v) == temp_set.end()) {
        temp_set.insert(static_cast<adjusted_result_type>(std::forward<value_type>(v)));
        result.emplace_back(v);
      } 
    }

    if constexpr (std::is_reference_v<TEnumerable>) //constexpr compile
      return cinq::Cinq(std::vector<adjusted_result_type>(result.begin(), result.end())); // copy
    else
      return cinq::Cinq(std::vector<adjusted_result_type>( // move
        std::make_move_iterator(result.begin()),
        std::make_move_iterator(result.end())));
  }

  template <class Fn>
  auto First(Fn fn) {
    using adjusted_result_type = std::decay_t<value_type>;

    adjusted_result_type result = 0;

    bool have_result = false;
    for (auto &v : enumerable_) 
      if (fn(static_cast<adjusted_value_type>(std::forward<value_type>(v))))  {// static_cast  int & -> static_cast<const int &> -> const int & 
        result = v;
        have_result = true;
        break;
      }

    if (!have_result) throw std::invalid_argument("Don't have first element.");

    return result;
  }

  auto First() {
    if (std::begin(enumerable_) == std::end(enumerable_)) throw std::invalid_argument("Don't have first element.");
    using adjusted_result_type = std::decay_t<value_type>;
    adjusted_result_type result = enumerable_[0];
    return result;
  }

  // template <class Fn> // TODO
  // auto FirstOrDefault(Fn fn) { // default(TSource) is null for reference-type elements, false for the bool type, and zero for numeric types.
  // }

  auto FirstOrDefault() { // TODO
  }

  template <class Fn>
  auto Last(Fn fn) {
    using adjusted_result_type = std::decay_t<value_type>;
    adjusted_result_type result;
    bool have_result = false;
    for (auto &v : enumerable_) {
      if (fn(static_cast<adjusted_value_type>(std::forward<value_type>(v))))  {// static_cast  int & -> static_cast<const int &> -> const int & 
        result = v;
        have_result = true;
      }
    }

    if (!have_result) throw std::invalid_argument("Don't have last element.");

    return result;
  }

  auto Last() {
    using adjusted_result_type = std::decay_t<value_type>;

    if (std::begin(enumerable_) == std::end(enumerable_)) throw std::invalid_argument("Don't have first element.");

    adjusted_result_type result = *(std::end(enumerable_) - 1);
    
    return result;
  }

  // template <class Fn>
  // auto FirstOrDefault(Fn fn) { // TODO

  // }

  auto LastOrDefault() { // TODO
    
  }

  template <class Fn>
  auto Single(Fn fn) {
    using adjusted_result_type = std::decay_t<value_type>;
    adjusted_result_type result = 0;
    bool have_result = false;
    for (auto &v : enumerable_) 
      if (fn(static_cast<adjusted_value_type>(std::forward<value_type>(v))))  {// static_cast  int & -> static_cast<const int &> -> const int & 
        if (have_result) throw std::invalid_argument("Don't have single element to satisfy function.");
        result = v;
        have_result = true;
      }

    if (!have_result) throw std::invalid_argument("Don't have element to satisfy function.");

    return result;
  }

  template <class Fn>
  auto SingleOrDefault(Fn fn) { // TODO

  }

  auto ElementAt(size_t index) { // TODO size

    size_t size = std::distance(begin(), end());

    if (size <= index) throw std::invalid_argument("Don't have element to satisfy function.");

    return enumerable_[index];
  }

  auto ElementAtOrDefault(size_t index) {
    using adjusted_result_type = std::decay_t<value_type>;
    adjusted_result_type result = enumerable_[index];
    return result;
  }

  auto DefaultIsEmpty() { // TODO
    using adjusted_result_type = std::decay_t<value_type>;

    if constexpr (std::is_reference_v<TEnumerable>) //constexpr compile
      return cinq::Cinq(std::vector<adjusted_result_type>(enumerable_.begin(), enumerable_.end())); // copy
    else
      return cinq::Cinq(std::vector<adjusted_result_type>( // move
        std::make_move_iterator(enumerable_.begin()),
        std::make_move_iterator(enumerable_.end())));
  }

  template <class Fn>
  int Count(Fn fn) {
    int result = 0;
    for (auto &v : enumerable_)
      if (fn(static_cast<adjusted_value_type>(std::forward<value_type>(v))))  // static_cast  int & -> static_cast<const int &> -> const int & 
        result++;
    return result;
  }

  template <class Fn>
  int64_t LongCount(Fn fn) {
    int64_t result = 0;
    for (auto &v : enumerable_)
      if (fn(static_cast<adjusted_value_type>(std::forward<value_type>(v))))  // static_cast  int & -> static_cast<const int &> -> const int & 
        result++;
    return result;
  }

  int Count() {
    return std::distance(begin(), end());
  }

  int64_t LongCount() {
    return std::distance(begin(), end());
  }

  template <class Fn>
  auto Min(Fn fn) {
    using adjusted_result_type = std::decay_t<value_type>;
    if (std::begin(enumerable_) == std::end(enumerable_)) throw std::invalid_argument("Container is empty.");
    adjusted_result_type result;
    bool flag = false;
    for (auto &v : enumerable_)
      if (fn(static_cast<adjusted_value_type>(std::forward<value_type>(v))))  {// static_cast  int & -> static_cast<const int &> -> const int & 
        if (flag == false) {
          result = v;
          flag = true;
        }
        else result = std::min(result, v);
      }
    return result;
  }

  auto Min() {
    using adjusted_result_type = std::decay_t<value_type>;
    if (std::begin(enumerable_) == std::end(enumerable_)) throw std::invalid_argument("Container is empty.");
    adjusted_result_type result;
    bool flag = false;
    for (auto &v : enumerable_)
      if (flag == false) {
        result = v;
        flag = true;
      }
      else result = std::min(result, v);
    return result;
  }

  template <class Fn>
  auto Max(Fn fn) {
    using adjusted_result_type = std::decay_t<value_type>;
    if (std::begin(enumerable_) == std::end(enumerable_)) throw std::invalid_argument("Container is empty.");
    adjusted_result_type result;
    bool flag = false;
    for (auto &v : enumerable_)
      if (fn(static_cast<adjusted_value_type>(std::forward<value_type>(v)))) {// static_cast  int & -> static_cast<const int &> -> const int & 
        if (flag == false) {
          result = v;
          flag = true;
        }
        else result = std::max(result, v);

      }
    return result;
  }

  auto Max() {
    using adjusted_result_type = std::decay_t<value_type>;
    if (std::begin(enumerable_) == std::end(enumerable_)) throw std::invalid_argument("Container is empty.");
    adjusted_result_type result;
    bool flag = false;
    for (auto &v : enumerable_) {
      if (flag == false) {
        result = v;
        flag = true;
      }
      else result = std::max(result, v);
    }
    return result;
  }

  auto Sum() {
    using adjusted_result_type = std::decay_t<value_type>;
    if (std::begin(enumerable_) == std::end(enumerable_)) throw std::invalid_argument("Container is empty.");
    adjusted_result_type result = enumerable_[0];
    for (auto it = std::begin(enumerable_) + 1; it < std::end(enumerable_); ++it) {
      result += *it;
    }
    return result;
  }

  template <class Fn>
  auto Sum(Fn fn) {
    using adjusted_result_type = std::decay_t<value_type>;
    if (std::begin(enumerable_) == std::end(enumerable_)) throw std::invalid_argument("Container is empty.");
    adjusted_result_type result = enumerable_[0] - enumerable_[0];
    for (auto it = std::begin(enumerable_) + 1; it < std::end(enumerable_); ++it) {
      if (fn(static_cast<adjusted_value_type>(std::forward<value_type>(*it))))  result += *it;
    }
    return result;
  }

  auto Average() { // TODO
    using adjusted_result_type = std::decay_t<value_type>;
    if (std::begin(enumerable_) == std::end(enumerable_)) throw std::invalid_argument("Container is empty.");
    adjusted_result_type result = enumerable_[0];
    auto size = 0;
    for (auto it = std::begin(enumerable_) + 1; it < std::end(enumerable_); ++it) {
      result += *it;
    }
    return result / size;
  }

  // template <class Fn>
  // auto Average(Fn fn) { // TODO
  // }

  // template <class Fn>
  // auto Aggregate(std::decay_t<value_type> seed = 0, Fn fn) {
  //   using adjusted_result_type = std::decay_t<value_type>;
  //   if (std::begin(enumerable_) == std::end(enumerable_)) throw std::invalid_argument("Container is empty.");
  //   adjusted_result_type result = seed;
  //   for (auto it = std::begin(enumerable_); it < std::end(enumerable_); ++it) {
  //     auto temp = fn(static_cast<adjusted_value_type>(std::forward<value_type>(*it)));
  //     result += temp;
  //   }
  //   return result;
  // }

  template <class Fn>
  auto Aggregate(Fn fn) {
    using adjusted_result_type = std::decay_t<value_type>;
    if (std::begin(enumerable_) == std::end(enumerable_)) throw std::invalid_argument("Container is empty.");
    adjusted_result_type result = enumerable_[0] - enumerable_[0];
    for (auto it = std::begin(enumerable_); it < std::end(enumerable_); ++it) {
      auto temp = fn(static_cast<adjusted_value_type>(std::forward<value_type>(*it)));
      result += temp;
    }
    return result;
  }

  bool Contains(const value_type target) {
    for (auto &v : enumerable_) {
      if (v == target) return true;
    }
    return false;
  }

  template <class Fn>
  bool Any(Fn fn) {
    for (auto &v : enumerable_) {
      if (fn(static_cast<adjusted_value_type>(std::forward<value_type>(v))))  return true;
    }
    return false;
  }

  template <class Fn>
  bool All(Fn fn) {
    for (auto &v : enumerable_) {
      if (!fn(static_cast<adjusted_value_type>(std::forward<value_type>(v))))  return false;
    }
    return true;

  }

  bool SequenceEqual(const TEnumerable& target) {
    auto it1 = std::begin(enumerable_), it2 = std::begin(target);
    for ( ; it1 < std::end(enumerable_) && it2 < std::end(target); ++it1, ++it2) {
      if (*it1 != *it2)  return false;
    }
    if (it1 == std::end(enumerable_) && it2 == std::end(target)) return true;
    return false;
  }

  auto Empty() { // TODO

  }

  auto Range() { // TODO

  }

  auto Repeat() { //TODO

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
