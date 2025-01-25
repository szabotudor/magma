#pragma once
#include "file.hpp"
#include "systems/editor.hpp"
#include "tools/mgmecs.hpp"
#include <memory>


namespace mgm {
    class HierarchyView : public EditorWindow {
        friend class InspectorWindow;
        friend class SceneViewport;

        struct Data;
        std::shared_ptr<Data> data{};

        public:
        HierarchyView();

        void draw_contents() override;
    };

    class InspectorWindow : public EditorWindow {
        friend class HierarchyView;
        friend class SceneViewport;

        using Data = HierarchyView::Data;
        std::shared_ptr<Data> data{};

        int current_type_n{};

        public:
        InspectorWindow();

        void draw_contents() override;
    };


    class SceneViewport : public EditorWindow {
        friend class HierarchyView;
        friend class InspectorWindow;

        static inline thread_local MGMecs<>::Entity current_scene_root{};
        
        static inline float time_since_last_edit = 0.0f;

        MGMecs<>::Entity this_viewport_scene_root{};
        Path this_viewport_scene_path{};

        void do_save();

        public:
        SceneViewport(const Path& scene_path);

        void draw_contents() override;

        ~SceneViewport() override;
    };
}
