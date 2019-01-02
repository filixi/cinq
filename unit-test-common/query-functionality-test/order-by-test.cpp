
#include <vector>

#include <cinq.h>
#include "../cinq-test-utility.h"
#include "detail/utility.h"

using cinq::Cinq;

namespace cinq_test {
inline void TestCinqOrderBy() {
  Cinq(one_element).OrderBy([](int x) { return x; });
}

} // namespace cinq_test
