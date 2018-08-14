// XLinq.cpp : Defines the entry point for the console application.
//

#include <cassert>

#include <forward_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>
#include <numeric>

#include "cinq-v3.h"
using cinq_v3::Cinq;
using namespace cinq::utility;

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
  std::vector<int> vtr{1, 3, 5};

  auto genrator = [](int x) {return std::vector<int>(x, x);};
  auto query1 = Cinq(vtr).SelectMany(genrator);
  std::vector<int> result;
  for (auto x : query1)
    result.push_back(x);
  assert(result.size() == 9);
  assert(std::accumulate(result.begin(), result.end(), 0) == 35);

  int count = 0;
  auto query2 = Cinq(std::vector<int>{}).SelectMany(genrator);
  for (auto x : query2)
    ++count;
  assert(count == 0);
}

void TestCinqSelect() {
  std::vector<int> vtr{1, 3, 5};

  auto genrator = [](int x) {return x*2;};
  auto query1 = Cinq(vtr).Select(genrator);
  std::vector<int> result;
  for (auto x : query1)
    result.push_back(x);
  assert(result.size() == 3);
  assert(std::accumulate(result.begin(), result.end(), 0) == 18);

  int count = 0;
  auto query2 = Cinq(std::vector<int>{}).Select(genrator);
  for (auto x : query2)
    ++count;
  assert(count == 0);
}

void TestCinqJoin() {
  std::vector<int> vtr{1, 3, 5};
  std::vector<int> vtr2{2, 4, 6};

  auto query = Cinq(vtr).Join(vtr2, [](auto x) { return x; }, [](auto y) { return y; }, [](auto z) { return z; });
  std::vector<std::tuple<int, int>> result;
  for (auto v : query)
    result.push_back(v);
  assert(result.size() == 9);
}

void TestCinqWhere() {
  std::vector<int> vtr{1,2,3,4,5};

  auto predicate = [](int x) {return x%2 == 0;};
  auto query1 = Cinq(vtr).Where(predicate);
  for (auto x : query1)
    assert(predicate(x));

  int count = 0;
  auto query2 = Cinq(std::vector<int>{}).Where(predicate);
  for (auto x : query2)
    ++count;
  assert(count == 0);
}

void TestCinqToVector() {
  std::vector<int> vtr{1,2,3,4,5};

  auto raw = Cinq(vtr).ToVector();
  assert(raw.size() == vtr.size());
  
  auto where = Cinq(vtr).Where([](auto x) { return x%2==0; }).ToVector();
  assert(where.size() == 2);

  auto select_many = Cinq(vtr).SelectMany([](auto) {return std::vector<int>{1};}).ToVector();
  assert(select_many.size() == 5);
}

int main() {
  std::clog << "cinq-v3 test" << std::endl;

  TestCinqInitialization();
  TestCinqValueCategory();
  TestCinqSelectMany();
  TestCinqSelect();
  TestCinqJoin();
  TestCinqWhere();
  TestCinqToVector();

  std::clog << std::endl;
  std::clog << "test finished" << std::endl;

  std::cin.get();
  return 0;
}
