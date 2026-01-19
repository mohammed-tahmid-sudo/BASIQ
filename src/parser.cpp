#include "lexer.h"
#include <iostream>

int main() {
    Lexer lex;
    auto statements = lex.lexerSplitStatements(
        "VAR A = 3 AS INTEGER; IF A < 10 THEN PRINT(\"Hello\"); END IF");

    for (size_t idx = 0; idx < statements.size(); ++idx) {
        std::cout << "Statement " << idx + 1 << ":\n";
        for (const auto &tok : statements[idx]) {
            std::cout << "  " << tokenTypeToString(tok.Type) << " : " << tok.Value << "\n";
        }
        std::cout << "---------------------\n";
    }
}

