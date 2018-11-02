
#include <vector>

#include "../cinq/utility.h"
#include "query-generator.h"

namespace cinq_test {
void LifeTimeRefVtrTest() {
  std::vector<SpecialInt> vtr{1, 1, 2, 2, 3, 11 ,13 ,15 ,17};

  BasicCombinedQueryTest(std::ref(vtr));
}

} // namespace cinq_test
