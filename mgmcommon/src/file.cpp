#include "file.hpp"
#include <string>


namespace mgm {
    Path Path::project_dir{};
    Path Path::assets_dir{};
    Path Path::game_data_dir{};

    Path Path::engine_exe_dir = FileIO::exe_dir();


    std::string Path::parse_prefix() const {
        const auto it = data.find("://");
        if (it == std::string::npos) {
            return data;
        }
        if (it > 0) {
            const auto prefix = data.substr(0, it);
            const auto prefix_it = prefixes.find(prefix);
            if (prefix_it != prefixes.end()) {
                const auto real_name = prefix_it->first;
                const auto real_path = prefix_it->second->data;
                return real_path + data.substr(it + 2 + (data == prefix + "://"));
            }
        }
        if (it == 0)
            return prefixes.at("assets")->data + data.substr(2 + (data == "assets://"));
        return data;
    }


    Path Path::as_platform_independent() const {
        if (data.empty() || this == &project_dir || this == &assets_dir || this == &game_data_dir || this == &engine_resources_dir)
            return {};

        Path res{};

        auto it = data.find(assets_dir.data);
        if (it == 0) {
            res.data = "assets://" + data.substr(assets_dir.data.size());
            if (res.data[9] == '/')
                res.data.erase(9, 1);
            return res;
        }

        it = data.find(game_data_dir.data);
        if (it == 0) {
            res.data = "data://" + data.substr(game_data_dir.data.size());
            if (res.data[7] == '/')
                res.data.erase(7, 1);
            return res;
        }

        it = data.find(project_dir.data);
        if (it == 0) {
            res.data = "project://" + data.substr(project_dir.data.size());
            if (res.data[10] == '/')
                res.data.erase(10, 1);
            return res;
        }

        return *this;
    }

    void Path::setup_project_dirs(const std::string& platform_project_dir, const std::string& platform_assets_dir, const std::string& platform_game_data_dir) {
        project_dir.data = platform_project_dir;
        assets_dir.data = platform_assets_dir;
        game_data_dir.data = platform_game_data_dir;
    }

    Path Path::direct_append(const Path& other) const {
        if (data.back() == '/' && other.data.front() == '/')
            return Path{data + other.data.substr(1)};
        else if (data.back() != '/' && other.data.front() != '/')
            return Path{data + "/" + other.data};
        return Path{data + other.data};
    }
    Path Path::direct_remove(const Path& other) const {
        const auto where = data.find(other.data);
        if (where == std::string::npos)
            return *this;
        if (where == 0)
            return Path{data.substr(other.data.size())};
        return Path{data.substr(0, where - 1)};
    }

    Path Path::back() const {
        const auto last_slash = data.find_last_of('/');
        if (last_slash == std::string::npos)
            return Path{};
        if (last_slash == 0)
            return Path{"/"};
        if (last_slash == data.size() - 1) {
            const auto prev_slash = data.find_last_of('/', last_slash - 1);
            if (prev_slash == last_slash - 1) {
                Path res{};
                res.data = platform_path();
                return res.back().as_platform_independent();
            }
            return Path{data.substr(0, last_slash)}.as_platform_independent();
        }
        const auto prev_slash = data.find_last_of('/', last_slash - 1);
        if (prev_slash == last_slash - 1)
            return Path{platform_path()}.back().as_platform_independent();
        return Path{data.substr(0, last_slash)}.as_platform_independent();
    }

    std::string Path::file_name() const {
        const auto res = platform_path();
        size_t last_slash = res.find_last_of('/');
        if (last_slash == std::string::npos)
            return res;
        else
            return res.substr(last_slash + 1);
    }
}
