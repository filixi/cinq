
#include <vector>

#include <cinq.h>
#include "../cinq-test-utility.h"
#include "detail/utility.h"

using cinq::Cinq;

namespace cinq_test {
void TestCinqContain() {
//  - equals to zero (if possible)
  cinq::utility::CinqAssert(!Cinq(empty_source).Contain(0));

//  - equals to one due to the first element of data source which contains one and multiple element(s) (if possible)
  cinq::utility::CinqAssert(Cinq(one_element).Contain(0));
  cinq::utility::CinqAssert(Cinq(five_elements).Contain(0));

//  - equals to one due to the last element of data source which contains one and multiple element(s) (if possible)
  cinq::utility::CinqAssert(Cinq(five_elements).Contain(4));

//  - equals to one due to an element in middle of data source
  cinq::utility::CinqAssert(Cinq(five_elements).Contain(3));
}

} // namespace cinq_test
