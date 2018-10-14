#include <any>
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>
#include <string>
#include <functional>

#include "cinq-v3.h"
using cinq_v3::Cinq;

namespace cinq_test {

const std::vector<int> empty_source;
const std::vector<int> one_element{ 0 };
const std::vector<int> five_elements{ 0, 1, 2, 3, 4 }; // elements must be unique

using VCRetType = std::shared_ptr<int>;

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
  // when the type is from std::xbegin(query), Const stands for cbegin, NonConst stands for begin.
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

    local_info.push_back(ValueCategoryInfo::CreateExprInfo<ConstIteratorYieldType>("IteratorRet"));
    local_info.back().IteartorConstness = "Const";

    for (auto i : {local_info.rbegin(), std::next(local_info.rbegin())}) {
      i->QueryConstness = query_constness;
      i->SourceCategory = source_category;
      if (last.YieldConstness == "") { // When iterator yield type is from base_query
        i->SourceYieldConstness = GetConstness<SourceYieldType>();
        i->SourceYieldValueCategory = GetValueCategory<SourceYieldType>();
      } else { // When iterator yield type is from function object
        i->SourceYieldConstness = last.YieldConstness;
        i->SourceYieldValueCategory = last.YieldValueCategory;
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
    SingleTest(std::make_tuple(&QueryType::template SelectMany<FunctionObject<FakeConatiner<VCRetType>, false, true>>, "NonConst", "Iterator"), Cinq(container), "SelectMany");
    SingleTest(std::make_tuple(&QueryType::template SelectMany<FunctionObject<FakeConatiner<const VCRetType>, false, true>>, "NonConst", "Iterator"), Cinq(container), "SelectMany");
    SingleTest(std::make_tuple(&QueryType::template SelectMany<FunctionObject<FakeConatiner<VCRetType &>, false, true>>, "NonConst", "Iterator"), Cinq(container), "SelectMany");
    SingleTest(std::make_tuple(&QueryType::template SelectMany<FunctionObject<FakeConatiner<const VCRetType &>, false, true>>, "NonConst", "Iterator"), Cinq(container), "SelectMany");
    SingleTest(std::make_tuple(&QueryType::template SelectMany<FunctionObject<FakeConatiner<VCRetType &&>, false, true>>, "NonConst", "Iterator"), Cinq(container), "SelectMany");
    SingleTest(std::make_tuple(&QueryType::template SelectMany<FunctionObject<FakeConatiner<const VCRetType &&>, false, true>>, "NonConst", "Iterator"), Cinq(container), "SelectMany");
  
    using ConstQueryType = decltype(Cinq(container).Const());
    SingleTest(std::make_tuple(&ConstQueryType::template SelectMany<FunctionObject<FakeConatiner<VCRetType>, false, true>>, "Const", "Iterator"), Cinq(container).Const(), "SelectMany");
    SingleTest(std::make_tuple(&ConstQueryType::template SelectMany<FunctionObject<FakeConatiner<const VCRetType>, false, true>>, "Const", "Iterator"), Cinq(container).Const(), "SelectMany");
    SingleTest(std::make_tuple(&ConstQueryType::template SelectMany<FunctionObject<FakeConatiner<VCRetType &>, false, true>>, "Const", "Iterator"), Cinq(container).Const(), "SelectMany");
    SingleTest(std::make_tuple(&ConstQueryType::template SelectMany<FunctionObject<FakeConatiner<const VCRetType &>, false, true>>, "Const", "Iterator"), Cinq(container).Const(), "SelectMany");
    SingleTest(std::make_tuple(&ConstQueryType::template SelectMany<FunctionObject<FakeConatiner<VCRetType &&>, false, true>>, "Const", "Iterator"), Cinq(container).Const(), "SelectMany");
    SingleTest(std::make_tuple(&ConstQueryType::template SelectMany<FunctionObject<FakeConatiner<const VCRetType &&>, false, true>>, "Const", "Iterator"), Cinq(container).Const(), "SelectMany");
  }

  template <class Container>
  void TestSelect(Container container) {
    using QueryType = decltype(Cinq(container));
    SingleTest(std::make_tuple(&QueryType::template Select<FunctionObject<VCRetType, true>>, "NonConst", "FunctionObject"), Cinq(container), "Select");
    SingleTest(std::make_tuple(&QueryType::template Select<FunctionObject<const VCRetType, true>>, "NonConst", "FunctionObject"), Cinq(container), "Select");
    SingleTest(std::make_tuple(&QueryType::template Select<FunctionObject<VCRetType &, true>>, "NonConst", "FunctionObject"), Cinq(container), "Select");
    SingleTest(std::make_tuple(&QueryType::template Select<FunctionObject<const VCRetType &, true>>, "NonConst", "FunctionObject"), Cinq(container), "Select");
    SingleTest(std::make_tuple(&QueryType::template Select<FunctionObject<VCRetType &&, true>>, "NonConst", "FunctionObject"), Cinq(container), "Select");
    SingleTest(std::make_tuple(&QueryType::template Select<FunctionObject<const VCRetType &&, true>>, "NonConst", "FunctionObject"), Cinq(container), "Select");
  
    using ConstQueryType = decltype(Cinq(container).Const());
    SingleTest(std::make_tuple(&ConstQueryType::template Select<FunctionObject<VCRetType, true>>, "Const", "FunctionObject"), Cinq(container).Const(), "Select");
    SingleTest(std::make_tuple(&ConstQueryType::template Select<FunctionObject<const VCRetType, true>>, "Const", "FunctionObject"), Cinq(container).Const(), "Select");
    SingleTest(std::make_tuple(&ConstQueryType::template Select<FunctionObject<VCRetType &, true>>, "Const", "FunctionObject"), Cinq(container).Const(), "Select");
    SingleTest(std::make_tuple(&ConstQueryType::template Select<FunctionObject<const VCRetType &, true>>, "Const", "FunctionObject"), Cinq(container).Const(), "Select");
    SingleTest(std::make_tuple(&ConstQueryType::template Select<FunctionObject<VCRetType &&, true>>, "Const", "FunctionObject"), Cinq(container).Const(), "Select");
    SingleTest(std::make_tuple(&ConstQueryType::template Select<FunctionObject<const VCRetType &&, true>>, "Const", "FunctionObject"), Cinq(container).Const(), "Select");
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
    query.ToVector();

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
    TestJoinImpl<VCRetType, VCRetType, VCRetType, false>(container, Cinq(container));
    // In join, selector is implemented as Cinq::Select. Thus the constness is removed.
    // TestJoinImpl<const VCRetType, VCRetType, VCRetType, false>(container, Cinq(container));
    TestJoinImpl<VCRetType &, VCRetType, VCRetType, false>(container, Cinq(container));
    TestJoinImpl<const VCRetType &, VCRetType, VCRetType, false>(container, Cinq(container));
    TestJoinImpl<VCRetType &&, VCRetType, VCRetType, false>(container, Cinq(container));
    TestJoinImpl<const VCRetType &&, VCRetType, VCRetType, false>(container, Cinq(container));

    TestJoinImpl<VCRetType, VCRetType, VCRetType, false>(container, Cinq(container));
    // TestJoinImpl<VCRetType, const VCRetType, VCRetType, false>(container, Cinq(container));
    TestJoinImpl<VCRetType, VCRetType &, VCRetType, false>(container, Cinq(container));
    TestJoinImpl<VCRetType, const VCRetType &, VCRetType, false>(container, Cinq(container));
    TestJoinImpl<VCRetType, VCRetType &&, VCRetType, false>(container, Cinq(container));
    TestJoinImpl<VCRetType, const VCRetType &&, VCRetType, false>(container, Cinq(container));

    TestJoinImpl<VCRetType, VCRetType, VCRetType, false>(container, Cinq(container));
    TestJoinImpl<VCRetType, VCRetType, const VCRetType, false>(container, Cinq(container));
    TestJoinImpl<VCRetType, VCRetType, VCRetType &, false>(container, Cinq(container));
    TestJoinImpl<VCRetType, VCRetType, const VCRetType &, false>(container, Cinq(container));
    TestJoinImpl<VCRetType, VCRetType, VCRetType &&, false>(container, Cinq(container));
    TestJoinImpl<VCRetType, VCRetType, const VCRetType &&, false>(container, Cinq(container));



    TestJoinImpl<VCRetType, VCRetType, VCRetType, true>(container, Cinq(container).Const());
    // TestJoinImpl<const VCRetType, VCRetType, VCRetType, true>(container, Cinq(container).Const());
    TestJoinImpl<VCRetType &, VCRetType, VCRetType, true>(container, Cinq(container).Const());
    TestJoinImpl<const VCRetType &, VCRetType, VCRetType, true>(container, Cinq(container).Const());
    TestJoinImpl<VCRetType &&, VCRetType, VCRetType, true>(container, Cinq(container).Const());
    TestJoinImpl<const VCRetType &&, VCRetType, VCRetType, true>(container, Cinq(container).Const());

    TestJoinImpl<VCRetType, VCRetType, VCRetType, true>(container, Cinq(container).Const());
    // TestJoinImpl<VCRetType, const VCRetType, VCRetType, true>(container, Cinq(container).Const());
    TestJoinImpl<VCRetType, VCRetType &, VCRetType, true>(container, Cinq(container).Const());
    TestJoinImpl<VCRetType, const VCRetType &, VCRetType, true>(container, Cinq(container).Const());
    TestJoinImpl<VCRetType, VCRetType &&, VCRetType, true>(container, Cinq(container).Const());
    TestJoinImpl<VCRetType, const VCRetType &&, VCRetType, true>(container, Cinq(container).Const());

    TestJoinImpl<VCRetType, VCRetType, VCRetType, true>(container, Cinq(container).Const());
    // The last selector isn't implemented with Select.
    TestJoinImpl<VCRetType, VCRetType, const VCRetType, true>(container, Cinq(container).Const());
    TestJoinImpl<VCRetType, VCRetType, VCRetType &, true>(container, Cinq(container).Const());
    TestJoinImpl<VCRetType, VCRetType, const VCRetType &, true>(container, Cinq(container).Const());
    TestJoinImpl<VCRetType, VCRetType, VCRetType &&, true>(container, Cinq(container).Const());
    TestJoinImpl<VCRetType, VCRetType, const VCRetType &&, true>(container, Cinq(container).Const());
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
    TestBatch(FakeConatiner<VCRetType>());
    TestBatch(FakeConatiner<const VCRetType>());
    TestBatch(FakeConatiner<VCRetType &>());
    TestBatch(FakeConatiner<const VCRetType &>());
    TestBatch(FakeConatiner<VCRetType &&>());
    TestBatch(FakeConatiner<const VCRetType &&>());
  }
};

void RuntimeValueCategoryTest() {
  ValueCategoryTestUnit().Test();
}

void CompileTimeValueCategoryTest() {
  // run-time test
  cinq_test::RuntimeValueCategoryTest();

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
}

} // namespace cinq_test
