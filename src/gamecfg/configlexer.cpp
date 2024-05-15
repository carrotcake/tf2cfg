#include "configlexer.h"

cfgLexer::cfgLexer() {}

bool cfgLexer::open(const QString &name)
{
    in_file.setFileName(name);
    if(!in_file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QTextStream in(&in_file);
    while(!in.atEnd()){
        QString str = in.readLine();
        if (str.contains("//"))
            str.chop(str.size() - str.indexOf("//"));
        arr += str;
    }
    line = 0;
    col = 0;
    return true;
}

cfgLexer::Token cfgLexer::next_token(){

    if(line >= arr.size())
        return Token("EOF", TOK_EOF);
    const QString str = arr.at(line);
    QString tok("");
    LexerState state = STATE_START;
    while(state != STATE_ERR){
        const QChar c = col < str.size() ? str.at(col) : '\n';
        const LexerInput input = char_to_input(c);
        if(state == STATE_START && (input == IN_SEMI || input == IN_EOL)){
            if(col < str.size()) {
                col++;
            } else {
                col = 0; line++;
            }
            return Token("endcmd", TOK_END);
        }
        const LexerState next = STATE[state][input];
        switch (next) {
        case STATE_CHAR:
        case STATE_ESCP:
            tok.append(c);
            break;
        case STATE_END:
            if(input == IN_QUOT){
                tok.append(c);
                col++;
            }
            return Token(tok, TOK_STR);
        default:
            break;
        }
        if(col < str.size()) {
            col++;
        } else {
            col = 0; line++;
        }
        state = next;
    }
    return Token("error", TOK_ERR);
}

bool cfgLexer::has_next(){
    return line < arr.size();
}

cfgLexer::LexerInput cfgLexer::char_to_input(const QChar c){

    if(c == '\n')
        return IN_EOL;
    if(c == '"')
        return IN_QUOT;
    if(c == ';')
        return IN_SEMI;
    if(c.isSpace())
        return IN_WHITESPACE;
    return IN_CHAR;
}
