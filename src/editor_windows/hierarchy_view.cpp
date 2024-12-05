#include "editor_windows/hierarchy_view.hpp"
#include "ecs.hpp"
#include "engine.hpp"
#include "imgui.h"
#include "tools/mgmecs.hpp"


namespace mgm {
    struct HierarchyView::Data {
        MGMecs<>::Entity selected{};
        MGMecs<>::Entity last_drawn_entity{};
        bool selection_came_from_navigating_with_down_arrow: 1 = false;
    };

    HierarchyView::HierarchyView() : data{new Data{}} {
        window_name = "Hierarchy";
    }

    void HierarchyView::draw_contents() {
        auto engine = MagmaEngine{};

        if (!engine.editor().is_a_project_loaded()) {
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
            const auto new_parent = data->selected == MGMecs<>::null ? engine.ecs().root : data->selected;
            const auto name = name_entity(new_parent, "Node");
            ecs.emplace<HierarchyNode>(ecs.create(), new_parent).name = name;
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Create new entity");

        ImGui::SameLine();

        if (ImGui::Button((const char*)(u8"\u00D7"))) {
            if (data->selected != MGMecs<>::null) {
                ecs.destroy(data->selected);
                data->selected = MGMecs<>::null;
            }
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Delete selected entity");
        
        const auto num_entities = ecs.entities_count() - 1;
        switch (num_entities) {
            case 0: {
                ImGui::Text("No entities (besides root)");
                return;
            }
            case 1: {
                ImGui::Text("1 entity");
                break;
            }
            default: {
                ImGui::Text("%u entities", num_entities);
                break;
            }
        }

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
                            data->selected = node.parent == engine.ecs().root ? MGMecs<>::null : node.parent;
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

        draw_parent_and_children(engine.ecs().root);

        if (ImGui::IsWindowFocused()) {
            if (data->selected == MGMecs<>::null) {
                if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
                    data->selected = ecs.get<HierarchyNode>(engine.ecs().root).child;
                if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
                    data->selected = data->last_drawn_entity;
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                data->selected = MGMecs<>::null;
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered())
            data->selected = MGMecs<>::null;
    }
} // namespace mgm
