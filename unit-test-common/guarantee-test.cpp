
#include <cassert>
#include <algorithm>
#include <memory>
#include <vector>

#include <cinq.h>
using cinq::Cinq;

namespace cinq_test {
// TODO : add test case for other containers
// TODO : add test case for longer query
void NoCopyGuaranteeTest()
{
  struct CopyDetector {
    CopyDetector() { ++GetCounter().ctor_counter_; }
    CopyDetector(const CopyDetector &) { ++GetCounter().copy_counter_; }
    CopyDetector(CopyDetector &&) noexcept { ++GetCounter().move_counter_; }

    CopyDetector &operator=(const CopyDetector &) { ++GetCounter().copy_counter_; return *this; }
    CopyDetector &operator=(CopyDetector &&) { ++GetCounter().move_counter_; return *this; }

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
  // auto print_counter = [&x = CopyDetector::GetCounter()]() { std::cout << x.ctor_counter_ << ", " << x.copy_counter_ << ", " << x.move_counter_ << std::endl; };
  auto test_query = [&](auto &&query) {
      reset_counter();
      volatile uintptr_t dummy = 0;
      for (auto &&x : query)
        dummy += reinterpret_cast<uintptr_t>(&x);
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
  test_query(Cinq(vtr).Concat(vtr));
  test_query(Cinq(vtr).Concat(vtr, vtr));
  test_query(Cinq(vtr).Concat(vtr, vtr, vtr));
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
  test_query(Cinq(svtr).Concat(svtr));
  test_query(Cinq(svtr).Concat(svtr, svtr));
  test_query(Cinq(svtr).Concat(svtr, svtr, svtr));
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
  test_query(Cinq(std::ref(uvtr)).Concat(std::ref(uvtr)));
  test_query(Cinq(std::ref(uvtr)).Concat(std::ref(uvtr), std::ref(uvtr)));
  test_query(Cinq(std::ref(uvtr)).Concat(std::ref(uvtr), std::ref(uvtr), std::ref(uvtr)));
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

// TODO : add test case for longer query
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

} // namespace cinq_test
