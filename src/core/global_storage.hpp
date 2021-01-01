#include "types.hpp"
#include "helper_macros.hpp"
#include "print.hpp"

namespace core
{
template <typename K, typename T>
class global_storage {
    SINGLETON_IMPL(global_storage);

public:
    global_storage()  = default;
    ~global_storage() = default;

    [[nodiscard]]
    bool try_insert(const K& key, T value) {
        return _storage.insert(key, move(value));
    }

    void insert(const K& key, T value) {
        if (!try_insert(key, move(value)))
            throw std::runtime_error(format("Key \"{}\" already occupied", key));
    }

    [[nodiscard]]
    bool try_remove(const K& key) {
        return _storage.erase(key);
    }

    void remove(const K& key) {
        if (!try_remove(key))
            throw std::runtime_error(format("Key \"{}\" not found", key));
    }

    [[nodiscard]]
    try_opt<T> try_get(const K& key) const {
        try_opt<T> result;
        _storage.find_fn(key, [&](const T& value) { result = value; });

        if (!result)
            result = std::runtime_error(format("Can't find key \"{}\"", key));

        return result;
    }

    T get(const K& key) const {
        return try_get(key).value();
    }

private:
    mpmc_hash_map<K, T> _storage;
};
} // namespace core
