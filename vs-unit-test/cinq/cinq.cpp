// Cinq.cpp : Defines the entry point for the console application.
//

//
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
//
// Supplementary rules :
//  - test case can be combined.
//  - test can use arbitary element type. Case of using user-defined types is tested by semantics test.
//  - if the query can take infinit data sources, test up to the minimum number of data source requirement + 1.
//      And an additional trivial test for at least minimum count + 2.
//  - if the query takes multiple data sources, the input should consist of the full permutation of full combination of the case for data source if possible.
//  - if the query takes multiple data sources, the output should be tested for applying on every single data source.
//  - chained query is tested by chained query generator, and the result is compared with the result of
//      step by step query (insert ToVector in between). The correctness of this test is base on the assumption that each individual query is correct.
//
//
// Value category guarantees:
// In non-const verion:
//  For type of deferencing iterator
//    - if the result is stored in cinq's internal storage (such as set operation and Sort, except SelectMany), const lvalue
//    - if the result is from function object or iterator, no change
//    Note: Constness of prvalue will be removed.
//    Note: If the result is from multiple sources, if they all yield same reference to same cv type, this reference will be used,
//      otherwise, std::common_type will be used.
//    Note: cinq's internal storage should alise the element in origin source when possible.
//  For function object's parameter
//    - if source yields cv-lvalue, cv-lvalue
//    - otherwise, cv-xvalue
// In const version:
//  For type of deferencing iterator
//    - if the source yields xvalue or prvalue, no change
//    - otherwise, const lvalue
//    Note: Constness of prvalue will be removed.
//  For function object's parameter
//    - if source yields cv-rvalue, cv-xvalue
//    - otherwise, const lvalue
//
// Copy/Move guarantee
//  No copy/move will be performed, when all following requirements are met,
//    - User provided iterators yield ref. (except for set operation)
//    - User provided functions doesn't copy. (parameter and return value)
//    - In set operation, all source yield reference to same cv. (to be relaxed in future)
//
//
// .Const() makes all sources appear to be const
// .Cached() makes the final result cached (for accelerating multi-pass)
// .Trans()
//
//
// Requirement on predicate and container.

#include <cassert>

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

#include "cinq-v3.h"
using cinq_v3::Cinq;

#include "cinq-test.h"

void TestCinqNoCopyGuarantee()
{
  struct CopyDetector {
    CopyDetector() { ++GetCounter().ctor_counter_; }
    CopyDetector(const CopyDetector &) { ++GetCounter().copy_counter_; }
    CopyDetector(CopyDetector &&) noexcept { ++GetCounter().move_counter_; }

    CopyDetector &operator=(const CopyDetector &) { ++GetCounter().copy_counter_; }
    CopyDetector &operator=(CopyDetector &&) { ++GetCounter().move_counter_; }

    bool operator<(const CopyDetector &rhs) const {
      return this < &rhs;
    }

    struct Counter {
      size_t copy_counter_ = 0;
      size_t move_counter_ = 0;
      size_t ctor_counter_ = 0;
    };

    static volatile Counter &GetCounter() {
      static volatile Counter counter;
      return counter;
    }

    std::shared_ptr<int> ptr;
  };

  auto reset_counter = [&x = CopyDetector::GetCounter()]() { x.ctor_counter_ = x.copy_counter_ = x.move_counter_ = 0; };
  auto print_counter = [&x = CopyDetector::GetCounter()]() { std::cout << x.ctor_counter_ << ", " << x.copy_counter_ << ", " << x.move_counter_ << std::endl; };
  auto test_query = [&](auto &&query) {
      reset_counter();
      for (auto &&x : query) x;
      auto &x = CopyDetector::GetCounter();
      assert(x.ctor_counter_ == 0 && x.copy_counter_ == 0 && x.move_counter_ == 0);
    };

  std::vector<CopyDetector> vtr;
  vtr.resize(5);
  auto svtr = std::make_shared<std::vector<CopyDetector>>();
  svtr->resize(5);
  auto uvtr = std::make_unique<std::vector<CopyDetector>>();
  uvtr->resize(5);

  test_query(Cinq(vtr));
  test_query(Cinq(std::ref(vtr)));
  test_query(Cinq(std::cref(vtr)));
  test_query(Cinq(std::make_shared<std::vector<CopyDetector>>(vtr)));
  test_query(Cinq(std::make_unique<std::vector<CopyDetector>>(vtr)));

  test_query(Cinq(vtr).Where([](auto &&) {return true;}));
  test_query(Cinq(vtr).Select([](auto &&x) -> decltype(auto) {return x;}));
  test_query(Cinq(vtr).SelectMany([](auto &&){return std::vector<int>();}));
  test_query(Cinq(vtr).Join(vtr, [](auto &&x) -> decltype(auto) {return x;}, [](auto &&x) -> decltype(auto) {return x;}, [](auto &&a, auto &&b) {return std::tie(a, b);}));
  test_query(Cinq(vtr).Where([](auto &) {return true;}));
  test_query(Cinq(vtr).Select([](auto &x) -> decltype(auto) {return x;}));
  test_query(Cinq(vtr).SelectMany([](auto &){return std::vector<int>();}));
  test_query(Cinq(vtr).Join(vtr, [](auto &x) -> decltype(auto) {return x;}, [](auto &x) -> decltype(auto) {return x;}, [](auto &a, auto &b) {return std::tie(a, b);}));

  test_query(Cinq(vtr).Intersect(vtr));
  test_query(Cinq(vtr).Intersect(vtr, vtr));
  test_query(Cinq(vtr).Intersect(vtr, vtr, vtr));
  test_query(Cinq(vtr).Union(vtr));
  test_query(Cinq(vtr).Union(vtr, vtr));
  test_query(Cinq(vtr).Union(vtr, vtr, vtr));
  test_query(Cinq(vtr).Distinct());


  test_query(Cinq(svtr));
  test_query(Cinq(std::ref(svtr)));
  test_query(Cinq(std::cref(svtr)));

  test_query(Cinq(svtr).Where([](auto &&) {return true;}));
  test_query(Cinq(svtr).Select([](auto &&x) -> decltype(auto) {return x;}));
  test_query(Cinq(svtr).SelectMany([](auto &&){return std::vector<int>();}));
  test_query(Cinq(svtr).Join(svtr, [](auto &&x) -> decltype(auto) {return x;}, [](auto &&x) -> decltype(auto) {return x;}, [](auto &&a, auto &&b) {return std::tie(a, b);}));
  test_query(Cinq(svtr).Where([](auto &) {return true;}));
  test_query(Cinq(svtr).Select([](auto &x) -> decltype(auto) {return x;}));
  test_query(Cinq(svtr).SelectMany([](auto &){return std::vector<int>();}));
  test_query(Cinq(svtr).Join(svtr, [](auto &x) -> decltype(auto) {return x;}, [](auto &x) -> decltype(auto) {return x;}, [](auto &a, auto &b) {return std::tie(a, b);}));

  test_query(Cinq(svtr).Intersect(svtr));
  test_query(Cinq(svtr).Intersect(svtr, svtr));
  test_query(Cinq(svtr).Intersect(svtr, svtr, svtr));
  test_query(Cinq(svtr).Union(svtr));
  test_query(Cinq(svtr).Union(svtr, svtr));
  test_query(Cinq(svtr).Union(svtr, svtr, svtr));
  test_query(Cinq(svtr).Distinct());

  test_query(Cinq(std::ref(uvtr)));
  test_query(Cinq(std::ref(uvtr)).Where([](auto &&) {return true;}));
  test_query(Cinq(std::ref(uvtr)).Select([](auto &&x) -> decltype(auto) {return x;}));
  test_query(Cinq(std::ref(uvtr)).SelectMany([](auto &&){return std::vector<int>();}));
  test_query(Cinq(std::ref(uvtr)).Join(std::ref(uvtr), [](auto &&x) -> decltype(auto) {return x;}, [](auto &&x) -> decltype(auto) {return x;}, [](auto &&a, auto &&b) {return std::tie(a, b);}));
  test_query(Cinq(std::ref(uvtr)).Where([](auto &) {return true;}));
  test_query(Cinq(std::ref(uvtr)).Select([](auto &x) -> decltype(auto) {return x;}));
  test_query(Cinq(std::ref(uvtr)).SelectMany([](auto &){return std::vector<int>();}));
  test_query(Cinq(std::ref(uvtr)).Join(std::ref(uvtr), [](auto &x) -> decltype(auto) {return x;}, [](auto &x) -> decltype(auto) {return x;}, [](auto &a, auto &b) {return std::tie(a, b);}));

  test_query(Cinq(std::ref(uvtr)).Intersect(std::ref(uvtr)));
  test_query(Cinq(std::ref(uvtr)).Intersect(std::ref(uvtr), std::ref(uvtr)));
  test_query(Cinq(std::ref(uvtr)).Intersect(std::ref(uvtr), std::ref(uvtr), std::ref(uvtr)));
  test_query(Cinq(std::ref(uvtr)).Union(std::ref(uvtr)));
  test_query(Cinq(std::ref(uvtr)).Union(std::ref(uvtr), std::ref(uvtr)));
  test_query(Cinq(std::ref(uvtr)).Union(std::ref(uvtr), std::ref(uvtr), std::ref(uvtr)));
  test_query(Cinq(std::ref(uvtr)).Distinct());
}

void SetOperationInternalContainerTest() {
  struct LessThanComparable {
    bool operator<(const LessThanComparable &) const { return true; }
  };

  static_assert(!cinq::utility::ReferenceWrapper<LessThanComparable>::hash_version);
  Cinq(std::vector<LessThanComparable>()).Intersect(std::vector<LessThanComparable>());
  Cinq(std::vector<LessThanComparable>()).Union(std::vector<LessThanComparable>());
  Cinq(std::vector<LessThanComparable>()).Distinct();

  static_assert(cinq::utility::ReferenceWrapper<int>::hash_version);
}

void SetOperationAliasTest() {
  std::vector<int> vtr{1,2,3};
  auto svtr = std::make_shared<std::vector<int>>(std::vector{1, 2, 3});
  auto uvtr = std::make_unique<std::vector<int>>(std::vector{1, 2, 3});
  
  // ref
  auto address_of = [](const auto &e) { return &e; };
  auto address = Cinq<int>().Concat(std::ref(vtr), svtr, std::ref(uvtr)).Select(address_of).ToSet();

  auto aliased_address =  Cinq<int>().Concat(
      Cinq(std::ref(vtr)).Intersect(std::ref(vtr)),
      Cinq(std::cref(vtr)).Intersect(std::cref(vtr)),
      Cinq(std::ref(vtr)).Union(std::ref(vtr)),
      Cinq(std::cref(vtr)).Union(std::cref(vtr)),
      Cinq(std::ref(vtr)).Intersect(std::ref(vtr), std::ref(vtr)),
      Cinq(std::cref(vtr)).Intersect(std::cref(vtr), std::cref(vtr)),
      Cinq(std::ref(vtr)).Union(std::ref(vtr), std::ref(vtr)),
      Cinq(std::cref(vtr)).Union(std::cref(vtr), std::cref(vtr)),
      Cinq(std::ref(vtr)).Distinct(),
      Cinq(std::cref(vtr)).Distinct(),

      Cinq(std::ref(uvtr)).Intersect(std::ref(uvtr)),
      Cinq(std::cref(uvtr)).Intersect(std::cref(uvtr)),
      Cinq(std::ref(uvtr)).Union(std::ref(uvtr)),
      Cinq(std::cref(uvtr)).Union(std::cref(uvtr)),
      Cinq(std::ref(uvtr)).Intersect(std::ref(uvtr), std::ref(uvtr)),
      Cinq(std::cref(uvtr)).Intersect(std::cref(uvtr), std::cref(uvtr)),
      Cinq(std::ref(uvtr)).Union(std::ref(uvtr), std::ref(uvtr)),
      Cinq(std::cref(uvtr)).Union(std::cref(uvtr), std::cref(uvtr)),
      Cinq(std::ref(uvtr)).Distinct(),
      Cinq(std::cref(uvtr)).Distinct(),

      Cinq(svtr).Intersect(svtr),
      Cinq(svtr).Union(svtr),
      Cinq(svtr).Intersect(svtr, svtr),
      Cinq(svtr).Union(svtr, svtr),
      Cinq(svtr).Distinct(),
      Cinq(svtr).Intersect(svtr),
      Cinq(svtr).Union(svtr),
      Cinq(svtr).Intersect(svtr, svtr),
      Cinq(svtr).Union(svtr, svtr)
    ).Select(address_of).ToSet();

  assert(std::all_of(aliased_address.begin(), aliased_address.end(), [&address](auto x) { return address.insert(x).second == false; }));
}

int main() try {
  TestCinqNoCopyGuarantee();
  SetOperationInternalContainerTest();
  SetOperationAliasTest();

#ifdef ENABLE_TEST
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
#endif // ENABLE_TEST

  std::cin.get();
  return 0;
} catch(std::exception &e) {
  std::cout << e.what() << std::endl;
  std::cin.get();
}
