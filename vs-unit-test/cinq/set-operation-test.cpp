
#include <cassert>
#include <vector>
#include <string>
#include <algorithm>

#include "cinq-v3.h"
using cinq_v3::Cinq;

namespace cinq_test {

const std::vector<int> empty_source;
const std::vector<int> one_element{ 0 };
const std::vector<int> five_elements{ 0, 1, 2, 3, 4 }; // elements must be unique

void IntersectTest() {
  auto copy_forward = [](auto x) {return x; };

  std::vector<int> source1, source2, source3;

  auto source_from_int = [](int x) {
    auto result = Cinq(std::to_string(x)).Select([](char c) -> int {return c - '0'; }).ToVector();
    if (result[0] == 0 && result.size() == 1)
      return std::vector<int>();
    return result;
  };

  std::vector<int> test_set{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    12345, 23456, 34567, 45678, 56789, 67890, 78901, 89012, 90123,
    123, 456, 789
  };

  for (int i : test_set) {
    for (int j : test_set) {
      source1 = source_from_int(i);
      source2 = source_from_int(j);

      std::sort(source1.begin(), source1.end());
      std::sort(source2.begin(), source2.end());

      std::vector<int> result;
      std::set_intersection(source1.begin(), source1.end(), source2.begin(), source2.end(), std::back_inserter(result));
      std::sort(result.begin(), result.end());
      auto ite = std::unique(result.begin(), result.end());
      result.erase(ite, result.end());

      auto query = Cinq(source1).Intersect(source2);
      auto vtr = query.ToVector();
      std::sort(vtr.begin(), vtr.end());
      assert(vtr.size() == result.size() &&
        std::equal(vtr.begin(), vtr.end(), result.begin()));
      vtr = query.ToVector();
      std::sort(vtr.begin(), vtr.end());
      assert(vtr.size() == result.size() &&
        std::equal(vtr.begin(), vtr.end(), result.begin()));

      for (int k : test_set) {
        source3 = source_from_int(k);
        std::sort(source3.begin(), source3.end());

        std::vector<int> result3;
        std::set_intersection(source3.begin(), source3.end(), result.begin(), result.end(), std::back_inserter(result3));
        std::sort(result3.begin(), result3.end());
        auto ite2 = std::unique(result3.begin(), result3.end());
        result3.erase(ite2, result3.end());

        auto query2 = Cinq(source1).Intersect(source2, source3);
        auto vtr2 = query2.ToVector();
        std::sort(vtr2.begin(), vtr2.end());
        assert(vtr2.size() == result3.size() &&
          std::equal(vtr2.begin(), vtr2.end(), result3.begin()));
        vtr2 = query2.ToVector();
        std::sort(vtr2.begin(), vtr2.end());
        assert(vtr2.size() == result3.size() &&
          std::equal(vtr2.begin(), vtr2.end(), result3.begin()));
      }
    }
  }

  // # additional trivial test with 4 sources
  {
    auto query = Cinq(empty_source).Intersect(empty_source, empty_source, empty_source);
    auto vtr = query.ToVector();
    assert(vtr.size() == 0);
    vtr = query.ToVector();
    assert(vtr.size() == 0);
  }
}

void UnionTest() {
  auto copy_forward = [](auto x) {return x; };

  std::vector<int> source1, source2, source3;

  auto source_from_int = [](int x) {
    auto result = Cinq(std::to_string(x)).Select([](char c) -> int {return c - '0'; }).ToVector();
    if (result[0] == 0 && result.size() == 1)
      return std::vector<int>();
    return result;
  };

  std::vector<int> test_set{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    12345, 23456, 34567, 45678, 56789, 67890, 78901, 89012, 90123,
    123, 456, 789
  };

  for (int i : test_set) {
    for (int j : test_set) {
      source1 = source_from_int(i);
      source2 = source_from_int(j);

      std::sort(source1.begin(), source1.end());
      std::sort(source2.begin(), source2.end());

      std::vector<int> result;
      std::set_union(source1.begin(), source1.end(), source2.begin(), source2.end(), std::back_inserter(result));
      std::sort(result.begin(), result.end());
      auto ite = std::unique(result.begin(), result.end());
      result.erase(ite, result.end());

      auto query = Cinq(source1).Union(source2);
      auto vtr = query.ToVector();
      std::sort(vtr.begin(), vtr.end());
      assert(vtr.size() == result.size() &&
        std::equal(vtr.begin(), vtr.end(), result.begin()));
      vtr = query.ToVector();
      std::sort(vtr.begin(), vtr.end());
      assert(vtr.size() == result.size() &&
        std::equal(vtr.begin(), vtr.end(), result.begin()));

      for (int k : test_set) {
        source3 = source_from_int(k);
        std::sort(source3.begin(), source3.end());

        std::vector<int> result3;
        std::set_union(source3.begin(), source3.end(), result.begin(), result.end(), std::back_inserter(result3));
        std::sort(result3.begin(), result3.end());
        auto ite2 = std::unique(result3.begin(), result3.end());
        result3.erase(ite2, result3.end());

        auto query2 = Cinq(source1).Union(source2, source3);
        auto vtr2 = query2.ToVector();
        std::sort(vtr2.begin(), vtr2.end());
        assert(vtr2.size() == result3.size() &&
          std::equal(vtr2.begin(), vtr2.end(), result3.begin()));
        vtr2 = query2.ToVector();
        std::sort(vtr2.begin(), vtr2.end());
        assert(vtr2.size() == result3.size() &&
          std::equal(vtr2.begin(), vtr2.end(), result3.begin()));
      }
    }
  }

  // # additional trivial test with 4 sources
  {
    auto query = Cinq(empty_source).Union(empty_source, empty_source, empty_source);
    auto vtr = query.ToVector();
    assert(vtr.size() == 0);
    vtr = query.ToVector();
    assert(vtr.size() == 0);
  }
}

void ConcatTest() {
  std::vector<std::vector<int>> sources{
    empty_source, empty_source, empty_source,
    one_element, one_element, one_element,
    five_elements, five_elements, five_elements
  };

  // $ is empty $ has one element $ has five elements
  // # is empty # has one element # has multiple elements
  // # includes all
  for (auto &s1 : sources) {
    for (auto &s2 : sources) {
      std::vector<int> result1(s1);
      std::copy(s2.begin(), s2.end(), std::back_inserter(result1));

      auto query1 = Cinq(s1).Concat(s2);
      auto vtr = query1.ToVector();
      assert(vtr.size() == result1.size() &&
        std::equal(vtr.begin(), vtr.end(), result1.begin()));
      vtr = query1.ToVector();
      assert(vtr.size() == result1.size() &&
        std::equal(vtr.begin(), vtr.end(), result1.begin()));

      for (auto &s3 : sources) {
        std::vector<int> result2(result1);
        std::copy(s3.begin(), s3.end(), std::back_inserter(result2));

        auto query2 = Cinq(s1).Concat(s2, s3);
        auto vtr2 = query2.ToVector();
        assert(vtr2.size() == vtr2.size() &&
          std::equal(vtr2.begin(), vtr2.end(), result2.begin()));
        vtr2 = query2.ToVector();
        assert(vtr2.size() == vtr2.size() &&
          std::equal(vtr2.begin(), vtr2.end(), result2.begin()));
      }
    }
  }

  // # additional trivial test with 4 sources
  {
    auto query = Cinq(empty_source).Concat(empty_source, empty_source, empty_source);
    auto vtr = query.ToVector();
    assert(vtr.size() == 0);
    vtr = query.ToVector();
    assert(vtr.size() == 0);
  }
}

void DistinctTest() {
  // Distinct is also used in some set operation internally

  // $ is empty
  {
    auto query = Cinq(empty_source).Distinct();
    auto vtr = query.ToVector();
    assert(vtr.size() == 0);
    vtr = query.ToVector();
    assert(vtr.size() == 0);
  }

  // $ has one # has one # include first
  {
    auto query = Cinq(one_element).Distinct();
    auto vtr = query.ToVector();
    assert(vtr.size() == 1 && vtr[0] == one_element[0]);
    vtr = query.ToVector();
    assert(vtr.size() == 1 && vtr[0] == one_element[0]);
  }

  // $ has five # has multiple # include last # include a middle
  // # exclude a middle
  {
    std::vector<int> special{
      1, 12, 12, 12, 99
    };
    auto query = Cinq(special).Distinct();
    auto vtr = query.ToVector();
    assert(vtr.size() == 3 &&
      vtr[0] == 1 && vtr[1] == 12 && vtr[2] == 99);
    vtr = query.ToVector();
    assert(vtr.size() == 3 &&
      vtr[0] == 1 && vtr[1] == 12 && vtr[2] == 99);
  }

  // # has multiple and include all
  {
    std::vector<int> special{
      1, 2, 3, 4, 5
    };
    auto query = Cinq(special).Distinct();
    auto vtr = query.ToVector();
    assert(vtr.size() == special.size() &&
      std::equal(vtr.begin(), vtr.end(), special.begin()));
    vtr = query.ToVector();
    assert(vtr.size() == special.size() &&
      std::equal(vtr.begin(), vtr.end(), special.begin()));
  }
}

} // namespace cinq_test
