#pragma once

#include "types.hpp"
#include "platform_dependent.hpp"


#define EVO_SERIALIZE(...) void serialize(serializer& _s) const { _s.write(__VA_ARGS__); } \
                           void deserialize(deserializer_view& _d) { _d.read(__VA_ARGS__); }

namespace core
{
    // Array with size < 32 only (prevent std::array<T, 10000000> compile-time jokes)
    template <typename T>
    concept SerializableStaticCortege =
            std::tuple_size<std::remove_reference_t<T>>::value < 32;

    template <typename T>
    concept SerializableIterable =
            (!SerializableStaticCortege<T>) && requires (T&& t) { std::begin(t); std::end(t); };

    template <typename T>
    concept SerializableArray = SerializableIterable<T> && StdArray<T>;

    using byte_vector = vector<byte>;

    template <size_t size>
    using byte_array = array<byte, size>;

    template <typename... Ts>
    auto make_byte_vector(Ts... bytes) {
        return vector{static_cast<byte>(bytes)...};
    }

    //==================== Unsigned Integral

    template <typename T> requires Integral<T> && Unsigned<T>
    inline void serialize(T val, byte_vector& out) {
        if constexpr (std::endian::native == std::endian::big)
            val = platform_dependent::byte_swap(val);

        out.resize(out.size() + sizeof(T));
        memcpy(out.data() + out.size() - sizeof(T), &val, sizeof(T));
    }

    template <typename T> requires Integral<T> && Unsigned<T>
    inline void deserialize(T& val, span<byte>& in) {
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
    inline void deserialize(T& out, span<byte>& in) {
        std::make_unsigned_t<T> val;

        deserialize(val, in);

        T value;
        memcpy(&value, &val, sizeof(T));

        out = value;
    }

    //===================== FloatingPoint

    template <typename T>
    struct FloatSizeEqualTypeHelper;

    template <>
    struct FloatSizeEqualTypeHelper<float> { using type = uint32_t; };

    template <>
    struct FloatSizeEqualTypeHelper<double> { using type = uint64_t; };

    template <typename T> requires FloatingPoint<T>
    inline void serialize(T float_val, byte_vector& out) {
        static_assert(AnyOfType<T, float, double>, "Unsupported floating point type");
        static_assert(std::numeric_limits<T>::is_iec559, "IEEE 754 required");

        typename FloatSizeEqualTypeHelper<T>::type val;
        memcpy(&val, &float_val, sizeof(T));
        serialize(val, out);
    }

    template <typename T> requires FloatingPoint<T>
    inline void deserialize(T& out, span<byte>& in) {
        static_assert(AnyOfType<T, float, double>, "Unsupported floating point type");
        static_assert(std::numeric_limits<T>::is_iec559, "IEEE 754 required");

        typename FloatSizeEqualTypeHelper<T>::type size_eq_int;
        deserialize(size_eq_int, in);

        T value;
        memcpy(&value, &size_eq_int, sizeof(T));
        out = value;
    }

    template <SerializableStaticCortege T>
    inline void serialize(const T& p, byte_vector& out);

    template <SerializableStaticCortege T>
    inline void deserialize(T& p, span<byte>& in);

    template <SerializableArray T>
    inline void serialize(const T& vec, byte_vector& out);

    template <SerializableIterable T>
    inline void serialize(const T& vec, byte_vector& out);

    template <SerializableArray T>
    inline void deserialize(T& vec, span<byte>& in);

    template <SerializableIterable T>
    inline void deserialize(T& vec, span<byte>& in);

    //===================== Optional

    template <typename T>
    inline void serialize(const optional<T>& o, byte_vector& out) {
        serialize(o.has_value(), out);
        if (o)
            serialize(*o, out);
    }

    template <typename T>
    inline void deserialize(optional<T>& out, span<byte>& in) {
        bool has_value;
        deserialize(has_value, in);

        if (has_value) {
            T val;
            deserialize(val, in);
            out = optional<T>(std::move(val));
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
    inline void deserialize_helper(T& p, span<byte>& in, std::index_sequence<idxs...>&&) {
        using std::get;
        (deserialize(get<idxs>(p), in), ...);
    }

    template <SerializableStaticCortege T>
    inline void deserialize(T& p, span<byte>& in) {
        deserialize_helper(p, in,
                std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<T>>>());
    }

    //===================== Iterable

    // Special for array
    template <SerializableArray T>
    inline void serialize(const T& vec, byte_vector& out) {
        for (auto p = std::begin(vec); p != std::end(vec); ++p)
            serialize(*p, out);
    }

    template <SerializableArray T>
    inline void deserialize(T& vec, span<byte>& in) {
        for (auto& e : vec)
            deserialize(e, in);
    }

    template <SerializableIterable T>
    inline void serialize(const T& vec, byte_vector& out) {
        serialize(std::end(vec) - std::begin(vec), out);

        for (auto p = std::begin(vec); p != std::end(vec); ++p)
            serialize(*p, out);
    }

    template <SerializableIterable T>
    inline void deserialize(T& vec, span<byte>& in) {
        auto inserter = std::back_inserter(vec);

        typename T::size_type size;
        deserialize(size, in);

        for (decltype(size) i = 0; i < size; ++i) {
            std::decay_t<decltype(*std::begin(vec))> v;
            deserialize(v, in);
            inserter = move(v);
        }
    }

    class serializer;
    class deserializer_view;
    template <typename T>
    concept Serializable = requires (T v, serializer& s) { v.serialize(s); };
    template <typename T>
    concept Deserializable = requires (T v, deserializer_view& d) { v.deserialize(d); };


    class serializer {
    private:
        template <typename T> requires Serializable<T>
        void serialize_impl(const T& value) {
            value.serialize(*this);
        }

        template <typename T> requires (!Serializable<T>)
        void serialize_impl(const T& value) {
            serialize(value, _data);
        }

    public:
        template <typename... Ts>
        void write(const Ts&... values) {
            (serialize_impl(values), ...);
        }

        const auto& data() const {
            return _data;
        }

    private:
        byte_vector _data;
    };


    class deserializer_view {
    private:
        template <typename T> requires Deserializable<T>
        void deserialize_impl(T& value) {
            value.deserialize(*this);
        }

        template <typename T> requires (!Deserializable<T>)
        void deserialize_impl(T& value) {
            deserialize(value, _range);
        }

    public:
        deserializer_view(span<byte> range): _range(range) {}

        deserializer_view(deserializer_view&&) = delete;
        deserializer_view& operator=(deserializer_view&&) = delete;

        template <typename... Ts>
        void read(Ts&... values) {
            (deserialize_impl(values), ...);
        }


    private:
        span<byte> _range;
    };

} // namespace core
