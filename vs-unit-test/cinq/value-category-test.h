#pragma once

#include <iostream>
#include <iterator>
#include <utility>
#include <vector>
#include <string>
#include <functional>

#include "cinq-v3.h"
using cinq_v3::Cinq;

namespace cinq_test {
template <class T>
std::string GetValueCategory() {
  return std::is_lvalue_reference_v<T> ? "lvalue" :
    (std::is_rvalue_reference_v<T> ? "xvalue" : "prvalue");
}

template <class T>
std::string GetConstness() {
  return std::is_const_v<std::remove_reference_t<T>> ? "Const" : "NonConst";
}

struct ValueCategoryInfo {
  std::string QueryConstness; // Const, NonConst

  std::string SourceCategory; // Internal, FunctionObject, Iterator
  std::string SourceYieldConstness; // Const, NonConst
  std::string SourceYieldValueCategory; // prvalue, xvalue, lvalue

  std::string ExprOriginal; // FunctionParameter, IteratorRet
  // when the type is from std::xbegin(query), Const stands for cbegin or c-query, NonConst stands for begin on non const query.
  std::string IteartorConstness; // Const, NonConst
  // The following two stores the constness and valuecategory of the testing expr, which is of FunctionParam or dereferencing iterator.
  std::string ExprTypeConstness; // Const, NonConst
  std::string ExprValueCategory; // prvalue, xvalue, lvalue

  // type info of the function object yield type 
  std::string YieldConstness;
  std::string YieldValueCategory;

  template <class T>
  static ValueCategoryInfo CreateExprInfo(std::string original) {
    ValueCategoryInfo info;
    FillExprInfo<T>(info, original);
    return info;
  }

  template <class T>
  static void FillExprInfo(ValueCategoryInfo &info, std::string original) {
    info.ExprOriginal = std::move(original);
    info.ExprTypeConstness = GetConstness<T>();
    info.ExprValueCategory = GetValueCategory<T>();
  }

  template <class T>
  static ValueCategoryInfo CreateSourceInfo(std::string category) {
    ValueCategoryInfo info;
    FillSourceInfo<T>(info, category);
    return info;
  }

  template <class T>
  static void FillSourceInfo(ValueCategoryInfo &info, std::string category) {
    info.SourceCategory = std::move(category);
    info.SourceYieldConstness = GetConstness<T>();
    info.SourceYieldValueCategory = GetValueCategory<T>();
  }

  static bool IsConformant(const ValueCategoryInfo &info) {
    const bool ConstIteratorCase = info.IteartorConstness == "Const" && info.ExprOriginal == "IteratorRet";

    if (info.QueryConstness == "NonConst" && !ConstIteratorCase) {
      if (info.ExprOriginal == "IteratorRet") {
        if (info.SourceCategory == "Internal") {
          if (info.ExprTypeConstness == "Const" && info.ExprValueCategory == "lvalue")
            return true;
        } else if (info.SourceCategory == "FunctionObject" || info.SourceCategory == "Iterator") {
          if (info.SourceYieldValueCategory == "prvalue")
            return info.ExprValueCategory == "prvalue" && info.ExprTypeConstness == "NonConst";
          else if (info.ExprTypeConstness == info.SourceYieldConstness && info.ExprValueCategory == info.SourceYieldValueCategory)
            return true;
        }
      } else if (info.ExprOriginal == "FunctionParameter") {
        if (info.SourceYieldValueCategory == "lvalue") {
          if (info.ExprTypeConstness == info.SourceYieldConstness && info.ExprValueCategory == info.SourceYieldValueCategory)
            return true;
        } else if (info.SourceYieldValueCategory == "prvalue" || info.SourceYieldValueCategory == "xvalue") {
          if (info.ExprTypeConstness == info.SourceYieldConstness && (info.ExprValueCategory == "prvalue" || info.ExprValueCategory == "xvalue"))
            return true;
        }
      }
    } else if (info.QueryConstness == "Const" || ConstIteratorCase) {
      if (info.ExprOriginal == "IteratorRet") {
        if (info.SourceCategory == "Internal") {
          if (info.ExprTypeConstness == "Const" && info.ExprValueCategory == "lvalue")
            return true;
        } else if (info.SourceYieldValueCategory == "prvalue") {
          return info.ExprValueCategory == "prvalue" && info.ExprTypeConstness == "NonConst";
        } else if (info.SourceYieldValueCategory == "xvalue") {
          if (info.ExprTypeConstness == info.SourceYieldConstness && info.ExprValueCategory == info.SourceYieldValueCategory)
            return true;
        } else if (info.SourceYieldValueCategory == "lvalue") {
          if (info.ExprTypeConstness == "Const" && info.ExprValueCategory == "lvalue")
            return true;
        }
      } else if (info.ExprOriginal == "FunctionParameter") {
        if (info.SourceYieldValueCategory == "prvalue" || info.SourceYieldValueCategory == "xvalue") {
          if (info.ExprTypeConstness == info.SourceYieldConstness && (info.ExprValueCategory == "prvalue" || info.ExprValueCategory == "xvalue"))
            return true;
        } else if (info.SourceYieldValueCategory == "lvalue") {
          if (info.ExprTypeConstness == "Const" && info.ExprValueCategory == "lvalue")
            return true;
        }
      }
    }
    return false;
  }
};

template <class Ret>
struct FakeConatiner {
  struct Iterator {
    bool operator==(const Iterator &rhs) const {return i == rhs.i;}
    bool operator!=(const Iterator &rhs) const {return i != rhs.i;}
    Iterator &operator++() { ++i; return *this;}
    Iterator operator++(int) { Iterator x; x.i = i++; return x;}
    Ret operator*() {
      if constexpr (std::is_reference_v<Ret>)
        return static_cast<Ret>(x);
      else
        return {};
    }
    Ret operator*() const {
      if constexpr (std::is_reference_v<Ret>)
        return static_cast<Ret>(x);
      else
        return {};
    }
    int i = 0;
    mutable std::decay_t<Ret> x;
  };

  Iterator begin() {return {};}
  Iterator end() {Iterator x; x.i = 1; return x;}
  Iterator begin() const {return {};}
  Iterator end() const {Iterator x; x.i = 1; return x;}
};

template <class... Args>
void PopulateInfo(std::vector<ValueCategoryInfo> &info) {
  (info.push_back(ValueCategoryInfo::CreateExprInfo<Args>("FunctionParameter")), ...);
}

template <class Ret, bool SourceFromHere, bool SelectManyTag = false>
struct FunctionObject {
  FunctionObject(std::vector<ValueCategoryInfo> &info) : info_(info) {}

  template <class... Args>
  Ret operator()(Args&&...) {
    PopulateInfo<Args...>(info_);

    if constexpr (SourceFromHere) {
      info_.back().YieldConstness = GetConstness<Ret>();
      info_.back().YieldValueCategory = GetValueCategory<Ret>();
    }
    if constexpr (SelectManyTag) {
      info_.back().YieldConstness = GetConstness<decltype(*std::begin(std::declval<Ret>()))>();
      info_.back().YieldValueCategory = GetValueCategory<decltype(*std::begin(std::declval<Ret>()))>();
    }

    ret_.emplace<std::decay_t<Ret>>();
    return static_cast<Ret>(std::any_cast<std::decay_t<Ret> &>(ret_));
  }

  std::any ret_;
  std::vector<ValueCategoryInfo> &info_;
};

struct ValueCategoryTestUnit {
  template <class QueryType, class YieldQueryType, class... Args>
  YieldQueryType AppendQuery(QueryType &&query_base, YieldQueryType (QueryType::*query)(Args...) &&, std::vector<ValueCategoryInfo> &info,
    const std::string &query_constness, const std::string &source_category) {
    std::vector<ValueCategoryInfo> local_info;
    auto yield_query = (std::move(query_base).*query)(Args(local_info)...);
    yield_query.ToVector();

    using SourceYieldType = decltype(*std::begin(query_base));
    for (auto &i : local_info) {
      i.QueryConstness = query_constness;
      i.SourceCategory = "Iterator";
      i.SourceYieldConstness = GetConstness<SourceYieldType>();
      i.SourceYieldValueCategory = GetValueCategory<SourceYieldType>();
    }

    using IteratorYieldType = decltype(*yield_query.begin());
    using ConstIteratorYieldType = decltype(*yield_query.cbegin());

    auto last = local_info.empty() ? ValueCategoryInfo{} : local_info.back();
    local_info.push_back(ValueCategoryInfo::CreateExprInfo<IteratorYieldType>("IteratorRet"));
    local_info.back().IteartorConstness = "NonConst";
    if (last.YieldConstness != "") { // When iterator yield type is from function object
      local_info.back().SourceYieldConstness = last.YieldConstness;
      local_info.back().SourceYieldValueCategory = last.YieldValueCategory;
    }

    local_info.push_back(ValueCategoryInfo::CreateExprInfo<ConstIteratorYieldType>("IteratorRet"));
    local_info.back().IteartorConstness = "Const";
    if (last.YieldConstness != "") { // When iterator yield type is from function object
      local_info.back().SourceYieldConstness = last.YieldConstness;
      local_info.back().SourceYieldValueCategory = last.YieldValueCategory;
    }

    for (auto i : {local_info.rbegin(), std::next(local_info.rbegin())}) {
      i->QueryConstness = query_constness;
      i->SourceCategory = source_category;
      if (last.YieldConstness == "") { // When iterator yield type is from base_query
        i->SourceYieldConstness = GetConstness<SourceYieldType>();
        i->SourceYieldValueCategory = GetValueCategory<SourceYieldType>();
      }
    }

    info.insert(info.end(), std::make_move_iterator(local_info.begin()), std::make_move_iterator(local_info.end()));

    return yield_query;
  }

  template <class It>
  void AssertAllInfo(It first, It last) {
    int i = 0;
    for (; first != last; ++first) {
      auto flag = ValueCategoryInfo::IsConformant(*first);
      assert(flag);
      ++i;
    }
  }

  template <class TestInfo, class QueryType>
  void SingleTest(const TestInfo &test_info, QueryType &&query, std::string query_name) {
    std::vector<ValueCategoryInfo> info;
 
    AppendQuery(std::move(query), std::get<0>(test_info), info, std::get<1>(test_info), std::get<2>(test_info));

    AssertAllInfo(info.begin(), info.end());
  }

  template <class Container>
  void TestSelectMany(Container container) {
    using QueryType = decltype(Cinq(container));
    SingleTest(std::make_tuple(&QueryType::template SelectMany<FunctionObject<FakeConatiner<std::string>, false, true>>, "NonConst", "Iterator"), Cinq(container), "SelectMany");
    SingleTest(std::make_tuple(&QueryType::template SelectMany<FunctionObject<FakeConatiner<const std::string>, false, true>>, "NonConst", "Iterator"), Cinq(container), "SelectMany");
    SingleTest(std::make_tuple(&QueryType::template SelectMany<FunctionObject<FakeConatiner<std::string &>, false, true>>, "NonConst", "Iterator"), Cinq(container), "SelectMany");
    SingleTest(std::make_tuple(&QueryType::template SelectMany<FunctionObject<FakeConatiner<const std::string &>, false, true>>, "NonConst", "Iterator"), Cinq(container), "SelectMany");
    SingleTest(std::make_tuple(&QueryType::template SelectMany<FunctionObject<FakeConatiner<std::string &&>, false, true>>, "NonConst", "Iterator"), Cinq(container), "SelectMany");
    SingleTest(std::make_tuple(&QueryType::template SelectMany<FunctionObject<FakeConatiner<const std::string &&>, false, true>>, "NonConst", "Iterator"), Cinq(container), "SelectMany");
  
    using ConstQueryType = decltype(Cinq(container).Const());
    SingleTest(std::make_tuple(&ConstQueryType::template SelectMany<FunctionObject<FakeConatiner<std::string>, false, true>>, "Const", "Iterator"), Cinq(container).Const(), "SelectMany");
    SingleTest(std::make_tuple(&ConstQueryType::template SelectMany<FunctionObject<FakeConatiner<const std::string>, false, true>>, "Const", "Iterator"), Cinq(container).Const(), "SelectMany");
    SingleTest(std::make_tuple(&ConstQueryType::template SelectMany<FunctionObject<FakeConatiner<std::string &>, false, true>>, "Const", "Iterator"), Cinq(container).Const(), "SelectMany");
    SingleTest(std::make_tuple(&ConstQueryType::template SelectMany<FunctionObject<FakeConatiner<const std::string &>, false, true>>, "Const", "Iterator"), Cinq(container).Const(), "SelectMany");
    SingleTest(std::make_tuple(&ConstQueryType::template SelectMany<FunctionObject<FakeConatiner<std::string &&>, false, true>>, "Const", "Iterator"), Cinq(container).Const(), "SelectMany");
    SingleTest(std::make_tuple(&ConstQueryType::template SelectMany<FunctionObject<FakeConatiner<const std::string &&>, false, true>>, "Const", "Iterator"), Cinq(container).Const(), "SelectMany");
  }

  template <class Container>
  void TestSelect(Container container) {
    using QueryType = decltype(Cinq(container));
    SingleTest(std::make_tuple(&QueryType::template Select<FunctionObject<std::string, true>>, "NonConst", "FunctionObject"), Cinq(container), "Select");
    SingleTest(std::make_tuple(&QueryType::template Select<FunctionObject<const std::string, true>>, "NonConst", "FunctionObject"), Cinq(container), "Select");
    SingleTest(std::make_tuple(&QueryType::template Select<FunctionObject<std::string &, true>>, "NonConst", "FunctionObject"), Cinq(container), "Select");
    SingleTest(std::make_tuple(&QueryType::template Select<FunctionObject<const std::string &, true>>, "NonConst", "FunctionObject"), Cinq(container), "Select");
    SingleTest(std::make_tuple(&QueryType::template Select<FunctionObject<std::string &&, true>>, "NonConst", "FunctionObject"), Cinq(container), "Select");
    SingleTest(std::make_tuple(&QueryType::template Select<FunctionObject<const std::string &&, true>>, "NonConst", "FunctionObject"), Cinq(container), "Select");
  
    using ConstQueryType = decltype(Cinq(container).Const());
    SingleTest(std::make_tuple(&ConstQueryType::template Select<FunctionObject<std::string, true>>, "Const", "FunctionObject"), Cinq(container).Const(), "Select");
    SingleTest(std::make_tuple(&ConstQueryType::template Select<FunctionObject<const std::string, true>>, "Const", "FunctionObject"), Cinq(container).Const(), "Select");
    SingleTest(std::make_tuple(&ConstQueryType::template Select<FunctionObject<std::string &, true>>, "Const", "FunctionObject"), Cinq(container).Const(), "Select");
    SingleTest(std::make_tuple(&ConstQueryType::template Select<FunctionObject<const std::string &, true>>, "Const", "FunctionObject"), Cinq(container).Const(), "Select");
    SingleTest(std::make_tuple(&ConstQueryType::template Select<FunctionObject<std::string &&, true>>, "Const", "FunctionObject"), Cinq(container).Const(), "Select");
    SingleTest(std::make_tuple(&ConstQueryType::template Select<FunctionObject<const std::string &&, true>>, "Const", "FunctionObject"), Cinq(container).Const(), "Select");
  }

  template <class Selector1Ret, class Selector2Ret, class Selector3Ret, bool QueryConstness, class Container, class BasicQuery>
  void TestJoinImpl(Container container, BasicQuery &&basic_query) {
    using ContainerYieldType = decltype(*std::begin(container));

    std::decay_t<Selector1Ret> ret1;
    std::decay_t<Selector2Ret> ret2;
    std::decay_t<Selector3Ret> ret3;

    std::vector<ValueCategoryInfo> info;

    auto query = std::move(basic_query).Join(container,
      [&ret1, &info](auto &&x) -> Selector1Ret {
          PopulateInfo<decltype(x)>(info);
          ValueCategoryInfo::FillSourceInfo<ContainerYieldType>(info.back(), "Iterator");
          return static_cast<Selector1Ret>(ret1);
        },

      [&ret2, &info](auto &&x) -> Selector2Ret {
          PopulateInfo<decltype(x)>(info);
          ValueCategoryInfo::FillSourceInfo<ContainerYieldType>(info.back(), "Iterator");
          return static_cast<Selector2Ret>(ret2); 
        },

      [&ret3, &info](auto &&x, auto &&y) -> Selector3Ret {
          info.push_back(ValueCategoryInfo::CreateExprInfo<decltype(x)>("FunctionParameter"));
          ValueCategoryInfo::FillSourceInfo<Selector1Ret>(info.back(), "FunctionObject");
          info.push_back(ValueCategoryInfo::CreateExprInfo<decltype(y)>("FunctionParameter"));
          ValueCategoryInfo::FillSourceInfo<Selector2Ret>(info.back(), "FunctionObject");
          return static_cast<Selector3Ret>(ret3);
        });
    for (auto &&x : query);

    info.push_back(ValueCategoryInfo::CreateExprInfo<decltype(*std::begin(query))>("IteratorRet"));
    info.back().IteartorConstness = "NonConst";
    ValueCategoryInfo::FillSourceInfo<Selector3Ret>(info.back(), "FunctionObject");
    info.push_back(ValueCategoryInfo::CreateExprInfo<decltype(*std::cbegin(query))>("IteratorRet"));
    info.back().IteartorConstness = "Const";
    ValueCategoryInfo::FillSourceInfo<Selector3Ret>(info.back(), "FunctionObject");

    for (auto &i : info)
      i.QueryConstness = QueryConstness ? "Const" : "NonConst";

    AssertAllInfo(info.begin(), info.end());
  }

  template <class Container>
  void TestJoin(Container container) {
    TestJoinImpl<std::string, std::string, std::string, false>(container, Cinq(container));
    // In join, selector is implemented as Cinq::Select. Thus the constness is removed.
    // TestJoinImpl<const std::string, std::string, std::string, false>(container, Cinq(container));
    TestJoinImpl<std::string &, std::string, std::string, false>(container, Cinq(container));
    TestJoinImpl<const std::string &, std::string, std::string, false>(container, Cinq(container));
    TestJoinImpl<std::string &&, std::string, std::string, false>(container, Cinq(container));
    TestJoinImpl<const std::string &&, std::string, std::string, false>(container, Cinq(container));

    TestJoinImpl<std::string, std::string, std::string, false>(container, Cinq(container));
    // TestJoinImpl<std::string, const std::string, std::string, false>(container, Cinq(container));
    TestJoinImpl<std::string, std::string &, std::string, false>(container, Cinq(container));
    TestJoinImpl<std::string, const std::string &, std::string, false>(container, Cinq(container));
    TestJoinImpl<std::string, std::string &&, std::string, false>(container, Cinq(container));
    TestJoinImpl<std::string, const std::string &&, std::string, false>(container, Cinq(container));

    TestJoinImpl<std::string, std::string, std::string, false>(container, Cinq(container));
    TestJoinImpl<std::string, std::string, const std::string, false>(container, Cinq(container));
    TestJoinImpl<std::string, std::string, std::string &, false>(container, Cinq(container));
    TestJoinImpl<std::string, std::string, const std::string &, false>(container, Cinq(container));
    TestJoinImpl<std::string, std::string, std::string &&, false>(container, Cinq(container));
    TestJoinImpl<std::string, std::string, const std::string &&, false>(container, Cinq(container));



    TestJoinImpl<std::string, std::string, std::string, true>(container, Cinq(container).Const());
    // TestJoinImpl<const std::string, std::string, std::string, true>(container, Cinq(container).Const());
    TestJoinImpl<std::string &, std::string, std::string, true>(container, Cinq(container).Const());
    TestJoinImpl<const std::string &, std::string, std::string, true>(container, Cinq(container).Const());
    TestJoinImpl<std::string &&, std::string, std::string, true>(container, Cinq(container).Const());
    TestJoinImpl<const std::string &&, std::string, std::string, true>(container, Cinq(container).Const());

    TestJoinImpl<std::string, std::string, std::string, true>(container, Cinq(container).Const());
    // TestJoinImpl<std::string, const std::string, std::string, true>(container, Cinq(container).Const());
    TestJoinImpl<std::string, std::string &, std::string, true>(container, Cinq(container).Const());
    TestJoinImpl<std::string, const std::string &, std::string, true>(container, Cinq(container).Const());
    TestJoinImpl<std::string, std::string &&, std::string, true>(container, Cinq(container).Const());
    TestJoinImpl<std::string, const std::string &&, std::string, true>(container, Cinq(container).Const());

    TestJoinImpl<std::string, std::string, std::string, true>(container, Cinq(container).Const());
    TestJoinImpl<std::string, std::string, const std::string, true>(container, Cinq(container).Const());
    TestJoinImpl<std::string, std::string, std::string &, true>(container, Cinq(container).Const());
    TestJoinImpl<std::string, std::string, const std::string &, true>(container, Cinq(container).Const());
    TestJoinImpl<std::string, std::string, std::string &&, true>(container, Cinq(container).Const());
    TestJoinImpl<std::string, std::string, const std::string &&, true>(container, Cinq(container).Const());
  }

  template <class Container>
  void TestBatch(Container container) {
    TestSelectMany(container);
    TestSelect(container);
    TestJoin(container);

    using QueryType = decltype(Cinq(container));
    SingleTest(std::make_tuple(&QueryType::template Where<FunctionObject<bool, false>>, "NonConst", "Iterator"), Cinq(container), "Where");
    SingleTest(std::make_tuple(&QueryType::template Intersect<Container>, "NonConst", "Internal"), Cinq(container), "Intersect");
    SingleTest(std::make_tuple(&QueryType::template Intersect<Container, Container>, "NonConst", "Internal"), Cinq(container), "Intersect");
    SingleTest(std::make_tuple(&QueryType::template Union<Container>, "NonConst", "Internal"), Cinq(container), "Union");
    SingleTest(std::make_tuple(&QueryType::template Union<Container, Container>, "NonConst", "Internal"), Cinq(container), "Union");
    SingleTest(std::make_tuple(&QueryType::template Concat<Container>, "NonConst", "Iterator"), Cinq(container), "Concat");
    SingleTest(std::make_tuple(&QueryType::template Concat<Container, Container>, "NonConst", "Iterator"), Cinq(container), "Concat");
    SingleTest(std::make_tuple(&QueryType::Distinct, "NonConst", "Iterator"), Cinq(container), "Distinct");

    using ConstQueryType = decltype(Cinq(container).Const());
    SingleTest(std::make_tuple(&ConstQueryType::template Where<FunctionObject<bool, false>>, "Const", "Iterator"), Cinq(container).Const(), "Where");
    SingleTest(std::make_tuple(&ConstQueryType::template Intersect<Container>, "Const", "Internal"), Cinq(container).Const(), "Intersect");
    SingleTest(std::make_tuple(&ConstQueryType::template Intersect<Container, Container>, "Const", "Internal"), Cinq(container).Const(), "Intersect");
    SingleTest(std::make_tuple(&ConstQueryType::template Union<Container>, "Const", "Internal"), Cinq(container).Const(), "Union");
    SingleTest(std::make_tuple(&ConstQueryType::template Union<Container, Container>, "Const", "Internal"), Cinq(container).Const(), "Union");
    SingleTest(std::make_tuple(&ConstQueryType::template Concat<Container>, "Const", "Iterator"), Cinq(container).Const(), "Concat");
    SingleTest(std::make_tuple(&ConstQueryType::template Concat<Container, Container>, "Const", "Iterator"), Cinq(container).Const(), "Concat");
    SingleTest(std::make_tuple(&ConstQueryType::Distinct, "Const", "Iterator"), Cinq(container).Const(), "Distinct");
  }

  void Test() {
    TestBatch(FakeConatiner<std::string>());
    TestBatch(FakeConatiner<const std::string>());
    TestBatch(FakeConatiner<std::string &>());
    TestBatch(FakeConatiner<const std::string &>());
    TestBatch(FakeConatiner<std::string &&>());
    TestBatch(FakeConatiner<const std::string &&>());
  }
};

} // namespace cinq_test
