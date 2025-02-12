#pragma once
#include "ecs.hpp"
#include "editor_windows/file_browser.hpp"
#include "engine.hpp"
#include "file.hpp"
#include "systems.hpp"
#include "systems/editor.hpp"
#include "tools/base64/base64.hpp"
#include "tools/mgmecs.hpp"

#if defined(ENABLE_EDITOR)
#include "imgui.h"
#endif

#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>


namespace mgm {
    class ResourceManager;


    class Resource {
        friend class ResourceManager;
        mutable std::mutex mutex{};

      public:
        /**
         * @brief Create a lock on the resource (tells the Resource Manager not to touch it until it is unlocked)
         *
         * @return std::unique_lock<std::mutex> A lock on the resource
         */
        std::unique_lock<std::mutex> lock_resource() const { return std::unique_lock{mutex}; }

        Resource() = default;

        /**
         * @brief Load this resource from raw data provided as bytes (has the lowest presidence, behind "load_from_text" and "load_from_file", so will not be called if one of them is implemented)
         *
         * @param bytes A vector containing the raw data
         *
         * @return true If loading was successful
         * @return false If loading was unsuccessful and the resource should be discarded
         */
        virtual bool load_from_bytes([[maybe_unused]] const std::vector<uint8_t>& bytes) { return false; }

        /**
         * @brief Load this resource from some text provided as a string (has presidence above "load_from_bytes", but "load_from_file" has presidence over this, so if "load_from_file" is implemented, this will not be called)
         *
         * @param text A string containing the text
         *
         * @return true If loading was successful
         * @return false If loading was unsuccessful and "load_from_bytes" should be tried next
         */
        virtual bool load_from_text([[maybe_unused]] const std::string& text) { return false; }

        /**
         * @brief Load this resource from the file at the given path (has presidence above "load_from_bytes" and "load_from_text", so if this is implemented, the others will not be called)
         *
         * @param file_path The path to the file that the resource should be loaded from
         *
         * @return true If loading was successful
         * @return false If loading was unsuccessful and "load_from_text" should be tried next
         */
        virtual bool load_from_file([[maybe_unused]] const Path& file_path) { return false; }


        /**
         * @brief Dump the contents of this resource into bytes (presidence rules same as "load_from_bytes")
         *
         * @return std::vector<uint8_t> A vector containing the raw data
         */
        virtual std::vector<uint8_t> save_to_bytes() const { return {}; }

        /**
         * @brief Dump the contents of this resource into some text (presidence rules same as "load_from_text")
         *
         * @return std::string A string containing the dumped text
         */
        virtual std::string save_to_text() const { return {}; }

        /**
         * @brief Dump the contents of this resource into a file at the given path (presidence rules same as "load_from_file")
         *
         * @param file_path The path to the file that the data should be dumped into
         *
         * @return true If saving was successful
         */
        virtual bool save_to_file([[maybe_unused]] const Path& file_path) const { return false; }


        virtual ~Resource() {}
    };


    struct ResourceContainer {
        Resource* resource = nullptr;
        std::string ident{};
        size_t type = 0;
        size_t refs = 1;
        bool from_file : 1 = false;
        bool probably_modified : 1 = false;
        bool loaded : 1 = false;
        bool has_no_original : 1 = true;

        ~ResourceContainer() {
            if (resource)
                delete resource;
        }
    };

    inline std::vector<std::function<void()>>& prepare_for_resource_type_serialization() {
        static std::vector<std::function<void()>> types{};
        return types;
    }

    template<typename T>
    class ResourceReference {
        friend class ResourceManager;

        ResourceContainer* container{};

        // If this resource is not associated with a file, the first reference becomes the original
        mutable bool is_original = false;

        ResourceReference(ResourceContainer* known_good_container)
            : container(known_good_container) {
            const auto lock = container->resource->lock_resource();
            ++container->refs;
        }

      public:
        ResourceReference() = default;

        ResourceReference(const std::string& identifier);

        ResourceReference(ResourceReference&&);
        ResourceReference(const ResourceReference& other);

        ResourceReference& operator=(ResourceReference&&);
        ResourceReference& operator=(const ResourceReference& other);

        /**
         * @brief Get the identifier of the resource this reference is pointing to, or an empty string if it's invalid
         */
        std::string identifier() const {
            if (container != nullptr)
                return container->ident;
            return "";
        }

        /**
         * @brief Get a mutable reference to the resource (use the const version unless you REALLY NEED to make changes to the resource)
         * You should also probably lock the resource if you're going to modify it
         */
        T& get_mutable();

        /**
         * @brief Get a const reference to the resource
         */
        const T& get() const;

        /**
         * @brief Stop referencing the resource that this reference is currently pointing to
         * if this is the last reference, the resource will be destroyed during the next frame
         */
        void invalidate();

        /**
         * @brief Check if this reference is pointing to a valid resource (or any resource at all)
         */
        bool valid() const;

        SerializedData<ResourceReference<T>> serialize() const;
        void deserialize(const SerializedData<ResourceReference<T>>& data);

#if defined(ENABLE_EDITOR)
        bool inspect();
#endif

        static bool enable_for_serialization(const std::string& identifier);

        ~ResourceReference();
    };


    class ResourceManager : public System {
        template<typename>
        friend class ResourceReference;

        std::unordered_map<std::string, ResourceContainer*> resources{};

        std::unordered_set<std::string> to_destroy{};

        struct ResourceTypeInfo {
            std::string ext{};
        };
        std::unordered_map<size_t, ResourceTypeInfo> resource_types{};

      public:
        ResourceManager();


        /**
         * @brief Asociate a resource type with a certain file extension
         *
         * @tparam T The resource type
         * @param extension The file extension without the '.' before it
         */
        template<typename T>
            requires std::is_base_of_v<Resource, T>
        void asociate_resource_with_file_extension(const std::string& extension) {
            resource_types[typeid(T).hash_code()].ext = extension;
        }

        /**
         * @brief Get the file extension asociated with the given resource type
         *
         * @tparam T The resource type
         * @return std::string The extension without the '.' before it
         */
        template<typename T>
            requires std::is_base_of_v<Resource, T>
        std::string get_resource_asociated_file_extension() const {
            const auto it = resource_types.find(typeid(T).hash_code());
            if (it == resource_types.end())
                return "";
            return it->second.ext;
        }

        /**
         * @brief Change the identifier of a resource with another
         *
         * @param identifier The identifier of the resource to rename
         * @param new_identifier The new identifier to use for the resource. If this is a valid path to a file, the resource will be associated with that file
         */
        void rename(const std::string& identifier, const std::string& new_identifier) {
            const auto it = resources.find(identifier);
            if (it == resources.end()) {
                Logging{"ResourceManager"}.error("No resource with the identifier \"", identifier, "\"");
                return;
            }

            if (resources.contains(new_identifier)) {
                Logging{"ResourceManager"}.error("Resource with identifier \"", new_identifier, "\" already exists");
                return;
            }

            const auto container = it->second;
            resources.erase(it);
            resources.emplace(new_identifier, container);

            container->from_file = MagmaEngine{}.file_io().exists(identifier);
        }

        /**
         * @brief Create a resource of the given type
         *
         * @tparam T The resource type
         * @param identifier The identifier of the new resource
         * @param args Arguments to forward to the constructor
         * @return ResourceReference<T> A shared pointer to the resource
         */
        template<typename T, typename... Ts>
            requires std::is_base_of_v<Resource, T> && std::is_constructible_v<T, Ts...>
        ResourceReference<T> create(const std::string& identifier, Ts&&... args) {
            resources.emplace(
                identifier,
                new ResourceContainer{
                    .resource = new T{std::forward<Ts>(args)...},
                    .ident = identifier,
                    .type = typeid(T).hash_code(),
                    .refs = 0
                }
            );
            ResourceReference<T> res{identifier};
            res.is_original = true;
            return res;
        }

        /**
         * @brief Get the resource with the given identifier
         *
         * @tparam T The resource type
         * @param identifier The identifier of the existing resource to get
         * @return ResourceReference<T> A shared pointer to the resource
         */
        template<typename T>
            requires std::is_base_of_v<Resource, T>
        ResourceReference<T> get(const std::string& identifier) {
            return ResourceReference<T>{identifier};
        }

        /**
         * @brief Get the resource with the given identifier, or create it if it doesn't exist yet forwarding the arguments to the constructor
         *
         * @tparam T The type of the resource
         * @tparam Ts Argument types for the constructor's arguments
         * @param identifier The resource's unique identifier
         * @param args Arguments for the constructor of the resource
         * @return ResourceReference<T> A shared pointer to the resource
         */
        template<typename T, typename... Ts>
            requires std::is_base_of_v<Resource, T> && std::is_constructible_v<T, Ts...>
        ResourceReference<T> get_or_create(const std::string& identifier, Ts&&... args) {
            const auto it = resources.find(identifier);
            if (it == resources.end())
                return create<T>(identifier, std::forward<Ts>(args)...);

            return ResourceReference<T>{identifier};
        }

        /**
         * @brief Get the resource with the given identifier, or load it from the given raw data
         *
         * @tparam T The type of the resource
         * @param identifier The identifier of the resource
         * @param data A vector of bytes to send to "load_from_text" if the resource doesn't already exist
         * @return ResourceReference<T> A shared pointer to the resource
         */
        template<typename T>
            requires std::is_default_constructible_v<T> && std::is_same_v<decltype(&T::load_from_bytes), bool (T::*)(const std::vector<uint8_t>&)>
        ResourceReference<T> get_or_load_from_bytes(const std::string& identifier, const std::vector<uint8_t>& data) {
            const auto it = resources.find(identifier);
            if (it != resources.end())
                return ResourceReference<T>{identifier};

            auto resource = create<T>(identifier);
            const auto lock = resource->lock_resource();
            resource.container->loaded = resource->load_from_bytes(data);

            return resource;
        }

        /**
         * @brief Get the resource with the given identifier, or load it from the given string
         *
         * @tparam T The type of the resource
         * @param identifier The identifier of the resource
         * @param text A string to send to "load_from_text" if the resource doesn't already exist
         * @return ResourceReference<T> A shared pointer to the resource
         */
        template<typename T>
            requires std::is_default_constructible_v<T> && std::is_same_v<decltype(&T::load_from_text), bool (T::*)(const std::string&)>
        ResourceReference<T> get_or_load_from_text(const std::string& identifier, const std::string& text) {
            const auto it = resources.find(identifier);
            if (it != resources.end())
                return ResourceReference<T>{it->second};

            auto resource = create<T>(identifier);
            const auto lock = resource.get().lock_resource();
            resource.container->loaded = resource.get_mutable().load_from_text(text);
            resource.container->probably_modified = false;

            return resource;
        }

        /**
         * @brief Load the resource from the given path, or return the existing one if it has already been loaded once
         *
         * @tparam T The type of the resource
         * @param file_path Path to the file the resource should be loaded from
         * @return ResourceReference<T> A shared pointer to the resource
         */
        template<typename T>
            requires std::is_default_constructible_v<T>
        ResourceReference<T> get_or_load(const Path& file_path) {
            const auto identifier = file_path.as_platform_independent().data;

            const auto it = resources.find(identifier);
            if (it != resources.end())
                return ResourceReference<T>{it->second};

            auto resource = create<T>(identifier);
            auto lock = resource.get().lock_resource();
            resource.container->from_file = true;

            bool success = false;

            if constexpr (std::is_same_v<decltype(&T::load_from_file), bool (T::*)(const Path&)>)
                success = resource.get_mutable().load_from_file(file_path);
            if constexpr (std::is_same_v<decltype(&T::load_from_text), bool (T::*)(const std::string&)>) {
                if (!success) {
                    const auto text = MagmaEngine{}.file_io().read_text(file_path);
                    if (!text.empty())
                        success = resource.get_mutable().load_from_text(text);
                }
            }
            if constexpr (std::is_same_v<decltype(&T::load_from_bytes), bool (T::*)(const std::vector<uint8_t>&)>) {
                if (!success) {
                    const auto bin = MagmaEngine{}.file_io().read_binary(file_path);
                    if (!bin.empty())
                        success = resource.get_mutable().load_from_text(bin);
                }
            }
            resource.container->probably_modified = false;
            resource.container->loaded = success;

            lock.unlock();
            if (!success) {
                resource.invalidate();
                return ResourceReference<T>{};
            }

            lock.lock();

            return resource;
        }


        void update(float) override;

#if defined(ENABLE_EDITOR)
        void in_editor_update(float) override { update(0.0f); }
#endif
    };


    template<typename T>
    ResourceReference<T>::ResourceReference(const std::string& identifier) {
        container = MagmaEngine{}.resource_manager().resources.at(identifier);
        const auto lock = container->resource->lock_resource();
        ++container->refs;
    }

    template<typename T>
    ResourceReference<T>::ResourceReference(ResourceReference<T>&& other)
        : container(other.container),
          is_original(other.is_original) {
        other.container = nullptr;
    }
    template<typename T>
    ResourceReference<T>::ResourceReference(const ResourceReference<T>& other)
        : container(other.container) {
        const auto lock = container->resource->lock_resource();
        ++container->refs;
    }

    template<typename T>
    ResourceReference<T>& ResourceReference<T>::operator=(const ResourceReference<T>& other) {
        if (this == &other || !other.valid())
            return *this;

        invalidate();

        container = other.container;

        const auto lock = container->resource->lock_resource();
        ++container->refs;

        return *this;
    }
    template<typename T>
    ResourceReference<T>& ResourceReference<T>::operator=(ResourceReference<T>&& other) {
        if (this == &other || !other.valid())
            return *this;

        container = other.container;
        is_original = other.is_original;

        other.container = nullptr;

        return *this;
    }

    template<typename T>
    T& ResourceReference<T>::get_mutable() {
        if (!valid())
            throw std::runtime_error("Attempt to get a non-valid resource");
        container->probably_modified = true;
        return *dynamic_cast<T*>(container->resource);
    }
    template<typename T>
    const T& ResourceReference<T>::get() const {
        if (!valid())
            throw std::runtime_error("Attempt to get a non-valid resource");
        return *dynamic_cast<const T*>(container->resource);
    }

    template<typename T>
    void ResourceReference<T>::invalidate() {
        if (container == nullptr)
            return;

        const auto lock = container->resource->lock_resource();

        if (is_original)
            container->has_no_original = true;

        if (--container->refs == 0)
            MagmaEngine{}.resource_manager().to_destroy.emplace(container->ident);

        container = nullptr;
    }

    template<typename T>
    bool ResourceReference<T>::valid() const {
        return container != nullptr;
    }

    template<typename T>
    SerializedData<ResourceReference<T>> ResourceReference<T>::serialize() const {
        if (!valid())
            return {};

        const auto lock = container->resource->lock_resource();

        SerializedData<ResourceReference<T>> res{};
        if (container->from_file) {
            res["file_path"] = container->ident;

            if (container->probably_modified) {
                bool success = false;

                if constexpr (std::is_same_v<decltype(&T::save_to_file), void (T::*)(const Path& file_path) const>) {
                    success = container->resource->save_to_file(container->ident);
                    if (success)
                        res["is_direct"] = true;
                }
                if constexpr (std::is_same_v<decltype(&T::save_to_text), std::string (T::*)() const>) {
                    if (!success) {
                        const auto text = container->resource->save_to_text();
                        if (!text.empty()) {
                            MagmaEngine{}.file_io().write_text(container->ident, text);
                            res["is_text_file"] = true;
                            success = true;
                        }
                    }
                }
                if constexpr (std::is_same_v<decltype(&T::save_to_bytes), std::vector<uint8_t> (T::*)() const>) {
                    if (!success) {
                        const auto bytes = container->resource->save_to_bytes();
                        if (!bytes.empty()) {
                            MagmaEngine{}.file_io().write_binary(container->ident, bytes);
                            res["is_bin_file"] = true;
                            success = true;
                        }
                    }
                }

                container->probably_modified = false;
            }
        }
        else {
            res["identifier"] = container->ident;

            if (container->has_no_original) {
                is_original = true;
                container->has_no_original = false;
            }

            if (is_original) {
                bool success = false;
                if constexpr (std::is_same_v<decltype(&T::save_to_text), std::string (T::*)() const>) {
                    if (!success) {
                        const auto text = container->resource->save_to_text();
                        if (!text.empty()) {
                            res["text"] = text;
                            success = true;
                        }
                    }
                }
                if constexpr (std::is_same_v<decltype(&T::save_to_bytes), std::vector<uint8_t> (T::*)() const>) {
                    if (!success) {
                        const auto bytes = container->resource->save_to_bytes();
                        if (!bytes.empty()) {
                            res["bytes"] = base64::encode_into<std::string>(bytes.begin(), bytes.end());
                            success = true;
                        }
                    }
                }
            }
        }

        return res;
    }

    template<typename T>
    void ResourceReference<T>::deserialize(const SerializedData<ResourceReference<T>>& data) {
        invalidate();

        if (data.has("file_path")) {
            const Path path = std::string(data["file_path"]);

            *this = MagmaEngine{}.resource_manager().get_or_load<T>(path);
        }
        else if (data.has("identifier")) {
            if (data.has("text")) {
                if constexpr (std::is_same_v<decltype(&T::load_from_text), bool (T::*)(const std::string&)>) {
                    const auto text = data["text"];
                    if (!text.empty())
                        *this = MagmaEngine{}.resource_manager().get_or_load_from_text<T>(std::string(data["identifier"]), std::string(text));
                }
            }
            else if (data.has("bytes")) {
                if constexpr (std::is_same_v<decltype(&T::load_from_bytes), bool (T::*)(const std::vector<uint8_t>&)>) {
                    const auto bytes = base64::decode_into<std::vector<uint8_t>>(std::string(data["bytes"]));
                    if (!bytes.empty())
                        *this = MagmaEngine{}.resource_manager().get_or_load_from_bytes<T>(std::string(data["identifier"]), bytes);
                }
            }
        }
    }

#if defined(ENABLE_EDITOR)
    template<typename T>
    bool ResourceReference<T>::inspect() {
        const auto e = MagmaEngine{}.ecs().ecs.as_entity(*this);
        if (e == MGMecs<>::null) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::Text("Resource is not a component on an entity, so it cannot be inspected properly");
            ImGui::PopStyleColor();
            return false;
        }
        ImGui::Text("%s", ("Resource Type: \"" + MagmaEngine{}.ecs().type_unique_identifier<ResourceReference<T>>() + "\"").c_str());

        static thread_local bool last_time_had_container = false;
        bool was_modified = false;

        if (container == nullptr) {
            last_time_had_container = false;

            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::Text("Empty Resource");
            ImGui::PopStyleColor();

            if constexpr (std::is_same_v<decltype(&T::load_from_file), bool (T::*)(const Path&)>
                          || std::is_same_v<decltype(&T::load_from_bytes), bool (T::*)(const std::vector<uint8_t>&)>
                          || std::is_same_v<decltype(&T::load_from_text), bool (T::*)(const std::string&)>) {
                if (ImGui::Button("Open")) {
                    MagmaEngine{}.editor().add_window<FileBrowser>(true, FileBrowser::Args{.mode = FileBrowser::Mode::READ, .type = FileBrowser::Type::FILE, .callback = [e](const Path path) {
                        auto& self = MagmaEngine{}.ecs().ecs.get<ResourceReference<T>>(e);
                        self = MagmaEngine{}.resource_manager().get_or_load<T>(path);
                        if (!self.valid()) {
                            const auto name = MagmaEngine{}.ecs().type_unique_identifier<ResourceReference<T>>();
                            Logging{"Resource Manager"}.error("Failed to load ", name, " from path \"", path.as_platform_independent().data, "\"");
                            self.invalidate();
                        }
                    }, .default_file_extension = MagmaEngine{}.resource_manager().get_resource_asociated_file_extension<T>(), .only_show_files_with_proper_extension = true});
                }
            }

            return false;
        }

        if (container->from_file) {
            was_modified = !last_time_had_container;
            last_time_had_container = true;

            ImGui::Text("%s", ("Resource loaded from \"" + container->ident + "\"").c_str());
            ImGui::SameLine();
            if (ImGui::Button("Close")) {
                invalidate();
                return true;
            }
        }
        return was_modified;
    }
#endif

    template<typename T>
    bool ResourceReference<T>::enable_for_serialization(const std::string& identifier) {
        auto& types = prepare_for_resource_type_serialization();
        types.emplace_back([identifier]() {
            MagmaEngine engine{};
            engine.ecs().enable_type_serialization<ResourceReference<T>>(identifier, true);
        });

        return true;
    }

    template<typename T>
    ResourceReference<T>::~ResourceReference() {
        invalidate();
    }
} // namespace mgm
