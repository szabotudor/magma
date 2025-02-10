#include "editor_windows/scene_view.hpp"
#include "backend_settings.hpp"
#include "ecs.hpp"
#include "engine.hpp"
#include "imgui.h"
#include "imgui_impl_mgmgpu.h"
#include "imgui_stdlib.h"
#include "json.hpp"
#include "systems/notifications.hpp"
#include "tools/mgmecs.hpp"
#include "systems/renderer.hpp"
#include <memory>
#include <string>


namespace mgm {
    static inline thread_local Path current_scene_path{};
    constexpr auto save_interval = 5.0f;
    struct HierarchyView::Data {
        MGMecs<>::Entity selected{};
        JObject selected_serialized_data{};

        MGMecs<>::Entity last_drawn_entity{};
        bool selection_came_from_navigating_with_down_arrow: 1 = false;
    };


    void SceneViewport::do_save() {
        MagmaEngine engine{};

        if (engine.ecs().ecs.try_get<HierarchyNode>(current_scene_root) == nullptr)
            return;

        JObject scene_data{};
        scene_data["name"] = "Root";
        scene_data["components"] = {};
        scene_data["children"] = engine.ecs().serialize_node(current_scene_root);
        engine.file_io().write_text(current_scene_path, scene_data);
        engine.notifications().push("Saved scene: \"" + current_scene_path.as_platform_independent().data + "\"");
    }

    SceneViewport::SceneViewport(const Path& scene_path) {
        window_name = "Viewport \"" + scene_path.file_name() + "\"##" + scene_path.platform_path();
        MagmaEngine engine{};
        const auto data = engine.editor().add_window<InspectorWindow>()->data = engine.editor().add_window<HierarchyView>()->data;

        this_viewport_scene_root = engine.ecs().load_scene_into_new_root(scene_path);
        this_viewport_scene_path = scene_path;
        current_scene_root = this_viewport_scene_root;
        current_scene_path = this_viewport_scene_path;
    }

    void SceneViewport::draw_contents() {
        MagmaEngine engine{};
        auto& renderer = engine.renderer();
        const vec2i32 new_size = {(int32_t)ImGui::GetContentRegionAvail().x, (int32_t)ImGui::GetContentRegionAvail().y};

        if (new_size != old_size) {
            auto& gpu = MagmaEngine{}.graphics();

            gpu.destroy_texture(viewport_texture);
            viewport_texture = gpu.create_texture(TextureCreateInfo{
                .name = "Texture",
                .size = new_size
            });
            old_size = new_size;

            renderer.settings.canvas = viewport_texture;
            renderer.projection = mat4f::gen_perspective_projection(90.0f, (float)new_size.y / (float)new_size.x, 0.1f, 1000.0f);
            renderer.settings.backend.viewport.top_left = {0, 0};
            renderer.settings.backend.viewport.bottom_right = new_size;
        }

        ImGui::Image(ImGui::as_imgui_texture(viewport_texture), {(float)new_size.x, (float)new_size.y});

        if (current_scene_root != this_viewport_scene_root || first_draw) {
            if (ImGui::IsWindowFocused()) {
                first_draw = false;
                engine.ecs().current_editing_scene = this_viewport_scene_root;
                renderer.settings.canvas = viewport_texture;
                renderer.projection = mat4f::gen_perspective_projection(90.0f, (float)new_size.y / (float)new_size.x, 0.1f, 1000.0f);
                renderer.settings.backend.viewport.top_left = {0, 0};
                renderer.settings.backend.viewport.bottom_right = new_size;

                if (time_since_last_edit < save_interval)
                    do_save();

                current_scene_root = this_viewport_scene_root;
                current_scene_path = this_viewport_scene_path;
                time_since_last_edit = save_interval;
            }
        }

        if (time_since_last_edit < save_interval) {
            time_since_last_edit += engine.delta_time();
            if (time_since_last_edit >= save_interval)
                do_save();
        }
    }

    SceneViewport::~SceneViewport() {
        if (time_since_last_edit < save_interval)
            do_save();
    }


    HierarchyView::HierarchyView() : data{std::make_shared<Data>()} {
        window_name = "Hierarchy";
    }

    void HierarchyView::draw_contents() {
        auto engine = MagmaEngine{};

        if (!engine.editor().is_a_project_loaded() || SceneViewport::current_scene_root == MGMecs<>::null) {
            close_window();
            return;
        }
        
        auto& ecs = engine.ecs().ecs;

        const auto name_entity = [&](const MGMecs<>::Entity new_parent, std::string name) {
            size_t tries = 0;
            while (ecs.get<HierarchyNode>(new_parent).get_child_by_name(name) != MGMecs<>::null) {
                if (tries == 0)
                    name += " (" + std::to_string(tries++) + ")";
                else
                    name = name.substr(0, name.find_last_of('(') + 1) + std::to_string(tries++) + ")";
            }

            return name;
        };

        if (ImGui::Button("+")) {
            const auto new_parent = data->selected == MGMecs<>::null ? SceneViewport::current_scene_root : data->selected;
            const auto name = name_entity(new_parent, "Node");
            ecs.emplace<HierarchyNode>(ecs.create(), new_parent).name = name;
            SceneViewport::time_since_last_edit = 0.0f;
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Create new entity");

        ImGui::SameLine();

        if (ImGui::Button((const char*)(u8"\u00D7"))) {
            if (data->selected != MGMecs<>::null) {
                ecs.destroy(data->selected);
                data->selected = MGMecs<>::null;
                SceneViewport::time_since_last_edit = 0.0f;
            }
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Delete selected entity");

        const auto selected_initially = data->selected;

        ImGui::Separator();

        std::function<void(const MGMecs<>::Entity)> draw_parent_and_children;
        
        draw_parent_and_children = [&] (const MGMecs<>::Entity parent) {
            if (parent == MGMecs<>::null)
                return;

            const auto& node = ecs.get<HierarchyNode>(parent);

            if (node.is_root()) {
                for (const auto child : node)
                    draw_parent_and_children(child);
                return;
            }

            const auto is_selected = data->selected == parent ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None;

            const auto do_drag_drop = [&]() {
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload("HIERARCHY_NODE", &parent, sizeof(parent), ImGuiCond_Once);
                    ImGui::Text("%s", node.name.c_str());
                    ImGui::EndDragDropSource();
                }
                
                auto payload = ImGui::GetDragDropPayload();
                if (!payload || !payload->IsDataType("HIERARCHY_NODE"))
                    return;

                auto entity_in_payload = *static_cast<const MGMecs<>::Entity*>(payload->Data);
                auto parent_of_node = node.parent;
                while (parent_of_node != MGMecs<>::null) {
                    if (parent_of_node == entity_in_payload)
                        return;
                    parent_of_node = ecs.get<HierarchyNode>(parent_of_node).parent;
                }

                if (ImGui::BeginDragDropTarget()) {
                    const auto item_start = ImGui::GetItemRectMin();
                    const auto item_end = ImGui::GetItemRectMax();
                    const auto cursor_pos = ImGui::GetMousePos();

                    const auto third = (item_end.y - item_start.y) * 0.3333333333f;
                    const auto sixth = third * 0.5f;
                    const auto top = item_start.y + third;
                    const auto bottom = item_end.y - third;

                    const auto window_end = ImGui::GetContentRegionMax().x;

                    if (cursor_pos.y < top)
                        ImGui::GetWindowDrawList()->AddRectFilled(
                            {item_start.x, item_start.y - sixth},
                            {window_end, item_start.y + sixth},
                            ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_DragDropTarget))
                        );
                    else if (cursor_pos.y > bottom)
                        ImGui::GetWindowDrawList()->AddRectFilled(
                            {item_start.x, item_end.y - sixth},
                            {window_end, item_end.y + sixth},
                            ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_DragDropTarget))
                        );

                    if (payload = ImGui::AcceptDragDropPayload("HIERARCHY_NODE"); payload) {
                        const auto entity = *static_cast<const MGMecs<>::Entity*>(payload->Data);
                        auto& entity_node = ecs.get<HierarchyNode>(entity);
                        entity_node.reparent(MGMecs<>::null);

                        if (cursor_pos.y < top) {
                            auto& parent_node = ecs.get<HierarchyNode>(node.parent);
                            const auto index = parent_node.find_child_index(parent);
                            entity_node.name = name_entity(node.parent, entity_node.name);
                            entity_node.reparent(node.parent, index);
                        }
                        else if (cursor_pos.y > bottom) {
                            auto& parent_node = ecs.get<HierarchyNode>(node.parent);
                            const auto index = parent_node.find_child_index(parent) + 1;
                            entity_node.name = name_entity(node.parent, entity_node.name);
                            entity_node.reparent(node.parent, index);
                        }
                        else {
                            entity_node.name = name_entity(parent, entity_node.name);
                            entity_node.reparent(parent);
                        }
                        
                        SceneViewport::time_since_last_edit = 0.0f;
                    }
                    ImGui::EndDragDropTarget();
                }
            };

            const auto do_down_arrow_not_open = [&]() {
                if (node.next != MGMecs<>::null)
                    data->selected = node.next;
                else if (node.parent != MGMecs<>::null) {
                    const auto& parent_node = ecs.get<HierarchyNode>(node.parent);
                    data->selected = parent_node.next;
                }
                else
                    data->selected = MGMecs<>::null;
            };

            const auto do_up_arrow = [&]() {
                if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && data->selected == parent && ImGui::IsWindowFocused()) {
                    if (node.parent != MGMecs<>::null) {
                        if (node.prev == MGMecs<>::null)
                            data->selected = node.parent == SceneViewport::current_scene_root ? MGMecs<>::null : node.parent;
                        else
                            data->selected = data->last_drawn_entity;
                    }
                }
            };

            if (node.has_children()) {
                const auto node_open = ImGui::TreeNodeEx((node.name + "##" + std::to_string(parent.value_)).c_str(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | is_selected);
                do_up_arrow();
                data->last_drawn_entity = parent;

                if (data->selection_came_from_navigating_with_down_arrow)
                    data->selection_came_from_navigating_with_down_arrow = false;
                else if (data->selected == parent && ImGui::IsKeyPressed(ImGuiKey_DownArrow) && ImGui::IsWindowFocused()) {
                    if (node_open)
                        data->selected = node.child;
                    else
                        do_down_arrow_not_open();
                    data->selection_came_from_navigating_with_down_arrow = true;
                }

                if (ImGui::IsItemClicked())
                    data->selected = parent;

                do_drag_drop();

                if (node_open) {
                    for (const auto entity : node)
                        draw_parent_and_children(entity);

                    ImGui::TreePop();
                }
            }
            else {
                ImGui::TreeNodeEx((node.name + "##" + std::to_string(parent.value_)).c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | is_selected);
                do_up_arrow();
                data->last_drawn_entity = parent;

                if (data->selection_came_from_navigating_with_down_arrow)
                    data->selection_came_from_navigating_with_down_arrow = false;
                else if (data->selected == parent && ImGui::IsKeyPressed(ImGuiKey_DownArrow) && ImGui::IsWindowFocused()) {
                    do_down_arrow_not_open();
                    data->selection_came_from_navigating_with_down_arrow = true;
                }
                
                if (ImGui::IsItemClicked())
                    data->selected = parent;

                do_drag_drop();
            }
        };

        draw_parent_and_children(SceneViewport::current_scene_root);

        if (ImGui::IsWindowFocused()) {
            if (data->selected == MGMecs<>::null) {
                if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
                    data->selected = ecs.get<HierarchyNode>(SceneViewport::current_scene_root).child;
                if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
                    data->selected = data->last_drawn_entity;
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                data->selected = MGMecs<>::null;
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered())
            data->selected = MGMecs<>::null;
        
        if (selected_initially != data->selected && data->selected != MGMecs<>::null)
            data->selected_serialized_data = engine.ecs().serialize_entity_components(data->selected);
    }


    InspectorWindow::InspectorWindow() {
        window_name = "Inspector";
    }

    void InspectorWindow::draw_contents() {
        auto engine = MagmaEngine{};

        if (!engine.editor().is_a_project_loaded() || SceneViewport::current_scene_root == MGMecs<>::null) {
            close_window();
            return;
        }

        if (data->selected == MGMecs<>::null) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::Text("No Node Selected");
            ImGui::PopStyleColor();
            return;
        }
        
        std::function<bool(const std::string& type_id, JObject& json)> inspect_func{};
        inspect_func = [&](const std::string& type_id, JObject& json) {
            const auto func = engine.ecs().all_serialized_types().find(type_id);
            if (func != engine.ecs().all_serialized_types().end())
                if (func->second.inspect_function)
                    return func->second.inspect_function(data->selected);

            bool any_edited = false;

            const std::string name = type_id;
            switch (json.type()) {
                case JObject::Type::NUMBER: {
                    if (json.is_number_decimal()) {
                        double num = (double)json;
                        any_edited |= ImGui::InputDouble(name.c_str(), &num);
                        json = num;
                    }
                    else {
                        int64_t num = (int64_t)json;
                        any_edited |= ImGui::InputScalar(name.c_str(), ImGuiDataType_S64, &num);
                        json = num;
                    }
                    break;
                }
                case JObject::Type::STRING: {
                    std::string str = json;
                    any_edited |= ImGui::InputText(name.c_str(), &str);
                    json = str;
                    break;
                }
                case JObject::Type::BOOLEAN: {
                    bool b = (bool)json;
                    any_edited |= ImGui::Checkbox(name.c_str(), &b);
                    json = b;
                    break;
                }
                case JObject::Type::OBJECT:
                case JObject::Type::ARRAY: {
                    for (auto [key, val] : json) {
                        if (std::string(key).starts_with("__"))
                            continue;

                        if (val.type() == JObject::Type::OBJECT || val.type() == JObject::Type::ARRAY) {
                            if (ImGui::TreeNodeEx(std::string(key).c_str(), ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow)) {
                                if (val.has("__type"))
                                    any_edited |= inspect_func(val["__type"], val);
                                else
                                    any_edited |= inspect_func(key, val);
                                ImGui::TreePop();
                            }
                        }
                        else {
                            if (val.has("__type"))
                                any_edited |= inspect_func(val["__type"], val);
                            else
                                any_edited |= inspect_func(key, val);
                        }
                    }
                }
                default: {
                    break;
                }
            }

            if (any_edited)
                engine.ecs().deserialize_entity_components(data->selected, data->selected_serialized_data);
            return any_edited;
        };

        bool any_edited = false;
        for (auto [type, value] : data->selected_serialized_data) {
            if (ImGui::CollapsingHeader(std::string(type).c_str(), ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow)) {
                ImGui::Indent();
                any_edited |= inspect_func(std::string(type), value);
                ImGui::Unindent();
            }
        }

        if (any_edited)
            SceneViewport::time_since_last_edit = 0.0f;

        ImGui::Separator();

        std::vector<const char*> type_ids{};
        type_ids.reserve(engine.ecs().all_serialized_types().size() + 1);
        type_ids.emplace_back("None");

        static thread_local std::string search_for{};
        ImGui::InputText("Search", &search_for);

        for (const auto& [id, type] : engine.ecs().all_serialized_types())
            if (type.enable_as_raw_component && (search_for.empty() || id.find(search_for) != std::string::npos))
                type_ids.push_back(id.c_str());

        ImGui::ListBox("Component Type", &current_type_n, type_ids.data(), static_cast<int>(type_ids.size()));

        if (!current_type_n) ImGui::BeginDisabled();
        if (ImGui::Button("Add Component +", {ImGui::GetContentRegionAvail().x, 0.0f})) {
            engine.ecs().add_component_of_type_to_entity(type_ids[static_cast<size_t>(current_type_n)], data->selected);
            data->selected_serialized_data = engine.ecs().serialize_entity_components(data->selected);
            SceneViewport::time_since_last_edit = 0.0f;
        }
        if (!current_type_n) ImGui::EndDisabled();
    }
} // namespace mgm
