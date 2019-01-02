
#include <vector>

#include <cinq.h>
#include "../cinq-test-utility.h"
#include "detail/utility.h"

using cinq::Cinq;

namespace cinq_test {
void TestCinqAny() {
//  - equals to zero (if possible)
  cinq::utility::CinqAssert(!Cinq(empty_source).Any([](auto &&) {return true;}));
  cinq::utility::CinqAssert(!Cinq(empty_source).Any([](auto &&) {return false;}));
  cinq::utility::CinqAssert(!Cinq(one_element).Any([](auto &&) {return false;}));
  cinq::utility::CinqAssert(!Cinq(five_elements).Any([](auto &&) {return false;}));

//  - equals to one due to the first element of data source which contains one and multiple element(s) (if possible)
  cinq::utility::CinqAssert(Cinq(one_element).Any([](auto &&) {return true;}));

//  - equals to one due to the last element of data source which contains one and multiple element(s) (if possible)
  cinq::utility::CinqAssert(Cinq(five_elements).Any([](auto &&x) {return x==4;}));

//  - equals to one due to an element in middle of data source
  cinq::utility::CinqAssert(Cinq(five_elements).Any([](auto &&x) {return x==2;}));
}

} // namespace cinq_test
