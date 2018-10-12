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

#include "value-category-test.h"

#define ENABLE_TEST

#ifdef ENABLE_TEST

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

void TestCinqValueCategory() {
  // run-time test
  cinq_test::ValueCategoryTestUnit().Test();

  // compile-time test
#define CONST_LVALUE_REFERENCE_ASSERT(T) static_assert(cinq::utility::is_const_lvalue_reference_v<T>, "const lvalue ref assert failed.")
#define NON_CONST_LVALUE_REFERENCE_ASSERT(T) static_assert(cinq::utility::is_non_const_lvalue_reference_v<T>, "non-const lvalue ref assert failed.")
#define NON_CONST_RVALUE_REFERENCE_ASSERT(T) static_assert(cinq::utility::is_non_const_rvalue_reference_v<T>, "non-const lvalue ref assert failed.")
#define CONST_NON_REFERENCE_ASSERT(T) static_assert(!std::is_class_v<T> && !std::is_array_v<T> || std::is_const_v<T> && !std::is_reference_v<T>, "const non-ref assert failed.")
#define NON_CONST_NON_REFERENCE_ASSERT(T) static_assert(!std::is_const_v<T> && !std::is_reference_v<T>, "non-const non-ref assert failed.")

#define ASSERT_ITERATOR_YIELD_TYPE(query, norm, cnst)\
  using IteratorYieldType = decltype(*query.begin()); using ConstIteratorYieldType = decltype(*query.cbegin());\
  norm(IteratorYieldType); cnst(ConstIteratorYieldType);

#define ASSERT_QUERY_ITERATOR_YIELD_LVALUE(query) {ASSERT_ITERATOR_YIELD_TYPE(query, NON_CONST_LVALUE_REFERENCE_ASSERT, CONST_LVALUE_REFERENCE_ASSERT)}
#define ASSERT_CONST_QUERY_ITERATOR_YIELD_LVALUE(query) {ASSERT_ITERATOR_YIELD_TYPE(query, CONST_LVALUE_REFERENCE_ASSERT, CONST_LVALUE_REFERENCE_ASSERT)}

#define ASSERT_QUERY_ITERATOR_YIELD_CONST_LVALUE(query) {ASSERT_ITERATOR_YIELD_TYPE(query, CONST_LVALUE_REFERENCE_ASSERT, CONST_LVALUE_REFERENCE_ASSERT)}
#define ASSERT_CONST_QUERY_ITERATOR_YIELD_CONST_LVALUE(query) {ASSERT_ITERATOR_YIELD_TYPE(query, CONST_LVALUE_REFERENCE_ASSERT, CONST_LVALUE_REFERENCE_ASSERT)}

#define ASSERT_QUERY_ITERATOR_YIELD_PRVALUE(query) {ASSERT_ITERATOR_YIELD_TYPE(query, NON_CONST_NON_REFERENCE_ASSERT, NON_CONST_NON_REFERENCE_ASSERT)}
#define ASSERT_CONST_QUERY_ITERATOR_YIELD_PRVALUE(query) {ASSERT_ITERATOR_YIELD_TYPE(query, NON_CONST_NON_REFERENCE_ASSERT, NON_CONST_NON_REFERENCE_ASSERT)}

#define ASSERT_QUERY_ITERATOR_YIELD_CONST_PRVALUE(query) {ASSERT_ITERATOR_YIELD_TYPE(query, CONST_NON_REFERENCE_ASSERT, CONST_NON_REFERENCE_ASSERT)}
#define ASSERT_CONST_QUERY_ITERATOR_YIELD_CONST_PRVALUE(qeury) {ASSERT_ITERATOR_YIELD_TYPE(query, CONST_NON_REFERENCE_ASSERT, CONST_NON_REFERENCE_ASSERT)}

  // create a list of tester

  // create a list of visitor

  // create a list of source (returning different value category)

  auto x = Cinq(std::vector<std::string>());
  using CinqType = decltype(x);
  auto p = &CinqType::Select<std::function<int(std::string)>>;

  // std::cout << typeid(p).name() << std::endl;

  (std::move(x).*p)([](std::string) {return 0;});

  // Select
  {
    {
      auto query = Cinq(one_element).Select([](auto &&x) -> decltype(auto) {
        NON_CONST_LVALUE_REFERENCE_ASSERT(decltype(x));
        return std::forward<decltype(x)>(x);
      });
      query.ToVector();

      ASSERT_QUERY_ITERATOR_YIELD_LVALUE(query)
    }

    {
      auto query = Cinq(one_element).Const().Select([](auto &&x) -> decltype(auto) {
        CONST_LVALUE_REFERENCE_ASSERT(decltype(x));
        return std::forward<decltype(x)>(x);
      });
      query.ToVector();

      ASSERT_CONST_QUERY_ITERATOR_YIELD_LVALUE(query)
    }

    {
      auto query = Cinq(one_element).Select([](auto &&) { return std::string(); });
      ASSERT_QUERY_ITERATOR_YIELD_PRVALUE(query);
    }

    {
      auto query = Cinq(one_element).Const().Select([](auto &&) { return std::string(); });
      ASSERT_CONST_QUERY_ITERATOR_YIELD_PRVALUE(query);
    }

    {
      auto query = Cinq(one_element).Select([](auto &&x) -> const std::string { return std::string(); });
      ASSERT_QUERY_ITERATOR_YIELD_PRVALUE(query);
    }

    {
      auto query = Cinq(one_element).Const().Select([](auto &&x) -> const std::string { return std::string(); });
      ASSERT_CONST_QUERY_ITERATOR_YIELD_PRVALUE(query);
    }
  }
  
  // SelectMany
  {
    {
      auto query = Cinq(one_element).SelectMany([](auto &&x) {
        NON_CONST_LVALUE_REFERENCE_ASSERT(decltype(x));
        return std::vector<int>();
      });
      query.ToVector();

      ASSERT_QUERY_ITERATOR_YIELD_LVALUE(query)
    }

    {
      auto query = Cinq(one_element).Const().SelectMany([](auto &&x) {
        CONST_LVALUE_REFERENCE_ASSERT(decltype(x));
        return std::vector<int>();
      });
      query.ToVector();

      ASSERT_CONST_QUERY_ITERATOR_YIELD_LVALUE(query)
    }

    struct Conatiner {
      struct Iterator {
        bool operator==(Iterator) const {return true;}
        bool operator!=(Iterator) const {return false;}
        Iterator &operator++() {return *this;}
        Iterator operator++(int) {return {};}
        std::string operator*() {return {};}
        std::string operator*() const {return {};}
      };

      Iterator begin() {return {};}
      Iterator end() {return {};}
      Iterator begin() const {return {};}
      Iterator end() const {return {};}
    };

    struct CConatiner {
      struct Iterator {
        bool operator==(Iterator) const {return true;}
        bool operator!=(Iterator) const {return false;}
        Iterator &operator++() {return *this;}
        Iterator operator++(int) {return {};}
        const std::string operator*() {return {};}
        const std::string operator*() const {return {};}
      };

      Iterator begin() {return {};}
      Iterator end() {return {};}
      Iterator begin() const {return {};}
      Iterator end() const {return {};}
    };

    {
      auto query = Cinq(one_element).SelectMany([](auto &&) { return Conatiner(); });

      ASSERT_QUERY_ITERATOR_YIELD_PRVALUE(query)
    }

    {
      auto query = Cinq(one_element).Const().SelectMany([](auto &&) { return Conatiner(); });

      ASSERT_CONST_QUERY_ITERATOR_YIELD_PRVALUE(query)
    }

    {
      auto query = Cinq(one_element).SelectMany([](auto &&) { return CConatiner(); });

      ASSERT_QUERY_ITERATOR_YIELD_PRVALUE(query)
    }

    {
      auto query = Cinq(one_element).Const().SelectMany([](auto &&) { return CConatiner(); });

      ASSERT_CONST_QUERY_ITERATOR_YIELD_PRVALUE(query)
    }
  }

  // Where

  // Join
  {
    {
      std::string s;
      auto query = Cinq(one_element).Join(one_element, [](auto &&x) -> decltype(auto) {
          NON_CONST_LVALUE_REFERENCE_ASSERT(decltype(x));
          return std::forward<decltype(x)>(x);
        }, [](auto &&x) -> decltype(auto) {
          NON_CONST_LVALUE_REFERENCE_ASSERT(decltype(x));
          return x;
        }, [&s](auto &&first, auto &&second) mutable -> std::string & {
          NON_CONST_LVALUE_REFERENCE_ASSERT(decltype(first));
          NON_CONST_LVALUE_REFERENCE_ASSERT(decltype(second));
          return s;
        });
      query.ToVector();

      ASSERT_QUERY_ITERATOR_YIELD_LVALUE(query)
    }

    {
      auto query = Cinq(one_element).Const().Join(one_element, [](auto &&x) -> decltype(auto) {
          CONST_LVALUE_REFERENCE_ASSERT(decltype(x));
          return std::forward<decltype(x)>(x);
        }, [](auto &&x) -> decltype(auto) {
          CONST_LVALUE_REFERENCE_ASSERT(decltype(x));
          return x;
        }, [](auto &&first, auto &&second) {
          CONST_LVALUE_REFERENCE_ASSERT(decltype(first));
          CONST_LVALUE_REFERENCE_ASSERT(decltype(second));
          return std::tie(first, second);
        });
      query.ToVector();

      ASSERT_CONST_QUERY_ITERATOR_YIELD_PRVALUE(query)
    }

    {
      auto query = Cinq(one_element).Join(one_element, [](auto &&x) -> decltype(auto) {
          NON_CONST_LVALUE_REFERENCE_ASSERT(decltype(x));
          return std::forward<decltype(x)>(x);
        }, [](auto &&x) -> decltype(auto) {
          NON_CONST_LVALUE_REFERENCE_ASSERT(decltype(x));
          return x;
        }, [](auto &&first, auto &&second) {
          NON_CONST_LVALUE_REFERENCE_ASSERT(decltype(first));
          NON_CONST_LVALUE_REFERENCE_ASSERT(decltype(second));
          return std::tie(first, second);
        });
      query.ToVector();

      ASSERT_QUERY_ITERATOR_YIELD_PRVALUE(query)
    }

    {
      auto query = Cinq(one_element).Const().Join(one_element, [](auto &&x) -> decltype(auto) {
          CONST_LVALUE_REFERENCE_ASSERT(decltype(x));
          return std::forward<decltype(x)>(x);
        }, [](auto &&x) -> decltype(auto) {
          CONST_LVALUE_REFERENCE_ASSERT(decltype(x));
          return x;
        }, [](auto &&first, auto &&second) {
          CONST_LVALUE_REFERENCE_ASSERT(decltype(first));
          CONST_LVALUE_REFERENCE_ASSERT(decltype(second));
          return std::tie(first, second);
        });
      query.ToVector();

      ASSERT_CONST_QUERY_ITERATOR_YIELD_PRVALUE(query)
    }
  }

  // Intersect

  // Union

  // Concat

  // Distinct

  // Const
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

void TestCinqIntersect() {
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

  // # additional trivial test with 4 sources
  {
    auto query = Cinq(empty_source).Intersect(empty_source, empty_source, empty_source).ToVector();
    assert(query.size() == 0);
  }
}

void TestCinqUnion() {
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

  // # additional trivial test with 4 sources
  {
    auto query = Cinq(empty_source).Union(empty_source, empty_source, empty_source).ToVector();
    assert(query.size() == 0);
  }
}

void TestCinqConcat() {
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

  // # additional trivial test with 4 sources
  {
    auto query = Cinq(empty_source).Concat(empty_source, empty_source, empty_source).ToVector();
    assert(query.size() == 0);
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
    std::vector<int> special{
      1, 12, 12, 12, 99
    };
    auto query = Cinq(special).Distinct().ToVector();
    assert(query.size() == 3 &&
      query[0] == 1 && query[1] == 12 && query[2] == 99);
  }

  // # has multiple and include all
  {
    std::vector<int> special{
      1, 2, 3, 4, 5
    };
    auto query = Cinq(special).Distinct().ToVector();
    assert(query.size() == special.size() &&
      std::equal(query.begin(), query.end(), special.begin()));
  }
}

#endif // ENABLE_TEST

