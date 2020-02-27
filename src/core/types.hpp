#include <gsl/gsl>
#include <chrono>
#include <variant>
#include <any>
#include <flat_hash_map.hpp>

using gsl::byte;

namespace chrono = std::chrono;
using namespace std::literals;

using std::tuple;
using std::pair;
using std::vector;
using std::string;
using std::array;
using std::move;
using std::forward;
using std::optional;
using std::variant;
using std::any;

template <
        typename K, typename V,
        typename H = std::hash<K>,
        typename E = std::equal_to<K>,
        typename A = std::allocator<std::pair<K, V>>>
using hash_map = ska::flat_hash_map<K, V, H, E, A>;

template <
        typename T,
        typename H = std::hash<T>,
        typename E = std::equal_to<T>,
        typename A = std::allocator<T>>
using hash_set = ska::flat_hash_set<T, H, E, A>;

using gsl::not_null;
using gsl::span;

