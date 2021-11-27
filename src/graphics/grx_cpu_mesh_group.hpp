#pragma once

#include <core/serialization.hpp>
#include <core/slice_range_view.hpp>

#include <core/files.hpp>
#include <core/async.hpp>
#include <core/string_hash.hpp>
#include <cstring>
#include "graphics/grx_utils.hpp"
#include "grx_skeleton.hpp"

class aiMesh;
class aiScene;

namespace grx {
using core::vector;
using core::vec3f;
using core::vec2f;

template <typename T>
concept MeshBuf =
    std::same_as<T, vector<vec3f>> || std::same_as<T, vector<vec2f>> ||
    std::same_as<T, vector<core::u32>> || std::same_as<T, vector<grx_bone_vertex_data>>;

enum class mesh_buf_tag : core::u32 {
    index = 0,
    position,
    normal,
    tangent,
    bitangent,
    uv,
    bone
};

template <mesh_buf_tag B, MeshBuf T>
struct mesh_buf_spec {
    static constexpr mesh_buf_tag tag = B;
    using type = T;
};

/* TODO: fix this */
template <typename T>
concept MeshBufT = true;

template <size_t I, MeshBufT... Ts>
constexpr mesh_buf_tag index_to_mesh_buf_tag =
    std::tuple_element_t<I, std::tuple<Ts...>>::tag;

template <size_t I, MeshBufT... Ts>
using index_to_mesh_buf_spec =
    typename std::tuple_element_t<I, std::tuple<Ts...>>::type;

namespace details {
    template <mesh_buf_tag tag, MeshBufT... Ts, size_t... Idxs>
    constexpr size_t mesh_buf_tag_to_index(std::index_sequence<Idxs...>&&) {
        return ((tag == Ts::tag ? Idxs : size_t(0)) + ...);
    }
}

template <mesh_buf_tag tag, MeshBufT... Ts>
constexpr bool has_mesh_buf_tag() {
    return false || ((Ts::tag == tag) || ...);
}

template <mesh_buf_tag tag, MeshBufT... Ts>
constexpr size_t mesh_buf_tag_to_index =
    details::mesh_buf_tag_to_index<tag, Ts...>(std::make_index_sequence<sizeof...(Ts)>());


namespace details {
    uint assimp_vertices_count (const aiMesh*);
    uint assimp_indices_count  (const aiMesh*);
    uint assimp_load_indices   (const aiMesh*, vector<core::u32>&            buff, uint start_index);
    uint assimp_load_positions (const aiMesh*, vector<vec3f>&                buff, uint start_index);
    uint assimp_load_uvs       (const aiMesh*, vector<vec2f>&                buff, uint start_index);
    uint assimp_load_normals   (const aiMesh*, vector<vec3f>&                buff, uint start_index);
    uint assimp_load_tangents  (const aiMesh*, vector<vec3f>&                buff, uint start_index);
    uint assimp_load_bitangents(const aiMesh*, vector<vec3f>&                buff, uint start_index);
    uint assimp_load_bones     (const aiMesh*, vector<grx_bone_vertex_data>& buff, uint start_index);
    grx_aabb assimp_get_aabb   (const aiMesh*);
    grx_skeleton_data assimp_load_skeleton_data_map(const aiScene* scene);
    core::span<aiMesh*> assimp_get_meshes(const aiScene*);
    uint assimp_get_material_index(const aiMesh*);

    const aiScene* assimp_load_scene(core::string_view dae_data);
    core::string assimp_get_error();
    void assimp_release_scene(const aiScene* scene);
}

/**
 * @brief Stores helper data of one mesh element
 */
struct grx_mesh_element { // NOLINT
    PE_SERIALIZE(indices_count,
                 vertices_count,
                 material_index,
                 start_vertex_pos,
                 start_index_pos,
                 aabb.max,
                 aabb.min)

    uint     indices_count = 0;
    uint     vertices_count = 0;
    uint     start_vertex_pos = 0;
    uint     start_index_pos = 0;
    grx_aabb aabb;
    uint     material_index = std::numeric_limits<uint>::max();

    [[nodiscard]]
    bool operator==(const grx_mesh_element& rhs) const {
        return !std::memcmp(this, &rhs, sizeof(grx_mesh_element));
    }

    [[nodiscard]]
    bool operator!=(const grx_mesh_element& rhs) const {
        return !(*this == rhs);
    }
};

/**
 * @brief Represents a mesh (or mesh group) with typed buffers
 *
 * Buffers of all mesh elements with same target type stores in
 * the one flat buffer and ready to be copied into video memory
 *
 * @tparam Ts - buffer types
 */
template <MeshBufT... Ts>
class grx_cpu_mesh_group {
public:
    using grx_cpu_mesh_group_tag = void;
    static constexpr size_t buffers_count = sizeof...(Ts);

    void serialize(core::vector<core::byte>& _s) const {
        core::u64 buf_count = sizeof...(Ts);
        core::serialize(buf_count, _s);

        auto write_tag = [&_s](mesh_buf_tag tag) {
            auto t = static_cast<core::u32>(tag);
            core::serialize(t, _s);
        };

        (write_tag(Ts::tag), ...);

        core::serialize_all(_s, _elements, _data);
    }

    void deserialize(core::span<const core::byte>& _d) {
        core::u64 buf_count; // NOLINT
        core::deserialize(buf_count, _d);

        if (buf_count != sizeof...(Ts))
            throw std::runtime_error(
                core::format("Mesh data contains {} buffers, but type requires {} buffers",
                             buf_count,
                             sizeof...(Ts)));

        auto check_tag = [&_d](mesh_buf_tag tag) {
            core::u32 t; // NOLINT
            core::deserialize(t, _d);

            if (static_cast<mesh_buf_tag>(t) != tag)
                throw std::runtime_error("Mesh buffers tag order differs with mesh data");
        };

        (check_tag(Ts::tag), ...);

        core::deserialize_all(_d, _elements, _data);
    }

    /**
     * @brief Loads mesh from assimp aiMesh
     *
     * @param mesh - assimp aiMesh
     *
     * @return loaded mesh
     */
    static grx_cpu_mesh_group
    from_assimp(const aiMesh* mesh) {
        auto m = grx_cpu_mesh_group{};
        m.push_from_assimp(mesh);
        return m;
    }

    /**
     * @brief Loads mesh all mesh group from assimp aiScene
     *
     * @param scene - assimp aiScene
     *
     * @return loaded mesh group
     */
    static grx_cpu_mesh_group
    from_assimp(const aiScene* scene) {
        auto m = grx_cpu_mesh_group{};
        for (auto mesh : details::assimp_get_meshes(scene))
            m.push_from_assimp(mesh);

        if constexpr (has_bone_buf()) {
            auto skeleton_data = details::assimp_load_skeleton_data_map(scene);
            m.map_bone_ids(skeleton_data);
        }

        return m;
    }

    template <size_t... Idxs>
    void set_from_assimp(const aiMesh* mesh, uint mesh_element, std::index_sequence<Idxs...>) {
        (set_from_assimp<index_to_mesh_buf_tag<Idxs, Ts...>>(mesh, mesh_element), ...);
    }

    /**
     * @brief Sets all element data from assimp
     *
     * @param mesh - assimp aiMesh
     * @param mesh_element - index of mesh element
     */
    void set_from_assimp(const aiMesh* mesh, uint mesh_element = 0) {
        set_from_assimp(mesh, mesh_element, std::make_index_sequence<sizeof...(Ts)>());
    }

    /**
     * @brief Sets data from assimp aiMesh
     *
     * If mesh_element equals to mesh elements count then
     * new mesh element will be inserted
     *
     * @tparam tag - buffer tag
     * @param mesh - assimp aiMesh
     * @param mesh_element - index of mesh element
     */
    template <mesh_buf_tag tag>
    void set_from_assimp(const aiMesh* mesh, uint mesh_element = 0) {
        constexpr size_t Idx = mesh_buf_tag_to_index<tag, Ts...>;
        auto& buff = std::get<Idx>(_data);

        if (_elements.size() == mesh_element) {
            auto new_element = grx_mesh_element{};

            if (_elements.size() > 0) {
                new_element.start_index_pos  = _elements.back().start_index_pos + _elements.back().indices_count;
                new_element.start_vertex_pos = _elements.back().start_vertex_pos + _elements.back().vertices_count;
            }
            _elements.emplace_back(new_element);
        }
        auto& element = _elements[mesh_element];

        uint start_index; // NOLINT
        if constexpr (tag == mesh_buf_tag::index) {
            start_index = element.start_index_pos;
            if (element.indices_count && mesh_element + 1 < _elements.size() &&
                element.indices_count != details::assimp_indices_count(mesh))
                throw std::runtime_error("Can't reset indices in mesh_element " + std::to_string(mesh_element) +
                        ": element has " +  std::to_string(element.indices_count) + " indices and it can not be resized");
        } else {
            start_index = element.start_vertex_pos;
            if (element.vertices_count && mesh_element + 1 < _elements.size() &&
                element.vertices_count != details::assimp_vertices_count(mesh))
                throw std::runtime_error("Can't reset vertices in mesh_element " + std::to_string(mesh_element) +
                        ": element has " +  std::to_string(element.vertices_count) + " vertices and it can not be resized");
        }

        using namespace details;

        element.material_index = assimp_get_material_index(mesh);

        uint count; // NOLINT

             if constexpr (tag == mesh_buf_tag::position)  count = assimp_load_positions (mesh, buff, start_index);
        else if constexpr (tag == mesh_buf_tag::normal)    count = assimp_load_normals   (mesh, buff, start_index);
        else if constexpr (tag == mesh_buf_tag::tangent)   count = assimp_load_tangents  (mesh, buff, start_index);
        else if constexpr (tag == mesh_buf_tag::bitangent) count = assimp_load_bitangents(mesh, buff, start_index);
        else if constexpr (tag == mesh_buf_tag::uv)        count = assimp_load_uvs       (mesh, buff, start_index);
        else if constexpr (tag == mesh_buf_tag::index)     count = assimp_load_indices   (mesh, buff, start_index);
        else if constexpr (tag == mesh_buf_tag::bone)      count = assimp_load_bones     (mesh, buff, start_index);

        element.aabb = assimp_get_aabb(mesh);

        if constexpr (tag == mesh_buf_tag::index) {
            if (element.indices_count == 0)
                element.indices_count = count;
        } else {
            if (element.vertices_count == 0)
                element.vertices_count = count;
        }
    }
    /**
     * @brief Push back data from assimp aiMesh
     *
     * If mesh_element equals to mesh elements count then
     * new mesh element will be inserted
     *
     * @tparam tag - tag of buffer
     * @param mesh - assimp aiMesh
     */

    template <mesh_buf_tag tag>
    void push_from_assimp(const aiMesh* mesh) {
        set_from_assimp<tag>(mesh, static_cast<uint>(_elements.size()));
    }

    /**
     * @brief Push all element data from assimp
     *
     * @param mesh - assimp aiMesh
     */
    void push_from_assimp(const aiMesh* mesh) {
        set_from_assimp(mesh, static_cast<uint>(_elements.size()));
    }

    /*
    template <size_t I, typename BuffType = std::tuple_element_t<I, std::tuple<Ts...>>>
    void set(const BuffType& buff) {
        std::get<I>(_data) = buff;
    }

    template <size_t I, typename BuffType = std::tuple_element_t<I, std::tuple<Ts...>>>
    void set(BuffType&& buff) {
        std::get<I>(_data) = std::move(buff);
    }

    template <size_t I, typename BuffType = std::tuple_element_t<I, std::tuple<Ts...>>>
    const BuffType& get() const {
        return std::get<I>(_data);
    }

    template <size_t I, typename BuffType = std::tuple_element_t<I, std::tuple<Ts...>>>
    BuffType& get() {
        return std::get<I>(_data);
    }
    */

    /**
     * @brief Sets data to specified buffer
     *
     * @tparam tag - the tag of the buffer
     * @param buff - the buffer
     */
    template <
        mesh_buf_tag tag,
        size_t BuffIdx = mesh_buf_tag_to_index<tag, Ts...>,
        typename BuffType = index_to_mesh_buf_spec<BuffIdx, Ts...>>
    void set(const BuffType& buff) {
        std::get<BuffIdx>(_data) = buff;
    }

    /**
     * @brief Sets data to specified buffer
     *
     * @tparam tag - the tag of the buffer
     * @param buff - the buffer
     */
    template <
        mesh_buf_tag tag,
        size_t BuffIdx = mesh_buf_tag_to_index<tag, Ts...>,
        typename BuffType = index_to_mesh_buf_spec<BuffIdx, Ts...>>
    void set(BuffType&& buff) {
        std::get<BuffIdx>(_data) = std::move(buff);
    }

    /**
     * @brief Sets data to specified buffer
     *
     * @tparam idx - the index of the buffer
     * @param buff - the buffer
     */
    template <
        size_t idx,
        typename BuffType = index_to_mesh_buf_spec<idx, Ts...>>
    void set(const BuffType& buff) {
        std::get<idx>(_data) = buff;
    }

    /**
     * @brief Sets data to specified buffer
     *
     * @tparam idx - the index of the buffer
     * @param buff - the buffer
     */
    template <
        size_t idx,
        typename BuffType = index_to_mesh_buf_spec<idx, Ts...>>
    void set(BuffType&& buff) {
        std::get<idx>(_data) = std::move(buff);
    }



    /**
     * @brief Gets data of buffer
     *
     * @tparam tag - the tag of the buffer
     *
     * @return result buffer
     */
    template <
        mesh_buf_tag tag,
        size_t BuffIdx = mesh_buf_tag_to_index<tag, Ts...>,
        typename BuffType = index_to_mesh_buf_spec<BuffIdx, Ts...>>
    const BuffType& get() const {
        return std::get<BuffIdx>(_data);
    }

    /**
     * @brief Gets data of buffer
     *
     * @tparam tag - the tag of the buffer
     *
     * @return result buffer
     */
    template <
        mesh_buf_tag tag,
        size_t BuffIdx = mesh_buf_tag_to_index<tag, Ts...>,
        typename BuffType = index_to_mesh_buf_spec<BuffIdx, Ts...>>
    BuffType& get() {
        return std::get<BuffIdx>(_data);
    }

    /**
     * @brief Gets count of mesh elements
     *
     * @return count of mesh elements
     */
    [[nodiscard]]
    size_t elements_count() const {
        return _elements.size();
    }

    template <mesh_buf_tag tag>
    static constexpr core::pair<uint, uint>
    get_drawable_count(const grx_mesh_element& e) {
        if constexpr (tag == mesh_buf_tag::index)
            return {e.start_index_pos, e.indices_count};
        else
            return {e.start_vertex_pos, e.vertices_count};
    }

    template <size_t... Idxs>
    static core::tuple<typename Ts::type...>
    _submesh_helper(const core::tuple<typename Ts::type...>& d,
                    const grx_mesh_element&                  e,
                    std::index_sequence<Idxs...>&&) {
        return core::tuple<typename Ts::type...>{
            (typename Ts::type(std::get<Idxs>(d).begin() + get_drawable_count<Ts::tag>(e).first,
                               std::get<Idxs>(d).begin() + get_drawable_count<Ts::tag>(e).first +
                                   get_drawable_count<Ts::tag>(e).second))...};
    }

    [[nodiscard]]
    grx_cpu_mesh_group<Ts...> submesh(uint element_index) const {
        grx_cpu_mesh_group<Ts...> mesh;
        auto element = _elements.at(element_index);
        mesh._data = _submesh_helper(_data, element, std::make_index_sequence<sizeof...(Ts)>());
        element.start_index_pos = 0;
        element.start_vertex_pos = 0;

        return mesh;
    }

    template <size_t... Idxs>
    static bool _tuple_buffers_equal(const core::tuple<typename Ts::type...>& a,
                                     const core::tuple<typename Ts::type...>& b,
                                     std::index_sequence<Idxs...>&&) {
        bool sizes_equal = ((std::get<Idxs>(a).size() == std::get<Idxs>(b).size()) && ...);
        if (!sizes_equal)
            return false;
        else
            return (!std::memcmp(std::get<Idxs>(a).data(), std::get<Idxs>(b).data(), std::get<Idxs>(a).size()) && ...);
    }

    [[nodiscard]]
    bool operator==(const grx_cpu_mesh_group& mesh) const {
        return _elements == _elements && _tuple_buffers_equal(_data, mesh._data, std::make_index_sequence<sizeof...(Ts)>());
    }

    [[nodiscard]]
    bool operator!=(const grx_cpu_mesh_group& mesh) const {
        return !(*this == mesh);
    }

    static constexpr bool has_bone_buf() {
        return has_mesh_buf_tag<mesh_buf_tag::bone, Ts...>();
    }

    template <bool C = has_bone_buf()>
    auto map_bone_ids(const grx_skeleton_data& skeleton_data) -> std::enable_if_t<C> {
        float zero = 0.f;

        core::hash_map<core::u32, core::string_view> hash_to_str;
        for (auto& [name, _] : skeleton_data.mapping) {
            auto [pos, was_inserted] =
                hash_to_str.emplace(core::hash_fnv1a32(name), core::string_view(name));
            if (!was_inserted)
                throw std::runtime_error("Bone name " + name + " has fnv1a32 hash collision with " +
                                         core::string(pos->second));
        }

        for (grx_bone_vertex_data& bone : get<mesh_buf_tag::bone>()) {
            for (size_t i = 0; i < MAX_BONES_PER_VERTEX; ++i)
                if (std::memcmp(&bone.weights[i], &zero, sizeof(zero)) != 0)
                    bone.ids[i] =
                        skeleton_data.mapping.at(core::string(hash_to_str.at(bone.ids[i])));
        }
    }

private:
    core::tuple<typename Ts::type...> _data;
    core::vector<grx_mesh_element>    _elements;

public:
    [[nodiscard]]
    const core::vector<grx_mesh_element>& elements() const {
        return _elements;
    }

    void elements(core::vector<grx_mesh_element> ielements) {
        _elements = core::move(ielements);
    }

    [[nodiscard]]
    const core::tuple<Ts...>& data() const {
        return _data;
    }

    [[nodiscard]]
    core::tuple<Ts...>& data() {
        return _data;
    }

    void data(const core::tuple<Ts...>& d) {
        _data = d;
    }

    void data(core::tuple<Ts...>&& d) {
        _data = core::move(d);
    }

    template <mesh_buf_tag... tags>
    [[nodiscard]]
    auto view() const {
        return core::zip_view(std::get<mesh_buf_tag_to_index<tags, Ts...>>(_data)...);
    }

    template <mesh_buf_tag... tags>
    [[nodiscard]]
    auto view() {
        return core::zip_view(std::get<mesh_buf_tag_to_index<tags, Ts...>>(_data)...);
    }
};

template <typename T>
concept CpuMeshGroup = requires { typename T::grx_cpu_mesh_group_tag; };

using grx_cpu_mesh_group_t = grx_cpu_mesh_group<
        mesh_buf_spec<mesh_buf_tag::index,     vector<core::u32>>,
        mesh_buf_spec<mesh_buf_tag::position,  vector<vec3f>>,
        mesh_buf_spec<mesh_buf_tag::uv,        vector<vec2f>>,
        mesh_buf_spec<mesh_buf_tag::normal,    vector<vec3f>>,
        mesh_buf_spec<mesh_buf_tag::tangent,   vector<vec3f>>,
        mesh_buf_spec<mesh_buf_tag::bitangent, vector<vec3f>>>;

using grx_cpu_boned_mesh_group_t = grx_cpu_mesh_group<
        mesh_buf_spec<mesh_buf_tag::index,     vector<core::u32>>,
        mesh_buf_spec<mesh_buf_tag::position,  vector<vec3f>>,
        mesh_buf_spec<mesh_buf_tag::uv,        vector<vec2f>>,
        mesh_buf_spec<mesh_buf_tag::normal,    vector<vec3f>>,
        mesh_buf_spec<mesh_buf_tag::tangent,   vector<vec3f>>,
        mesh_buf_spec<mesh_buf_tag::bitangent, vector<vec3f>>,
        mesh_buf_spec<mesh_buf_tag::bone,      vector<grx_bone_vertex_data>>>;

template <CpuMeshGroup T = grx_cpu_mesh_group_t>
[[nodiscard]]
inline core::try_opt<T>
try_load_mesh_group(const core::string& file_path) {
    T result;

    if (core::has_extension(file_path, ".dae")) {
        auto data = core::try_read_file(file_path);

        if (!data)
            return std::runtime_error("Can't load mesh at path '" + file_path + "'");

        auto str_data = grx_utils::collada_bake_bind_shape_matrix(*data);
        auto scene = details::assimp_load_scene(str_data);
        auto guard = core::scope_guard{[&](){ details::assimp_release_scene(scene); }};

        if (!scene)
            return std::runtime_error(details::assimp_get_error());

        result = T::from_assimp(scene);
    }
    else {
        auto data = core::try_read_binary_file(file_path);

        if (!data)
            return std::runtime_error("Can't load mesh at path '" + file_path + "'");

        try {
            auto ds = core::deserializer_view(*data);
            ds.read(result);
        } catch (const std::exception& e) {
            return std::runtime_error(e.what());
        }
    }

    return result;
}

template <CpuMeshGroup T = grx_cpu_mesh_group_t>
inline T load_mesh_group(const core::string& file_path) {
    return try_load_mesh_group<T>(file_path).value();
}

template <CpuMeshGroup T = grx_cpu_mesh_group_t>
[[nodiscard]]
inline core::job_future<core::try_opt<T>>
try_async_load_mesh_group(const core::string& file_path) {
    return core::submit_job([file_path] {
        return try_load_mesh_group<T>(file_path);
    });
}

template <CpuMeshGroup T = grx_cpu_mesh_group_t>
[[nodiscard]]
inline core::job_future<T>
async_load_mesh_group(const core::string& file_path) {
    return core::submit_job([file_path] {
        return load_mesh_group<T>(file_path);
    });
}

template <CpuMeshGroup T>
[[nodiscard]]
inline bool try_save_mesh_group(const core::string& file_path, const T& mesh_group) {
    core::serializer s;
    s.write(mesh_group);

    return core::try_write_file(file_path, s.data());
}

template <CpuMeshGroup T>
inline void save_mesh_group(const core::string& file_path, const T& mesh_group) {
    core::serializer s;
    s.write(mesh_group);

    core::write_file(file_path, s.data());
}

template <CpuMeshGroup T>
[[nodiscard]]
inline core::job_future<bool>
try_async_save_mesh_group(const core::string& file_path, const T& mesh_group) {
    return core::submit_job([file_path, mesh_group] {
        return try_save_mesh_group(file_path, mesh_group);
    });
}

template <CpuMeshGroup T>
inline void async_save_mesh_group(const core::string& file_path, const T& mesh_group) {
    return core::submit_job([file_path, mesh_group] {
        [[maybe_unused]] auto wtf = try_save_mesh_group(file_path, mesh_group);
    });
}

}
