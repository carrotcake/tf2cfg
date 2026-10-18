#include "lexers.h"

using namespace Lexers;

// Returned by value, not `const` value: a const return blocks the move and copies the
// token's QString on every call (review finding 14).
cfgLex::Token cfgLex::nextToken() {
    if (line >= arr.size()) {
        return Token("EOF", Token::TOK_EOF);
    }
    const QString str = arr.at(line);
    QString tok("");
    LexState state = STATE_START;
    bool escaped = false;
    while (state != STATE_ERR) {
        const QChar c = col < str.size() ? str.at(col) : '\n';
        // Inside a quoted token a backslash escapes the next character, so
        // `alias t "say \"hi\""` stays one token instead of splitting at the escaped
        // quote. The backslash is kept: a token is what the line says, and unquoted()
        // (src/ui/commandlistmodel.cpp) is the one place that reads the escapes.
        if (escaped) {
            if (c != '\n') {
                tok.append(c);
            }
            escaped = false;
            advanceOneChar();
            continue;
        }
        if (state == STATE_ESCP && c == '\\') {
            escaped = true;
            tok.append(c);
            advanceOneChar();
            continue;
        }
        const LexInput input = charToInput(c);
        if (state == STATE_START && (input == IN_SEMI || input == IN_EOL)) {
            // empty token
            advanceOneChar();
            return Token("endcmd", Token::TOK_END);
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
            return Token(tok, Token::TOK_STR);
        default:
            break;
        }
        advanceOneChar();
        state = next;
    }
    return Token("error", Token::TOK_ERR);
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
