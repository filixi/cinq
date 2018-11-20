#pragma once

#include <cassert>
#include <algorithm>
#include <type_traits>

#include "cinq/enumerable-source.h"
#include "cinq/enumerable.h"
#include "cinq.h"
#include "detail/utility.h"

namespace cinq::utility {
template <class T>
struct CinqTraits {
};

template <class T>
struct EnumerableTraits {
  static constexpr bool is_member = false;

  static constexpr bool is_elements_unique = false;
};

template <bool ConstVersion, OperatorType operator_type, class Fn, class... Sources>
struct EnumerableTraits<detail::Enumerable<ConstVersion, operator_type, Fn, Sources...>> {
  static constexpr bool is_member = true;

  static constexpr bool is_elements_unique = operator_type == OperatorType::Distinct || right_fold_and_v<EnumerableTraits<Sources>::is_elements_unique...>;
};

template <bool ConstVersion, class Source>
struct EnumerableTraits<detail::EnumerableSource<ConstVersion, Source>> {
  static constexpr bool is_member = true;

  static constexpr bool is_elements_unique = EnumerableTraits<Source>::is_elements_unique;
};

template <class T>
struct SpecializedIteratorTraits {
  
};

} // namespace cinq::utility
