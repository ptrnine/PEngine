#include "compression.hpp"

#include <zlib.h>

using namespace core;

template <bool Inflate>
void throw_zlib(int error_code, z_streamp to_free = nullptr) {
    const char* msg = "zlib: unknown error";

    switch (error_code) {
    case Z_MEM_ERROR: pe_throw std::bad_alloc();
    case Z_STREAM_ERROR: msg = "zlib: stream error"; break;
    case Z_BUF_ERROR: msg = "zlib: buffer error"; break;
    case Z_DATA_ERROR: msg = "zlib: data error"; break;
    case Z_VERSION_ERROR: msg = "zlib: zlib.h and zlib library versions do not match"; break;
    default: break;
    }

    if constexpr (Inflate)
        inflateEnd(to_free);
    else
        deflateEnd(to_free);
    std::cout << "THROW: " << msg << std::endl;
    pe_throw std::runtime_error(msg);
}

constexpr const size_t chunk_size = 16384;

template <bool Inflate>
vector<byte> compress(span<const byte>                data,
                      [[maybe_unused]] int            compression_level,
                      const function<void(u64, u64)>& progress) {
    z_stream stream;

    Bytef  in[chunk_size];  // NOLINT
    Bytef  out[chunk_size]; // NOLINT
    size_t count = 0;

    vector<byte> res;
    res.reserve(static_cast<size_t>(data.size()));

    stream.zalloc   = Z_NULL;
    stream.zfree    = Z_NULL;
    stream.opaque   = Z_NULL;
    stream.avail_in = 0;
    stream.next_in  = Z_NULL;

    int rc; // NOLINT
    if constexpr (Inflate)
        rc = inflateInit2(&stream, MAX_WBITS + 16);
    else
        rc = deflateInit2(&stream,
                          util::compress_level_clamp(compression_level),
                          Z_DEFLATED,
                          MAX_WBITS + 16,
                          8,
                          Z_DEFAULT_STRATEGY);

    if (rc != Z_OK)
        throw_zlib<Inflate>(rc);

    bool work = true;
    do {
        stream.avail_in = uInt(static_cast<size_t>(data.size()) - count >= chunk_size
                                   ? chunk_size
                                   : static_cast<size_t>(data.size()) - count);
        memcpy(in, data.data() + count, stream.avail_in);
        count += stream.avail_in;

        if (progress)
            progress(count, static_cast<u64>(data.size()));

        if (stream.avail_in == 0)
            break;

        stream.next_in = in;

        int flsh; // NOLINT

        do {
            stream.avail_out = chunk_size;
            stream.next_out  = out;

            if constexpr (Inflate) {
                flsh = Z_NO_FLUSH;
                rc = inflate(&stream, flsh);
            } else {
                flsh = count < static_cast<size_t>(data.size()) ? Z_NO_FLUSH : Z_FINISH;
                rc = deflate(&stream, flsh);
            }

            if (rc != Z_OK && rc != Z_STREAM_END) {
                if (rc == Z_NEED_DICT)
                    rc = Z_DATA_ERROR;

                throw_zlib<Inflate>(rc, &stream);
            }

            auto have = chunk_size - stream.avail_out;
            auto start = res.size();
            res.resize(res.size() + have);
            std::memcpy(res.data() + start, out, have);

        } while (stream.avail_out == 0);

        if constexpr (Inflate)
            work = rc != Z_STREAM_END;
        else
            work = flsh != Z_FINISH;
    } while (work);

    if constexpr (Inflate)
        inflateEnd(&stream);
    else
        deflateEnd(&stream);

    return res;
}

template <bool IsConst>
struct block_storage {
    block_storage(size_t) {}
};

template <>
struct block_storage<true> {
    block_storage(size_t size) { data.resize(size); }
    vector<Bytef> data;
};

template <typename T>
vector<byte> compress_block(span<T> data, int compression_level) {
    vector<byte> out;
    out.resize(static_cast<size_t>(data.size()));

    z_stream stream;
    stream.zalloc   = Z_NULL;
    stream.zfree    = Z_NULL;
    stream.opaque   = Z_NULL;
    stream.avail_in = static_cast<uInt>(data.size());
    stream.avail_out = static_cast<uInt>(out.size());
    stream.next_out  = reinterpret_cast<Bytef*>(out.data()); // NOLINT

    auto in_tmp = block_storage<std::is_const_v<T>>(static_cast<size_t>(data.size()));
    if constexpr (std::is_const_v<T>) {
        stream.next_in = in_tmp.data.data();
        std::memcpy(in_tmp.data.data(), data.data(), static_cast<size_t>(data.size()));
    } else {
        stream.next_in = reinterpret_cast<Bytef*>(data.data()); // NOLINT
    }

    int rc = deflateInit2(&stream,
                          util::compress_level_clamp(compression_level),
                          Z_DEFLATED,
                          MAX_WBITS + 16,
                          8,
                          Z_DEFAULT_STRATEGY);

    if (rc != Z_OK)
        throw_zlib<false>(rc);

    rc = deflate(&stream, Z_FINISH);
    if (rc != Z_OK && rc != Z_STREAM_END) {
        if (rc == Z_NEED_DICT)
            rc = Z_DATA_ERROR;
        throw_zlib<false>(rc, &stream);
    }
    deflateEnd(&stream);
    out.resize(out.size() - stream.avail_out);

    return out;
}

template <typename T>
vector<byte> decompress_block(span<T> data, size_t output_size) {
    vector<byte> out;
    out.resize(output_size);

    z_stream stream;
    stream.zalloc   = Z_NULL;
    stream.zfree    = Z_NULL;
    stream.opaque   = Z_NULL;
    stream.avail_in = static_cast<uInt>(data.size());
    stream.avail_out = static_cast<uInt>(out.size());
    stream.next_out  = reinterpret_cast<Bytef*>(out.data()); // NOLINT

    auto in_tmp = block_storage<std::is_const_v<T>>(static_cast<size_t>(data.size()));
    if constexpr (std::is_const_v<T>) {
        stream.next_in = in_tmp.data.data();
        std::memcpy(in_tmp.data.data(), data.data(), static_cast<size_t>(data.size()));
    } else {
        stream.next_in = reinterpret_cast<Bytef*>(data.data()); // NOLINT
    }

    int rc = inflateInit2(&stream, MAX_WBITS + 16);

    if (rc != Z_OK)
        throw_zlib<true>(rc);

    rc = inflate(&stream, Z_NO_FLUSH);
    if (rc != Z_OK && rc != Z_STREAM_END) {
        if (rc == Z_NEED_DICT)
            rc = Z_DATA_ERROR;
        throw_zlib<true>(rc, &stream);
    }
    inflateEnd(&stream);

    return out;
}

namespace details {
template <>
vector<byte> compress_block(span<byte> data, int compression_level) {
    return ::compress_block(data, compression_level);
}

template <>
vector<byte> compress_block(span<const byte> data, int compression_level) {
    return ::compress_block(data, compression_level);
}

template <>
vector<byte> decompress_block(span<byte> data, size_t output_size) {
    return ::decompress_block(data, output_size);
}

template <>
vector<byte> decompress_block(span<const byte> data, size_t output_size) {
    return ::decompress_block(data, output_size);
}
}

namespace util
{
vector<byte> decompress(span<const byte> data, const function<void(u64, u64)>& progress) {
    return ::compress<true>(data, -1, progress);
}

vector<byte>
compress(span<const byte> data, int compression_level, const function<void(u64, u64)>& progress) {
    return ::compress<false>(data, compression_level, progress);
}

} // namespace util
