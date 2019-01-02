
#include <vector>

#include <cinq.h>
#include "../cinq-test-utility.h"
#include "detail/utility.h"

using cinq::Cinq;

namespace cinq_test {
void TestCinqAll() {
  cinq::utility::CinqAssert(Cinq(empty_source).All([](auto) {return false;}));
  cinq::utility::CinqAssert(Cinq(empty_source).All([](auto) {return true;}));

//  - equals to zero (if possible)
  cinq::utility::CinqAssert(!Cinq(one_element).All([](auto) {return false;}));

//  - equals to one due to the first element of data source which contains one and multiple element(s) (if possible)
  cinq::utility::CinqAssert(Cinq(one_element).All([](auto) {return true;}));
  cinq::utility::CinqAssert(Cinq(five_elements).All([](auto x) {return true;}));

//  - equals to one due to the last element of data source which contains one and multiple element(s) (if possible)
//  - equals to one due to an element in middle of data source
  cinq::utility::CinqAssert(Cinq(five_elements).All([](auto x) {return x < 5;}));
}

} // namespace cinq_test
