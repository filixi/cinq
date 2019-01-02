
#include <vector>

#include <cinq.h>
#include "../cinq-test-utility.h"
#include "detail/utility.h"

using cinq::Cinq;

namespace cinq_test {
void TestCinqAverage() {
  cinq::utility::CinqAssert(Cinq(empty_source).Average() == 0);
  cinq::utility::CinqAssert(Cinq(one_element).Average() == 0);
  cinq::utility::CinqAssert(Cinq(five_elements).Average() == 2);

  cinq::utility::CinqAssert(Cinq(empty_source).Average([](auto &&x) {return x*2;}) == 0);
  cinq::utility::CinqAssert(Cinq(one_element).Average([](auto &&x) {return x*2;}) == 0);
  cinq::utility::CinqAssert(Cinq(five_elements).Average([](auto &&x) {return x*2;}) == 4);
}

} // namespace cinq_test
