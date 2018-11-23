#pragma once

namespace cinq::detail {
// For each Query iterator, specialize this class to provide it's implementation.
template <bool ArgConstness, bool RetConstness, class TEnumerable>
class QueryIterator;

template <class QueryTag, class TFn, class... TSources>
class BasicEnumerable;

} // namespace cinq::detail
