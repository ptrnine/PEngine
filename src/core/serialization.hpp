#pragma once

#include "types.hpp"
#include "platform_dependent.hpp"


#define PE_SERIALIZE(...) void serialize(core::vector<core::byte>& _s) const { core::serialize_all(_s, __VA_ARGS__); } \
                           void deserialize(core::span<const core::byte>& _d) { core::deserialize_all(_d, __VA_ARGS__); }

#define PE_SERIALIZE_OVERRIDE(...)                                                                 \
    void serialize(core::vector<core::byte>& _s) const override {                                  \
        core::serialize_all(_s, __VA_ARGS__);                                                      \
    }                                                                                              \
    void deserialize(core::span<const core::byte>& _d) override {                                  \
        core::deserialize_all(_d, __VA_ARGS__);                                                    \
    }

namespace core
{
    class serializable_base {
    public:
        virtual ~serializable_base() = default;
        virtual void serialize(core::vector<core::byte>& _s) const = 0;
        virtual void deserialize(core::span<const core::byte>& _d) = 0;
    };

    using std::begin;
    using std::end;
    using std::back_inserter;

    template <typename T>
    struct pe_serialize;

    template <typename T>
    struct pe_deserialize;

    using byte_vector = vector<byte>;

    template <typename T>
    concept SerializableExternalF = requires (T& v, byte_vector& b, span<const byte>& s) {
        {pe_serialize<T>()(v, b)};
        {pe_deserialize<T>()(v, s)};
    };

    template <typename T>
    concept SerializableMemberF = requires (T v, byte_vector& b, span<const byte>& s) {
        {v.serialize(b)}; {v.deserialize(s)};
    } && !SerializableExternalF<T>;

    // Array with size < 32 only (prevent std::array<T, 10000000> compile-time jokes)
    template <typename T>
    concept SerializableStaticCortege =
            !SerializableExternalF<T> &&
            !SerializableMemberF<T> &&
            std::tuple_size<std::remove_const_t<std::remove_reference_t<T>>>::value < 32;

    template <typename T>
    concept SerializableIterable =
            !SerializableExternalF<T> &&
            !SerializableMemberF<T> &&
            !SerializableStaticCortege<T> &&
            requires (T& t) { begin(t); end(t); back_inserter(t) = *begin(t); end(t) - begin(t); };

    template <typename T>
    concept SerializableArray =
            !SerializableExternalF<T> &&
            !SerializableMemberF<T> &&
            !SerializableStaticCortege<T> &&
            StdArray<T>;

    template <typename T>
    concept SerializableMap =
            !SerializableExternalF<T> &&
            !SerializableMemberF<T> &&
            !SerializableStaticCortege<T> &&
            !SerializableIterable<T> &&
            !SerializableArray<T> &&
            requires (T& t) {
                begin(t);
                end(t);
                t.size();
                {*begin(t)} -> SerializableStaticCortege;
                t.emplace(get<0>(*begin(t)), get<1>(*begin(t)));
            };

    template <size_t size>
    using byte_array = array<byte, size>;

    template <typename... Ts>
    auto make_byte_vector(Ts... bytes) {
        return vector{static_cast<byte>(bytes)...};
    }

    template <typename T>
    struct FloatSizeEqualTypeHelper;

    template <>
    struct FloatSizeEqualTypeHelper<float> { using type = uint32_t; };

    template <>
    struct FloatSizeEqualTypeHelper<double> { using type = uint64_t; };

    template <typename T> requires Integral<T> && Unsigned<T>
    inline void serialize(T val, byte_vector& s);

    template <typename T> requires Integral<T> && (!Unsigned<T>)
    inline void serialize(T signed_val, byte_vector& s);

    template <typename T> requires FloatingPoint<T>
    inline void serialize(T float_val, byte_vector& out);

    template <typename T> requires Enum<T> && (!Integral<T>)
    inline void serialize(T enum_val, byte_vector& out);

    template <typename T>
    inline void serialize(const optional<T>& o, byte_vector& out);

    template <SerializableStaticCortege T>
    inline void serialize(const T& p, byte_vector& out);

    template <SerializableArray T>
    inline void serialize(const T& vec, byte_vector& out);

    template <SerializableIterable T>
    inline void serialize(const T& vec, byte_vector& out);

    template <SerializableMap T>
    inline void serialize(const T& map, byte_vector& out);

    template <SerializableMemberF T>
    inline void serialize(const T& v, byte_vector& out) {
        v.serialize(out);
    }

    template <SerializableExternalF T>
    inline void serialize(const T& v, byte_vector& out) {
        pe_serialize<T>()(v, out);
    }

    template <typename... Ts>
    static inline void serialize_all(byte_vector& out, const Ts&... args) {
        ((serialize(args, out)), ...);
    }


    class serializer {
    public:
        template <typename... Ts>
        void write(const Ts&... values) {
            (serialize(values, _data), ...);
        }

        [[nodiscard]]
        const auto& data() const {
            return _data;
        }

        [[nodiscard]]
        byte_vector detach_data() {
            auto result = move(_data);
            _data = {};
            return result;
        }

    private:
        byte_vector _data;
    };


    template <typename T> requires Integral<T> && Unsigned<T>
    inline void deserialize(T& val, span<const byte>& in);

    template <typename T> requires Integral<T> && (!Unsigned<T>)
    inline void deserialize(T& out, span<const byte>& in);

    template <typename T> requires FloatingPoint<T>
    inline void deserialize(T& out, span<const byte>& in);

    template <typename T> requires Enum<T> && (!Integral<T>)
    inline void deserialize(T& out, span<const byte>& in);

    template <typename T>
    inline void deserialize(optional<T>& out, span<const byte>& in);

    template <SerializableStaticCortege T>
    inline void deserialize(T& p, span<const byte>& in);

    template <SerializableArray T>
    inline void deserialize(T& vec, span<const byte>& in);

    template <SerializableIterable T>
    inline void deserialize(T& vec, span<const byte>& in);

    template <SerializableMap T>
    inline void deserialize(T& map, span<const byte>& in);

    template <SerializableMemberF T>
    inline void deserialize(T& v, span<const byte>& in) {
        v.deserialize(in);
    }

    template <SerializableExternalF T>
    inline void deserialize(T& v, span<const byte>& in) {
        pe_deserialize<T>()(v, in);
    }

    template <typename... Ts>
    static inline void deserialize_all(span<const byte>& in, Ts&... args) {
        ((deserialize(args, in)), ...);
    }

    class deserializer_view {
    public:
        deserializer_view(span<const byte> range): _range(range) {}

        deserializer_view(deserializer_view&&) = delete;
        deserializer_view& operator=(deserializer_view&&) = delete;

        template <typename... Ts>
        void read(Ts&... values) {
            (deserialize(values, _range), ...);
        }

        template <typename T>
        T read_get() {
            T v;
            read(v);
            return v;
        }

    private:
        span<const byte> _range;
    };


    //==================== Unsigned Integral

    template <typename T> requires Integral<T> && Unsigned<T>
    inline void serialize(T val, byte_vector& out) {
        if constexpr (std::endian::native == std::endian::big)
            val = platform_dependent::byte_swap(val);

        out.resize(out.size() + sizeof(T));
        memcpy(out.data() + out.size() - sizeof(T), &val, sizeof(T));
    }

    template <typename T> requires Integral<T> && Unsigned<T>
    inline void deserialize(T& val, span<const byte>& in) {
        Expects(sizeof(T) <= static_cast<size_t>(in.size()));

        T value;
        memcpy(&value, in.data(), sizeof(T));
        in = in.subspan(sizeof(T));

        if constexpr (std::endian::native == std::endian::big)
            value = platform_dependent::byte_swap(value);

        val = value;
    }

    //===================== Signed Integral

    template <typename T> requires Integral<T> && (!Unsigned<T>)
    inline void serialize(T signed_val, byte_vector& out) {
        std::make_unsigned_t<T> val;
        memcpy(&val, &signed_val, sizeof(T));

        serialize(val, out);
    }

    template <typename T> requires Integral<T> && (!Unsigned<T>)
    inline void deserialize(T& out, span<const byte>& in) {
        std::make_unsigned_t<T> val;

        deserialize(val, in);

        T value;
        memcpy(&value, &val, sizeof(T));

        out = value;
    }

    //===================== FloatingPoint

    template <typename T> requires FloatingPoint<T>
    inline void serialize(T float_val, byte_vector& out) {
        static_assert(AnyOfType<T, float, double>, "Unsupported floating point type");
        static_assert(std::numeric_limits<T>::is_iec559, "IEEE 754 required");

        typename FloatSizeEqualTypeHelper<T>::type val;
        memcpy(&val, &float_val, sizeof(T));
        serialize(val, out);
    }

    template <typename T> requires FloatingPoint<T>
    inline void deserialize(T& out, span<const byte>& in) {
        static_assert(AnyOfType<T, float, double>, "Unsupported floating point type");
        static_assert(std::numeric_limits<T>::is_iec559, "IEEE 754 required");

        typename FloatSizeEqualTypeHelper<T>::type size_eq_int;
        deserialize(size_eq_int, in);

        T value;
        memcpy(&value, &size_eq_int, sizeof(T));
        out = value;
    }

    //===================== Enum

    template <typename T> requires Enum<T> && (!Integral<T>)
    inline void serialize(T enum_val, byte_vector& out) {
        serialize(static_cast<std::underlying_type_t<T>>(enum_val), out);
    }

    template <typename T> requires Enum<T> && (!Integral<T>)
    inline void deserialize(T& out, span<const byte>& in) {
        using underlying_t = std::underlying_type_t<T>;
        underlying_t res;
        deserialize(res, in);
        out = static_cast<T>(res);
    }

    //===================== Optional

    template <typename T>
    inline void serialize(const optional<T>& o, byte_vector& out) {
        serialize(o.has_value(), out);
        if (o)
            serialize(*o, out);
    }

    template <typename T>
    inline void deserialize(optional<T>& out, span<const byte>& in) {
        bool has_value;
        deserialize(has_value, in);

        if (has_value) {
            T val;
            deserialize(val, in);
            out = optional<T>(move(val));
        }
    }

    //====================== Static Cortege
    template <SerializableStaticCortege T, size_t... idxs>
    inline void serialize_helper(const T& p, byte_vector& out, std::index_sequence<idxs...>&&) {
        using std::get;
        (serialize(get<idxs>(p), out), ...);
    }

    template <SerializableStaticCortege T>
    inline void serialize(const T& p, byte_vector& out) {
        serialize_helper(p, out,
                std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<T>>>());
    }

    template <SerializableStaticCortege T, size_t... idxs>
    inline void deserialize_helper(T& p, span<const byte>& in, std::index_sequence<idxs...>&&) {
        using std::get;
        (deserialize(get<idxs>(p), in), ...);
    }

    template <SerializableStaticCortege T>
    inline void deserialize(T& p, span<const byte>& in) {
        deserialize_helper(p, in,
                std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<T>>>());
    }

    //===================== Iterable

    // Special for array
    template <SerializableArray T>
    inline void serialize(const T& vec, byte_vector& out) {
        for (auto p = begin(vec); p != end(vec); ++p)
            serialize(*p, out);
    }

    template <SerializableArray T>
    inline void deserialize(T& vec, span<const byte>& in) {
        for (auto& e : vec)
            deserialize(e, in);
    }

    template <SerializableIterable T>
    inline void serialize(const T& vec, byte_vector& out) {
        serialize(static_cast<u64>(end(vec) - begin(vec)), out);

        for (auto p = begin(vec); p != end(vec); ++p)
            serialize(*p, out);
    }

    template <SerializableIterable T>
    inline void deserialize(T& vec, span<const byte>& in) {
        auto inserter = std::back_inserter(vec);

        u64 size;
        deserialize(size, in);

        for (decltype(size) i = 0; i < size; ++i) {
            std::decay_t<decltype(*begin(vec))> v;
            deserialize(v, in);
            inserter = move(v);
        }
    }

    //==================== Map
    template <SerializableMap T>
    void serialize(const T& map, byte_vector& out) {
        serialize(static_cast<u64>(map.size()), out);

        for (auto p = begin(map); p != end(map); ++p)
            serialize(*p, out);
    }

    template <SerializableMap T>
    void deserialize(T& map, span<const byte>& in) {
        u64 size;
        deserialize(size, in);

        for (decltype(size) i = 0; i < size; ++i) {
            std::decay_t<decltype(*begin(map))> v;
            deserialize(v, in);
            map.emplace(move(v));
        }
    }

    //===================== Span
    template <typename T>
    void serialize(span<const T> data, const T* storage_ptr, byte_vector& out) {
        if (data.empty()) {
            serialize(static_cast<u64>(0), out);
            serialize(static_cast<u64>(0), out);
        } else {
            serialize(static_cast<u64>(&(*data.begin()) - storage_ptr), out);
            serialize(static_cast<u64>(data.size()), out);
        }
    }

    template <typename T>
    void deserialize(span<T>& data, T* storage_ptr, span<const byte>& in) {
        u64 start, size;
        deserialize(start, in);
        deserialize(size, in);
        data = span<T>(storage_ptr + static_cast<ptrdiff_t>(start), static_cast<ptrdiff_t>(size));
    }
} // namespace core
