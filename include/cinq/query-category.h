#pragma once

namespace cinq::detail {
struct QueryCategory {
  struct SelectMany {};
  struct Select {};
  struct Join {};
  struct Where {};
  struct Intersect {};
  struct Union {};
  struct Concat {};
  struct Distinct  {};
};

} // namespace cinq::detail
