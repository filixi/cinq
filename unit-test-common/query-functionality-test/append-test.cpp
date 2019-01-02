
#include <vector>

#include <cinq.h>
#include "../cinq-test-utility.h"
#include "detail/utility.h"

using cinq::Cinq;

namespace cinq_test {
void TestCinqAppend() {
  {
    auto query = ToVector(
      Cinq(empty_source).Append(10));
    cinq::utility::CinqAssert(query.size() == 1 && query.front() == 10);
  }

  {
    auto query = ToVector(
      Cinq(one_element).Append(10));
    cinq::utility::CinqAssert(query.size() == 2 && query.back() == 10);
  }

  {
    auto query = ToVector(
      Cinq(five_elements).Append(10));
    cinq::utility::CinqAssert(query.size() == 6 && query.back() == 10);
  }
}

} // namespace cinq_test
