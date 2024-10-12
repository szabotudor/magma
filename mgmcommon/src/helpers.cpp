#include "helpers.hpp"


std::string beautify_name(std::string name) {
    for (size_t i = 0; i < name.size(); i++) {
        auto& c = name[i];
        if (c == '_') {
            if (name[i - 1] == ' ') {
                name.erase(i - 1, 1);
                i--;
            }
            else
                name[i] = ' ';
        }
        else if (i > 0) {
            if (c >= 'A' && c <= 'Z' && name[i - 1] >= 'a' && name[i - 1] <= 'z') {
                name.insert(i, " ");
            }
        }

        c = static_cast<char>(std::tolower(c));
    }

    name[0] = static_cast<char>(std::toupper(name[0]));
    for (size_t i = 1; i < name.size(); i++) {
        if (name[i - 1] == ' ')
            name[i] = static_cast<char>(std::toupper(name[i]));
    }

    return name;
}
