
#include <vector>

#include <cinq.h>
#include "../cinq-test-utility.h"
#include "detail/utility.h"

using cinq::Cinq;

namespace cinq_test {
void TestCinqGroupBy() {
  Cinq(one_element).GroupBy([](int x) { return x%2; });
  Cinq(one_element).GroupBy([](int x) { return x%2; }, [](int x) { return x; });
}

} // namespace cinq_test
