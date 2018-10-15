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

#include <iostream>
#include <exception>
#include <thread>
#include <vector>

#include "cinq-test.h"

int main() try {

#ifdef ENABLE_TEST
  std::clog << "cinq-v3 test" << std::endl;

  std::vector<std::thread> threads;

  threads.emplace_back(TestCinqInitialization);
  threads.emplace_back(cinq_test::CompileTimeValueCategoryTest);
  threads.emplace_back(cinq_test::RuntimeValueCategoryTest);
  
  threads.emplace_back(cinq_test::BasicCombinedQueryTest);

  threads.emplace_back(cinq_test::NoCopyGuaranteeTest);
  threads.emplace_back(cinq_test::SetOperationInternalContainerTest);
  threads.emplace_back(cinq_test::SetOperationAliasTest);

  threads.emplace_back(TestCinqSelectMany);
  threads.emplace_back(TestCinqSelect);
  threads.emplace_back(TestCinqJoin);
  threads.emplace_back(TestCinqWhere);
  threads.emplace_back(TestCinqToVector);
  threads.emplace_back(cinq_test::IntersectTest);
  threads.emplace_back(cinq_test::UnionTest);
  threads.emplace_back(cinq_test::ConcatTest);
  threads.emplace_back(cinq_test::DistinctTest);

  for (auto &th : threads)
    th.join();

  std::clog << std::endl;
  std::clog << "test finished" << std::endl;
#endif // ENABLE_TEST

  std::cin.get();
  return 0;
} catch(std::exception &e) {
  std::cout << e.what() << std::endl;
  std::cin.get();
}
