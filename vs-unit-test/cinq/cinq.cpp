// XLinq.cpp : Defines the entry point for the console application.
//

#include <cassert>

#include <deque>
#include <forward_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>
#include <numeric>
#include <string>

#include "cinq-v3.h"
using cinq_v3::Cinq;

// Unit test for semantics
//  - const reference guarante (if applies)
//  - move semantic of data source (if applies)
//  - copying semantic of data source (if applies)
//  - std::reference_wrapper (if applyies)
//  - life time guarante of element(s) and result
//  - no modifying guarante (if applies)
//
//
// Unit test for functionalities
// unit test of each single query should cover the following cases:
// For data source $:
//  - is empty
//  - has one element
//  - has at least five elements
//
// For Result #:
//  if the result is a enumerable
//  - is empty (if possible, data source must not be empty)
//  - has one element
//  - has multiple elements (if possible)
//  - includes the first element of data source (if possible)
//  - excludes the first element of data source (if possible)
//  - includes the last element of data source, size(data source) > 1 (if possible)
//  - excludes the last element of data source, size(data source) > 1 (if possible)
//  - includes an element in middle, size(data source) > 1 (if possible)
//  - excludes an element in middle, size(data source) > 1 (if possible)
//  - is of size greater than one and includes all elements of all data source(s) (if possible)
//  if the result is a single arithmetic type which doesn't come from the element(s) of data source(s)
//  - equals to zero (if possible)
//  - equals to one due to the first element of data source which contains one and multiple element(s) (if possible)
//  - equals to one due to the last element of data source which contains one and multiple element(s) (if possible)
//  - equals to one due to an element in middle of data source
//  - is non-zero but is less than the size of data source (if possible)
//  - equals to the size of data source (if possible)

// Supplementary rules :
//  - test case can be combined.
//  - test can use arbitary element type. Case of using user-defined types is tested by semantics test.
//  - if the query can take infinit data sources, test up to the minimum count of data source + 1.
//      And an additional trivial test for at least minimum count + 2.
//  - if the query takes multiple data sources, the input should consist of the full permutation of full combination of the case for data source if possible.
//  - if the query takes multiple data sources, the output should be tested for applying on every single data source.
//  - chained query is tested by chained query generator, and the result is compared with the result of
//      step by step query (insert ToVector in between). The correctness of this test is base on the assumption that each individual query is correct.
//

const std::vector<int> empty_source;
const std::vector<int> one_element{0};
const std::vector<int> five_elements{0,1,2,3,4}; // elements must be unique

class MiniContainer {
public:
  auto begin() { return std::begin(data); }
  auto end() { return std::end(data); }

  auto begin() const { return std::cbegin(data); }
  auto end() const { return std::cend(data); }

private:
  int data[5] = {1,2,3,4,5};
};

void TestCinqInitialization() {
  std::vector<int> vtr{1,2,3,4,5};
  int arr[] = {1,2,3,4,5};
  MiniContainer c;
  auto shared_vtr = std::make_shared<std::vector<int>>();

  Cinq(vtr);
  Cinq(std::ref(vtr));
  Cinq(std::cref(vtr));
  Cinq(std::move(vtr));

  Cinq(arr);
  Cinq(std::ref(arr));
  Cinq(std::cref(arr));

  Cinq(c);
  Cinq(std::ref(c));
  Cinq(std::cref(c));
  Cinq(std::move(c));

  Cinq(std::vector<int>{1,2,3,4,5});
  Cinq(std::forward_list<int>{1,2,3,4,5});
  Cinq(MiniContainer());
  Cinq({1,2,3,4,5});

  Cinq(shared_vtr);
  Cinq(std::make_unique<std::vector<int>>());
}

void TestCinqValueCategory() {
  using namespace cinq::utility;

  std::vector<int> vtr{1,2,3,4,5};
  int arr[] = {1,2,3,4,5};
  auto shared_vtr = std::make_shared<std::vector<int>>();

  Cinq({1,2,3,4,5}).SelectMany([](auto &x) {
    static_assert(is_const_reference_v<decltype(x)>);
    return std::vector<int>{};
  });

  Cinq(vtr).SelectMany([](auto &x) {
    static_assert(is_const_reference_v<decltype(x)>);
    return std::vector<int>{};
  });
  Cinq(std::ref(vtr)).SelectMany([](auto &x) {
    static_assert(is_const_reference_v<decltype(x)>);
    return std::vector<int>{};
  });
  Cinq(std::cref(vtr)).SelectMany([](auto &x) {
    static_assert(is_const_reference_v<decltype(x)>);
    return std::vector<int>{};
  });

  Cinq(arr).SelectMany([](auto &x) {
    static_assert(is_const_reference_v<decltype(x)>);
    return std::vector<int>{};
  });
  Cinq(std::ref(arr)).SelectMany([](auto &x) {
    static_assert(is_const_reference_v<decltype(x)>);
    return std::vector<int>{};
  });
  Cinq(std::cref(arr)).SelectMany([](auto &x) {
    static_assert(is_const_reference_v<decltype(x)>);
    return std::vector<int>{};
  });

  Cinq(vtr).Where([](auto &x) {
    static_assert(is_const_reference_v<decltype(x)>);
    return true;
  }).Where([](auto &x) {
    static_assert(is_const_reference_v<decltype(x)>);
    return true;
  });
  Cinq(std::ref(vtr)).Where([](auto &x) {
    static_assert(is_const_reference_v<decltype(x)>);
    return true;
  }).Where([](auto &x) {
    static_assert(is_const_reference_v<decltype(x)>);
    return true;
  });
  Cinq(std::cref(vtr)).Where([](auto &x) {
    static_assert(is_const_reference_v<decltype(x)>);
    return true;
  }).Where([](auto &x) {
    static_assert(is_const_reference_v<decltype(x)>);
    return true;
  });

  Cinq(shared_vtr).Where([](auto &x) {
    static_assert(is_const_reference_v<decltype(x)>);
    return true;
  });
   Cinq(std::make_unique<std::vector<int>>()).Where([](auto &x) {
    static_assert(is_const_reference_v<decltype(x)>);
    return true;
  });
}

void TestCinqSelectMany() {
  // $ is empty
  {
    assert(
      Cinq(empty_source).SelectMany([](auto) {return std::vector<int>{};})
        .ToVector().size() == 0
    );
  }

  // $ has one element # is empty
  {
    assert(
      Cinq(one_element).SelectMany([](auto) {return std::vector<int>{};})
        .ToVector().size() == 0
    );
  }

  // # has one element
  {
    auto query = Cinq(one_element).SelectMany([](auto x) {return std::vector<int>(1, x);})
      .ToVector();
    assert(
      query.size() == 1 && query[0] == 0
    );
  }

  // $ has five_elements # has multiple elements
  {
    auto query = Cinq(five_elements).SelectMany([](auto x) {return std::vector<int>(1, x);})
      .ToVector();
    assert(
      query.size() == five_elements.size() &&
      std::equal(query.begin(), query.begin(), five_elements.begin())
    );
  }
}

void TestCinqSelect() {
  // $ is empty # is empty
  {
    assert(
      Cinq(empty_source).Select([](auto) {return 0;})
        .ToVector().size() == 0
    );
  }

  // $ has one element # has one element
  {
    auto query = Cinq(one_element).Select([](auto x) {return x;})
      .ToVector();
    assert(
      query.size() == 1 && query[0] == one_element[0]
    );
  }

  // $ has five_elements # has multiple elements
  {
    auto query = Cinq(five_elements).Select([](auto x) {return x;})
      .ToVector();
    assert(
      query.size() == five_elements.size() &&
      std::equal(query.begin(), query.begin(), five_elements.begin())
    );
  }
}

void TestCinqJoin() {
  auto copy_forward = [](auto &&x) { return std::forward<decltype(x)>(x); };

  // $ is empty $ is empty
  {
    assert(
      Cinq(empty_source).Join(empty_source, copy_forward, copy_forward, copy_forward)
        .ToVector().size() == 0
    );
  }

  // $ has one element $ has one element # has one element
  {
    auto query = Cinq(one_element).Join(one_element, copy_forward, copy_forward, copy_forward)
      .ToVector();
    assert(
      query.size() == 1 &&
      query[0] == std::make_tuple(one_element[0], one_element[0])
    );
  }

  // $ has five element $ has five element # has multiple elements
  {
    auto query = Cinq(five_elements).Join(five_elements, copy_forward, copy_forward, copy_forward)
      .ToVector();
    assert(
      query.size() == five_elements.size() * five_elements.size() &&
      query[0] == std::make_tuple(five_elements[0], five_elements[0]) &&
      query.back() == std::make_tuple(five_elements.back(), five_elements.back())
    );
  }

  // $ is empty $ has one element # is empty
  {
    assert(
      Cinq(empty_source).Join(one_element, copy_forward, copy_forward, copy_forward)
        .ToVector().size() == 0
    );
  }

  // $ has one element $ is empty
  {
    assert(
      Cinq(one_element).Join(empty_source, copy_forward, copy_forward, copy_forward)
        .ToVector().size() == 0
    );
  }

  // $ is empty $ has five elements
  {
    assert(
      Cinq(empty_source).Join(five_elements, copy_forward, copy_forward, copy_forward)
        .ToVector().size() == 0
    );
  }

  // $ has five elements $ is empty
  {
    assert(
      Cinq(five_elements).Join(empty_source, copy_forward, copy_forward, copy_forward)
        .ToVector().size() == 0
    );
  }
  
  // $ has one element $ has five elements
  {
    auto query = Cinq(one_element).Join(five_elements, copy_forward, copy_forward, copy_forward)
      .ToVector();
    assert(
      query.size() == 5 &&
      query.front() == std::make_tuple(one_element.front(), five_elements.front()) &&
      query.back() == std::make_tuple(one_element.back(), five_elements.back()) 
    );
  }
  
  // $ has five elements $ has one element
  { 
    auto query = Cinq(five_elements).Join(one_element, copy_forward, copy_forward, copy_forward)
      .ToVector();
    assert(
      query.size() == 5 &&
      query.front() == std::make_tuple(five_elements.front(), one_element.front()) &&
      query.back() == std::make_tuple(five_elements.back(), one_element.back()) 
    );
  }
}

void TestCinqWhere() {
  // $ is empty
  {
    auto query = Cinq(empty_source).Where([](auto) {return true;}).ToVector();
    assert(query.size() == 0);
  }

  // $ has one element # is empty
  { 
    auto query = Cinq(one_element).Where([](auto) {return false;}).ToVector();
    assert(query.size() == 0);
  }

  // $ has five element # has multiple elements # excludes the first # includes a middle
  {
    auto query = Cinq(five_elements).Where([](auto x) {return x!=five_elements.front();})
      .ToVector();
    std::deque<int> result(five_elements.begin(), five_elements.end());
    result.pop_front();
    assert(query.size() == result.size() &&
      std::equal(query.begin(), query.end(), result.begin()));
  }

  // # includes the first # excludes the last
  {
    auto query = Cinq(five_elements).Where([](auto x) {return x!=five_elements.back();})
      .ToVector();
    std::deque<int> result(five_elements.begin(), five_elements.end());
    result.pop_back();
    assert(query.size() == result.size() &&
      std::equal(query.begin(), query.end(), result.begin()));
  }

  // # includes the last # excludes a middle
  {
    auto query = Cinq(five_elements).Where([](auto x) {return x!=five_elements[3];})
      .ToVector();
    std::list<int> result(five_elements.begin(), five_elements.end());
    result.remove_if([](auto x) { return x == five_elements[3]; });
    assert(query.size() == result.size() &&
      std::equal(query.begin(), query.end(), result.begin()));
  }

  // # includes all
  {
    auto query = Cinq(five_elements).Where([](auto) {return true;})
      .ToVector();
    assert(query.size() == five_elements.size() &&
      std::equal(query.begin(), query.end(), five_elements.begin()));
  }
}

void TestCinqToVector() {
  // postpond. ToVector is trivial and is used in most unit test.
}

void TestCinqIntersect() {
  auto copy_forward = [](auto x) {return x;};

  std::vector<int> source1, source2, source3;

  auto source_from_int = [](int x) {
    auto result = Cinq(std::to_string(x)).Select([](char c) -> int {return c-'0';}).ToVector();
    if (result[0] == 0 && result.size() == 1)
      return std::vector<int>();
    return result;
  };
  
  std::vector<int> test_set {
      0,1,2,3,4,5,6,7,8,9,10,
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

      auto query = Cinq(source1).Intersect(source2).ToVector();
      std::sort(query.begin(), query.end());
      assert(query.size() == result.size() &&
        std::equal(query.begin(), query.end(), result.begin()));

      for (int k : test_set) {
        source3 = source_from_int(k);
        std::sort(source3.begin(), source3.end());

        std::vector<int> result3;
        std::set_intersection(source3.begin(), source3.end(), result.begin(), result.end(), std::back_inserter(result3));
        std::sort(result3.begin(), result3.end());
        auto ite2 = std::unique(result3.begin(), result3.end());
        result3.erase(ite2, result3.end());

        auto query2 = Cinq(source1).Intersect(source2, source3).ToVector();
        std::sort(query2.begin(), query2.end());
        assert(query2.size() == result3.size() &&
          std::equal(query2.begin(), query2.end(), result3.begin()));
      }
    }
  }
}

void TestCinqUnion() {
  auto copy_forward = [](auto x) {return x;};

  std::vector<int> source1, source2, source3;

  auto source_from_int = [](int x) {
    auto result = Cinq(std::to_string(x)).Select([](char c) -> int {return c-'0';}).ToVector();
    if (result[0] == 0 && result.size() == 1)
      return std::vector<int>();
    return result;
  };
  
  std::vector<int> test_set {
      0,1,2,3,4,5,6,7,8,9,10,
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

      auto query = Cinq(source1).Union(source2).ToVector();
      std::sort(query.begin(), query.end());
      assert(query.size() == result.size() &&
        std::equal(query.begin(), query.end(), result.begin()));

      for (int k : test_set) {
        source3 = source_from_int(k);
        std::sort(source3.begin(), source3.end());

        std::vector<int> result3;
        std::set_union(source3.begin(), source3.end(), result.begin(), result.end(), std::back_inserter(result3));
        std::sort(result3.begin(), result3.end());
        auto ite2 = std::unique(result3.begin(), result3.end());
        result3.erase(ite2, result3.end());

        auto query2 = Cinq(source1).Union(source2, source3).ToVector();
        std::sort(query2.begin(), query2.end());
        assert(query2.size() == result3.size() &&
          std::equal(query2.begin(), query2.end(), result3.begin()));
      }
    }
  }
}

void TestCinqConcat() {
  std::vector<std::vector<int>> sources {
    empty_source, empty_source, empty_source,
    one_element, one_element, one_element,
    five_elements, five_elements, five_elements
  };

  // $ is empty $ has one element $ has five elements
  // # is empty # has one element # has multiple elements
  // # includes all
  for (auto &s1 : sources) {
    for (auto &s2 : sources) {
      
      auto query1 = Cinq(s1).Concat(s2).ToVector();
      std::vector<int> result1(s1);
      std::copy(s2.begin(), s2.end(), std::back_inserter(result1));
      assert(query1.size() == result1.size() &&
        std::equal(query1.begin(), query1.end(), result1.begin()));

      for (auto &s3 : sources) {
        auto query2 = Cinq(s1).Concat(s2, s3).ToVector();
        std::vector<int> result2(result1);
        std::copy(s3.begin(), s3.end(), std::back_inserter(result2));
        assert(query2.size() == result2.size() &&
          std::equal(query2.begin(), query2.end(), result2.begin()));
      }
    }
  }
}

void TestDistinct() {
  // Distinct is also used in some set operation internally

  // $ is empty
  {
    auto query = Cinq(empty_source).Distinct().ToVector();
    assert(query.size() == 0);
  }

  // $ has one # has one # include first
  {
    auto query = Cinq(one_element).Distinct().ToVector();
    assert(query.size() == 1 && query[0] == one_element[0]);
  }

  // $ has five # has multiple # include last # include a middle
  // # exclude a middle
  {
    std::vector<int> special {
      1, 12, 12, 12, 99
    };
    auto query = Cinq(special).Distinct().ToVector();
    assert(query.size() == 3 &&
      query[0] == 1 && query[1] == 12 && query[2] == 99);
  }

  // # has multiple and include all
  {
    std::vector<int> special {
      1, 2, 3, 4, 5
    };
    auto query = Cinq(special).Distinct().ToVector();
    assert(query.size() == special.size() &&
      std::equal(query.begin(), query.end(), special.begin()));
  }
}

int main() try {
  std::clog << "cinq-v3 test" << std::endl;

  TestCinqInitialization();
  TestCinqValueCategory();
  TestCinqSelectMany();
  TestCinqSelect();
  TestCinqJoin();
  TestCinqWhere();
  TestCinqToVector();
  TestCinqIntersect();
  TestCinqUnion();
  TestCinqConcat();
  TestDistinct();

  std::clog << std::endl;
  std::clog << "test finished" << std::endl;

  std::cin.get();
  return 0;
} catch(std::exception &e) {
  std::cout << e.what() << std::endl;
  std::cin.get();
}
