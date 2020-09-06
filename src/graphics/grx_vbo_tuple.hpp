#pragma once

#include <core/types.hpp>
#include <core/vec.hpp>
#include <glm/mat4x4.hpp>
#include <type_traits>

#include "grx_types.hpp"
#include <core/assert.hpp>

#define BONES_PER_VERTEX 4

namespace grx
{
using win_vao_map_t = core::hash_map<void*, uint>;

using vbo_vector_vec2f = core::vector<core::vec2f>;
template <typename T>
concept VboVectorVec2f = std::is_same_v<T, vbo_vector_vec2f>;

using vbo_vector_vec3f = core::vector<core::vec3f>;
template <typename T>
concept VboVectorVec3f = std::is_same_v<T, vbo_vector_vec3f>;

using vbo_vector_indices = core::vector<uint>;
template <typename T>
concept VboVectorIndices = std::is_same_v<T, vbo_vector_indices>;

using vbo_vector_matrix4 = core::vector<glm::mat4>;
template <typename T>
concept VboVectorMatrix4 = std::is_same_v<T, vbo_vector_matrix4>;

template <size_t S>
struct vbo_bone_data_tmpl {
    using vbo_bone_data_check = void;

    vbo_bone_data_tmpl() {
        std::fill(ids.begin(), ids.end(), 0);
        std::fill(weights.begin(), weights.end(), 0.f);
    }

    [[nodiscard]]
    static constexpr size_t size() {
        return S;
    }

    void append(uint bone_id, float weight) {
        float zero = 0.0f;
        for (size_t i = 0; i < size(); ++i) {
            if (!memcmp(&weights[i], &zero, sizeof(float))) {
                ids[i]     = bone_id;
                weights[i] = weight;
                return;
            }
        }

        RABORTF("Not enough bone size. Weights: {}", weights);
    }

    core::array<uint, S>  ids;
    core::array<float, S> weights;
};

using vbo_vector_bone = core::vector<vbo_bone_data_tmpl<BONES_PER_VERTEX>>;

template <typename T>
concept VboVectorBone = std::is_same_v<T, vbo_vector_bone>;

template <typename T>
concept VboData =
    VboVectorVec2f<T> || VboVectorVec3f<T> || VboVectorIndices<T> || VboVectorMatrix4<T> || VboVectorBone<T>;

void _grx_gen_vao_and_vbos(win_vao_map_t& vao_map, uint* vbos_ptr, size_t vbos_size);
void _grx_delete_vao_and_vbos(win_vao_map_t& vao_map, uint* vbos_ptr, size_t vbos_size);

bool _grx_bind_vao(win_vao_map_t& vao_map);
void _grx_bind_vbo(uint target, uint vbo_id);

void _grx_rebind_vector_vec2f_vbo(uint vbo_id, uint location);
void _grx_set_data_vector_vec2f_vbo(uint vbo_id, uint location, const vbo_vector_vec2f& data);

void _grx_rebind_vector_vec3f_vbo(uint vbo_id, uint location);
void _grx_set_data_vector_vec3f_vbo(uint vbo_id, uint location, const vbo_vector_vec3f& data);

void _grx_rebind_vector_indices_ebo(uint vbo_id);
void _grx_set_data_vector_indices_ebo(uint vbo_id, const vbo_vector_indices& data);

void _grx_set_data_vector_matrix_vbo(uint vbo_id, const glm::mat4* matrices, size_t size);

void _grx_rebind_vector_bone_vbo(uint vbo_id, uint location, size_t bone_per_vertex);
void _grx_set_data_vector_bone_vbo(uint vbo_id, uint location, const void* data, size_t bone_per_vertex, size_t size);

void _grx_setup_matrix_vbo(uint vbo_id, uint location);
void _grx_draw_elements_base_vertex(size_t indices_count, size_t start_indices_pos, size_t start_vertex_pos);
void _grx_draw_arrays(size_t vertex_count, size_t start_vertex_pos);

void _grx_draw_elements_instanced_base_vertex(size_t instances_count,
                                              size_t indices_count,
                                              size_t start_indices_pos,
                                              size_t start_vertex_pos);

vbo_vector_vec2f   _get_vector_vec2f  (uint vbo_id, uint size);
vbo_vector_vec3f   _get_vector_vec3f  (uint vbo_id, uint size);
vbo_vector_indices _get_vector_indices(uint vbo_id, uint size);
vbo_vector_bone    _get_vector_bone   (uint vbo_id, uint size);
vbo_vector_matrix4 _get_vector_matrix4(uint vbo_id, uint size);

template <VboData... Ts>
class grx_vbo_tuple {
protected:
    friend class grx_window;
    template <typename>
    friend class grx_postprocess_mgr;
    grx_vbo_tuple([[maybe_unused]] void* no_raii) {}

    template <size_t Idx>
    void _rebind_buffer() const {
        auto id     = _vbo_ids[Idx];
        auto loc    = location<Idx>();
        using DataT = std::tuple_element_t<Idx, std::tuple<Ts...>>;

        if constexpr (VboVectorVec2f<DataT>)
            _grx_rebind_vector_vec2f_vbo(id, loc);
        else if constexpr (VboVectorVec3f<DataT>)
            _grx_rebind_vector_vec3f_vbo(id, loc);
        else if constexpr (VboVectorIndices<DataT>)
            _grx_rebind_vector_indices_ebo(id);
        else if constexpr (VboVectorBone<DataT>)
            _grx_rebind_vector_bone_vbo(id, loc, BONES_PER_VERTEX);
    }

    template <size_t... Idx>
    void _rebind_buffers(std::index_sequence<Idx...>&&) const {
        ((_rebind_buffer<Idx>()), ...);
    }

    void _rebind_buffers() const {
        _rebind_buffers(std::make_index_sequence<sizeof...(Ts)>());
    }

    template <size_t... Idxs>
    void _set_data(const core::tuple<Ts...>& data, std::index_sequence<Idxs...>) {
        (set_data<Idxs>(std::get<Idxs>(data)), ...);
    }

public:
    template <size_t S>
    using vbo_type = std::tuple_element_t<S, std::tuple<Ts...>>;

    [[nodiscard]]
    static constexpr size_t count() {
        return sizeof...(Ts);
    }

    template <size_t N, size_t I = 0, uint Location = 0>
    static constexpr uint _location_iter() {
        if constexpr (I == N)
            return Location;
        else {
            if constexpr (VboVectorIndices<std::tuple_element_t<I, std::tuple<Ts...>>>)
                return _location_iter<N, I + 1, Location>();
            else if constexpr (VboVectorMatrix4<std::tuple_element_t<I, std::tuple<Ts...>>>)
                return _location_iter<N, I + 1, Location + 4>();
            else if constexpr (VboVectorBone<std::tuple_element_t<I, std::tuple<Ts...>>>)
                return _location_iter<N, I + 1, Location + 2>();
            else
                return _location_iter<N, I + 1, Location + 1>();
        }
    }

    template <size_t I>
    static constexpr uint location() {
        return _location_iter<I>();
    }

    template <size_t I = 0>
    constexpr void _setup_matrices_iter() const {
        if constexpr (I < sizeof...(Ts)) {
            if constexpr (VboVectorMatrix4<std::tuple_element_t<I, std::tuple<Ts...>>>)
                _grx_setup_matrix_vbo(_vbo_ids[I], location<I>());
            _setup_matrices_iter<I + 1>();
        }
    }

    template <VboData T, size_t C = 0, size_t I = 0>
    //template <VboData T, size_t... Idxs>
    static constexpr size_t _vbo_type_count_iter() {
        //return ((std::is_same_v<std::tuple_element_t<Idxs, std::tuple<Ts...>>> ? 1U : 0U) + ...);
        if constexpr (I < sizeof...(Ts)) {
            if constexpr (std::is_same_v<T, std::tuple_element_t<I, std::tuple<Ts...>>>)
                return _vbo_type_count_iter<T, C + 1, I + 1>();
            else
                return _vbo_type_count_iter<T, C, I + 1>();
        }
        return C;
    }

    template <VboData T>
    static constexpr size_t vbo_type_count() {
        return _vbo_type_count_iter<T>();
    }

    static constexpr bool has_indices_vbo() {
        return vbo_type_count<vbo_vector_indices>();
    }

    template <VboData T, size_t I = 0>
    static constexpr size_t first_type_idx() {
        if constexpr (I < sizeof...(Ts)) {
            if constexpr (std::is_same_v<T, std::tuple_element_t<I, std::tuple<Ts...>>>)
                return I;
            else
                return first_type_idx<T, I + 1>();
        }
        return core::numlim<size_t>::max();
    }

    template <VboData T, size_t ArrayPos = 0, size_t Idx = 0, size_t ArraySize>
    static constexpr void _vbo_type_positions_iter(std::array<size_t, ArraySize>& positions) {
        if constexpr (Idx < sizeof...(Ts) && ArrayPos < ArraySize) {
            if constexpr (std::is_same_v<T, std::tuple_element_t<Idx, std::tuple<Ts...>>>) {
                positions[ArrayPos] = Idx;
                _vbo_type_positions_iter<T, ArrayPos + 1, Idx + 1, ArraySize>(positions);
            }
            else {
                _vbo_type_positions_iter<T, ArrayPos, Idx + 1, ArraySize>(positions);
            }
        }
    }

    template <VboData T>
    static constexpr auto vbo_type_positions() {
        constexpr auto            count  = vbo_type_count<T>();
        std::array<size_t, count> result = {0};
        _vbo_type_positions_iter<T>(result);

        return result;
    }

    grx_vbo_tuple() {
        _grx_gen_vao_and_vbos(_win_vao_map, _vbo_ids.data(), _vbo_ids.size());
        _setup_matrices_iter();
    }

    grx_vbo_tuple(grx_vbo_tuple&& vbo_tuple) noexcept = default;
    grx_vbo_tuple& operator=(grx_vbo_tuple&& vbo_tuple) noexcept = default;

    ~grx_vbo_tuple() {
        // if (_vao_id != std::numeric_limits<uint>::max())
        if (!_win_vao_map.empty())
            _grx_delete_vao_and_vbos(_win_vao_map, _vbo_ids.data(), _vbo_ids.size());
    }

    void set_data(const core::tuple<Ts...>& data) {
        _set_data(data, std::make_index_sequence<sizeof...(Ts)>());
    }

    template <size_t I>
    void set_data(const std::tuple_element_t<I, std::tuple<Ts...>>& vbo_data) {
        using DataT = std::tuple_element_t<I, std::tuple<Ts...>>;
        static_assert(VboVectorVec2f<DataT> || VboVectorVec3f<DataT> || VboVectorIndices<DataT> ||
                      VboVectorMatrix4<DataT> || VboVectorBone<DataT>);

        auto                            id  = _vbo_ids[I];
        [[maybe_unused]] constexpr auto loc = location<I>();

        if constexpr (VboVectorVec2f<DataT>)
            _grx_set_data_vector_vec2f_vbo(id, loc, vbo_data);
        else if constexpr (VboVectorVec3f<DataT>)
            _grx_set_data_vector_vec3f_vbo(id, loc, vbo_data);
        else if constexpr (VboVectorIndices<DataT>)
            _grx_set_data_vector_indices_ebo(id, vbo_data);
        else if constexpr (VboVectorMatrix4<DataT>)
            _grx_set_data_vector_matrix_vbo(id, vbo_data.data(), vbo_data.size());
        else if constexpr (VboVectorBone<DataT>)
            _grx_set_data_vector_bone_vbo(id, loc, vbo_data.data(), vbo_data[0].size(), vbo_data.size());

        _vbo_sizes[I] = static_cast<uint>(vbo_data.size());
    }

    template <size_t I>
    auto get_data() const -> std::tuple_element_t<I, std::tuple<Ts...>> {
        using DataT = std::tuple_element_t<I, std::tuple<Ts...>>;
        static_assert(VboVectorVec2f<DataT> || VboVectorVec3f<DataT> || VboVectorIndices<DataT> ||
                      VboVectorMatrix4<DataT> || VboVectorBone<DataT>);

        uint id   = _vbo_ids[I];
        uint size = _vbo_sizes[I];

        if constexpr (VboVectorVec2f<DataT>)
            return _get_vector_vec2f(id, size);
        else if constexpr (VboVectorVec3f<DataT>)
            return _get_vector_vec3f(id, size);
        else if constexpr (VboVectorIndices<DataT>)
            return _get_vector_indices(id, size);
        else if constexpr (VboVectorBone<DataT>)
            return _get_vector_bone(id, size);
        else if constexpr (VboVectorMatrix4<DataT>)
            return _get_vector_matrix4(id, size);
    }

    template <size_t I>
    size_t get_size() const {
        return std::get<I>(_vbo_sizes);
    }

    void bind_vao() const {
        if (_grx_bind_vao(_win_vao_map)) {
            _setup_matrices_iter();
            _rebind_buffers();
        }
    }

    template <size_t I>
    void bind_vbo(uint gl_target) const {
        _grx_bind_vbo(gl_target, std::get<I>(_vbo_ids));
    }

    void bind_vbo(size_t pos, uint gl_target) const {
        _grx_bind_vbo(gl_target, _vbo_ids.at(pos));
    }

    core::optional<size_t> indices_count() const {
        if (has_indices_vbo())
            return std::get<first_type_idx<vbo_vector_indices>()>(_vbo_sizes);
        else
            return {};
    }

    void draw(size_t indices_count, size_t start_vertex_pos = 0, [[maybe_unused]] size_t start_index_pos = 0) const {
        if constexpr (has_indices_vbo())
            _grx_draw_elements_base_vertex(indices_count, start_index_pos, start_vertex_pos);
        else
            _grx_draw_arrays(indices_count, start_vertex_pos);
    }

    void draw_instanced(size_t                  instances_count,
                        size_t                  indices_count,
                        size_t                  start_vertex_pos = 0,
                        [[maybe_unused]] size_t start_index_pos  = 0) const {
        _grx_draw_elements_instanced_base_vertex(instances_count, indices_count, start_index_pos, start_vertex_pos);
    }

private:
    core::array<uint, count()> _vbo_ids;
    core::array<uint, count()> _vbo_sizes = {0};
    mutable win_vao_map_t      _win_vao_map;
};

class grx_vbo_tuple_generic {
private:
    struct holder_base {
        virtual ~holder_base()                                                                         = default;
        virtual void bind_vao() const                                                                  = 0;
        virtual void bind_vbo(size_t pos, uint gl_target) const                                        = 0;
        virtual void draw(size_t indices_count, size_t start_vertex_pos, size_t start_index_pos) const = 0;
        virtual void draw_instanced(size_t instances_count,
                                    size_t indices_count,
                                    size_t start_vertex_pos,
                                    size_t start_index_pos) const                                      = 0;
    };

    template <typename T>
    struct holder : public holder_base {
        explicit holder(T&& v): val(std::forward<T>(v)) {}

        void bind_vao() const override {
            val.bind_vao();
        }

        void bind_vbo(size_t pos, uint gl_target) const override {
            val.bind_vbo(pos, gl_target);
        }

        void draw(size_t indices_count, size_t start_vertex_pos, size_t start_index_pos) const override {
            val.draw(indices_count, start_vertex_pos, start_index_pos);
        }

        void draw_instanced(size_t instances_count,
                            size_t indices_count,
                            size_t start_vertex_pos,
                            size_t start_index_pos) const override {
            val.draw_instanced(instances_count, indices_count, start_vertex_pos, start_index_pos);
        }

        T val;
    };

private:
    core::unique_ptr<holder_base> _holder;

public:
    grx_vbo_tuple_generic(grx_vbo_tuple_generic&& v) noexcept: _holder(std::move(v._holder)) {}

    template <typename... Ts>
    explicit grx_vbo_tuple_generic(grx_vbo_tuple<Ts...>&& vbo_tuple):
        _holder(core::make_unique<holder<grx_vbo_tuple<Ts...>>>(std::move(vbo_tuple))) {}

    inline void bind_vao() const {
        _holder->bind_vao();
    }

    inline void bind_vbo(size_t pos, uint gl_target) const {
        _holder->bind_vbo(pos, gl_target);
    }

    inline void draw(size_t indices_count, size_t start_vertex_pos = 0, size_t start_index_pos = 0) const {
        _holder->draw(indices_count, start_vertex_pos, start_index_pos);
    }

    inline void draw_instanced(size_t instances_count,
                               size_t indices_count,
                               size_t start_vertex_pos,
                               size_t start_index_pos) const {
        _holder->draw_instanced(instances_count, indices_count, start_vertex_pos, start_index_pos);
    }

    template <typename T>
    T* try_cast() {
        auto res = dynamic_cast<holder<T>*>(_holder.get());
        return res ? &res->val : nullptr;
    }

    template <typename T>
    const T* try_cast() const {
        auto res = dynamic_cast<holder<T>*>(_holder.get());
        return res ? &res->val : nullptr;
    }

    template <typename T>
    const T& cast() const {
        const T* v = try_cast<T>();
        if (!v)
            throw std::runtime_error("Invalid cast (Maybe you try draw_instanced on not instanced mesh?)");
        return *v;
    }

    template <typename T>
    T& cast() {
        T* v = try_cast<T>();
        if (!v)
            throw std::runtime_error("Invalid cast (Maybe you try draw_instanced on not instanced mesh?)");
        return *v;
    }

    template <typename... Ts>
    bool try_set_data(const core::tuple<Ts...>& data) {
        if (auto v = cast<grx_vbo_tuple<Ts...>>()) {
            v->set_data(data);
            return true;
        }
        return false;
    }
};
} // namespace grx
