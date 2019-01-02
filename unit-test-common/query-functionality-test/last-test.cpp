
#include <vector>

#include <cinq.h>
#include "../cinq-test-utility.h"
#include "detail/utility.h"

using cinq::Cinq;

namespace cinq_test {
inline void TestCinqLast() {
  Cinq(one_element).Last();
  Cinq(one_element).Back();
  Cinq(one_element).Last([](auto) {return true;});
}

} // namespace cinq_test
