#pragma once
#include "engine.hpp"
#include "file.hpp"
#include "json.hpp"
#include "systems.hpp"
#include "tools/mgmecs.hpp"
#include <cstddef>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>


namespace mgm {
    struct HierarchyNode {
        std::string name = "Node";
        mgm::MGMecs<>::Entity parent{};
        mgm::MGMecs<>::Entity child{};
        mgm::MGMecs<>::Entity prev{};
        mgm::MGMecs<>::Entity next{};

        mgm::MGMecs<>::Entity::Type num_children = 0;

        HierarchyNode(mgm::MGMecs<>::Entity parent_node) : parent{parent_node} {}

        void on_construct(mgm::MGMecs<>* ecs, const mgm::MGMecs<>::Entity self);

	    void on_destroy(mgm::MGMecs<>* ecs, const mgm::MGMecs<>::Entity self);

        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using value_type = mgm::MGMecs<>::Entity;
            using difference_type = mgm::MGMecs<>::Entity::Type;
            using pointer = mgm::MGMecs<>::Entity*;
            using reference = mgm::MGMecs<>::Entity&;

            MGMecs<>::Entity current;

            Iterator& operator++();
            Iterator operator++(int);

            Iterator& operator--();
            Iterator operator--(int);

            Iterator operator+(size_t i) const;
            Iterator& operator+=(size_t i);

            Iterator operator-(size_t i) const;
            Iterator& operator-=(size_t i);

            bool operator==(const Iterator& other) const { return current == other.current; }
            bool operator!=(const Iterator& other) const { return current != other.current; }

            mgm::MGMecs<>::Entity& operator*() { return current; }
            const mgm::MGMecs<>::Entity& operator*() const { return current; }
        };

        Iterator begin() const { return Iterator{child}; }
        Iterator end() const { return Iterator{mgm::MGMecs<>::null}; }

        /**
         * @brief Get a vector of entities representing the children of this node
         */
        std::vector<MGMecs<>::Entity> children() const {
            return {begin(), end()};
        }

        bool has_children() const { return child != mgm::MGMecs<>::null; }

        /**
         * @brief Check if this node is the root of some hierarchy
         * 
         * @return true If the node has no parent
         */
        bool is_root() const { return parent == mgm::MGMecs<>::null; }

        /**
         * @brief Remove this node from its parent's children and make it the first child of new_parent
         * 
         * @param new_parent The entity to make the parent of this node
         * @param index The index in the children of new_parent to make this node
         */
        void reparent(mgm::MGMecs<>::Entity new_parent, size_t index = 0);

        /**
         * @brief Get the index of the child in the children of this node
         * 
         * @param child The entity to get the index of
         * @return size_t The index of the child, or static_cast<size_t>(-1) if the entity is not a child of this node
         */
        size_t find_child_index(MGMecs<>::Entity entity) const;

        /**
         * @brief Get the entity at index i in the children of this node
         * 
         * @param i The index of the child to get
         * @return MGMecs<>::Entity The entity at index i
         */
        MGMecs<>::Entity get_child_at(size_t i) const;

        /**
         * @brief Get the entity with the name name in the children of this node
         * 
         * @param name The name of the child to get
         * @return MGMecs<>::Entity The entity with the name name, or null if no such entity exists
         */
        MGMecs<>::Entity get_child_by_name(const std::string& child_name) const;
    };


    template<typename T>
    struct SerializedData {
        private:
        JObject json{};

        public:
        SerializedData() = default;

        SerializedData(JObject& raw_json) : json(std::move(raw_json)) {}
        SerializedData(const JObject& raw_json) : json(raw_json) {}

        SerializedData(JObject&& raw_json) : json{std::forward<JObject>(raw_json)} {}

        operator JObject();

        template<typename I>
        decltype(std::declval<JObject>().operator[](std::declval<I>())) operator[](I i) { return json[i]; }
        template<typename I>
        decltype(std::declval<const JObject>().operator[](std::declval<I>())) operator[](I i) const { return json[i]; }

        bool has(const std::string& key) const { return json.has(key); }
        bool has(const size_t& index) const { return json.has(index); }
    };

#if defined(ENABLE_EDITOR)
    template <typename, typename = void>
    struct has_inspect : std::false_type {};

    template <typename T>
    struct has_inspect<T, std::void_t<decltype(std::declval<T>().inspect())>> : std::is_same<bool, decltype(std::declval<T>().inspect())> {};

    template <typename T>
    inline constexpr bool has_inspect_v = has_inspect<T>::value;


    template <typename, typename = void>
    struct has_external_inspect : std::false_type {};

    template <typename T>
    struct has_external_inspect<T, std::void_t<decltype(inspect(std::declval<T&>()))>> : std::is_same<bool, decltype(inspect(std::declval<T&>()))> {};

    template <typename T>
    inline constexpr bool has_external_inspect_v = has_external_inspect<T>::value;
#endif


    template <typename, typename = void>
    struct has_serialize : std::false_type {};

    template <typename T>
    struct has_serialize<T, std::void_t<decltype(std::declval<const T&>().serialize())>> : std::is_same<SerializedData<T>, decltype(std::declval<const T&>().serialize())> {};

    template <typename T>
    inline constexpr bool has_serialize_v = has_serialize<T>::value;


    template <typename, typename = void>
    struct has_external_serialize : std::false_type {};

    template <typename T>
    struct has_external_serialize<T, std::void_t<decltype(inspect(std::declval<const T&>()))>> : std::is_same<SerializedData<T>, decltype(serialize(std::declval<const T&>()))> {};

    template <typename T>
    inline constexpr bool has_external_serialize_v = has_external_serialize<T>::value;


    template <typename, typename = void>
    struct has_deserialize : std::false_type {};

    template <typename T>
    struct has_deserialize<T, std::void_t<decltype(std::declval<T&>().deserialize(std::declval<const SerializedData<T>&>()))>> : std::is_same<void, decltype(std::declval<T&>().deserialize(std::declval<const SerializedData<T>&>()))> {};

    template <typename T>
    inline constexpr bool has_deserialize_v = has_deserialize<T>::value;


    template <typename, typename = void>
    struct has_external_deserialize : std::false_type {};

    template <typename T>
    struct has_external_deserialize<T, std::void_t<decltype(deserialize(std::declval<T&>(), std::declval<const SerializedData<T>&>()))>> : std::is_same<void, decltype(deserialize(std::declval<T&>(), std::declval<const SerializedData<T>&>()))> {};

    template <typename T>
    inline constexpr bool has_external_deserialize_v = has_external_deserialize<T>::value;


    class EntityComponentSystem : public System {
        template<typename T>
        friend struct SerializedData;

        public:
        struct SerializedType {
            std::function<JObject(const MGMecs<>::Entity entity)> serialize{};
            std::function<void(MGMecs<>::Entity entity, const JObject& json)> deserialize{};
            std::function<void(const MGMecs<>::Entity entity)> add_component_to_entity{};
            std::function<void(const MGMecs<>::Entity entity)> remove_component_from_entity{};

#if defined(ENABLE_EDITOR)
            std::function<bool(const MGMecs<>::Entity entity)> inspect_function{};
#endif

            bool enable_as_raw_component = false;
        };

        private:
        std::unordered_map<std::string, SerializedType> serialized_types{};
        std::unordered_map<size_t, std::string> types_unique_ids{};

        public:
        MGMecs<> ecs;
        MGMecs<>::Entity root;

#if defined(ENABLE_EDITOR)
        std::unordered_map<Path, MGMecs<>::Entity> editable_scenes{};
        MGMecs<>::Entity current_editing_scene{};

        MGMecs<>::Entity load_scene_into_new_root(const Path& path);
#endif

        EntityComponentSystem() : ecs{}, root{ecs.create()} {
            system_name = "EntityComponentSystem";
            ecs.emplace<HierarchyNode>(root, MGMecs<>::null).name = "Root";
        }

        mutable std::mutex mutex{};
        std::unique_lock<std::mutex> ecs_lock() {
            return std::unique_lock{mutex};
        }

        /**
         * @brief Enable serialization for the given type
         * 
         * @tparam T A type which implements a constructor from JObject, and a JObject conversion operator
         * @param unique_identifier A unique identifier for the type, used to save the type of the serialized json data
         * @param enable_as_raw_component If true, the type will be allowed to be placed as a raw component directly on an entity in the heirarchy
         */
        template<typename T>
        void enable_type_serialization(const std::string& unique_identifier, bool enable_as_raw_component = false) {
            std::unique_lock lock {mutex};

            const auto it = serialized_types.find(unique_identifier);
            if (it != serialized_types.end())
                throw std::runtime_error("Registered type with identifier \"" + unique_identifier + "\" twice");

            auto& type = serialized_types[unique_identifier];
            type.enable_as_raw_component = enable_as_raw_component;

            if constexpr (std::is_constructible_v<T, SerializedData<T>> && std::is_constructible_v<SerializedData<T>, T>) {
                type.serialize = [unique_identifier](const MGMecs<>::Entity entity) {
                    const auto t = MagmaEngine{}.ecs().ecs.try_get<T>(entity);
                    SerializedData<T> res{};
                    if (t != nullptr)
                        return JObject(SerializedData<T>(*t));
                    return JObject{};
                };
                type.deserialize = [](const MGMecs<>::Entity entity, const JObject& json) {
                    MagmaEngine engine{};
                    const auto t = engine.ecs().ecs.try_get<T>(entity);
                    if (t == nullptr)
                        engine.ecs().ecs.emplace<T>(entity, T(SerializedData<T>(json)));
                    else
                        *t = T(SerializedData<T>(json));
                };
            }
            else {
                if constexpr (has_serialize_v<T>) {
                    type.serialize = [](const MGMecs<>::Entity entity) {
                        const auto t = MagmaEngine{}.ecs().ecs.try_get<T>(entity);
                        if (t == nullptr)
                            return JObject{};
                        return JObject(t->serialize());
                    };
                }
                else if constexpr (has_external_serialize_v<T>) {
                    type.serialize = [](const MGMecs<>::Entity entity) {
                        const auto t = MagmaEngine{}.ecs().ecs.try_get<T>(entity);
                        if (t == nullptr)
                            return JObject{};
                        return JObject(serialize(*t));
                    };
                }

                if constexpr (has_deserialize_v<T>) {
                    type.deserialize = [](const MGMecs<>::Entity entity, const JObject& json) {
                        MagmaEngine engine{};
                        engine.ecs().ecs.get_or_emplace<T>(entity).deserialize(SerializedData<T>(json));
                    };
                }
                else if constexpr (has_external_deserialize_v<T>) {
                    type.deserialize = [](const MGMecs<>::Entity entity, const JObject& json) {
                        MagmaEngine engine{};
                        deserialize(engine.ecs().ecs.get_or_emplace<T>(entity), SerializedData<T>(json));
                    };
                }
            }

            if constexpr (std::is_default_constructible_v<T>) {
                type.add_component_to_entity = [](const MGMecs<>::Entity entity) {
                    MagmaEngine{}.ecs().ecs.get_or_emplace<T>(entity);
                };
            }
            type.remove_component_from_entity = [](const MGMecs<>::Entity entity) {
                MagmaEngine{}.ecs().ecs.try_remove<T>(entity);
            };

            if constexpr (has_inspect_v<T>) {
                type.inspect_function = [](const MGMecs<>::Entity entity) {
                    return MagmaEngine{}.ecs().ecs.get<T>(entity).inspect();
                };
            }
            else if constexpr(has_external_inspect_v<T>) {
                type.inspect_function = [](const MGMecs<>::Entity entity) {
                    return inspect(MagmaEngine{}.ecs().ecs.get<T>(entity));
                };
            }

            types_unique_ids[typeid(T).hash_code()] = unique_identifier;
        }
        
        /**
         * @brief Serialize the component of type T if it's part of the entity
         * 
         * @tparam T The type of the component to try and serialize
         * @param entity The entity to get the component from
         * @return Serialized Json of the component
         */
        template<typename T>
        JObject serialize_component(const MGMecs<>::Entity entity) {
            std::unique_lock lock {mutex};
            return serialized_types[types_unique_ids[typeid(T).hash_code()]].serialize(entity);
        }

        /**
         * @brief Deserialize a component from some json data, and set its value, or add it to the entity if it doesn't exist already
         * 
         * @tparam T The type of the component to deserialize
         * @param entity The entity to deserialize the component into
         * @param json Raw json data to be sent to the entity
         */
        template<typename T>
        void deserialize_component(const MGMecs<>::Entity entity, const JObject& json) {
            std::unique_lock lock {mutex};
            serialized_types[types_unique_ids[typeid(T).hash_code()]].deserialize(entity, json);
        }

        /**
         * @brief Get a map of all registered types unique IDs, and their contents
         */
        const std::unordered_map<std::string, SerializedType>& all_serialized_types() const {
            std::unique_lock lock {mutex};
            return serialized_types;
        }

        /**
         * @brief Get the unique identifier of a type using its typeid hash
         * 
         * @tparam T The type to get the identifier of
         * @return std::string The identifier of the type
         */
        template<typename T>
        std::string type_unique_identifier() const {
            std::unique_lock lock {mutex};
            const auto it = types_unique_ids.find(typeid(T).hash_code());
            if (it == types_unique_ids.end())
                return "";
            return it->second;
        }

        /**
         * @brief Add a component to the entity using its registered unique ID (if it's registered)
         * 
         * @param component_type The unique ID of the component's type
         * @param entity The entity to add the component to
         */
        void add_component_of_type_to_entity(const std::string& component_type, const MGMecs<>::Entity entity) const {
            std::unique_lock lock {mutex};
            const auto it = serialized_types.find(component_type);
            if (it == serialized_types.end())
                throw std::runtime_error("Type \"" + component_type + "\" not enabled for serialization");
            it->second.add_component_to_entity(entity);
        }

        /**
         * @brief Remove a component from the entity using its registered unique ID (if it's registered)
         * 
         * @param component_type The unique ID of the component's type
         * @param entity The entity to remove the component from
         */
        void remove_component_of_type_from_entity(const std::string& component_type, const MGMecs<>::Entity entity) const {
            std::unique_lock lock {mutex};
            const auto it = serialized_types.find(component_type);
            if (it == serialized_types.end())
                throw std::runtime_error("Type \"" + component_type + "\" not enabled for serialization");
            it->second.remove_component_from_entity(entity);
        }

        /**
         * @brief Dump all components which are a registered type into Json
         * 
         * @param entity The entity who's components to serialize
         * @return JObject A Json array of all components serialized into objects
         */
        JObject serialize_entity_components(const MGMecs<>::Entity entity);

        /**
         * @brief Load components which are a registered type from Json, and add them to the entity, or set the existing component on the entity to the new deserialized value
         * 
         * @param entity The entity to add the components to
         * @param json The Json data to build the components from
         */
        void deserialize_entity_components(const MGMecs<>::Entity entity, const JObject& json);

        /**
         * @brief Dump a Hierarchy Node into json (its name, all of its components, and all of its children, all the way down the hierarchy)
         * 
         * @param entity The root node of the tree (prefferably a root node, but can start from a child in another tree as well)
         * @return JObject A json object with the information
         */
        JObject serialize_node(const MGMecs<>::Entity entity);

        /**
         * @brief Create a hierarchy with the given entity as its root, and using the given Json data to load the children
         * 
         * @param entity The entity to make into the new root of the tree to deserialize (can be non-root, new tree will pe placed under the given antity anyway)
         * @param json The json to load the data from
         */
        void deserialize_node(const MGMecs<>::Entity entity, const JObject& json);

#if defined(ENABLE_EDITOR)
        bool draw_palette_options() override;
#endif

        ~EntityComponentSystem() {
            std::unique_lock lock {mutex};
            ecs.destroy(root);
            for (const auto& [scene_path, scene_root] : editable_scenes)
                ecs.destroy(scene_root);
        }
    };

    template<typename T>
    SerializedData<T>::operator JObject() {
        MagmaEngine engine{};
        const auto it = engine.ecs().types_unique_ids.find(typeid(T).hash_code());
        if (it != engine.ecs().types_unique_ids.end() && !json.has("__type"))
            json["__type"] = it->second;
        return json;
    }
}
