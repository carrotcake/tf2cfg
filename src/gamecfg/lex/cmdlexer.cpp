#include "lexers.h"

using namespace Lexers;

cmdLex::Token const cmdLex::nextToken() {
    if (line >= arr.size()) {
        return Token(Token::TOK_EOF);
    }
    const QString str = arr.at(line);
    QStringList arr;
    QString temp("");
    LexState state = STATE_START;
    while (state != STATE_ERR) {
        const QChar c = col < str.size() ? str.at(col) : '\n';
        const LexInput input = charToInput(c);
        if (state == STATE_START && input == IN_EOL) {
            // nextlineplease
            advanceOneChar();
            continue;
        }
        const LexState next = STATE[state][input];
        switch (next) {
        case STATE_VAL:
            if (input != IN_QUOT) {
                temp.append(c);
            }
            break;
        case STATE_COMMA:
            arr.push_back(temp);
            temp.clear();
            break;
        case STATE_END:
            if (line > 2099) {
                int a = 2 + 3;
            }
            advanceOneChar();
            return Token(arr[0], arr[1], arr[2], arr[3], arr[4], temp);
        default:
            break;
        }
        advanceOneChar();
        state = next;
    }
    return Token(Token::TOK_ERR);
}

inline cmdLex::LexInput cmdLex::charToInput(const QChar c) const {
    if (c == '\n') {
        return IN_EOL;
    }
    if (c == '"') {
        return IN_QUOT;
    }
    if (c == ',') {
        return IN_COMMA;
    }
    return IN_CHAR;
}
