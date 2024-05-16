#include "lexers.h"

using namespace Lexers;

cfgLex::cfgLex() {}

cfgLex::Token const cfgLex::nextToken() {
    if (line >= arr.size()) {
        return Token("EOF", TOK_EOF);
    }
    const QString str = arr.at(line);
    QString tok("");
    LexState state = STATE_START;
    while (state != STATE_ERR) {
        const QChar c = col < str.size() ? str.at(col) : '\n';
        const LexInput input = charToInput(c);
        if (state == STATE_START && (input == IN_SEMI || input == IN_EOL)) {
            // empty token
            advanceOneChar();
            return Token("endcmd", TOK_END);
        }
        const LexState next = STATE[state][input];
        switch (next) {
        case STATE_CHAR:
        case STATE_ESCP:
            tok.append(c);
            break;
        case STATE_END:
            if (input == IN_QUOT) {
                // we're ending a quote-escaped token
                tok.append(c);
                advanceOneChar();
            }
            return Token(tok, TOK_STR);
        default:
            break;
        }
        advanceOneChar();
        state = next;
    }
    return Token("error", TOK_ERR);
}

inline cfgLex::LexInput cfgLex::charToInput(const QChar c) const {
    if (c == '\n') {
        return IN_EOL;
    }
    if (c == '"') {
        return IN_QUOT;
    }
    if (c == ';') {
        return IN_SEMI;
    }
    if (c.isSpace()) {
        return IN_WHITESPACE;
    }
    return IN_CHAR;
}
