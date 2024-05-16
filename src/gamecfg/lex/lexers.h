#ifndef LEXERS_H
#define LEXERS_H
#include <QApplication>
#include <QFile>
#include <QTextStream>

namespace Lexers {

class Lexer {
public:
    Lexer(){};
    bool open(const QString &name) {
        in_file.setFileName(name);
        if (!in_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }
        QTextStream in(&in_file);
        while (!in.atEnd()) {
            QString str = in.readLine();
            if (str.contains("//")) {
                str.chop(str.size() - str.indexOf("//"));
            }

            arr += str;
        }
        line = 0;
        col = 0;
        return true;
    }
    bool hasNext() const { return line < arr.size(); }
    void close() { arr.clear(); }

protected:
    int line, col;
    QFile in_file;
    QStringList arr;
    inline void advanceOneChar() {
        if (col < arr.at(line).size()) {
            col++;
        } else {
            col = 0;
            line++;
        }
    }
};

class cfgLex : public Lexer {
public:
    cfgLex();

    enum TokenClass { TOK_ERR, TOK_EOF, TOK_STR, TOK_END };

    struct Token {
    private:
        QString str;
        TokenClass type;

    public:
        Token(const QString s, const TokenClass t)
            : str(s)
            , type(t){};
        inline bool isEndToken() const { return type == TOK_END; }
        inline QString const &string() const { return str; }
    };

    const Token nextToken();

private:
    enum LexState {
        STATE_START,
        STATE_ERR,
        STATE_CHAR,
        STATE_ESCP,
        STATE_END,
        STATECOUNT // not a valid state
    };

    enum LexInput {
        IN_WHITESPACE,
        IN_CHAR,
        IN_QUOT,
        IN_SEMI,
        IN_EOL,
        IN_EOF,
        INCOUNT // not a valid input type
    };

    static constexpr const LexState STATE[STATECOUNT][INCOUNT] =
        //IN_WHITESPACE   IN_CHAR     IN_QUOT     IN_SEMI     IN_EOL      IN_EOF
        {{STATE_START, STATE_CHAR, STATE_ESCP, STATE_END, STATE_END, STATE_END},
         {STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR},
         {STATE_END, STATE_CHAR, STATE_ESCP, STATE_END, STATE_END, STATE_END},
         {STATE_ESCP, STATE_ESCP, STATE_END, STATE_ESCP, STATE_ERR, STATE_ERR},
         {STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR}};

    inline LexInput charToInput(QChar c) const;
};
} // namespace Lexers
#endif // LEXERS_H
