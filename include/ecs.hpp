#pragma once
#include "systems.hpp"
#include "tools/mgmecs.hpp"


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

    class EntityComponentSystem : public System {
        public:
        MGMecs<> ecs;
        MGMecs<>::Entity root;

        EntityComponentSystem() : ecs{}, root{ecs.create()} {
            system_name = "EntityComponentSystem";
            ecs.emplace<HierarchyNode>(root, MGMecs<>::null).name = "Root";
        }

#if defined(ENABLE_EDITOR)
        bool draw_palette_options() override;
#endif

        ~EntityComponentSystem() {
            ecs.destroy(root);
        }
    };
}
