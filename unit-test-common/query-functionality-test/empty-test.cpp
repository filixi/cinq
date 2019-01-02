
#include <vector>

#include <cinq.h>
#include "../cinq-test-utility.h"
#include "detail/utility.h"

using cinq::Cinq;

namespace cinq_test {
void TestCinqEmpty() {
  cinq::utility::CinqAssert(Cinq(empty_source).Empty());
  cinq::utility::CinqAssert(!Cinq(one_element).Empty());
  cinq::utility::CinqAssert(!Cinq(five_elements).Empty());
}

} // namespace cinq_test
