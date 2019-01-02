#pragma once

#include <any>
#include <deque>
#include <forward_list>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <utility>
#include <vector>
#include <numeric>
#include <string>
#include <functional>

#include "cinq.h"
#include "cinq-test-utility.h"
#include "detail/utility.h"
using cinq::Cinq;

#define ENABLE_TEST

#ifdef ENABLE_TEST

namespace cinq_test {
void CompileTimeValueCategoryTest();
void RuntimeValueCategoryTest();
void IteratorDefaultConstructTest();

void LifeTimeVtrTest();
void LifeTimeRefVtrTest();
void LifeTimeCRefVtrTest();
void LifeTimeSharedPtrVtrTest();
void SelectManyAliasingTest();

void IntersectTest();
void UnionTest();
void ConcatTest();
void DistinctTest();

void NoCopyGuaranteeTest();
void SetOperationInternalContainerTest();
void SetOperationAliasTest();

void TestCinqAggregate();
void TestCinqEmpty();
void TestCinqAny();
void TestCinqAverage();
void TestCinqAll();
void TestCinqAppend();
void TestCinqContain();
void TestCinqCount();
void TestCinqDefaultIfEmpty();

class MiniContainer {
public:
  auto begin() { return std::begin(data); }
  auto end() { return std::end(data); }

  auto begin() const { return std::cbegin(data); }
  auto end() const { return std::cend(data); }

private:
  LifeTimeCheckInt data[5] = { 1, 2, 3, 4, 5 };
};

void TestCinqInitialization() {
  std::vector<LifeTimeCheckInt> vtr{ 1, 2, 3, 4, 5 };
  LifeTimeCheckInt arr[] = { 1, 2, 3, 4, 5 };
  MiniContainer c;
  auto shared_vtr = std::make_shared<std::vector<LifeTimeCheckInt>>();

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

  Cinq(std::vector<LifeTimeCheckInt>{1, 2, 3, 4, 5});
  Cinq(std::forward_list<LifeTimeCheckInt>{1, 2, 3, 4, 5});
  Cinq(MiniContainer());
  Cinq({ 1, 2, 3, 4, 5 });

  Cinq(shared_vtr);
  Cinq(std::make_unique<std::vector<LifeTimeCheckInt>>());

  Cinq(std::ref(shared_vtr));
  Cinq(std::cref(shared_vtr));
  Cinq(std::move(shared_vtr));

  auto unique_vtr = std::make_unique<std::vector<LifeTimeCheckInt>>();
  Cinq(std::ref(unique_vtr));
  Cinq(std::cref(unique_vtr));
}

void TestCinqSelectMany() {
  // $ is empty
  {
    auto query = Cinq(empty_source).SelectMany([](auto) {return std::vector<LifeTimeCheckInt>{}; });
    cinq::utility::CinqAssert(
      ToVector(query).size() == 0 &&
      ToVector(query).size() == 0
    );
  }

  // $ has one element # is empty
  {
    auto query = Cinq(one_element).SelectMany([](auto) {return std::vector<LifeTimeCheckInt>{}; });
    cinq::utility::CinqAssert(
      ToVector(query).size() == 0 &&
      ToVector(query).size() == 0
    );
  }

  // # has one element
  {
    auto query = Cinq(one_element).SelectMany([](auto x) {return std::vector<LifeTimeCheckInt>(1, x); });
    cinq::utility::CinqAssert(
      ToVector(query).size() == 1 && ToVector(query)[0] == 0 &&
      ToVector(query).size() == 1 && ToVector(query)[0] == 0
    );
  }

  // $ has five_elements # has multiple elements
  {
    auto query = Cinq(five_elements).SelectMany([](auto x) {return std::vector<LifeTimeCheckInt>(1, x); });
    auto vtr = ToVector(query);
    cinq::utility::CinqAssert(
      vtr.size() == five_elements.size() &&
      std::equal(vtr.begin(), vtr.begin(), five_elements.begin())
    );
    vtr = ToVector(query);
    cinq::utility::CinqAssert(
      vtr.size() == five_elements.size() &&
      std::equal(vtr.begin(), vtr.begin(), five_elements.begin())
    );
  }
}

void TestCinqSelect() {
  // $ is empty # is empty
  {
    auto query = Cinq(empty_source).Select([](auto) {return 0; });
    cinq::utility::CinqAssert(
      ToVector(query).size() == 0 &&
      ToVector(query).size() == 0
    );
  }

  // $ has one element # has one element
  {
    auto query = Cinq(one_element).Select([](auto x) {return x; });
    auto vtr = ToVector(query);
    cinq::utility::CinqAssert(
      vtr.size() == 1 && vtr[0] == one_element[0]
    );
    vtr = ToVector(query);
    cinq::utility::CinqAssert(
      vtr.size() == 1 && vtr[0] == one_element[0]
    );
  }

  // $ has five_elements # has multiple elements
  {
    auto query = Cinq(five_elements).Select([](auto x) {return x; });
    auto vtr = ToVector(query);
    cinq::utility::CinqAssert(
      vtr.size() == five_elements.size() &&
      std::equal(vtr.begin(), vtr.begin(), five_elements.begin())
    );
    vtr = ToVector(query);
    cinq::utility::CinqAssert(
      vtr.size() == five_elements.size() &&
      std::equal(vtr.begin(), vtr.begin(), five_elements.begin())
    );
  }
}

void TestCinqJoin() {
  // $ is empty
  {
    {
      auto query = ToVector(Cinq(empty_source).Join(empty_source, [](auto) {return 0;}, [](auto) {return 0;}, [](auto, auto) {return 0;}));
      cinq::utility::CinqAssert(query.size() == 0);
    }
  }

  // $ has one element # is empty
  {
    {
      auto query = ToVector(Cinq(one_element).Join(one_element, [](auto) {return 0;}, [](auto) {return 1;}, [](auto, auto) {return 0;}));
      cinq::utility::CinqAssert(query.size() == 0);
    }
    {
      auto query = ToVector(Cinq(one_element).Join(one_element, [](auto) {return 0;}, [](auto) {return 1;}, [](auto, auto) {return 0;}));
      cinq::utility::CinqAssert(query.size() == 0);
    }
  }

  // $ has five element # has multiple elements # includes a middle # includes the first # includes all
  {
    {
      auto query = ToVector(Cinq(five_elements).Join(five_elements, [](auto) {return 0;}, [](auto) {return 0;}, [](auto a, auto b) {return a*1000 + b;}));
      cinq::utility::CinqAssert(query.size() == 25 && query.front() == five_elements.front() *1001 && query.back() == five_elements.back() *1001);
    }
    {
      auto query = ToVector(Cinq(five_elements).Join(five_elements, [](auto) {return 0;}, [](auto) {return 0;}, [](auto a, auto b) {return a*1000 + b;}));
      cinq::utility::CinqAssert(query.size() == 25 && query.front() == five_elements.front() *1001 && query.back() == five_elements.back() *1001);
    }
  }

  // # excludes the first # excludes a middle
  {
    auto last = five_elements.back();
    {
      auto query = ToVector(Cinq(five_elements).Join(five_elements, [last](auto x) {return x==last ? 0 : 2;}, [last](auto x) {return x==last ? 0 : 1;}, [](auto, auto) {return 0;}));
      cinq::utility::CinqAssert(query.size() == 1);
    }
    {
      auto query = ToVector(Cinq(five_elements).Join(five_elements, [last](auto x) {return x==last ? 0 : 2;}, [last](auto x) {return x==last ? 0 : 1;}, [](auto, auto) {return 0;}));
      cinq::utility::CinqAssert(query.size() == 1);
    }
  }

  std::vector<std::vector<int>> sources;
  sources.resize(10);

  for (int i=0; i<10; ++i) {
    for (int j=0; j<10; ++j)
      sources[i].push_back((i+j)%10);
  }

  for (const auto &source1 : sources) {
    for (const auto &source2 : sources) {
      
      auto query = ToVector(Cinq(source1).Join(source2, [](auto x) {return x;}, [](auto x) {return x;}, [](auto x, auto y) {return std::make_tuple(x, y);}));

      std::vector<std::tuple<int, int>> hand_write_result;

      for (auto x : source1) {
        for (auto y : source2)
         if (x == y)
          hand_write_result.emplace_back(x, y);
      }

      cinq::utility::CinqAssert(query.size() == hand_write_result.size() &&
          std::equal(query.begin(), query.end(), hand_write_result.begin()));
    }
  }
}

void TestCinqWhere() {
  // $ is empty
  {
    auto query = ToVector(Cinq(empty_source).Where([](auto) {return true; }));
    cinq::utility::CinqAssert(query.size() == 0);
  }

  // $ has one element # is empty
  {
    auto query = ToVector(Cinq(one_element).Where([](auto) {return false; }));
    cinq::utility::CinqAssert(query.size() == 0 && query.size() == 0);
  }

  // $ has five element # has multiple elements # excludes the first # includes a middle
  {
    auto query = Cinq(five_elements).Where([](auto x) {return x != five_elements.front(); });
    std::deque<LifeTimeCheckInt> result(five_elements.begin(), five_elements.end());
    result.pop_front();

    auto vtr = ToVector(query);
    cinq::utility::CinqAssert(vtr.size() == result.size() &&
      std::equal(vtr.begin(), vtr.end(), result.begin()));
    vtr = ToVector(query);
    cinq::utility::CinqAssert(vtr.size() == result.size() &&
      std::equal(vtr.begin(), vtr.end(), result.begin()));
  }

  // # includes the first # excludes the last
  {
    auto query = Cinq(five_elements).Where([](auto x) {return x != five_elements.back(); });
    std::deque<LifeTimeCheckInt> result(five_elements.begin(), five_elements.end());
    result.pop_back();

    auto vtr = ToVector(query);
    cinq::utility::CinqAssert(vtr.size() == result.size() &&
      std::equal(vtr.begin(), vtr.end(), result.begin()));
    vtr = ToVector(query);
    cinq::utility::CinqAssert(vtr.size() == result.size() &&
      std::equal(vtr.begin(), vtr.end(), result.begin()));
  }

  // # includes the last # excludes a middle
  {
    auto query = Cinq(five_elements).Where([](auto x) {return x != five_elements[3]; });
    std::list<LifeTimeCheckInt> result(five_elements.begin(), five_elements.end());
    result.remove_if([](auto x) { return x == five_elements[3]; });

    auto vtr = ToVector(query);
    cinq::utility::CinqAssert(vtr.size() == result.size() &&
      std::equal(vtr.begin(), vtr.end(), result.begin()));
    vtr = ToVector(query);
    cinq::utility::CinqAssert(vtr.size() == result.size() &&
      std::equal(vtr.begin(), vtr.end(), result.begin()));
  }

  // # includes all
  {
    auto query = Cinq(five_elements).Where([](auto) {return true; });
    auto vtr = ToVector(query);
    cinq::utility::CinqAssert(vtr.size() == five_elements.size() &&
      std::equal(vtr.begin(), vtr.end(), five_elements.begin()));
    vtr = ToVector(query);
    cinq::utility::CinqAssert(vtr.size() == five_elements.size() &&
      std::equal(vtr.begin(), vtr.end(), five_elements.begin()));
  }
}

void TestCinqToVector() {
  // postpond. ToVector is trivial and is used in most unit test.
}

} // namespace cinq_test

#endif // ENABLE_TEST
