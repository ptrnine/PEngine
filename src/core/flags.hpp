#include <type_traits>
#include <cstdint>

/**
 * Flag type implementation
 *
 * example:
 *
 * // Define flag type with A,B,C,D,E,F,G,H states
 * DEF_FLAG_TYPE(Flagger, flag8_t,
        A = Flagger::def<0>,
        B = Flagger::def<1>,
        C = Flagger::def<2>,
        D = Flagger::def<3>,
        E = Flagger::def<4>,
        F = Flagger::def<5>,
        G = Flagger::def<6>,
        H = Flagger::def<7>,
        // I = Flagger::def<8>   <- compilation error, because Flag8 is 8-bit type
    );

    Flagger flag;

    flag.set(Flagger::A); // Set flag to A   (A)
    flag.set(flag.B);     // Set flag to B   (A|B)

    // Test
    if (flag.test(Flagger::A | Flagger::B)) {
        std::cout << "Success!" << std::endl;
    }
 */

namespace core
{
template <typename Type>
class flag_tmpl {
    static_assert(std::is_integral_v<Type> && std::is_unsigned_v<Type>,
                  "Flag type must be unsigned int!");
public:
    using value_type = Type;

    template <Type flag>
    struct _define_helper {
        static_assert(flag < sizeof(Type) * 8, "Can't define that bit!"); // NOLINT
        inline static constexpr Type def = Type(1) << flag;
    };

    template <Type flag>
    inline static constexpr Type def = _define_helper<flag>::def;

    flag_tmpl() = default;
    flag_tmpl(Type flags): _data(flags) {}

public:
    constexpr void set     (Type flags) noexcept            { _data |= flags; }
    constexpr void set_if  (Type flags, bool expr) noexcept { if (expr) set(flags); }
    constexpr bool test_or (Type flags) const noexcept      { return _data & flags; }
    constexpr bool test    (Type flags) const noexcept      { return _data == (_data | flags); }
    constexpr void reset   () noexcept                      { _data = 0; }
    constexpr Type data    () const noexcept                { return _data; }

private:
    Type _data = 0;
};

using flag8_t  = flag_tmpl<uint8_t>;
using flag16_t = flag_tmpl<uint16_t>;
using flag32_t = flag_tmpl<uint32_t>;
using flag64_t = flag_tmpl<uint64_t>;
using flag_t   = flag_tmpl<unsigned int>;

} // namespace core

#define DEF_FLAG_TYPE(NAME, TYPE, ...) /*NOLINT*/ \
struct NAME : TYPE {                              \
    using t____ = TYPE;                           \
    using t____::t____;                           \
    NAME() : t____() {}                           \
    enum : t____::value_type {                    \
        __VA_ARGS__                               \
    };                                            \
}

