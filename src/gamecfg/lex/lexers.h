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
            QString str = in.readLine().toLower();
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
        if (line >= arr.size()) {
            return;
        }
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
    cfgLex(){};

    struct Token {
    public:
        enum TokenClass { TOK_ERR, TOK_EOF, TOK_STR, TOK_END };
        Token(const QString s, const TokenClass t)
            : str(s)
            , type(t){};
        inline bool isEndToken() const { return type == TOK_END; }
        inline QString const &string() const { return str; }

    private:
        const QString str;
        const TokenClass type;
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
        // IN_WHITESPACE   IN_CHAR     IN_QUOT     IN_SEMI     IN_EOL      IN_EOF
        {{STATE_START, STATE_CHAR, STATE_ESCP, STATE_END, STATE_END, STATE_END},
         {STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR},
         {STATE_END, STATE_CHAR, STATE_ESCP, STATE_END, STATE_END, STATE_END},
         {STATE_ESCP, STATE_ESCP, STATE_END, STATE_ESCP, STATE_ERR, STATE_ERR},
         {STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR}};

    inline LexInput charToInput(QChar c) const;
};

class cmdLex : public Lexer {
public:
    cmdLex(){};

    struct Token {
    public:
        enum TokenClass { TOK_ERR, TOK_EOF, TOK_CMD };
        Token(const QString n,
              const QString c,
              const QString d,
              const QString mi,
              const QString ma,
              const QString docs)
            : cmd_name(n)
            , is_cmd(c)
            , def(d)
            , min(mi)
            , max(ma)
            , doc(docs)
            , type(TOK_CMD){};
        Token(const TokenClass t)
            : type(t){};
        inline bool isEndToken() const { return type == TOK_EOF; }
        inline QString const &name() const { return cmd_name; }
        inline QString const &defVal() const { return def; }
        inline QString const &minVal() const { return min; }
        inline QString const &maxVal() const { return max; }
        inline QString const &docStr() const { return doc; }
        inline bool isCmd() const { return !is_cmd.isEmpty(); }
        inline bool hasDefault() const { return !def.isEmpty(); };
        inline bool hasRange() const { return !min.isEmpty() && !max.isEmpty(); }
        inline bool hasUsage() const { return doc.contains("usage:"); };

    private:
        const QString cmd_name, is_cmd, def, min, max, doc;
        const TokenClass type;
    };

    const Token nextToken();

private:
    enum LexState {
        STATE_START,
        STATE_ERR,
        STATE_VAL,
        STATE_ENDQT,
        STATE_COMMA,
        STATE_END,
        STATECOUNT // not a valid state
    };

    enum LexInput {
        IN_CHAR,
        IN_QUOT,
        IN_COMMA,
        IN_EOL,
        IN_EOF,
        INCOUNT // not a valid input type
    };

    static constexpr const LexState STATE[STATECOUNT][INCOUNT] =
        // IN_CHAR     IN_QUOT     IN_COMMA    IN_EOL      IN_EOF
        /*start*/ {{STATE_ERR, STATE_VAL, STATE_ERR, STATE_START, STATE_END},
                   /* err */ {STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR},
                   /* val */ {STATE_VAL, STATE_ENDQT, STATE_VAL, STATE_VAL, STATE_ERR},
                   /*endqt*/ {STATE_ERR, STATE_ERR, STATE_COMMA, STATE_END, STATE_END},
                   /*comma*/ {STATE_ERR, STATE_VAL, STATE_ERR, STATE_ERR, STATE_ERR},
                   /* end */ {STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR, STATE_ERR}};

    inline LexInput charToInput(QChar c) const;
};

} // namespace Lexers
#endif // LEXERS_H
