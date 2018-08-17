#pragma once

#include <memory>
#include <type_traits>

namespace cinq {
namespace utility {
template <class T>
struct remove_reference_wrapper : std::false_type { using type = T; };
template <class T>
struct remove_reference_wrapper<std::reference_wrapper<T>> : std::true_type { using type = T; };
template <class T>
using remove_reference_wrapper_t = typename remove_reference_wrapper<T>::type;
template <class T>
inline constexpr bool is_reference_wrapper_v = remove_reference_wrapper<T>::value;

template <class T>
struct is_const_reference : std::false_type {};
template <class T>
struct is_const_reference<const T &> : std::true_type {};
template <class T>
struct is_const_reference<T &> : std::false_type {};
template <class T>
inline constexpr bool is_const_reference_v = is_const_reference<T>::value;

template <class T>
struct remove_smart_ptr : std::false_type { using type = T; };
template <class T>
struct remove_smart_ptr<std::shared_ptr<T>> : std::true_type { using type = T; };
template <class T>
struct remove_smart_ptr<std::unique_ptr<T>> : std::true_type { using type = T; };
template <class T>
using remove_smart_ptr_t = typename remove_smart_ptr<T>::type;
template <class T>
inline constexpr bool is_smart_ptr_v = remove_smart_ptr<T>::value;

template <bool... value>
constexpr bool right_fold_and() {
  return (value && ...);
}
template <bool... value>
inline constexpr bool right_fold_and_v = right_fold_and<value...>();

template <class Fn, class Arg1, class Arg2, class = std::invoke_result_t<Fn, Arg>>
constexpr bool is_callable(int) { return true; }
template <class Fn, class Arg1, class Arg2>
constexpr bool is_callable(...) { return false; }
template <class Fn, class Arg1, class Arg2>
inline constexpr bool is_callable_v = is_callable<Fn, Arg1, Arg2>(0);

template <class T>
using add_const_on_rvalue_reference_v = std::conditional_t<std::is_rvalue_reference_v<T>, const std::remove_reference_t<T> &, T>;

template <class T, class = void>
struct is_hashable : std::true_type {};
template <class T>
struct is_hashable<T, std::void_t<std::hash<T>>> : std::false_type {};
template <class T>
inline constexpr bool is_hashable_v = is_hashable<T>::value;

} // namespace utility

} // namespace cinq