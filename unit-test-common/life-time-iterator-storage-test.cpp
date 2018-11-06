#include <list>

#include "cinq-test-utility.h"
#include "cinq.h"
using cinq::Cinq;

namespace cinq_test {
// Purpose: Some query's iteartor may hold the ownership of result (e.g. SelectMany), some other query's iterator may refer to source's element (e.g. Intersect).
// This test is for testing whether such references risk to be invalid.
void SelectManyAliasingTest() {
  auto basic_query = []() {
      return Cinq(std::vector<LifeTimeCheckInt>{1, 2, 3}).SelectMany([](auto &&) { return std::vector<LifeTimeCheckInt>{4, 5, 6}; });
    };

  auto random_dereference_and_copy = [](auto query) {
      using IteartorType = decltype(query.begin());
      std::list<IteartorType> iterators;
      for (auto i = query.begin(); i != query.end(); ++i) {
        iterators.push_back(i);
        for (auto j = i; j != query.end(); ++j) {
          iterators.push_back(j);
          for (auto k = j; k != query.end(); ++k) {
            iterators.push_back(k);
          }
        }
      }

      while (!iterators.empty()) {
        int t = *iterators.front();
        iterators.pop_front();
        assert(t);
      }
    };

  random_dereference_and_copy(basic_query().Distinct());
  random_dereference_and_copy(basic_query().Intersect(basic_query()));
  random_dereference_and_copy(basic_query().Intersect(basic_query(), basic_query()));
  random_dereference_and_copy(basic_query().Union(basic_query()));
  random_dereference_and_copy(basic_query().Union(basic_query(), basic_query()));

  random_dereference_and_copy(basic_query().Where([](auto &&) {return true;}).Distinct());
  random_dereference_and_copy(basic_query().Where([](auto &&) {return true;}).Intersect(basic_query().Where([](auto &&) {return true;})));
  random_dereference_and_copy(basic_query().Where([](auto &&) {return true;}).Intersect(basic_query().Where([](auto &&) {return true;}), basic_query().Where([](auto &&) {return true;})));
  random_dereference_and_copy(basic_query().Where([](auto &&) {return true;}).Union(basic_query().Where([](auto &&) {return true;})));
  random_dereference_and_copy(basic_query().Where([](auto &&) {return true;}).Union(basic_query().Where([](auto &&) {return true;}), basic_query().Where([](auto &&) {return true;})));
}

} // namespace cinq_test
