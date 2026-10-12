#include "lexers.h"

using namespace Lexers;

cmdLex::Token const cmdLex::nextToken() {
    if (line >= arr.size()) {
        return Token(Token::TOK_EOF);
    }
    const QString str = arr.at(line);
    QStringList fields; // v5 CSV has ten fields (res/cfg/schema.md)
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
            fields.push_back(temp);
            temp.clear();
            break;
        case STATE_END:
            advanceOneChar();
            fields.push_back(temp);
            // Tolerate short rows from hand-edited lines: missing trailing
            // fields become empty strings instead of a parse error.
            while (fields.size() < 10) {
                fields.push_back(QString());
            }
            return Token(fields[0], fields[1], fields[2], fields[3], fields[4], fields[5],
                         fields[6], fields[7], fields[8], fields[9]);
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
