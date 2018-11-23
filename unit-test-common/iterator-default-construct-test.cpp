
#include <cstddef>
#include <vector>

#include <cinq.h>
#include "cinq-test-utility.h"
#include "query-generator.h"
#include "detail/utility.h"

using cinq::Cinq;

namespace cinq_test {
void IteratorDefaultConstructTest() {
  std::vector<int> source{1,2,3};
  auto result = QueryGenerator(std::move(source), [](auto &&) {return true;}, std::make_tuple(
      [](const int &x) -> const int & {return x;},
      [](const int &x) -> const int & {return x;},
      [](const int &x, const int &) -> const int & {return x;}
    ));

  auto appended = QueryAppendor(result, [](auto &&) {return true;}, std::make_tuple(
      [](const int &x) -> const int & {return x;},
      [](const int &x) -> const int & {return x;},
      [](const int &x, const int &) -> const int & {return x;}
    ));

  std::vector<uintptr_t> vtr;
  VisitTupleTree<decltype(result)>::Visit(
      result, [](auto &&) {}, [&vtr](auto &&query) {
          using IteratorType = decltype(query->begin());
          auto unique = std::make_unique<IteratorType>();
          vtr.emplace_back(reinterpret_cast<uintptr_t>(unique.get()));
        }
    );

  VisitTupleTree<decltype(appended)>::Visit(
      appended, [](auto &&) {}, [&vtr](auto &&query) {
          using IteratorType = decltype(query->begin());
          auto unique = std::make_unique<IteratorType>();
          vtr.emplace_back(reinterpret_cast<uintptr_t>(unique.get()));
        }
    );

  for (auto i : vtr)
    cinq::utility::CinqAssert(i != 0);
}

} // namespace cinq_test
