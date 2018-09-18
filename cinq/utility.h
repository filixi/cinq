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
inline constexpr bool is_const_lvalue_reference_v = std::is_const_v<std::remove_reference_t<T>> && std::is_lvalue_reference_v<T>;
template <class T>
inline constexpr bool is_non_const_lvalue_reference_v = !std::is_const_v<std::remove_reference_t<T>> && std::is_lvalue_reference_v<T>;
template <class T>
inline constexpr bool is_non_const_rvalue_reference_v = !std::is_const_v<std::remove_reference_t<T>> && std::is_rvalue_reference_v<T>;

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

template <class Fn, class... Args>
constexpr bool is_callable(std::invoke_result_t<Fn, Args...> *) { return true; }
template <class Fn, class... Args>
constexpr bool is_callable(...) { return false; }
template <class Fn, class... Args>
inline constexpr bool is_callable_v = is_callable<Fn, Args...>(nullptr);

template <class T, class = void>
struct is_hashable : std::true_type {};
template <class T>
struct is_hashable<T, std::void_t<std::hash<T>>> : std::false_type {};
template <class T>
inline constexpr bool is_hashable_v = is_hashable<T>::value;

enum class SourceType {
  InternalStorage,
  FunctionObject,
  Iterator
};

template <bool ConstVersion, class SourceYieldType, SourceType source_type>
struct transform_to_result_type;

template <bool ConstVersion, class SourceYieldType>
struct transform_to_result_type<ConstVersion, SourceYieldType, SourceType::InternalStorage> {
  using type = std::add_const_t<std::remove_reference_t<SourceYieldType>> &;
};

template <bool ConstVersion, class SourceYieldType>
struct transform_to_result_type<ConstVersion, SourceYieldType, SourceType::FunctionObject> {
  using type = std::conditional_t<ConstVersion,
      std::conditional_t<!std::is_reference_v<SourceYieldType> || std::is_rvalue_reference_v<SourceYieldType>,
          SourceYieldType,
          std::add_const_t<std::remove_reference_t<SourceYieldType>> &
        >,
      SourceYieldType
    >;
};

template <bool ConstVersion, class SourceYieldType>
struct transform_to_result_type<ConstVersion, SourceYieldType, SourceType::Iterator> {
  using type = typename transform_to_result_type<ConstVersion, SourceYieldType, SourceType::FunctionObject>::type;
};

template <bool ConstVersion, class SourceYieldType, SourceType source_type>
using transform_to_result_type_t = typename transform_to_result_type<ConstVersion, SourceYieldType, source_type>::type;

template <bool ConstVersion, class SourceYieldType>
struct transform_to_function_object_argument {
  using type = std::conditional_t<
      std::is_rvalue_reference_v<SourceYieldType> || !std::is_reference_v<SourceYieldType>,
      /* xvalue */ SourceYieldType &&,
      /* lvalue */ std::conditional_t<ConstVersion,
          const std::remove_reference_t<SourceYieldType> &,
          SourceYieldType
        >
    >;
};

template <bool ConstVersion, class SourceYieldType>
using transform_to_function_object_argument_t = typename transform_to_function_object_argument<ConstVersion, SourceYieldType>::type;

} // namespace utility

} // namespace cinq
