#ifndef CONFIGLEXER_H
#define CONFIGLEXER_H
#include <QApplication>
#include <QFile>
#include <QTextStream>

class cfgLexer
{
public:
    cfgLexer();

    typedef enum TokenClass { TOK_ERR, TOK_EOF, TOK_STR, TOK_END } TokenClass;

    struct Token{
        QString str;
        TokenClass type;
        Token(const QString s, const TokenClass t):str(s),type(t){};
    };

    bool open(const QString &name);
    cfgLexer::Token next_token();
    bool has_next();
    void close();

private:
    typedef enum LexerState {
        STATE_START,
        STATE_ERR,
        STATE_CHAR,
        STATE_ESCP,
        STATE_END,
        STATECOUNT // not a valid state
    } LexerState;

    typedef enum LexerInput {
        IN_WHITESPACE,
        IN_CHAR,
        IN_QUOT,
        IN_SEMI,
        IN_EOL,
        IN_EOF,
        INCOUNT // not a valid input type
    } LexerInput;

    static constexpr const LexerState STATE[STATECOUNT][INCOUNT] =
        {{STATE_START,STATE_CHAR,STATE_ESCP,STATE_END,STATE_END,STATE_END},
         {STATE_ERR,STATE_ERR,STATE_ERR,STATE_ERR,STATE_ERR,STATE_ERR},
         {STATE_END,STATE_CHAR,STATE_ESCP,STATE_END,STATE_END,STATE_END},
         {STATE_ESCP,STATE_ESCP,STATE_END,STATE_ESCP,STATE_ERR,STATE_ERR},
         {STATE_ERR,STATE_ERR,STATE_ERR,STATE_ERR,STATE_ERR,STATE_ERR}};

    int line, col;
    QFile in_file;
    QStringList arr;

    cfgLexer::LexerInput char_to_input(QChar c);

};

#endif // CONFIGLEXER_H
