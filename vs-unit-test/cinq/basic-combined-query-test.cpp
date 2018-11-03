#include <any>
#include <deque>
#include <forward_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>
#include <numeric>
#include <string>
#include <functional>

#include "query-generator.h"
#include "cinq.h"
using cinq::Cinq;

namespace cinq_test {

// hash combine copied from boost

void LifeTimeVtrTest();
void LifeTimeRefVtrTest();
void LifeTimeCRefVtrTest();
void LifeTimeSharedPtrVtrTest();

void LifeTimeTest() {
  LifeTimeVtrTest();
  LifeTimeRefVtrTest();
  LifeTimeCRefVtrTest();
  LifeTimeSharedPtrVtrTest();
}

} // namespace cinq_test
