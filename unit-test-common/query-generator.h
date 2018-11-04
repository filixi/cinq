#pragma once

#include <cassert>

#include <tuple>
#include <memory>
#include <utility>

#include "cinq.h"
#include "cinq-test-utility.h"
using cinq::Cinq;

namespace cinq_test {
template <class T>
auto SharedQuery(T &&query) {
  return std::make_shared<std::decay_t<T>>(std::move(query));
}

template <class Query, class Predicate, class... Selector>
auto QueryGenerator(Query query, Predicate predicate, std::tuple<Selector...> selectors) {
  assert(sizeof...(Selector) == 3);

  return std::make_tuple(
    /*0*/SharedQuery(Cinq(query).Where(predicate)),
    /*1*/SharedQuery(Cinq(query).Select(std::get<0>(selectors))),
    /*2*/SharedQuery(Cinq(query).Join(query, std::get<0>(selectors), std::get<1>(selectors), std::get<2>(selectors))),
    /*3*/SharedQuery(Cinq(query).Intersect(query)),
    /*4*/SharedQuery(Cinq(query).Intersect(query, query)),
    /*5*/SharedQuery(Cinq(query).Intersect(query, query, query)),
    /*6*/SharedQuery(Cinq(query).Union(query)),
    /*7*/SharedQuery(Cinq(query).Union(query, query)),
    /*8*/SharedQuery(Cinq(query).Union(query, query, query)),
    /*9*/SharedQuery(Cinq(query).Concat(query)),
    /*10*/SharedQuery(Cinq(query).Concat(query, query)),
    /*11*/SharedQuery(Cinq(query).Concat(query, query, query)),
    /*12*/SharedQuery(Cinq(query).Distinct())
    );
}

template <class... SQuerys, class Predicate, class... Selector, size_t... indexs>
auto QueryAppendorImpl(std::tuple<SQuerys...> squerys, Predicate predicate, std::tuple<Selector...> selectors, std::index_sequence<indexs...>) {
  return std::make_tuple(QueryGenerator(std::get<indexs>(squerys), predicate, selectors)...);
}

template <class... SQuerys, class Predicate, class... Selector>
auto QueryAppendor(std::tuple<SQuerys...> squerys, Predicate predicate, std::tuple<Selector...> selectors) {
  return QueryAppendorImpl(squerys, predicate, selectors, std::index_sequence_for<SQuerys...>{});
}

inline void hash_combine_impl(std::uint64_t& h, std::uint64_t k) {
  const std::uint64_t m = static_cast<uint64_t>(0xc6a4a7935bd1e995);
  const int r = 47;

  k *= m;
  k ^= k >> r;
  k *= m;

  h ^= k;
  h *= m;

  h += 0xe6546b64;
}

template <class T>
void BasicCombinedQueryTest(T source) {
  auto result = QueryGenerator(std::move(source), [](auto &&) {return true;}, std::make_tuple(
      [](const int &x) -> const int & {return x;},
      [](const int &x) -> const int & {return x;},
      [](const int &x, const int &) -> const int & {return x;}
    ));

  auto appended = QueryAppendor(result, [](auto &&) {return true;}, std::make_tuple(
      [](const int &x) -> const int & {return x;},
      [](const int &x) -> const int & {return x;},
      [](const int &x, const int &) -> const int & {return x;}
    ));
  
  const size_t last_hash = 0xde24b45c4e881882;
  size_t hash = 0;
  VisitTupleTree<decltype(appended)>::Visit(
      appended, [](auto &&) {}, [&hash](auto &&query) {
          for (auto x : *query)
            hash_combine_impl(hash, x);
        }
    );
  assert(last_hash == hash);

  // iterator equality/inequality test
  VisitTupleTree<decltype(appended)>::Visit(
      appended, [](auto &&) {}, [&hash](auto &&query) {
          using iterator_type = decltype(query->begin());

          std::vector<std::vector<iterator_type>> iterators;

          iterators.emplace_back();
          for (auto ite = query->begin(); ite!=query->end(); ++ite)
            iterators.front().push_back(ite);
          iterators.resize(iterators.front().size());

          if (iterators.size() > 100)
            iterators.resize(100);

          for (int i=1; i<iterators.size(); ++i) {
            iterators[i].resize(iterators.size());
            for (int j=i; j<iterators.size(); ++j) {
              iterators[i][j] = iterators[i-1][j-1];
              ++iterators[i][j];
            }
          }

          for (int i=0; i<iterators.size(); ++i) {
            for (int j=i; j<iterators.size(); ++j) {
              for (int m=0; m<iterators.size(); ++m) {
                for (int n=m; n<iterators.size(); ++n) {
                  if (j == n)
                    assert(iterators[i][j] == iterators[m][n] && !(iterators[i][j] != iterators[m][n]));
                  else
                    assert(iterators[i][j] != iterators[m][n] && !(iterators[i][j] == iterators[m][n]));
                }
              }
            }
          }
        }
    );
}

} // namespace cinq_test
