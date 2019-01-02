
#include <vector>

#include <cinq.h>
#include "../cinq-test-utility.h"
#include "detail/utility.h"

using cinq::Cinq;

namespace cinq_test {
void TestCinqCount() {
  cinq::utility::CinqAssert(Cinq(empty_source).Count() == 0);
  cinq::utility::CinqAssert(Cinq(one_element).Count() == 1);
  cinq::utility::CinqAssert(Cinq(five_elements).Count() == 5);

//  - equals to zero (if possible)
  cinq::utility::CinqAssert(Cinq(empty_source).Count([](auto) {return false;}) == 0);
  cinq::utility::CinqAssert(Cinq(empty_source).Count([](auto) {return true;}) == 0);
  cinq::utility::CinqAssert(Cinq(one_element).Count([](auto) {return false;}) == 0);
  cinq::utility::CinqAssert(Cinq(five_elements).Count([](auto) {return false;}) == 0);

//  - equals to one due to the first element of data source which contains one and multiple element(s) (if possible)
  cinq::utility::CinqAssert(Cinq(one_element).Count([](auto x) {return x == 0;}) == 1);
  cinq::utility::CinqAssert(Cinq(five_elements).Count([](auto x) {return x == 0;}) == 1);

//  - equals to one due to the last element of data source which contains one and multiple element(s) (if possible)
  cinq::utility::CinqAssert(Cinq(five_elements).Count([](auto x) {return x == 4;}) == 1);

//  - equals to one due to an element in middle of data source
  cinq::utility::CinqAssert(Cinq(five_elements).Count([](auto x) {return x == 2;}) == 1);

//  - is non-zero but is less than the size of data source (if possible)
  cinq::utility::CinqAssert(Cinq(five_elements).Count([](auto x) {return x < 2;}) == 2);

//  - equals to the size of data source (if possible)
  cinq::utility::CinqAssert(Cinq(five_elements).Count([](auto x) {return true;}) == 5);
}

} // namespace cinq_test
