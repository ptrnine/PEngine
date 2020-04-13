#pragma once

#include <glm/mat4x4.hpp>
#include <core/types.hpp>
#include <core/vec.hpp>
#include <type_traits>
#include "grx_types.hpp"

namespace grx {
    using vbo_vector_vec2f = core::vector<core::vec2f>;
    template <typename T>
    concept VboVectorVec2f = std::is_same_v<T, vbo_vector_vec2f>;

    using vbo_vector_vec3f = core::vector<core::vec3f>;
    template <typename T>
    concept VboVectorVec3f = std::is_same_v<T, vbo_vector_vec3f>;

    using vbo_vector_indices = core::vector<uint>;
    template <typename T>
    concept VboVectorIndices = std::is_same_v<T, vbo_vector_indices>;

    template <size_t S>
    using vbo_array_ids = core::array<uint, S>;
    template <typename T>
    concept VboArrayIds = core::StdArray<T> && std::is_same_v<typename T::value, uint>;

    template <size_t S>
    using vbo_array_weights = core::array<float, S>;
    template <typename T>
    concept VboArrayWeights = core::StdArray<T> && std::is_same_v<typename T::value, float>;

    using vbo_matrix4 = glm::mat4;
    template <typename T>
    concept VboMatrix4 = std::is_same_v<T, vbo_matrix4>;

    template <typename T>
    concept VboData = VboVectorVec2f<T> || VboVectorVec3f<T> || VboVectorIndices<T> ||
                      VboArrayIds<T> || VboArrayWeights<T> || VboMatrix4<T>;


    void _grx_gen_vao_and_vbos           (uint* vao, uint* vbos_ptr, size_t vbos_size);
    void _grx_delete_vao_and_vbos        (uint* vao, uint* vbos_ptr, size_t vbos_size);

    void _grx_bind_vao                   (uint vao_id);
    void _grx_bind_vbo                   (uint target, uint vbo_id);
    void _grx_set_data_vector_vec2f_vbo  (uint vbo_id, uint location, const vbo_vector_vec2f& data);
    void _grx_set_data_vector_vec3f_vbo  (uint vbo_id, uint location, const vbo_vector_vec3f& data);
    void _grx_set_data_vector_indices_vbo(uint vbo_id, const vbo_vector_indices& data);
    void _grx_set_data_array_ids_vbo     (uint vbo_id, uint location, vbo_array_ids<1>::value_type* data, size_t size);
    void _grx_set_data_array_weights_vbo (uint vbo_id, uint location, vbo_array_weights<1>::value_type* data, size_t size);
    void _grx_setup_matrix_vbo           (uint vbo_id, uint location);
    void _grx_draw_elements_base_vertex  (size_t indices_count, size_t start_indices_pos, size_t start_vertex_pos);
    void _grx_draw_arrays                (size_t vertex_count, size_t start_vertex_pos);

    template <VboData... Ts>
    class grx_vbo_tuple {
    protected:
        friend class grx_window;
        template <typename> friend class grx_postprocess_mgr;
        grx_vbo_tuple([[maybe_unused]] void* no_raii) {}

    public:
        using tuple_t = core::tuple<Ts...>;

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
                else if constexpr (VboMatrix4<std::tuple_element_t<I, std::tuple<Ts...>>>)
                    return _location_iter<N, I + 1, Location + 4>();
                else
                    return _location_iter<N, I + 1, Location + 1>();
            }
        }

        template <size_t I>
        static constexpr uint location() {
            return _location_iter<I>();
        }

        template <size_t I = 0>
        constexpr void _setup_matrices_iter() {
            if constexpr (I < sizeof...(Ts)) {
                if constexpr (VboMatrix4<std::tuple_element_t<I, std::tuple<Ts...>>>)
                    _grx_setup_matrix_vbo(_vbo_ids[I], location<I>());
                _setup_matrices_iter<I + 1>();
            }
        }

        template <VboData T, size_t C = 0, size_t I = 0>
        static constexpr size_t _vbo_type_count_iter() {
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
            return _vbo_type_count_iter<vbo_vector_indices>();
        }

        template <VboData T, size_t ArrayPos = 0, size_t Idx = 0, size_t ArraySize>
        static constexpr void _vbo_type_positions_iter(std::array<size_t, ArraySize>& positions) {
            if constexpr (Idx < sizeof...(Ts) && ArrayPos < ArraySize) {
                if constexpr (std::is_same_v<T, std::tuple_element_t<Idx, std::tuple<Ts...>>>) {
                    positions[ArrayPos] = Idx;
                    _vbo_type_positions_iter<T, ArrayPos + 1, Idx + 1, ArraySize>(positions);
                } else {
                    _vbo_type_positions_iter<T, ArrayPos, Idx + 1, ArraySize>(positions);
                }
            }
        }

        template <VboData T>
        static constexpr auto vbo_type_positions() {
            constexpr auto count = vbo_type_count<T>();
            std::array<size_t, count> result = {0};
            _vbo_type_positions_iter<T>(result);

            return result;
        }

        grx_vbo_tuple() {
            _grx_gen_vao_and_vbos(&_vao_id, _vbo_ids.data(), _vbo_ids.size());
        }

        grx_vbo_tuple(grx_vbo_tuple&& vbo_tuple) noexcept:
            _vbo_ids(vbo_tuple._vbo_ids),
            _vao_id (vbo_tuple._vao_id)
        {
            vbo_tuple._vao_id = std::numeric_limits<uint>::max();
        }

        grx_vbo_tuple& operator= (grx_vbo_tuple&& vbo_tuple) noexcept {
            _vbo_ids = vbo_tuple._vbo_ids;
            _vao_id  = vbo_tuple._vao_id;

            vbo_tuple._vao_id = std::numeric_limits<uint>::max();

            return *this;
        }

        ~grx_vbo_tuple() {
            if (_vao_id != std::numeric_limits<uint>::max())
                _grx_delete_vao_and_vbos(&_vao_id, _vbo_ids.data(), _vbo_ids.size());
        }

        template <size_t I>
        void set_data(const std::tuple_element_t<I, std::tuple<Ts...>>& vbo_data) {
            using DataT = std::tuple_element_t<I, std::tuple<Ts...>>;
            static_assert(VboVectorVec2f<DataT> || VboVectorVec3f<DataT> || VboVectorIndices<DataT> ||
                          VboArrayIds<DataT> || VboArrayWeights<DataT>);

            auto id = _vbo_ids[I];
            [[maybe_unused]] constexpr auto loc = location<I>();

            if constexpr (VboVectorVec2f<DataT>)
                _grx_set_data_vector_vec2f_vbo(id, loc, vbo_data);
            else if constexpr (VboVectorVec3f<DataT>)
                _grx_set_data_vector_vec3f_vbo(id, loc, vbo_data);
            else if constexpr (VboVectorIndices<DataT>)
                _grx_set_data_vector_indices_vbo(id, vbo_data);
            else if constexpr (VboArrayIds<DataT>)
                _grx_set_data_array_ids_vbo(id, loc, vbo_data.data(), vbo_data.size());
            else if constexpr (VboArrayWeights<DataT>)
                _grx_set_data_array_weights_vbo(id, loc, vbo_data.data(), vbo_data.size());
        }

        void bind_vao() {
            _grx_bind_vao(_vao_id);
        }

        template <size_t I>
        void bind_vbo(uint gl_target) {
            _grx_bind_vbo(gl_target, _vbo_ids[I]);
        }

        void draw(size_t indices_count, size_t start_vertex_pos = 0, [[maybe_unused]] size_t start_index_pos = 0) {
            if constexpr (has_indices_vbo())
                _grx_draw_elements_base_vertex(indices_count, start_index_pos, start_vertex_pos);
            else
                _grx_draw_arrays(indices_count, start_vertex_pos);
        }

    private:
        core::array<uint, count()> _vbo_ids;
        uint                       _vao_id = std::numeric_limits<uint>::max();
    };
}