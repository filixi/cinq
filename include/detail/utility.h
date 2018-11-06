#pragma once

#include <cassert>

#include <memory>
#include <type_traits>

namespace cinq::utility {
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
struct remove_smart_ptr_impl : std::false_type { using type = T; };
template <class T>
struct remove_smart_ptr_impl<std::shared_ptr<T>> : std::true_type { using type = T; };
template <class T>
struct remove_smart_ptr_impl<std::unique_ptr<T>> : std::true_type { using type = T; };
template <class T>
struct remove_smart_ptr : remove_smart_ptr_impl<std::decay_t<T>> {
  using type = std::conditional_t<remove_smart_ptr_impl<std::decay_t<T>>::value,
    typename remove_smart_ptr_impl<std::decay_t<T>>::type, T>;
};
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

template <class T, class = void>
struct is_hashable : std::false_type {};
template <class T>
struct is_hashable<T, std::void_t<std::hash<T>>> : std::true_type {};
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
          std::remove_const_t<SourceYieldType>,
          std::add_const_t<std::remove_reference_t<SourceYieldType>> &
        >,
      std::remove_const_t<SourceYieldType>
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

template <class...>
struct is_all_same;
template <class T1, class T2, class... Args>
struct is_all_same<T1, T2, Args...>
  : std::conditional_t<
    sizeof...(Args) == 0,
    std::is_same<T1, T2>,
    std::conditional_t<std::is_same_v<T1, T2>, is_all_same<T2, Args...>, std::false_type>
  > {};
template <class T>
struct is_all_same<T> : std::true_type {};
template <>
struct is_all_same<> : std::true_type {};
template <class... Args>
inline constexpr bool is_all_same_v = is_all_same<Args...>::value;

template <class... Args>
struct is_all_reference_to_same_cv : std::conditional_t<
      is_all_same_v<std::remove_reference_t<Args>...> &&
      (std::conjunction_v<std::is_rvalue_reference<Args>...> || std::conjunction_v<std::is_lvalue_reference<Args>...>),
    std::true_type, std::false_type> {};
template <class... Args>
inline constexpr bool is_all_reference_to_same_cv_v = is_all_reference_to_same_cv<Args...>::value;

template <class... Args>
struct is_all_reference_to_same : std::conjunction<is_all_same<std::decay_t<Args>...>, std::is_reference<Args>...> {};
template <class... Args>
inline constexpr bool is_all_reference_to_same_v = is_all_reference_to_same<Args...>::value;

template <class T, class = void>
struct is_less_than_comparable : std::false_type {};
template <class T>
struct is_less_than_comparable<T,
  std::void_t<decltype(std::declval<const std::decay_t<T> &>() < std::declval<const std::decay_t<T> &>())>> : std::true_type {};
template <class T>
inline constexpr bool is_less_than_comparable_v = is_less_than_comparable<T>::value;

template <class T, class = void>
struct is_equal_comparable : std::false_type {};
template <class T>
struct is_equal_comparable<T,
  std::void_t<decltype(std::declval<const std::decay_t<T> &>() == std::declval<const std::decay_t<T> &>())>> : std::true_type {};
template <class T>
inline constexpr bool is_equal_comparable_v = is_equal_comparable<T>::value;

template <class> static constexpr bool FakeFalse() { return false; }

template <class T, class = void>
struct ReferenceWrapper {
  static_assert(FakeFalse<T>(), "T must be less than comparable, or hashable and equal comparable.");
};

template <class T>
struct ReferenceWrapper<T, std::enable_if_t<!(is_hashable_v<T> && is_equal_comparable_v<T>) && is_less_than_comparable_v<T>, void>> {
  static_assert(!std::is_reference_v<T>, "T must not be reference.");
  static constexpr bool hash_version = false;

  ReferenceWrapper(const T &t) : ref_(&t) {}
  ReferenceWrapper(T &&) {
    static_assert(FakeFalse<T>(), "Can not bind to temporary object.");
  }

  friend decltype(auto) operator<(const ReferenceWrapper &lhs, const ReferenceWrapper &rhs) {
    return *lhs.ref_ < *rhs.ref_;
  }

  operator const T &() const {
    return *ref_;
  }

  const T *ref_;
};

template <class T>
struct ReferenceWrapper<T, std::enable_if_t<is_hashable_v<T> && is_equal_comparable_v<T>, void>> {
  static_assert(!std::is_reference_v<T>, "T must not be reference.");
  static constexpr bool hash_version = true;

  ReferenceWrapper(const T &t) : ref_(&t) {}
  ReferenceWrapper(T &&) {
    static_assert(FakeFalse<T>(), "Can not bind to temporary object.");
  }

  friend decltype(auto) operator==(const ReferenceWrapper &lhs, const ReferenceWrapper &rhs) {
    return *lhs.ref_ == *rhs.ref_;
  }

  operator const T &() const {
    return *ref_;
  }

  const T *ref_;
};

} // namespace cinq::utility

namespace std {
template <class T>
struct hash<cinq::utility::ReferenceWrapper<T>> {
  std::hash<std::decay_t<T>> h_;
  size_t operator()(const T &t) const noexcept(noexcept(h_(t))) {
    return static_cast<size_t>(h_(t));
  }
};

} // namespace std
