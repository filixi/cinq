#pragma once

namespace cinq_v3::detail {
enum class EnumerableCategory : int {
  Producer, // SelectMany, Join
  Subrange, // Where, Take, Skip, TakeWhile, Distinct
  SetOperation, // Concat, Union, Intersect, Except
  Converter // Order, GroupBy, ToArray, ToList, ToDictionary, ToLookup, ToLinq
};

// SelectMany, Join, Select, Zip => EnumerableCategory::Producer
// use the Current enumerable with TProvider

// Where, Take, Skip, TakeWhile, Distinct => EnumerableCategory::Subrange
// Replace TProvider with TPredicate
// Cheap to copy iterator

// Concat, Union, Intersect, Except => EnumerableCategory::SetOperation
// with multiple EnumerableSource

// Order, GroupBy, ToArray, ToList, ToDictionary, ToLookup, ToLinq => EnumerableCategory::Converter 
// return the result fully iterated directly
// Iterator is not provided by cinq

enum class OperatorType : int {
  SelectMany,
  Select,
  Join,
  Where,
  Intersect
};

} // namespace cinq_v3::detail
