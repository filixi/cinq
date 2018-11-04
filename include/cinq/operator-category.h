#pragma once

namespace cinq::detail {
enum class OperatorType : int {
  SelectMany,
  Select,
  Join,
  Where,
  Intersect,
  Union,
  Concat,
  Distinct
};

} // namespace cinq::detail
