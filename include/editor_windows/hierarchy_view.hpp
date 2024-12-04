#pragma once
#include "systems/editor.hpp"


namespace mgm {
    class HierarchyView : public EditorWindow {
        struct Data;
        Data* data = nullptr;

        public:
          HierarchyView();

          void draw_contents() override;
    };
}
