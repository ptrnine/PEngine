#pragma once

#include "core/container_extensions.hpp"
#include <core/types.hpp>
#include <core/serialization.hpp>
#include <core/flags.hpp>

namespace util {

enum class vtf_image_format : core::u32 {
    NONE     = core::u32(-1),
    RGBA8888 = 0,
    ABGR8888,
    RGB888,
    BGR888,
    RGB565,
    I8,
    IA88,
    P8,
    A8,
    RGB888_BLUESCREEN,
    BGR888_BLUESCREEN,
    ARGB8888,
    BGRA8888,
    DXT1,
    DXT3,
    DXT5,
    BGRX8888,
    BGR565,
    BGRX5551,
    BGRA4444,
    DXT1_ONEBITALPHA,
    BGRA5551,
    UV88,
    UVWQ8888,
    RGBA16161616F,
    RGBA16161616,
    UVLX8888
};

DEF_FLAG_TYPE(vtf_texture_flags, core::flag32_t,
    POINTSAMPLE       = 0x00000001,
    TRILINEAR         = 0x00000002,
    CLAMPS            = 0x00000004,
    CLAMPT            = 0x00000008,
    ANISOTROPIC       = 0x00000010,
    HINT_DXT5         = 0x00000020,
    PWL_CORRECTED     = 0x00000040,
    NORMAL            = 0x00000080,
    NOMIP             = 0x00000100,
    NOLOD             = 0x00000200,
    ALL_MIPS          = 0x00000400,
    PROCEDURAL        = 0x00000800,
    ONEBITALPHA       = 0x00001000,
    EIGHTBITALPHA     = 0x00002000,
    ENVMAP            = 0x00004000,
    RENDERTARGET      = 0x00008000,
    DEPTHRENDERTARGET = 0x00010000,
    NODEBUGOVERRIDE   = 0x00020000,
    SINGLECOPY        = 0x00040000,
    PRE_SRGB          = 0x00080000,
    UNUSED_00100000   = 0x00100000,
    UNUSED_00200000   = 0x00200000,
    UNUSED_00400000   = 0x00400000,
    NODEPTHBUFFER     = 0x00800000,
    UNUSED_01000000   = 0x01000000,
    CLAMPU            = 0x02000000,
    VERTEXTEXTURE     = 0x04000000,
    SSBUMP            = 0x08000000,
    UNUSED_10000000   = 0x10000000,
    BORDER            = 0x20000000,
    UNUSED_40000000   = 0x40000000,
    UNUSED_80000000   = 0x80000000
);

struct vtf_header_base {
    PE_SERIALIZE(signature, version, header_size)

    core::array<char, 4>      signature;    // 0
    core::array<core::u32, 2> version;      // 4
    core::u32                 header_size;  // 12
};

struct vtf_header_7_1 : public vtf_header_base {
    PE_SERIALIZE_SUPER(vtf_header_base,
                       size,
                       flags,
                       frames,
                       first_frame,
                       padding0,
                       reflectivity,
                       padding1,
                       bumpmap_scale,
                       high_res_image_format,
                       mipmap_count,
                       low_res_image_format,
                       low_res_image_size)

    core::vec2<core::u16>     size;                    // 16
    vtf_texture_flags         flags;                   // 20
    core::u16                 frames;                  // 24
    core::u16                 first_frame;             // 26
    core::u8                  padding0[4];             // 28
    core::array<float, 3>     reflectivity;            // 32
    core::u8                  padding1[4];             // 44
    float                     bumpmap_scale;           // 48
    vtf_image_format          high_res_image_format;   // 52
    core::u8                  mipmap_count;            // 56
    vtf_image_format          low_res_image_format;    // 57
    core::vec2<core::u8>      low_res_image_size;      // 61
};

struct vtf_header_7_2 : public vtf_header_7_1 {
    PE_SERIALIZE_SUPER(vtf_header_7_1, depth)
    core::u16 depth; // 63
};

struct vtf_header : public vtf_header_7_2 {
    PE_SERIALIZE_SUPER(vtf_header_7_2, padding2, num_resources, padding3)
    core::u8  padding2[3];    // 65
    core::u32 num_resources;  // 68
    core::u8  padding3[8];    // 72
};

constexpr size_t VTF_HEADER_SIZE = 80;

using vtf_resource_tag = core::array<char, 3>;

namespace vtf_resource_tags {
    inline constexpr auto low_res                 = vtf_resource_tag{'\x01', '\0', '\0'};
    inline constexpr auto high_res                = vtf_resource_tag{'\x30', '\0', '\0'};
    inline constexpr auto animated_particle_sheet = vtf_resource_tag{'\x10', '\0', '\0'};
    inline constexpr auto crc                     = vtf_resource_tag{'C', 'R', 'C'};
    inline constexpr auto lod                     = vtf_resource_tag{'L', 'O', 'D'};
    inline constexpr auto tsd                     = vtf_resource_tag{'T', 'S', 'D'};
    inline constexpr auto kvd                     = vtf_resource_tag{'K', 'V', 'D'};
} // namespace vtf_resource_tags

struct vtf_resource_entry_info {
    PE_SERIALIZE(tag, flags, offset)

    core::array<char, 3> tag;
    core::u8             flags;
    core::u32            offset;
};

inline core::vec<core::u8, 3> vtf_unpack_rgb565(core::u16 color) {
    return {
        core::u8((color >> 11) * 8),
        core::u8(((color >> 5) & 0x3f) * 4),
        core::u8((color & 0x1f) * 8)
    };
}

using vtf_dxt1_unpacked_pixel_t = core::array<core::array<core::vec<core::u8, 3>, 4>, 4>;
inline vtf_dxt1_unpacked_pixel_t vtf_dxt1_unpack_pixel(core::u64 comp_pixel) {
    using rgb_t = core::vec<core::u8, 3>;

    struct comp_data {
        core::u16 palette[2];
        core::u8  color_row[4];
    } cd;

    vtf_dxt1_unpacked_pixel_t unpacked;
    core::array<rgb_t, 4> palette;

    std::memcpy(&cd, &comp_pixel, sizeof(cd));
    palette[0] = vtf_unpack_rgb565(cd.palette[0]);
    palette[1] = vtf_unpack_rgb565(cd.palette[1]);
    palette[2] = static_cast<rgb_t>((palette[0] * 2) / 3 + (palette[1] / 3));
    palette[3] = static_cast<rgb_t>((palette[0] / 3) + (palette[1] * 2) / 3);

    for (size_t i = 0; i < 4; ++i) {
        core::u8 d = cd.color_row[i] >> 6;
        core::u8 c = (cd.color_row[i] >> 4) & 3;
        core::u8 b = (cd.color_row[i] >> 2) & 3;
        core::u8 a = (cd.color_row[i]) & 3;
        unpacked[i][0] = palette[a];
        unpacked[i][1] = palette[b];
        unpacked[i][2] = palette[c];
        unpacked[i][3] = palette[d];
    }

    return unpacked;
}

class vtf_view {
public:
    class resource_info_iterator {
    public:
        resource_info_iterator(core::u32 i, core::span<const core::byte> data):
            _i(i), _resource_infos(data.subspan<ssize_t(VTF_HEADER_SIZE)>()) {}

        vtf_resource_entry_info operator*() const {
            return core::deserializer_view(
                       _resource_infos.subspan(ssize_t(_i * sizeof(vtf_resource_entry_info))))
                .read_get<vtf_resource_entry_info>();
        }

        resource_info_iterator& operator++() {
            ++_i;
            return *this;
        }

        resource_info_iterator operator++(int) {
            resource_info_iterator res = *this;
            operator++();
            return res;
        }

        bool operator==(const resource_info_iterator& rhs) const {
            return _i == rhs._i;
        }

        bool operator!=(const resource_info_iterator& rhs) const {
            return !operator==(rhs);
        }

        ptrdiff_t operator-(const resource_info_iterator& rhs) const {
            return ptrdiff_t(_i) - ptrdiff_t(rhs._i);
        }

    private:
        core::u32 _i; // NOLINT
        core::span<const core::byte> _resource_infos;
    };

    class resource_info_view_t {
    public:
        resource_info_view_t(core::u32 num_resources, core::span<const core::byte> data):
            _begin(0, data),
            _end(num_resources, data),
            _size(num_resources) {}

        [[nodiscard]]
        resource_info_iterator begin() const { return _begin; }
        [[nodiscard]]
        resource_info_iterator end() const { return _end; }
        [[nodiscard]]
        size_t size() const { return _size; }

    private:
        resource_info_iterator _begin;
        resource_info_iterator _end;
        size_t                 _size;
    };

public:
    vtf_view(core::span<const core::byte> idata) : data(idata) {}

    [[nodiscard]] bool is_valid() const {
        return core::deserializer_view(data).read_get<vtf_header_base>().signature ==
               core::array{'V', 'T', 'F', '\0'};
    }

    operator bool() const {
        return is_valid();
    }

    [[nodiscard]] std::array<core::u32, 2> version() const {
        return core::deserializer_view(data).read_get<vtf_header_base>().version;
    }

    [[nodiscard]] vtf_header header() const {
        auto ver = version();
        vtf_header h = {};
        auto ds = core::deserializer_view(data);

        if (ver == core::array{7U, 1U})
            ds.read<vtf_header_7_1>(h);
        else if (ver == core::array{7U, 2U})
            ds.read<vtf_header_7_2>(h);
        else if ((ver[0] == 7 && ver[1] >= 3) || ver[0] > 7)
            ds.read<vtf_header>(h);
        else
            h.version = ver;

        return h;
    }

    [[nodiscard]] resource_info_view_t resource_info_view() const {
        auto ver = version();

        if ((ver[0] == 7 && ver[1] >= 3) || ver[0] > 7)
            return resource_info_view_t(
                core::deserializer_view(data.subspan</*num_resources*/68>())
                    .read_get<core::u32>(),
                data);
        else
            return resource_info_view_t(0, data);
    }

    [[nodiscard]] vtf_image_format low_res_format() const {
        return core::deserializer_view(data.subspan<57 /*low_res_image_size*/>())
            .read_get<vtf_image_format>();
    }

    [[nodiscard]] vtf_image_format high_res_format() const {
        return core::deserializer_view(data.subspan<52 /*high_res_image_size*/>())
            .read_get<vtf_image_format>();
    }

    [[nodiscard]] size_t low_res_channels_count() const {
        return format_to_channels_count(low_res_format());
    }

    [[nodiscard]] size_t high_res_channels_count() const {
        return format_to_channels_count(high_res_format());
    }

    [[nodiscard]] core::vec2<core::u8> low_res_size() const {
        return core::deserializer_view(data.subspan<61 /*low_res_image_size*/>())
            .read_get<core::vec2<core::u8>>();
    }

    [[nodiscard]] core::vec2<core::u16> high_res_size() const {
        return core::deserializer_view(data.subspan<16 /*size*/>())
            .read_get<core::vec2<core::u16>>();
    }

    [[nodiscard]] core::u32 low_res_offset() const {
        auto ver = version();

        if (ver == core::array{7U, 1U})
            return 80;
        else if (ver == core::array{7U, 2U})
            return 80;
        else if ((ver[0] == 7 && ver[1] >= 3) || ver[0] > 7) {
            for (auto resource_info : resource_info_view())
                if (resource_info.tag == vtf_resource_tags::low_res)
                    return resource_info.offset;
            throw std::runtime_error("VTF does not contain low resolution resource (!?)");
        } else
            throw std::runtime_error(core::format("Unknown VTF version {}.{}", ver[0], ver[1]));
    }

    [[nodiscard]] core::u32 high_res_offset() const {
        auto ver             = version();
        auto low_res_sz      = low_res_size();
        auto low_res_byte_sz = core::u32(((low_res_sz.x() * low_res_sz.y()) / 16U) *
                                         sizeof(core::u64)); // sizeof dxt1 quad

        if (ver == core::array{7U, 1U})
            return 80 + low_res_byte_sz;
        else if (ver == core::array{7U, 2U})
            return 80 + low_res_byte_sz;
        else if ((ver[0] == 7 && ver[1] >= 3) || ver[0] > 7) {
            for (auto resource_info : resource_info_view())
                if (resource_info.tag == vtf_resource_tags::high_res)
                    return resource_info.offset;
            throw std::runtime_error("VTF does not contain low resolution resource (!?)");
        } else
            throw std::runtime_error(core::format("Unknown VTF version {}.{}", ver[0], ver[1]));
    }

    [[nodiscard]] core::u32 mipmap_count() const {
        return core::deserializer_view(data.subspan<56>()).read_get<core::u8>();
    }

    void read_low_res(core::u8* output) const {
        auto comp_size = static_cast<core::vec2<core::u32>>(low_res_size()) / 4U;
        auto size = comp_size * 4U;
        core::u64 comp_data[16U * 16U];
        core::println("LOW RES OFFSET: {}", low_res_offset());
        std::memcpy(comp_data,
                    data.data() + low_res_offset(),
                    comp_size.x() * comp_size.y() * sizeof(core::u64));

        for (auto [x, y] : core::dimensional_seq(comp_size)) {
            auto output_offset = ((y * 4U * size.x()) + x * 4U) * 3U;
            auto comp_offset = y * comp_size.x() + x;
            auto unpacked = vtf_dxt1_unpack_pixel(comp_data[comp_offset]);
            constexpr auto unps = sizeof(unpacked[0]);
            std::memcpy(output + output_offset + size.x() * 0U, unpacked[0].data(), unps);
            std::memcpy(output + output_offset + size.x() * 3U, unpacked[1].data(), unps);
            std::memcpy(output + output_offset + size.x() * 6U, unpacked[2].data(), unps);
            std::memcpy(output + output_offset + size.x() * 9U, unpacked[3].data(), unps);
        }
    }

    void read_high_res(core::u8* output) const {
        auto format      = high_res_format();
        auto channels    = format_to_channels_count(format);
        auto size        = static_cast<core::vec2<core::u32>>(high_res_size());
        auto offset      = high_res_offset();
        auto last_mipmap = skip_mipmaps_pixels(mipmap_count(), size);
        auto end_largest = last_mipmap + size.x() * size.y();
        auto start       = data.subspan(offset);

        switch (format) {
            case vtf_image_format::A8: case vtf_image_format::I8: case vtf_image_format::P8: case vtf_image_format::IA88:
            case vtf_image_format::UV88: case vtf_image_format::RGB888_BLUESCREEN: case vtf_image_format::RGB888: case vtf_image_format::RGBA8888:
                std::memcpy(output, start.data(), size.x() * size.y() * channels);
                break;
            case vtf_image_format::BGR565:
                for (core::u32 i = last_mipmap; i < end_largest; ++i) {
                    core::u16 color; // NOLINT
                    std::memcpy(&color, start.data() + i * sizeof(core::u16), sizeof(core::u16));
                    auto unpacked_color = vtf_unpack_rgb565(color).bgr();
                    std::memcpy(output + (i - last_mipmap) * channels, &unpacked_color, sizeof(unpacked_color));
                }
                break;
            case vtf_image_format::RGB565:
                for (core::u32 i = last_mipmap; i < end_largest; ++i) {
                    core::u16 color; // NOLINT
                    std::memcpy(&color, start.data() + i * sizeof(core::u16), sizeof(core::u16));
                    auto unpacked_color = vtf_unpack_rgb565(color);
                    std::memcpy(output + (i - last_mipmap) * channels, &unpacked_color, sizeof(unpacked_color));
                }
                break;
            case vtf_image_format::BGR888: case vtf_image_format::BGR888_BLUESCREEN:
                for (core::u32 i = last_mipmap; i < end_largest; ++i) {
                    core::vec3<core::u8> color; // NOLINT
                    std::memcpy(&color, start.data() + i * sizeof(color), sizeof(color));
                    color = color.bgr();
                    std::memcpy(output + (i - last_mipmap) * channels, &color, sizeof(color));
                }
                break;
            case vtf_image_format::ARGB8888:
                for (core::u32 i = last_mipmap; i < end_largest; ++i) {
                    core::vec4<core::u8> color; // NOLINT
                    std::memcpy(&color, start.data() + i * sizeof(color), sizeof(color));
                    color = color.gbar();
                    std::memcpy(output + (i - last_mipmap) * channels, &color, sizeof(color));
                }
                break;
            case vtf_image_format::ABGR8888:
                for (core::u32 i = last_mipmap; i < end_largest; ++i) {
                    core::vec4<core::u8> color; // NOLINT
                    std::memcpy(&color, start.data() + i * sizeof(color), sizeof(color));
                    color = color.abgr();
                    std::memcpy(output + (i - last_mipmap) * channels, &color, sizeof(color));
                }
                break;
            case vtf_image_format::DXT1: {
                auto comp_size = size / 4U;
                auto cdata = start.subspan(skip_mipmaps_pixels(mipmap_count(), comp_size) * sizeof(core::u64));
                for (auto [x, y] : core::dimensional_seq(comp_size)) {
                    auto           output_offset = ((y * 4U * size.x()) + x * 4U) * 3U;
                    auto           comp_offset   = y * comp_size.x() + x;
                    core::u64      comp_quad; // NOLINT
                    std::memcpy(&comp_quad, cdata.data() + comp_offset * sizeof(core::u64), sizeof(core::u64));
                    auto           unpacked      = vtf_dxt1_unpack_pixel(comp_quad);
                    constexpr auto unps          = sizeof(unpacked[0]);
                    std::memcpy(output + output_offset + size.x() * 0U, unpacked[0].data(), unps);
                    std::memcpy(output + output_offset + size.x() * 3U, unpacked[1].data(), unps);
                    std::memcpy(output + output_offset + size.x() * 6U, unpacked[2].data(), unps);
                    std::memcpy(output + output_offset + size.x() * 9U, unpacked[3].data(), unps);
                }
            } break;
            case vtf_image_format::DXT1_ONEBITALPHA:
            case vtf_image_format::DXT3:
            case vtf_image_format::DXT5:
            case vtf_image_format::BGRA4444:
            case vtf_image_format::BGRA5551:
            case vtf_image_format::BGRX5551:
            case vtf_image_format::BGRX8888:
            case vtf_image_format::UVWQ8888:
            case vtf_image_format::UVLX8888:
                pe_throw std::runtime_error("Not implemented format " + core::string(magic_enum::enum_name(format)));
            case vtf_image_format::BGRA8888: case vtf_image_format::RGBA16161616: case vtf_image_format::RGBA16161616F:
                pe_throw std::runtime_error("Invalid image format: trying to read HDR texture as non-HDR");
            case vtf_image_format::NONE:
                pe_throw std::runtime_error("Invalid image format");
        }
    }

    void read_high_res(float* output) const {
        auto format      = high_res_format();
        auto channels    = format_to_channels_count(format);
        auto size        = static_cast<core::vec2<core::u32>>(high_res_size());
        auto offset      = high_res_offset();
        auto last_mipmap = skip_mipmaps_pixels(mipmap_count(), size);
        auto end_largest = last_mipmap + size.x() * size.y();
        auto start       = data.subspan(offset);

        switch (format) {
        case vtf_image_format::BGRA8888:
            for (core::u32 i = last_mipmap; i < end_largest; ++i) {
                core::vec4<core::u8> color; // NOLINT
                std::memcpy(&color, start.data() + i * sizeof(color), sizeof(color));
                color = color.bgra();
                core::vec3f decomp_color =
                    (static_cast<core::vec3f>(color.rgb()) * static_cast<float>(color.a()) * 16.f) /
                    262144.f;
                std::memcpy(output + (i - last_mipmap) * channels, &decomp_color, sizeof(decomp_color));
            }
            break;
        case vtf_image_format::RGBA16161616:
             for (core::u32 i = last_mipmap; i < end_largest; ++i) {
                core::vec4<core::u16> color; // NOLINT
                std::memcpy(&color, start.data() + i * sizeof(color), sizeof(color));
                core::vec4f float_color = static_cast<core::vec4f>(color) /
                                          static_cast<float>(core::numlim<core::u16>::max());
                std::memcpy(output + (i - last_mipmap) * channels, &float_color, sizeof(float_color));
            }
            break;
        case vtf_image_format::RGBA16161616F:
            pe_throw std::runtime_error("Not implemented");
        default:
            pe_throw std::runtime_error("Invalid image format: trying to read non-HDR texture as HDR");
        }
    }

    [[nodiscard]]
    bool is_high_res_hdr() const {
        switch (high_res_format()) {
            case vtf_image_format::BGRA8888: case vtf_image_format::RGBA16161616: case vtf_image_format::RGBA16161616F:
                return true;
            default:
                return false;
        }
    }

    static core::u32 skip_mipmaps_pixels(size_t mipmap_count, core::vec2<core::u32> size) {
        core::u32 skip = 0;
        if (mipmap_count == 0)
            return skip;

        --mipmap_count;
        while (mipmap_count != 0) {
            size     = size / 2u;
            size.x() = std::max(size.x(), 1u);
            size.y() = std::max(size.y(), 1u);
            skip += size.x() * size.y();
            --mipmap_count;
        }
        return skip;
    }

    static size_t format_to_channels_count(vtf_image_format fmt) {
        switch (fmt) {
            case vtf_image_format::A8: case vtf_image_format::I8: case vtf_image_format::P8:
                return 1;
            case vtf_image_format::IA88: case vtf_image_format::UV88:
                return 2;
            case vtf_image_format::BGR565: case vtf_image_format::BGR888: case vtf_image_format::RGB565:
                case vtf_image_format::RGB888_BLUESCREEN: case vtf_image_format::RGB888: case vtf_image_format::DXT1:
                case vtf_image_format::BGRA8888:
                return 3;
            default:
                return 4;
        }
    }

private:
    core::span<const core::byte> data;
};
}

template <>
struct std::iterator_traits<util::vtf_view::resource_info_iterator> {
    using difference_type = ptrdiff_t;
    using value_type = util::vtf_resource_entry_info;
    using reference = const value_type&;
    using iterator_category = std::forward_iterator_tag;
};


