
#include <vector>

#include <cinq.h>
#include "../cinq-test-utility.h"
#include "detail/utility.h"

using cinq::Cinq;

namespace cinq_test {
void TestCinqDefaultIfEmpty() {
  {
    auto query = ToVector(Cinq(empty_source).DefaultIfEmpty());
    cinq::utility::CinqAssert(query.size() == 1 && query.front() == 0);
  }
  {
    auto query = ToVector(Cinq(std::vector<double>{3}).DefaultIfEmpty());
    cinq::utility::CinqAssert(query.size() == 1 && query.back() == 3);
  }
  {
    auto query = ToVector(Cinq(five_elements).DefaultIfEmpty());
    cinq::utility::CinqAssert(query.size() == 5 && std::equal(five_elements.begin(), five_elements.end(), query.begin()));
  }

  {
    auto query = ToVector(Cinq(empty_source).DefaultIfEmpty(11));
    cinq::utility::CinqAssert(query.size() == 1 && query.front() == 11);
  }
  {
    auto query = ToVector(Cinq(std::vector<int>{3}).DefaultIfEmpty(11));
    cinq::utility::CinqAssert(query.size() == 1 && query.back() == 3);
  }
  {
    auto query = ToVector(Cinq(five_elements).DefaultIfEmpty(11));
    cinq::utility::CinqAssert(query.size() == 5 && std::equal(five_elements.begin(), five_elements.end(), query.begin()));
  }
}

} // namespace cinq_test
