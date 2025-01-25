#include "systems/resources.hpp"
#include "engine.hpp"


namespace mgm {
    ResourceManager::ResourceManager() {
        system_name = "Resource Manager";

        for (const auto& func : prepare_for_resource_type_serialization())
            func();
    }

    void ResourceManager::update(float) {
        for (const auto& ident : to_destroy) {
            const auto it = resources.find(ident);
            auto& container = it->second;
            if (container->refs)
                continue;

            delete container;
            resources.erase(it);
        }

        to_destroy.clear();
    }
}
