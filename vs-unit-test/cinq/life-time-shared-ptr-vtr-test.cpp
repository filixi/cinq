
#include <vector>

#include "../cinq/utility.h"
#include "query-generator.h"

namespace cinq_test {
void LifeTimeSharedPtrVtrTest() {
  std::vector<SpecialInt> vtr{1, 1, 2, 2, 3, 11 ,13 ,15 ,17};

  BasicCombinedQueryTest(std::make_shared<decltype(vtr)>(vtr));
}

} // namespace cinq_test
