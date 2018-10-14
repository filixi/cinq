#pragma once

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


#define ENABLE_TEST

#ifdef ENABLE_TEST

namespace cinq_test {
void CompileTimeValueCategoryTest();
void RuntimeValueCategoryTest();

void BasicCombinedQueryTest();

void IntersectTest();
void UnionTest();
void ConcatTest();
void DistinctTest();

void NoCopyGuaranteTeste();
void SetOperationInternalContainerTest();
void SetOperationAliasTest();

} // namespace cinq_test

const std::vector<int> empty_source;
const std::vector<int> one_element{ 0 };
const std::vector<int> five_elements{ 0, 1, 2, 3, 4 }; // elements must be unique

class MiniContainer {
public:
  auto begin() { return std::begin(data); }
  auto end() { return std::end(data); }

  auto begin() const { return std::cbegin(data); }
  auto end() const { return std::cend(data); }

private:
  int data[5] = { 1, 2, 3, 4, 5 };
};

void TestCinqInitialization() {
  std::vector<int> vtr{ 1, 2, 3, 4, 5 };
  int arr[] = { 1, 2, 3, 4, 5 };
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

  Cinq(std::vector<int>{1, 2, 3, 4, 5});
  Cinq(std::forward_list<int>{1, 2, 3, 4, 5});
  Cinq(MiniContainer());
  Cinq({ 1, 2, 3, 4, 5 });

  Cinq(shared_vtr);
  Cinq(std::make_unique<std::vector<int>>());
}

void TestCinqSelectMany() {
  // $ is empty
  {
    assert(
      Cinq(empty_source).SelectMany([](auto) {return std::vector<int>{}; })
      .ToVector().size() == 0
    );
  }

  // $ has one element # is empty
  {
    assert(
      Cinq(one_element).SelectMany([](auto) {return std::vector<int>{}; })
      .ToVector().size() == 0
    );
  }

  // # has one element
  {
    auto query = Cinq(one_element).SelectMany([](auto x) {return std::vector<int>(1, x); })
      .ToVector();
    assert(
      query.size() == 1 && query[0] == 0
    );
  }

  // $ has five_elements # has multiple elements
  {
    auto query = Cinq(five_elements).SelectMany([](auto x) {return std::vector<int>(1, x); })
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
      Cinq(empty_source).Select([](auto) {return 0; })
      .ToVector().size() == 0
    );
  }

  // $ has one element # has one element
  {
    auto query = Cinq(one_element).Select([](auto x) {return x; })
      .ToVector();
    assert(
      query.size() == 1 && query[0] == one_element[0]
    );
  }

  // $ has five_elements # has multiple elements
  {
    auto query = Cinq(five_elements).Select([](auto x) {return x; })
      .ToVector();
    assert(
      query.size() == five_elements.size() &&
      std::equal(query.begin(), query.begin(), five_elements.begin())
    );
  }
}

void TestCinqJoin() {
  // Remind user of the life-time trap of CinqJoin, that the last selector's parameter is from the previous selector.
  auto copy_forward_element = [](auto &&x) { return std::forward<decltype(x)>(x); };
  auto copy_forward_as_tuple = [](auto &&x, auto &&y) {
    return std::make_tuple(std::forward<decltype(x)>(x), std::forward<decltype(y)>(y));
  };
  auto copy_forward = [copy_forward_element, copy_forward_as_tuple](auto&&... args) -> decltype(auto) {
    if constexpr (sizeof...(args) == 1)
      return copy_forward_element(std::forward<decltype(args)>(args)...);
    else
      return copy_forward_as_tuple(std::forward<decltype(args)>(args)...);
  };

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
    auto query = Cinq(empty_source).Where([](auto) {return true; }).ToVector();
    assert(query.size() == 0);
  }

  // $ has one element # is empty
  {
    auto query = Cinq(one_element).Where([](auto) {return false; }).ToVector();
    assert(query.size() == 0);
  }

  // $ has five element # has multiple elements # excludes the first # includes a middle
  {
    auto query = Cinq(five_elements).Where([](auto x) {return x != five_elements.front(); })
      .ToVector();
    std::deque<int> result(five_elements.begin(), five_elements.end());
    result.pop_front();
    assert(query.size() == result.size() &&
      std::equal(query.begin(), query.end(), result.begin()));
  }

  // # includes the first # excludes the last
  {
    auto query = Cinq(five_elements).Where([](auto x) {return x != five_elements.back(); })
      .ToVector();
    std::deque<int> result(five_elements.begin(), five_elements.end());
    result.pop_back();
    assert(query.size() == result.size() &&
      std::equal(query.begin(), query.end(), result.begin()));
  }

  // # includes the last # excludes a middle
  {
    auto query = Cinq(five_elements).Where([](auto x) {return x != five_elements[3]; })
      .ToVector();
    std::list<int> result(five_elements.begin(), five_elements.end());
    result.remove_if([](auto x) { return x == five_elements[3]; });
    assert(query.size() == result.size() &&
      std::equal(query.begin(), query.end(), result.begin()));
  }

  // # includes all
  {
    auto query = Cinq(five_elements).Where([](auto) {return true; })
      .ToVector();
    assert(query.size() == five_elements.size() &&
      std::equal(query.begin(), query.end(), five_elements.begin()));
  }
}

void TestCinqToVector() {
  // postpond. ToVector is trivial and is used in most unit test.
}

#endif // ENABLE_TEST

