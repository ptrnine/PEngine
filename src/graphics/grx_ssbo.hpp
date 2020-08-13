#include "grx_types.hpp"

namespace grx {

namespace ssbo_details {
    void ssbo_gen(uint* ssbo_name);
    void ssbo_delete(uint ssbo_name);
    void ssbo_bind(uint ssbo_name);
    //void ssbo_data(uint ssbo_name, uint size, const void* data);
    void ssbo_setup(uint ssbo_name, uint index, uint size, const void* data);
    void ssbo_retrieve(uint ssbo_name, uint size, void* data);

    template <typename T>
    concept FlatBuffer = requires (const T& v) { v.data(); };

    template <typename T>
    core::pair<uint, const void*>
    get_range(const T& data) {
        if constexpr (FlatBuffer<T>)
            return {static_cast<uint>(data.size() * sizeof(data.data()[0])), data.data()};
        else
            return {static_cast<uint>(sizeof(data)), &data};
    }

    template <typename T>
    core::pair<uint, void*>
    get_range_const(T& data) {
        if constexpr (FlatBuffer<T>)
            return {static_cast<uint>(data.size() * sizeof(data.data()[0])), data.data()};
        else
            return {static_cast<uint>(sizeof(data)), &data};
    }
}

template <typename T>
class grx_ssbo {
public:
    grx_ssbo() {
        ssbo_details::ssbo_gen(&ssbo_);
    }

    grx_ssbo(uint index, T data) {
        ssbo_details::ssbo_gen(&ssbo_);
        set(index, std::move(data));
    }

    ~grx_ssbo() {
        ssbo_details::ssbo_delete(ssbo_);
    }

    void bind() {
        ssbo_details::ssbo_bind(ssbo_);
    }

    void set(uint index, T data) {
        data_ = std::move(data);
        auto [size, ptr] = ssbo_details::get_range(data_);
        ssbo_details::ssbo_setup(ssbo_, index, size, ptr);
    }

    void retrieve() {
        auto [size, ptr] = ssbo_details::get_range_const(data_);
        ssbo_details::ssbo_retrieve(ssbo_, size, ptr);
    }

    T& get() {
        return data_;
    }

    const T& get() const {
        return data_;
    }

private:
    uint ssbo_;
    T    data_;
};

} // namespace grx
