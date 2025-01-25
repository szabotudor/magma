#include "ecs.hpp"
#include "editor_windows/file_browser.hpp"
#include "editor_windows/scene_view.hpp"
#include "engine.hpp"
#include "file.hpp"
#include "imgui.h"
#include "json.hpp"
#include "systems/editor.hpp"
#include "systems/notifications.hpp"
#include "tools/mgmecs.hpp"


namespace mgm {
    void HierarchyNode::on_construct(mgm::MGMecs<>* ecs, const mgm::MGMecs<>::Entity self) {
        if (ecs == nullptr)
            return;
        if (parent == mgm::MGMecs<>::null)
            return;

        auto& parent_node = ecs->get<HierarchyNode>(parent);
        if (parent_node.child != mgm::MGMecs<>::null) {
            auto& child_node = ecs->get<HierarchyNode>(parent_node.child);
            child_node.prev = self;
            next = parent_node.child;
            parent_node.child = self;
        }
        else
            parent_node.child = self;

        ++parent_node.num_children;
    }

    void HierarchyNode::on_destroy(mgm::MGMecs<>* ecs, const mgm::MGMecs<>::Entity self) {
        if (ecs == nullptr)
            return;

        if (parent != mgm::MGMecs<>::null) {
            auto& parent_node = ecs->get<HierarchyNode>(parent);
            if (parent_node.child == self) {
                parent_node.child = next;
                if (next != mgm::MGMecs<>::null) {
                    auto& next_node = ecs->get<HierarchyNode>(next);
                    next_node.prev = mgm::MGMecs<>::null;
                    next = mgm::MGMecs<>::null;
                }
            }
            else if (prev != mgm::MGMecs<>::null) {
                auto& prev_node = ecs->get<HierarchyNode>(prev);
                prev_node.next = next;
                if (next != mgm::MGMecs<>::null) {
                    auto& next_node = ecs->get<HierarchyNode>(next);
                    next_node.prev = prev;
                }
            }

            --parent_node.num_children;
        }

        const auto children_copy = children();
        ecs->destroy(children_copy.begin(), children_copy.end());
    }

    HierarchyNode::Iterator& HierarchyNode::Iterator::operator++() {
        if (current == mgm::MGMecs<>::null)
            return *this;

        current = MagmaEngine{}.ecs().ecs.get<HierarchyNode>(current).next;

        return *this;
    }
    HierarchyNode::Iterator HierarchyNode::Iterator::operator++(int) {
        auto copy = *this;
        ++(*this);
        return copy;
    }

    HierarchyNode::Iterator& HierarchyNode::Iterator::operator--() {
        current = MagmaEngine{}.ecs().ecs.get<HierarchyNode>(current).prev;

        return *this;
    }
    HierarchyNode::Iterator HierarchyNode::Iterator::operator--(int) {
        auto copy = *this;
        --(*this);
        return copy;
    }

    HierarchyNode::Iterator HierarchyNode::Iterator::operator+(size_t i) const {
        if (current == mgm::MGMecs<>::null)
            return *this;

        auto copy = *this;
        while (i > 0) {
            ++copy;
            --i;
        }
        return copy;
    }
    HierarchyNode::Iterator& HierarchyNode::Iterator::operator+=(size_t i) {
        if (current == mgm::MGMecs<>::null)
            return *this;

        while (i > 0) {
            ++(*this);
            --i;
        }
        return *this;
    }

    HierarchyNode::Iterator HierarchyNode::Iterator::operator-(size_t i) const {
        auto copy = *this;
        while (i > 0) {
            --copy;
            --i;
        }
        return copy;
    }
    HierarchyNode::Iterator& HierarchyNode::Iterator::operator-=(size_t i) {
        while (i > 0) {
            --(*this);
            --i;
        }
        return *this;
    }

    void HierarchyNode::reparent(MGMecs<>::Entity new_parent, size_t index) {
        if (index == static_cast<size_t>(-1))
            index = 0;

        auto& ecs = MagmaEngine{}.ecs().ecs;

        const auto self = ecs.as_entity(*this);

        if (parent == new_parent) {
            auto& parent_node = ecs.get<HierarchyNode>(parent);

            const auto old_index = parent_node.find_child_index(self);
            if (old_index == index)
                return;

            if (index > old_index)
                --index;

            if (parent_node.child == self)
                parent_node.child = next;

            if (next != MGMecs<>::null)
                ecs.get<HierarchyNode>(next).prev = prev;
            if (prev != MGMecs<>::null)
                ecs.get<HierarchyNode>(prev).next = next;

            next = MGMecs<>::null;
            prev = MGMecs<>::null;

            if (index == 0) {
                next = parent_node.child;
                parent_node.child = self;
                ecs.get<HierarchyNode>(next).prev = self;
                return;
            }

            auto it = parent_node.begin();
            Iterator prev_it {MGMecs<>::null};
            while (index > 0 && it != parent_node.end()) {
                prev_it = it++;
                --index;
            }

            if (it == parent_node.end()) {
                ecs.get<HierarchyNode>(*prev_it).next = self;
                prev = *prev_it;
                return;
            }

            ecs.get<HierarchyNode>(*it).prev = self;
            ecs.get<HierarchyNode>(*prev_it).next = self;
            next = *it;
            prev = *prev_it;

            return;
        }
        
        if (parent != mgm::MGMecs<>::null) {
            auto& parent_node = ecs.get<HierarchyNode>(parent);
            if (parent_node.child == self) {
                parent_node.child = next;
            }
            if (prev != mgm::MGMecs<>::null) {
                auto& prev_node = ecs.get<HierarchyNode>(prev);
                prev_node.next = next;
            }
            if (next != mgm::MGMecs<>::null) {
                auto& next_node = ecs.get<HierarchyNode>(next);
                next_node.prev = prev;
            }
            prev = mgm::MGMecs<>::null;
            next = mgm::MGMecs<>::null;

            --parent_node.num_children;
            parent = mgm::MGMecs<>::null;
        }

        if (new_parent != mgm::MGMecs<>::null) {
            auto& new_parent_node = ecs.get<HierarchyNode>(new_parent);
            if (index == 0) {
                if (new_parent_node.child != mgm::MGMecs<>::null) {
                    auto& child_node = ecs.get<HierarchyNode>(new_parent_node.child);
                    child_node.prev = self;
                }
                next = new_parent_node.child;
                new_parent_node.child = self;
            }
            else {
                Iterator prev_it{};
                auto it = new_parent_node.begin();
                while (index > 0 && it != new_parent_node.end()) {
                    prev_it = it;
                    ++it;
                    --index;
                }

                if (it != new_parent_node.end()) {
                    auto& next_node = ecs.get<HierarchyNode>(*it);
                    next = *it;
                    next_node.prev = self;
                }
                auto& prev_node = ecs.get<HierarchyNode>(*prev_it);
                prev = *prev_it;
                prev_node.next = self;
            }
            ++new_parent_node.num_children;
            parent = new_parent;
        }
    }

    size_t HierarchyNode::find_child_index(MGMecs<>::Entity entity) const {
        size_t i = 0;
        for (const auto c : *this) {
            if (c == entity)
                return i;
            ++i;
        }

        return static_cast<size_t>(-1);
    }

    MGMecs<>::Entity HierarchyNode::get_child_at(size_t i) const {
        auto it = begin();
        while (i > 0 && it != end()) {
            ++it;
            --i;
        }

        return *it;
    }

    MGMecs<>::Entity HierarchyNode::get_child_by_name(const std::string& child_name) const {
        for (const auto entity : *this) {
            const auto& node = MagmaEngine{}.ecs().ecs.get<HierarchyNode>(entity);
            if (node.name == child_name)
                return entity;
        }

        return mgm::MGMecs<>::null;
    }


    MGMecs<>::Entity EntityComponentSystem::load_scene_into_new_root(const Path& path) {
        MagmaEngine engine{};

        const auto it = editable_scenes.find(path);
        if (it != editable_scenes.end()) {
            engine.notifications().push("Scene at path \"" + path.platform_path() + "\" is already opened");
            return MGMecs<>::null;
        }

        const auto new_scene_root = ecs.create();
        const JObject scene_data = engine.file_io().read_text(path);
        deserialize_node(new_scene_root, scene_data);
        
        editable_scenes[path] = new_scene_root;
        return new_scene_root;
    }

    JObject EntityComponentSystem::serialize_entity_components(const MGMecs<>::Entity entity) {
        JObject res{};
        for (const auto& [type, serializer] : serialized_types) {
            if (!serializer.serialize)
                continue;

            const auto json = serializer.serialize(entity);
            if (!json.empty())
                res[type] = json;
        }
        return res;
    }

    void EntityComponentSystem::deserialize_entity_components(const MGMecs<>::Entity entity, const JObject& json) {
        for (const auto& [key, value] : json) {
            const auto it = serialized_types.find(key);
            if (it == serialized_types.end() || !it->second.deserialize)
                continue;

            it->second.deserialize(entity, value);
        }
    }

    JObject EntityComponentSystem::serialize_node(const MGMecs<>::Entity entity) {
        JObject res{};
        res.array();

        for (const auto& e : ecs.get<HierarchyNode>(entity)) {
            JObject entry{};

            const auto& node = ecs.get<HierarchyNode>(e);
            entry["name"] = node.name;
            entry["components"] = serialize_entity_components(e);
            entry["children"] = serialize_node(e);

            res.emplace_back(entry);
        }

        return res;
    }

    void EntityComponentSystem::deserialize_node(const MGMecs<>::Entity entity, const JObject& json) {
        if (!json.has("components") || !json.has("name")) {
            ecs.emplace<HierarchyNode>(entity, MGMecs<>::null).name = "Root";
            return;
        }

        ecs.get_or_emplace<HierarchyNode>(entity, MGMecs<>::null).name = json["name"];

        deserialize_entity_components(entity, json["components"]);

        if (!json.has("children"))
            return;

        const auto& children = json["children"].array();

        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            const auto& child_json = *it;
            if (child_json.type() != JObject::Type::OBJECT)
                continue;
            const auto c = ecs.create();
            ecs.emplace<HierarchyNode>(c, entity).name = child_json["name"];
            deserialize_node(c, child_json);
        }
    }

#if defined(ENABLE_EDITOR)
    bool EntityComponentSystem::draw_palette_options() {
        auto& editor = MagmaEngine{}.editor();

        if (editor.begin_window_here("Scene", editor.is_a_project_loaded())) {
            if (ImGui::SmallButton("Open Scene In Editor")) {
                editor.add_window<FileBrowser>(true, FileBrowser::Args{
                    .mode = FileBrowser::Mode::READ,
                    .type = FileBrowser::Type::FILE,
                    .callback = [](const Path& path) {
                        MagmaEngine engine{};
                        engine.editor().add_window<SceneViewport>(true, path);

                        engine.ecs().load_scene_into_new_root(path);
                        
                        engine.notifications().push("Opened scene: " + path.platform_path());
                    },
                    .allow_paths_outside_project = false,
                });
                editor.end_window_here();
                return true;
            }

            if (ImGui::SmallButton("Create Scene")) {
                editor.add_window<FileBrowser>(true, FileBrowser::Args{
                    .mode = FileBrowser::Mode::WRITE,
                    .type = FileBrowser::Type::FILE,
                    .callback = [](const Path& path) {
                        MagmaEngine engine{};
                        engine.editor().add_window<SceneViewport>(true, path);

                        engine.ecs().load_scene_into_new_root(path);

                        engine.notifications().push("Created and opened scene: " + path.platform_path());
                    },
                    .allow_paths_outside_project = false
                });
            }

            editor.end_window_here();
        }

        return false;
    }
#endif
} // namespace mgm
