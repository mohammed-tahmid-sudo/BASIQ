#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <algorithm>

class Lexer {
public:
    std::vector<std::vector<std::string>> lexer(const std::string &input) {
        std::vector<std::vector<std::string>> output;
        std::string holder;

        auto flushIdentifier = [&]() {
            if (holder.empty()) return;

            std::string uholder = holder;
            std::transform(uholder.begin(), uholder.end(), uholder.begin(), ::toupper);

            if (uholder == "VAR") output.push_back({"VarKeyword", holder});
            else if (uholder == "AS") output.push_back({"AsKeyword", holder});
            else if (uholder == "IF") output.push_back({"IfKeyword", holder});
            else if (uholder == "ELSE") output.push_back({"ElseKeyword", holder});
            else if (uholder == "THEN") output.push_back({"ThenKeyword", holder});
            else if (uholder == "WHILE") output.push_back({"WhileKeyword", holder});
            else if (uholder == "DO") output.push_back({"DoKeyword", holder});
            else if (uholder == "END") output.push_back({"EndKeyword", holder});
            else if (uholder == "RETURN") output.push_back({"ReturnKeyword", holder});
            else if (uholder == "INTEGER" || uholder == "FLOAT" || uholder == "BOOLEAN" || uholder == "CHAR")
                output.push_back({"Type", holder});
            else if (uholder == "TRUE" || uholder == "FALSE")
                output.push_back({"Boolean", holder});
            else if (std::all_of(holder.begin(), holder.end(), ::isdigit))
                output.push_back({"Number", holder});
            else
                output.push_back({"Identifier", holder});

            holder.clear();
        };

        for (size_t i = 0; i < input.size(); ++i) {
            char c = input[i];

            if (std::isalnum((unsigned char)c) || c == '_') {
                holder.push_back(c);
            }
            else if (std::isspace((unsigned char)c)) {
                flushIdentifier();
            }
            else if (c == '"') { // String literal
                flushIdentifier();
                holder.push_back(c);
                ++i;
                while (i < input.size() && input[i] != '"') {
                    holder.push_back(input[i++]);
                }
                if (i < input.size()) holder.push_back('"');
                output.push_back({"String", holder});
                holder.clear();
            }
            else if (c == '\'') { // Char literal
                flushIdentifier();
                holder.push_back(c);
                ++i;
                if (i < input.size()) holder.push_back(input[i++]); // the character
                if (i < input.size() && input[i] == '\'') holder.push_back(input[i]);
                output.push_back({"Char", holder});
                holder.clear();
            }
            else { // Operators / punctuation
                flushIdentifier();
                if (c == '=' && i + 1 < input.size() && input[i + 1] == '=') {
                    output.push_back({"Equals", "=="});
                    ++i;
                }
                else if (c == '>' && i + 1 < input.size() && input[i + 1] == '=') {
                    output.push_back({"GreaterOrEqual", ">="});
                    ++i;
                }
                else if (c == '<' && i + 1 < input.size() && input[i + 1] == '=') {
                    output.push_back({"LessOrEqual", "<="});
                    ++i;
                }
                else {
                    switch (c) {
                        case '(': output.push_back({"LParen", "("}); break;
                        case ')': output.push_back({"RParen", ")"}); break;
                        case '+': output.push_back({"Plus", "+"}); break;
                        case '-': output.push_back({"Minus", "-"}); break;
                        case '*': output.push_back({"Multiply", "*"}); break;
                        case '/': output.push_back({"Divide", "/"}); break;
                        case '=': output.push_back({"Assign", "="}); break;
                        case '>': output.push_back({"GreaterThan", ">"}); break;
                        case '<': output.push_back({"LessThan", "<"}); break;
                        case ';': output.push_back({"Semicolon", ";"}); break;
                        default: holder.push_back(c); break; // Unknown / custom
                    }
                }
            }
        }

        flushIdentifier();
        output.push_back({"EOF", ""});
        return output;
    }
};

// Example usage
// int main() {
//     Lexer lex;
//     auto tokens = lex.lexer("VAR x = 5 AS INTEGER; IF x > 3 THEN RETURN x; END;");
//     for (auto &tok : tokens) std::cout << tok[0] << " : " << tok[1] << "\n";
// }

