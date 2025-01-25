#pragma once
#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace mgm {
    class MgmGPUShaderBuilder {
        public:

        struct Error {
            size_t pos{};
            size_t line{}, column{};
            std::string message{};

            Error(size_t i, std::string error_message, const std::string& original_str);
        };
        std::vector<Error> errors{};


        struct Buffer {
            std::string name{};
            std::string type_name{};
        };
        // A map of buffer locations pointing to buffer information
        std::unordered_map<size_t, Buffer> buffers{};


        struct Parameter {
            std::string type_name{};
        };
        // A map of parameter names pointing to other information about the parameter
        std::unordered_map<std::string, Parameter> parameters{};


        struct Texture {
            size_t dimensions = 2;
        };
        // A map of texture names pointing to texture information
        std::unordered_map<std::string, Texture> textures{};


        struct Struct {
            struct Member {
                std::string type_name{};
            };
            // A map of member names pointing to information about the member
            std::unordered_map<std::string, Member> members{};
        };
        // A map of struct names pointing to information about the structs
        std::unordered_map<std::string, Struct> structs{};


        struct Line {
            // A stack of operations
            // Processing this stack is done by walking it from the bottom to the top
            // When encountering an operator, the previous 2 items in the stack are the left and right operands
            // That operator and its operands must be replaced with the result ("a", "b", "+" will be replaced with "a+b")
            // Some operators, like the call operator "()", requires the item right before it to be the number of arguments that should be given to it, and the one before that to be the function name
            // Example for call operator: "a", "b", "c", "foo", "2", "()" will be replaced with "foo(b, c)". If the number was 3 instead, it would be replaced with "foo(a, b, c)"
            // The call operator called with an empty function name just groups the parameters in the parenthesies for the operation
            // Example for group operator: "a", "b", "c", "", "1", "()" will be replaced with "(c)". This is useful if the value of c is something like "x + y * z"
            // The group operator will fail if the number of arguments isn't "1"
            // When only one item remains in the stack, processing is complete, and the line can be added to the cross-compiled result
            // The "var" operator expects the operation right before it to be the type of the variable, the one before that the name, and the one before that to be the value, or an empty string if the default value should be used
            // Lastly, there is the "return" operator, which should return only the item right before it on the stack
            std::vector<std::string> operations{};

            // When this is set and operations are empty, it is treaded as an error message, and used internally, so this situation should never be encountered if no errors are in the errors vector
            // When state and operations are both set, the state points to the pseudo-function containing the body of the conditional, and the operations contain only one element, representing the condition of the conditional
            // You can check if the pseudo-function is an "if", or a "while" by checking if state ends with "if" or "while"
            std::string state{};
        };

        struct Function {
            std::string return_type{};
            // A vector of lines of code in the order that they were found in the source
            std::vector<Line> lines{};

            struct FunctionParameter {
                std::string name{};
                std::string type_name{};
            };
            // A vector of function parameter names and type names, in the order that they were found in the source
            std::vector<FunctionParameter> function_parameters{};

            static inline const std::vector<std::vector<std::string>> ops {
                {","},
                {"="},
                {"==", "!=", "+=", "-=", "<=", ">=", "<", ">"},
                {"+", "-"},
                {"*", "/"},
                {"."},
                {"[]"}
            };
            static inline const std::vector<std::string> all_ops {
                "=", "==", "!=", "+=", "-=", "<=", ">=", "<", ">", "+", "-", "*", "/", ".", "[]", "()"
            };
        };

        // A map of function names pointing to function bodies
        std::unordered_map<std::string, Function> functions{};

        // A map of function names pointing to pseudo-function bodies (a pseudo-function is a function created by something like a branch body, or while loop body)
        std::unordered_map<std::string, Function> pseudo_functions{};


        std::unordered_set<std::string> allowed_type_names{};


        using LoadFunc = std::function<std::string(std::string path)>;
        LoadFunc load_func{};


        MgmGPUShaderBuilder();

        /**
         * @brief Provide a function that loads a secondary source file using only relative paths, or returns an empty string to signal that the file doesn't exist
         * 
         * @param func The function to call
         */
        void set_load_function(const LoadFunc& func) { load_func = func; }

        /**
         * @brief Build shader source into source information
         * 
         * @param source The shader source code
         */
        void build(const std::string& source);

        private:

        Line parse_word_list(const std::string& name, const std::vector<std::string>& words);

        Line parse_line(const std::string& name, const std::string& line);

        /**
         * @brief Build the body of the given function into
         * 
         * @param name The name of the function
         * @param body The body of the function
         */
        std::vector<Line> build_function_contents(const std::string& name, const std::string& body);
    };
}
