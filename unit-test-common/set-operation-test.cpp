
#include <cassert>
#include <vector>
#include <string>
#include <algorithm>

#include "cinq.h"
#include "cinq-test-utility.h"
using cinq::Cinq;

namespace cinq_test {

void IntersectTest() {
  std::vector<LifeTimeCheckInt> source1, source2, source3;

  auto source_from_int = [](LifeTimeCheckInt x) {
    auto result = ToVector(Cinq(std::to_string(x)).Select([](char c) -> LifeTimeCheckInt {return c - '0'; }));
    if (result[0] == 0 && result.size() == 1)
      return std::vector<LifeTimeCheckInt>();
    return result;
  };

  std::vector<LifeTimeCheckInt> test_set{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    12345, 23456, 34567, 45678, 56789, 67890, 78901, 89012, 90123,
    123, 456, 789
  };

  for (LifeTimeCheckInt i : test_set) {
    for (LifeTimeCheckInt j : test_set) {
      source1 = source_from_int(i);
      source2 = source_from_int(j);

      std::sort(source1.begin(), source1.end());
      std::sort(source2.begin(), source2.end());

      std::vector<LifeTimeCheckInt> result;
      std::set_intersection(source1.begin(), source1.end(), source2.begin(), source2.end(), std::back_inserter(result));
      std::sort(result.begin(), result.end());
      auto ite = std::unique(result.begin(), result.end());
      result.erase(ite, result.end());

      auto query = Cinq(source1).Intersect(source2);
      auto vtr = ToVector(query);
      std::sort(vtr.begin(), vtr.end());
      assert(vtr.size() == result.size() &&
        std::equal(vtr.begin(), vtr.end(), result.begin()));
      vtr = ToVector(query);
      std::sort(vtr.begin(), vtr.end());
      assert(vtr.size() == result.size() &&
        std::equal(vtr.begin(), vtr.end(), result.begin()));

      for (LifeTimeCheckInt k : test_set) {
        source3 = source_from_int(k);
        std::sort(source3.begin(), source3.end());

        std::vector<LifeTimeCheckInt> result3;
        std::set_intersection(source3.begin(), source3.end(), result.begin(), result.end(), std::back_inserter(result3));
        std::sort(result3.begin(), result3.end());
        auto ite2 = std::unique(result3.begin(), result3.end());
        result3.erase(ite2, result3.end());

        auto query2 = Cinq(source1).Intersect(source2, source3);
        auto vtr2 = ToVector(query2);
        std::sort(vtr2.begin(), vtr2.end());
        assert(vtr2.size() == result3.size() &&
          std::equal(vtr2.begin(), vtr2.end(), result3.begin()));
        vtr2 = ToVector(query2);
        std::sort(vtr2.begin(), vtr2.end());
        assert(vtr2.size() == result3.size() &&
          std::equal(vtr2.begin(), vtr2.end(), result3.begin()));
      }
    }
  }

  // # additional trivial test with 4 sources
  {
    auto query = Cinq(empty_source).Intersect(empty_source, empty_source, empty_source);
    auto vtr = ToVector(query);
    assert(vtr.size() == 0);
    vtr = ToVector(query);
    assert(vtr.size() == 0);
  }
}

void UnionTest() {
  std::vector<LifeTimeCheckInt> source1, source2, source3;

  auto source_from_int = [](LifeTimeCheckInt x) {
    auto result = ToVector(Cinq(std::to_string(x)).Select([](char c) -> LifeTimeCheckInt {return c - '0'; }));
    if (result[0] == 0 && result.size() == 1)
      return std::vector<LifeTimeCheckInt>();
    return result;
  };

  std::vector<LifeTimeCheckInt> test_set{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    12345, 23456, 34567, 45678, 56789, 67890, 78901, 89012, 90123,
    123, 456, 789
  };

  for (LifeTimeCheckInt i : test_set) {
    for (LifeTimeCheckInt j : test_set) {
      source1 = source_from_int(i);
      source2 = source_from_int(j);

      std::sort(source1.begin(), source1.end());
      std::sort(source2.begin(), source2.end());

      std::vector<LifeTimeCheckInt> result;
      std::set_union(source1.begin(), source1.end(), source2.begin(), source2.end(), std::back_inserter(result));
      std::sort(result.begin(), result.end());
      auto ite = std::unique(result.begin(), result.end());
      result.erase(ite, result.end());

      auto query = Cinq(source1).Union(source2);
      auto vtr = ToVector(query);
      std::sort(vtr.begin(), vtr.end());
      assert(vtr.size() == result.size() &&
        std::equal(vtr.begin(), vtr.end(), result.begin()));
      vtr = ToVector(query);
      std::sort(vtr.begin(), vtr.end());
      assert(vtr.size() == result.size() &&
        std::equal(vtr.begin(), vtr.end(), result.begin()));

      for (LifeTimeCheckInt k : test_set) {
        source3 = source_from_int(k);
        std::sort(source3.begin(), source3.end());

        std::vector<LifeTimeCheckInt> result3;
        std::set_union(source3.begin(), source3.end(), result.begin(), result.end(), std::back_inserter(result3));
        std::sort(result3.begin(), result3.end());
        auto ite2 = std::unique(result3.begin(), result3.end());
        result3.erase(ite2, result3.end());

        auto query2 = Cinq(source1).Union(source2, source3);
        auto vtr2 = ToVector(query2);
        std::sort(vtr2.begin(), vtr2.end());
        assert(vtr2.size() == result3.size() &&
          std::equal(vtr2.begin(), vtr2.end(), result3.begin()));
        vtr2 = ToVector(query2);
        std::sort(vtr2.begin(), vtr2.end());
        assert(vtr2.size() == result3.size() &&
          std::equal(vtr2.begin(), vtr2.end(), result3.begin()));
      }
    }
  }

  // # additional trivial test with 4 sources
  {
    auto query = Cinq(empty_source).Union(empty_source, empty_source, empty_source);
    auto vtr = ToVector(query);
    assert(vtr.size() == 0);
    vtr = ToVector(query);
    assert(vtr.size() == 0);
  }
}

void ConcatTest() {
  std::vector<std::vector<LifeTimeCheckInt>> sources{
    empty_source, empty_source, empty_source,
    one_element, one_element, one_element,
    five_elements, five_elements, five_elements
  };

  // $ is empty $ has one element $ has five elements
  // # is empty # has one element # has multiple elements
  // # includes all
  for (auto &s1 : sources) {
    for (auto &s2 : sources) {
      std::vector<LifeTimeCheckInt> result1(s1);
      std::copy(s2.begin(), s2.end(), std::back_inserter(result1));

      auto query1 = Cinq(s1).Concat(s2);
      auto vtr = ToVector(query1);
      assert(vtr.size() == result1.size() &&
        std::equal(vtr.begin(), vtr.end(), result1.begin()));
      vtr = ToVector(query1);
      assert(vtr.size() == result1.size() &&
        std::equal(vtr.begin(), vtr.end(), result1.begin()));

      for (auto &s3 : sources) {
        std::vector<LifeTimeCheckInt> result2(result1);
        std::copy(s3.begin(), s3.end(), std::back_inserter(result2));

        auto query2 = Cinq(s1).Concat(s2, s3);
        auto vtr2 = ToVector(query2);
        assert(vtr2.size() == vtr2.size() &&
          std::equal(vtr2.begin(), vtr2.end(), result2.begin()));
        vtr2 = ToVector(query2);
        assert(vtr2.size() == vtr2.size() &&
          std::equal(vtr2.begin(), vtr2.end(), result2.begin()));
      }
    }
  }

  // # additional trivial test with 4 sources
  {
    auto query = Cinq(empty_source).Concat(empty_source, empty_source, empty_source);
    auto vtr = ToVector(query);
    assert(vtr.size() == 0);
    vtr = ToVector(query);
    assert(vtr.size() == 0);
  }
}

void DistinctTest() {
  // Distinct is also used in some set operation internally

  // $ is empty
  {
    auto query = Cinq(empty_source).Distinct();
    auto vtr = ToVector(query);
    assert(vtr.size() == 0);
    vtr = ToVector(query);
    assert(vtr.size() == 0);
  }

  // $ has one # has one # include first
  {
    auto query = Cinq(one_element).Distinct();
    auto vtr = ToVector(query);
    assert(vtr.size() == 1 && vtr[0] == one_element[0]);
    vtr = ToVector(query);
    assert(vtr.size() == 1 && vtr[0] == one_element[0]);
  }

  // $ has five # has multiple # include last # include a middle
  // # exclude a middle
  {
    std::vector<LifeTimeCheckInt> special{
      1, 12, 12, 12, 99
    };
    auto query = Cinq(special).Distinct();
    auto vtr = ToVector(query);
    assert(vtr.size() == 3 &&
      vtr[0] == 1 && vtr[1] == 12 && vtr[2] == 99);
    vtr = ToVector(query);
    assert(vtr.size() == 3 &&
      vtr[0] == 1 && vtr[1] == 12 && vtr[2] == 99);
  }

  // # has multiple and include all
  {
    std::vector<LifeTimeCheckInt> special{
      1, 2, 3, 4, 5
    };
    auto query = Cinq(special).Distinct();
    auto vtr = ToVector(query);
    assert(vtr.size() == special.size() &&
      std::equal(vtr.begin(), vtr.end(), special.begin()));
    vtr = ToVector(query);
    assert(vtr.size() == special.size() &&
      std::equal(vtr.begin(), vtr.end(), special.begin()));
  }
}

} // namespace cinq_test
