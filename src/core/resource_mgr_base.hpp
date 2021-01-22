#pragma once

#include "types.hpp"
#include "config_manager.hpp"
#include "global_storage.hpp"
#include "async.hpp"
#include "assert.hpp"

namespace core {

enum class load_significance_t : u32 {
    low = 0, medium, high
};

struct resource_path_t {
    PE_SERIALIZE(mgr_tag, path, load_significance)

    string              mgr_tag;
    cfg_path            path;
    load_significance_t load_significance;
};

template <typename MgrT>
auto mgr_lookup(const string& mgr_tag) {
    auto mgr = MgrT::mgr_lookup_t::instance().get(mgr_tag).lock();
    if (!mgr)
        throw std::runtime_error("Can't find resource manager at tag '" + mgr_tag + "'");
    return mgr;
}

template <typename MgrT>
class resource_provider_t {
public:
    void serialize(vector<byte>& s) const {
        serialize_all(s, _storage->mgr_tag(), path(), load_significance());
    }

    void deserialize(span<const byte>& d) {
        string              mgr_tag;
        cfg_path            filepath;
        load_significance_t load_sign;
        deserialize_all(d, mgr_tag, filepath, load_sign);

        *this = resource_provider_t(mgr_lookup<MgrT>(mgr_tag), filepath, load_sign);
    }

    resource_provider_t() = default;
    resource_provider_t(shared_ptr<MgrT>    mgr,
                        const cfg_path&     file_path,
                        load_significance_t load_significance = load_significance_t::medium):
        _storage(move(mgr)) {
        _resource_id = _storage->load_id(file_path, load_significance);
    }

    resource_provider_t(const resource_path_t& resource_path):
        resource_provider_t(mgr_lookup<MgrT>(resource_path.mgr_tag),
                            resource_path.path,
                            resource_path.load_significance) {}

    ~resource_provider_t() {
        if (_resource_id != numlim<u64>::max()) {
            PeAssert(_storage);
            try {
                _storage->decrement_usages(_resource_id);
            }
            catch (const std::exception& e) {
                auto trace = core_details::try_get_stacktrace_str(e);
                LOG_ERROR(
                    "resource_provider_t: exception in dtor: {}{}\n", e.what(), trace ? *trace : "");

                std::terminate();
            }
        }
    }

    resource_provider_t(const resource_provider_t& resource):
        _storage(resource._storage), _resource_id(resource._resource_id) {
        _storage->increment_usages(_resource_id);
    }

    resource_provider_t& operator=(const resource_provider_t& resource) {
        _storage     = resource._storage;
        _resource_id = resource._resource_id;
        _storage->increment_usages(_resource_id);

        return *this;
    }

    resource_provider_t(resource_provider_t&& resource) noexcept:
        _storage(move(resource._storage)), _resource_id(resource._resource_id) {
        resource._resource_id = numlim<u64>::max();
    }

    resource_provider_t& operator=(resource_provider_t&& resource) noexcept {
        _storage     = move(resource._storage);
        _resource_id = resource._resource_id;

        resource._resource_id = numlim<u64>::max();

        return *this;
    }

    [[nodiscard]]
    resource_path_t to_resource_path() const {
        return {_storage->mgr_tag(), path(), load_significance()};
    }

    void load_significance(load_significance_t load_significance) {
        _storage->load_significance(_resource_id, load_significance);
    }

    [[nodiscard]]
    load_significance_t load_significance() const {
        return _storage->load_significance(_resource_id);
    }

    [[nodiscard]]
    u32 usages() const {
        return _storage->usages(_resource_id);
    }

    [[nodiscard]]
    const cfg_path& path() const {
        return _storage->file_path(_resource_id);
    }

    [[nodiscard]]
    auto try_access() const {
        return _storage->try_access(_resource_id);
    }

private:
    shared_ptr<MgrT> _storage;
    u64              _resource_id = numlim<u64>::max();
};

template <typename CachedT, typename T, typename DerivedT, typename ProviderT>
class resource_mgr_base : public std::enable_shared_from_this<DerivedT> {
public:
    using resource_id_t = u64;
    using mgr_lookup_t  = global_storage<string, weak_ptr<DerivedT>>;

    struct resource_val_t {
        optional<CachedT> cached;
        optional<T>       value;
    };

    struct resource_spec_t {
        cfg_path            path;
        u32                 usages;
        load_significance_t load_significance;
    };

    static shared_ptr<DerivedT> create_shared(const string& mgr_tag) {
        auto ptr = make_shared<DerivedT>(constructor_accessor<resource_mgr_base>{}, mgr_tag);
        mgr_lookup_t::instance().insert(mgr_tag, ptr);
        return ptr;
    }

    resource_id_t load_id(const cfg_path& path, load_significance_t load_significance = load_significance_t::medium) {
        using namespace core;

        auto [position, was_inserted] = _path_to_id.emplace(path.absolute(), 0);
        if (!was_inserted) {
            auto id = position->second;
            increment_usages(id);

            auto& spec = _specs[position->second];
            spec.load_significance = load_significance;
            return position->second;
        }

        auto id = new_id();
        position->second = id;
        _specs[position->second] = resource_spec_t{path, 1, load_significance};
        _futures.emplace(id, static_cast<DerivedT*>(this)->load_async_cached(path));
        DLOG("resource_mgr[{}]: create resource: path = {} id = {} usages = {}",
             _mgr_tag,
             path,
             id,
             1);

        return id;
    }

    ProviderT load(const cfg_path& path, load_significance_t load_significance = load_significance_t::medium) {
        return ProviderT(this->shared_from_this(), path, load_significance);
    }

    void increment_usages(resource_id_t id) {
        auto found_spec = _specs.find(id);

        if (found_spec == _specs.end())
            throw std::runtime_error("Texture with specified id was not found");

        auto& spec = found_spec->second;
        ++spec.usages;
        DLOG("resource_mgr[{}]: increment usages: path = {} usages = {} -> {}",
             _mgr_tag,
             spec.path,
             spec.usages - 1,
             spec.usages);

        if (spec.usages == 1) {
            auto& resource = _resources.at(id);
            if (!resource.value) {
                if (resource.cached) {
                    DLOG("resource_mgr[{}]: resource {} will be load from cache",
                         _mgr_tag,
                         spec.path);
                    resource.value = DerivedT::from_cache(move(*resource.cached));
                    resource.cached.reset();
                }
                else {
                    DLOG("resource_mgr[{}]: resource {} will be load from file",
                         _mgr_tag,
                         spec.path);
                    auto found_spec = _specs.find(id);
                    PeAssert(found_spec != _specs.end());
                    _futures.emplace(
                        id,
                        static_cast<DerivedT*>(this)->load_async_cached(found_spec->second.path));
                }
            }
        }
    }

    void decrement_usages(resource_id_t id) {
        auto found_spec = _specs.find(id);

        if (found_spec == _specs.end())
            throw std::runtime_error("Texture with specified id was not found");

        auto& spec = found_spec->second;
        if (spec.usages == 0)
            return;

        DLOG("resource_mgr[{}]: decrement usages: path = {} usages = {} -> {}",
             _mgr_tag,
             spec.path,
             spec.usages,
             spec.usages - 1);
        if (spec.usages == 1) {
            auto found_resource = _resources.find(id);

            /* Resource may be on loading */
            auto future_pos = _futures.find(id);
            if (found_resource ==_resources.end() && future_pos != _futures.end()) {
                auto scope_exit = scope_guard{[&]() {
                    _futures.erase(future_pos);
                }};
                resource_val_t resource;
                resource.cached = move(future_pos->second.get());

                auto [position, _] = _resources.insert_or_assign(id, move(resource));
                found_resource = position;
            }

            if (found_resource == _resources.end()) {
                LOG_WARNING("resource_mgr[{}]: resource {} was destroyed by something or not loaded yet", _mgr_tag, spec.path);
                return;
            }

            auto& resource = found_resource->second;

            if (spec.load_significance == load_significance_t::medium) {
                DLOG("resource_mgr[{}]: resource {} will be cached", _mgr_tag, spec.path);
                if (resource.value) {
                    resource.cached = DerivedT::to_cache(move(resource.value.value()));
                    resource.value.reset();
                }
                else {
                    PeRequire(resource.cached);
                }
                resource.value.reset();
            }
            else if (spec.load_significance == load_significance_t::low) {
                DLOG("resource_mgr[{}]: resource {} will be unloaded", _mgr_tag, spec.path);
                resource.value.reset();
            }
        }

        --spec.usages;
    }

    T* try_access(resource_id_t id, bool wait = false) {
        auto found_resource = _resources.find(id);
        if (found_resource != _resources.end() && found_resource->second.value)
            return &found_resource->second.value.value();

        auto found_spec = _specs.find(id);
        if (found_spec == _specs.end()) {
            DLOG("resource_mgr[{}]: can't find spec for resource with id = {}", _mgr_tag, id);
            return nullptr;
        }
        auto& spec = found_spec->second;
        if (spec.usages == 0) {
            DLOG("resource_mgr[{}]: with id = {} has no usages", _mgr_tag, id);
            return nullptr;
        }

        auto found_future = _futures.find(id);
        if (found_future == _futures.end()) {
            DLOG("resource_mgr[{}]: can't find future for resource with id = {}", _mgr_tag, id);
            return nullptr;
        }

        auto& future = found_future->second;

        if (wait || future / is_ready()) {
            DLOG("resource_mgr[{}]: resource {} ready", _mgr_tag, spec.path);
            auto scope_exit = scope_guard{[&]() {
                _futures.erase(found_future);
            }};

            resource_val_t resource;
            resource.value = DerivedT::from_cache(move(future.get()));

            auto [position, _] = _resources.insert_or_assign(id, move(resource));
            if (position->second.value)
                return &(*position->second.value);
        }

        DLOG("resource_mgr[{}]: try access resource with id = {} but it is not ready now",
             _mgr_tag,
             id);
        return nullptr;
    }

    T& access(resource_id_t id) {
        auto resource_ptr = try_access(id, true);
        if (resource_ptr == nullptr)
            throw std::runtime_error("Resource with specified id was not found");
        return *resource_ptr;
    }

    [[nodiscard]]
    load_significance_t load_significance(resource_id_t id) const {
        return _specs.at(id).load_significance;
    }

    void load_significance(resource_id_t id, load_significance_t load_significance) {
        _specs.at(id).load_significance = load_significance;
    }

    [[nodiscard]]
    const cfg_path& file_path(resource_id_t id) const {
        return _specs.at(id).path;
    }

    [[nodiscard]]
    u32 usages(resource_id_t id) const {
        return _specs.at(id).usages;
    }

    resource_mgr_base(typename constructor_accessor<resource_mgr_base>::cref, const string& imgr_tag) {
        _mgr_tag = imgr_tag;
    }

    ~resource_mgr_base() noexcept {
        mgr_lookup_t::instance().remove(_mgr_tag);
    }

    resource_mgr_base(resource_mgr_base&&) noexcept = default;
    resource_mgr_base& operator=(resource_mgr_base&&) noexcept = default;
    resource_mgr_base(const resource_mgr_base&) noexcept = delete;
    resource_mgr_base& operator=(const resource_mgr_base&) noexcept = delete;

    [[nodiscard]]
    const string& mgr_tag() const {
        return _mgr_tag;
    }

private:
    resource_id_t new_id() {
        return _last_id++;
    }

private:
    hash_map<resource_id_t, job_future<CachedT>> _futures;
    hash_map<resource_id_t, resource_val_t>      _resources;
    hash_map<resource_id_t, resource_spec_t>     _specs;
    hash_map<string, resource_id_t>              _path_to_id;
    string                                       _mgr_tag;
    resource_id_t                                _last_id = 0;
};
}
