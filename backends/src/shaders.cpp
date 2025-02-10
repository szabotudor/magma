#include "shaders.hpp"
#include <algorithm>
#include <string>


namespace mgm {
    namespace shader_char_help {
        inline bool is_whitespace(char c) {
            return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
        }
        inline bool is_alpha(char c) {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        }
        inline bool is_num(char c) {
            return c >= '0' && c <= '9';
        }
        inline bool is_alphanum(char c) {
            return is_num(c) || is_alpha(c);
        }
        inline bool is_sym(char c) {
            return !is_alphanum(c) && !is_whitespace(c);
        }
    }
    using namespace shader_char_help;

    std::string first_word_at(const std::string& str, size_t& i) {
        const auto started_at = i;

        while (is_whitespace(str[i]))
            ++i;

        if (i >= str.size())
            return "";

        const auto started_after_whitespace = i;

        if (is_alpha(str[i]) || str[i] == '_') {
            const auto start = i;
            while (is_alphanum(str[i]) || str[i] == '_')
                ++i;
            return str.substr(start, i - start);
        }

        if (is_num(str[i])) {
            const auto start = i++;
            bool has_alpha = false, passed_point = false, found_float_spec = false;
            while ((is_alphanum(str[i]) || (!has_alpha && str[i] == '.') || (passed_point && str[i] == 'f')) && !found_float_spec) {
                if (!passed_point && str[i] == '.')
                    passed_point = true;
                else if (passed_point && str[i] == 'f')
                    found_float_spec = true;
                else if (is_alpha(str[i]))
                    has_alpha = true;
                ++i;
            }
            return str.substr(start, i - start);
        }

        if (is_sym(str[i])) {
            const auto start = i;
            char start_c = str[i];
            char end_c = ' ';
            switch (start_c) {
                case '(': end_c = ')'; break;
                case '[': end_c = ']'; break;
                case '{': end_c = '}'; break;
                case '"': {
                    while (str[i] != '"') {
                        if (str[i] == '\\')
                            ++i;
                        ++i;
                        if (i >= str.size()) {
                            i = started_at;
                            return "Expected closing quotes";
                        }
                    }
                    break;
                }
                case '/': {
                    if (i + 1 < str.size()) {
                        ++i;
                        if (str[i] == '/') {
                            while (i < str.size() && str[i] != '\n')
                                ++i;
                            if (i >= str.size())
                                return "";
                            return first_word_at(str, i);
                        }
                        else if (str[i] == '*') {
                            while (i < str.size() - 1 && !(str[i] == '*' && str[i + 1] == '/'))
                                ++i;
                            if (i >= str.size())
                                return "";
                            return first_word_at(str, i);
                        }
                    }
                }
                default: break;
            }

            if (end_c == ' ') {
                i = started_after_whitespace + 1;
                if (str[i] == '=') {
                    switch (start_c) {
                        case '=': ++i; return "==";
                        case '!': ++i; return "!=";
                        case '+': ++i; return "+=";
                        case '-': ++i; return "-=";
                        case '>': ++i; return ">=";
                        case '<': ++i; return "<=";
                        default: break;
                    }
                }
                return std::string{start_c};
            }
            
            size_t count = 1;
            ++i;
            while (count) {
                if (i >= str.size()) {
                    i = started_at;
                    return "Expected '" + std::string{end_c} + "' to close '" + std::string{start_c, '\''};
                }
                if (str[i] == start_c)
                    ++count;
                else if (str[i] == end_c)
                    --count;
                ++i;
            }
            return str.substr(start, i - start);
        }

        i = started_at;
        return "Broken character found";
    }

    std::string get_brace_contents(const std::string& str) {
        size_t start = 1;
        size_t end = str.size() - 2;

        while (is_whitespace(str[start]))
            ++start;
        while (is_whitespace(str[end]))
            --end;

        if (!is_whitespace(str[end]))
            ++end;

        if (start >= end)
            return "";

        return str.substr(start, end - start);
    }

    enum class WordType {
        NONE, NUMBER, NAME, NUMBERED_NAME, BRACE, STRING, SYMBOL
    };
    WordType get_word_type(const std::string& str) {
        if (str.empty())
            return WordType::NONE;
        if ((str.starts_with('(') && str.ends_with(')'))
        || (str.starts_with('[') && str.ends_with(']'))
        || (str.starts_with('{') && str.ends_with('}'))
        || (str.starts_with('<') && str.ends_with('>')))
            return WordType::BRACE;
        
        if (str.starts_with('"') && str.ends_with('"'))
            return WordType::STRING;

        if (is_alpha(str.front()) || str.starts_with('_')) {
            bool alphanum = true;
            for (const auto c : str) {
                if (!is_alphanum(c) && c != '_') {
                    alphanum = false;
                    break;
                }
            }
            if (alphanum)
                return WordType::NAME;
        }

        if (is_num(str.front())) {
            size_t i = 0;
            bool has_alpha = false, passed_point = false, found_float_spec = false;
            while ((is_alphanum(str[i]) || (!has_alpha && str[i] == '.') || (passed_point && str[i] == 'f')) && !found_float_spec) {
                if (!passed_point && str[i] == '.')
                    passed_point = true;
                else if (passed_point && str[i] == 'f')
                    found_float_spec = true;
                else if (is_alpha(str[i]))
                    has_alpha = true;
                ++i;
            }
            if (!has_alpha || passed_point)
                return WordType::NUMBER;
            return WordType::NUMBERED_NAME;
        }

        if (is_sym(str.front())) {
            if (str.size() == 1 || (str.size() == 2 && is_sym(str.back())))
                return WordType::SYMBOL;
        }

        return WordType::NONE;
    }


    std::vector<std::string> build_mode(const std::vector<WordType>& word_sequence, const std::vector<std::string>& error_messages, std::vector<MgmGPUShaderBuilder::Error>& errors, const std::string& str, size_t& i) {
        std::vector<std::string> words{};
        
        for (size_t w = 0; w < word_sequence.size(); ++w) {
            const auto start_i = i;
            const auto word = first_word_at(str, i);

            if (start_i == i) {
                errors.emplace_back(i, word, str);
                return {};
            }

            if (get_word_type(word) != word_sequence[w]) {
                errors.emplace_back(i, error_messages[w], str);
                return {};
            }

            words.emplace_back(word);
        }

        return words;
    }


    MgmGPUShaderBuilder::Error::Error(size_t i, std::string error_message, const std::string& original_str) : pos(i), message(error_message) {
        i = std::min(i, original_str.size());
        for (size_t j = 0; j < i; ++j) {
            ++column;
            if (original_str[j] == '\n') {
                ++line;
                column = 0;
            }
        }
    }


    MgmGPUShaderBuilder::MgmGPUShaderBuilder() {
        allowed_type_names.emplace("float");
        allowed_type_names.emplace("double");

        allowed_type_names.emplace("int8");
        allowed_type_names.emplace("int16");
        allowed_type_names.emplace("int32");
        allowed_type_names.emplace("int64");
        allowed_type_names.emplace("uint8");
        allowed_type_names.emplace("uint16");
        allowed_type_names.emplace("uint32");
        allowed_type_names.emplace("uint64");

        allowed_type_names.emplace("vec2");
        allowed_type_names.emplace("vec3");
        allowed_type_names.emplace("vec4");
        allowed_type_names.emplace("vec2d");
        allowed_type_names.emplace("vec3d");
        allowed_type_names.emplace("vec4d");

        allowed_type_names.emplace("mat2");
        allowed_type_names.emplace("mat3");
        allowed_type_names.emplace("mat4");
        allowed_type_names.emplace("mat2d");
        allowed_type_names.emplace("mat3d");
        allowed_type_names.emplace("mat4d");
    }


    // Something went wrong, because there is no word
    // Word variable instead has error message
    #define HANDLE_BROKEN_WORD(start, end, word) \
    if (end == start) { \
        errors.emplace_back(start, word, source); \
        return; \
    }


    MgmGPUShaderBuilder::Struct build_struct_body([[maybe_unused]] const std::string& body) {
        // TODO: Implement struct bodies
        return {};
    }

    void MgmGPUShaderBuilder::build(const std::string& source) {
        size_t i = 0;

        while (i < source.size()) {
            auto start_i = i;
            const auto word = first_word_at(source, i);
            if (word.empty() && i >= source.size())
                break;
            HANDLE_BROKEN_WORD(start_i, i, word)


            if (word == "struct") {
                const auto words = build_mode(
                    {WordType::NAME, WordType::BRACE},
                    {
                        "Expected name of struct",
                        "Expected struct body"
                    },
                    errors,
                    source,
                    i
                );

                if (words.empty())
                    continue;

                auto& s = structs[words[0]];
                s = build_struct_body(get_brace_contents(words[1]));
                allowed_type_names.emplace(words[0]);
            }


            else if (word == "buffer") {
                const auto words = build_mode(
                    {WordType::NAME, WordType::NAME},
                    {
                        "Expected a type name after `buffer`",
                        "Expected buffer name after type"
                    },
                    errors,
                    source,
                    i
                );

                if (words.empty())
                    continue;

                const auto location = get_brace_contents(words[0]);

                buffers[words[0]] = Buffer{words[1]};
                continue;
            }


            else if (word == "parameter") {
                const auto words = build_mode(
                    {WordType::NAME, WordType::NAME},
                    {
                        "Expected a type after `parameter`",
                        "Expected parameter name after type"
                    },
                    errors,
                    source,
                    i
                );

                if (words.empty())
                    continue;

                parameters[words[1]].type_name = words[0];
            }

            else if (word == "texture") {
                const auto words = build_mode(
                    {WordType::BRACE, WordType::NAME},
                    {
                        "Expected dimension and location specifiers after `texture`",
                        "Expected texture name"
                    },
                    errors,
                    source,
                    i
                );

                if (words.empty())
                    continue;

                const auto specs = get_brace_contents(words[0]);

                size_t j = 0;
                const auto specs_words = build_mode(
                    {WordType::NUMBERED_NAME},
                    {
                        "Expected dimensions (2D/3D)"
                    },
                    errors,
                    specs,
                    j
                );
                
                if (specs_words.empty())
                    continue;

                size_t dimensions = 2;
                if (specs_words[0] == "2D")
                    dimensions = 2;
                else if (specs_words[1] == "3D")
                    dimensions = 3;

                textures[words[1]] = Texture{.dimensions = dimensions};
            }


            else if (word == "func") {
                const auto words = build_mode(
                    {WordType::NAME, WordType::NAME, WordType::BRACE, WordType::BRACE},
                    {
                        "Expected return type after `func`",
                        "Expected function name after return type",
                        "Expected function parameters after function name",
                        "Expected function body after the parameters"
                    },
                    errors,
                    source,
                    i
                );

                if (words.empty())
                    continue;

                auto& func = functions[words[1]];
                func.return_type = words[0];

                bool something_went_wrong = false;

                const auto params = get_brace_contents(words[2]);
                if (!params.empty()) {
                    size_t j = 0;

                    while (true) {
                        const auto params_words = build_mode(
                            {WordType::NAME, WordType::NAME},
                            {
                                "Expected parameter type",
                                "Expected parameter name after type"
                            },
                            errors,
                            params,
                            j
                        );

                        if (params_words.empty()) {
                            something_went_wrong = true;
                            break;
                        }

                        func.function_parameters.emplace_back(params_words[1], params_words[0]);

                        const auto comma = first_word_at(params, j);

                        if (comma != ",")
                            break;
                    }
                }

                if (something_went_wrong) {
                    functions.erase(words[1]);
                    continue;
                }

                const auto error_count = errors.size();
                func.lines = build_function_contents(words[1], get_brace_contents(words[3]));

                // Errors generated by "build_function_contents" will be offset depending on where the function body starts
                const auto start_func = i - words[3].size() - 1;
                for (size_t e = error_count; e < errors.size(); ++e) {
                    auto& error = errors[e];
                    error.pos += start_func;
                    for (size_t j = start_func; j < error.pos; ++j) {
                        ++error.column;
                        if (source[j] == '\n') {
                            error.column = 0;
                            ++error.line;
                        }
                    }
                }
            }


            else if (word.starts_with("//") || word.starts_with("/*"))
                continue;


            else
                errors.emplace_back(start_i, "Unknown word \"" + word + "\"", source);
        }
    }


    thread_local size_t lcount = 0;
    
    MgmGPUShaderBuilder::Line MgmGPUShaderBuilder::parse_word_list(const std::string& name, const std::vector<std::string>& words) {
        if (words.empty())
            return {};

        else if (words.size() == 1) {
            const auto type = get_word_type(words.front());
            if (type == WordType::BRACE && words.front().starts_with('(')) {
                auto group_content = parse_line(name, get_brace_contents(words.front()));
                group_content.operations.emplace_back("");
                group_content.operations.emplace_back(std::to_string(group_content.operations.size() - 1));
                group_content.operations.emplace_back("()");
                return group_content;
            }
            else if (type == WordType::NAME || type == WordType::NUMBERED_NAME || type == WordType::NUMBER || type == WordType::STRING)
                return {.operations = words};
        }
        else if (words.size() == 2 && (get_word_type(words[0]) == WordType::NAME || get_word_type(words[0]) == WordType::NUMBERED_NAME) && get_word_type(words[1]) == WordType::BRACE && words[1].starts_with('(')) {
            auto group_content = parse_line(name, get_brace_contents(words[1]));
            group_content.operations.emplace_back(words[0]);
            group_content.operations.emplace_back(std::to_string(group_content.operations.size() - 1));
            group_content.operations.emplace_back("()");
            return group_content;
        }
        
        Line res{};

        for (const auto& priority : Function::ops) {
            for (const auto& op : priority) {
                auto last_it = words.begin();
                auto it = std::find(words.begin(), words.end(), op);

                if (it == words.end())
                    continue;

                while (last_it != words.end()) {
                    std::vector<std::string> left {last_it, it};

                    for (const auto& rw : parse_word_list(name, left).operations)
                        res.operations.emplace_back(rw);

                    if (last_it != words.begin() && &op != &Function::ops.front().front())
                        res.operations.emplace_back(op);

                    if (it == words.end())
                        break;
                    last_it = it + 1;
                    if (last_it != words.end())
                        it = std::find(last_it, words.end(), op);
                }
            }

            if (!res.operations.empty() || !res.state.empty())
                return res;
        }
        return {.state = "Expected some operator"};
    }

    MgmGPUShaderBuilder::Line MgmGPUShaderBuilder::parse_line(const std::string& name, const std::string& line) {
        size_t i = 0;
        auto extract_word = first_word_at(line, i);
        if (extract_word.empty() && i >= line.size())
            return {};

        if (i == 0)
            return {.state = extract_word};

        std::vector<std::string> words{};

        while (true) {
            if (get_word_type(extract_word) == WordType::BRACE && extract_word.starts_with('[')) {
                const auto first = get_brace_contents(extract_word);
                words.emplace_back("[]");
                words.emplace_back("(" + get_brace_contents(extract_word) + ")");
            }
            else
                words.emplace_back(extract_word);

            const auto start_i = i;
            extract_word = first_word_at(line, i);

            if (extract_word.empty())
                break;

            if (start_i == i)
                return {.state = extract_word};
        }

        const auto first_word_type = get_word_type(words[0]);

        if (words.size() == 1 && (first_word_type == WordType::NAME || first_word_type == WordType::NUMBERED_NAME || first_word_type == WordType::NUMBER || first_word_type == WordType::STRING))
            return {.operations = words};

        if (first_word_type != WordType::NAME && first_word_type != WordType::NUMBERED_NAME) {
            return {.state = "Unexpected token at beginning of line"};
        }

        if (words.front() == "if" || words.front() == "while") {
            const auto new_name = name + "::" + std::to_string(lcount++) + words.front();

            pseudo_functions[new_name].lines = build_function_contents(name, get_brace_contents(words[2]));

            const auto parsed_cond = parse_line(name, get_brace_contents(words[1]));
            return {.operations = parsed_cond.operations, .state = new_name};
        }
        else if (words.front() == "var") {
            const auto var_type = words[1];
            const auto var_name = words[2];
            if (words.size() > 3 && words[3] != "=")
                return {.state = "Unknown token after variable declaration. Expected '=' assignment operator"};

            if (words.size() > 4)
                words.erase(words.begin(), words.begin() + 4);
            else
                words.clear();

            auto parsed_definition = parse_word_list(name, words);
            if (parsed_definition.operations.empty())
                parsed_definition.operations.emplace_back("");
            parsed_definition.operations.emplace_back(var_name);
            parsed_definition.operations.emplace_back(var_type);
            parsed_definition.operations.emplace_back("var");

            return parsed_definition;
        }

        return parse_word_list(name, words);
    }


    std::vector<MgmGPUShaderBuilder::Line> MgmGPUShaderBuilder::build_function_contents(const std::string& name, const std::string& body) {
        std::vector<Line> commands{};
        size_t i = 0;
        auto start_i = i;
        auto word = first_word_at(body, i);
        if (word.empty() && i >= body.size())
            return {};

        while (i < body.size()) {
            const auto this_start_i = i;
            if (start_i == i) {
                errors.emplace_back(start_i, word, body);
                continue;
            }

            std::string line{};
            lcount = 0;

            bool declaring_var = false,
                expecting_conditional_body = false;
            
            auto type = get_word_type(word);
            while (true) {
                const auto next_word = first_word_at(body, i);
                const auto next_type = get_word_type(next_word);

                if (!line.empty())
                    line += ' ';
                line += word;

                switch (type) {
                    case WordType::NUMBER:
                    case WordType::NAME:
                    case WordType::NUMBERED_NAME:
                    case WordType::STRING: {
                        if (!(next_type == WordType::SYMBOL || next_type == WordType::BRACE
                        || word == "return" || word == "var" || declaring_var
                        || word == "if" || word == "while" || expecting_conditional_body)) {
                            word = next_word;
                            goto after;
                        }
                        declaring_var = (word == "var");
                        expecting_conditional_body = (word == "if" || word == "while");
                        break;
                    }
                    case WordType::BRACE: {
                        switch (next_type) {
                            case WordType::BRACE: {
                                if (!next_word.starts_with('[') && !(word.ends_with(')') && next_word.starts_with('{'))) {
                                    word = next_word;
                                    goto after;
                                }
                                    break;
                            }
                            case WordType::SYMBOL: {
                                break;
                            }
                            default: {
                                word = next_word;
                                goto after;
                            }
                        }
                        break;
                    }
                    case WordType::SYMBOL: {
                        if (next_type == WordType::BRACE && !next_word.starts_with('(')) {
                            word = next_word;
                            goto after;
                        }
                    }
                    default: {
                        break;
                    }
                }

                word = next_word;
                type = next_type;
            }

            after:

            Line parsed_line{};
            if (line.starts_with("return ")) {
                parsed_line = parse_line(name, line.substr(7));
                parsed_line.operations.emplace_back("return");
            }
            else
                parsed_line = parse_line(name, line);

            if (!parsed_line.state.empty() && parsed_line.operations.empty())
                errors.emplace_back(this_start_i, parsed_line.state, body);
            commands.emplace_back(parsed_line);
        }

        return commands;
    }
}
