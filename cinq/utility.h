#pragma once

#include <memory>

namespace cinq {
namespace utility {
template <class T>
struct remove_reference_wrapper : std::false_type { using type = T; };
template <class T>
struct remove_reference_wrapper<std::reference_wrapper<T>> : std::true_type { using type = T; };
template <class T>
using remove_reference_wrapper_t = typename remove_reference_wrapper<T>::type;
template <class T>
constexpr bool is_reference_wrapper_v = remove_reference_wrapper<T>::value;

template <class T>
struct is_const_reference : std::false_type {};
template <class T>
struct is_const_reference<const T &> : std::true_type {};
template <class T>
struct is_const_reference<T &> : std::false_type {};
template <class T>
constexpr bool is_const_reference_v = is_const_reference<T>::value;

template <class T>
struct remove_smart_ptr : std::false_type { using type = T; };
template <class T>
struct remove_smart_ptr<std::shared_ptr<T>> : std::true_type { using type = T; };
template <class T>
struct remove_smart_ptr<std::unique_ptr<T>> : std::true_type { using type = T; };
template <class T>
using remove_smart_ptr_t = typename remove_smart_ptr<T>::type;
template <class T>
constexpr bool is_smart_ptr_v = remove_smart_ptr<T>::value;

} // namespace utility

} // namespace cinq