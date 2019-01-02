
#include <vector>

#include <cinq.h>
#include "../cinq-test-utility.h"
#include "detail/utility.h"

using cinq::Cinq;

namespace cinq_test {
inline void TestCinqAggregate() {
  {
    auto result = Cinq(empty_source).Aggregate([](auto &accu, auto &curr) { return (int)accu + curr; });
    cinq::utility::CinqAssert(result == 0);
  }
  {
    auto result = Cinq(one_element).Aggregate([](auto &accu, auto &curr) { return (int)accu + curr; });
    cinq::utility::CinqAssert(result == one_element.front());
  }
  {
    auto result = Cinq(five_elements).Aggregate([](auto &accu, auto &curr) { return (int)accu + curr; });
    cinq::utility::CinqAssert(result == std::accumulate(
      std::next(five_elements.begin()), five_elements.end(), five_elements.front(), [](auto &accu, auto &curr) { return (int)accu + curr; }));
  }

  {
    auto result = Cinq(empty_source).Aggregate(LifeTimeCheckInt(1), [](auto &accu, auto &curr) { return (int)accu + curr; });
    cinq::utility::CinqAssert(result == 1);
  }
  {
    auto result = Cinq(one_element).Aggregate(LifeTimeCheckInt(1), [](auto &accu, auto &curr) { return (int)accu + curr; });
    cinq::utility::CinqAssert(result - 1 == one_element.front());
  }
  {
    auto result = Cinq(five_elements).Aggregate(LifeTimeCheckInt(1), [](auto &accu, auto &curr) { return (int)accu + curr; });
    cinq::utility::CinqAssert(result - 1 == std::accumulate(
      std::next(five_elements.begin()), five_elements.end(), five_elements.front(), [](auto &accu, auto &curr) { return (int)accu + curr; }));
  }

  {
    auto result = Cinq(empty_source).Aggregate(LifeTimeCheckInt(1), [](auto &accu, auto &curr) { return (int)accu + curr; }, [](auto &&x) {return x*0.5;});
    cinq::utility::CinqAssert(result == 0.5);
  }
  {
    auto result = Cinq(one_element).Aggregate(LifeTimeCheckInt(1), [](auto &accu, auto &curr) { return (int)accu + curr; }, [](auto &&x) {return x*0.5;});
    cinq::utility::CinqAssert(result - 1 == one_element.front() * 0.5);
  }
  {
    auto result = Cinq(five_elements).Aggregate(LifeTimeCheckInt(1), [](auto &accu, auto &curr) { return (int)accu + curr; }, [](auto &&x) {return x*0.5;});
    cinq::utility::CinqAssert(result - 1 == std::accumulate(
      std::next(five_elements.begin()), five_elements.end(), five_elements.front(), [](auto &accu, auto &curr) { return (int)accu + curr; }) * 0.5);
  }
}

} // namespace cinq_test
