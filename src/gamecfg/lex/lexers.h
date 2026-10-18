#ifndef LEXERS_H
#define LEXERS_H
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <utility>

namespace Lexers {

class Lexer {
public:
    Lexer(){};
    bool open(const QString &name) {
        in_file.setFileName(name);
        if (!in_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }
        tails.clear();
        QTextStream in(&in_file);
        while (!in.atEnd()) {
            QString str = in.readLine();
            tails += preprocess(str);
            arr += str;
        }
        line = 0;
        col = 0;
        return true;
    }
    bool hasNext() const { return line < arr.size(); }
    void close() { arr.clear(); }
    /// Parse-from-string entry point (docs/DESIGN-config-editor.md §8): feed raw lines
    /// instead of a file. Every line is preprocessed exactly like open() does, so a
    /// buffer tokenizes identically to the same text read from disk.
    void setBuffer(QStringList lines) {
        tails.clear();
        for (QString &str : lines) {
            tails += preprocess(str);
            arr += str;
        }
        line = 0;
        col = 0;
    }
    /// Comment tail of buffer line i (`// ...` raw text, "" when none); parallels the
    /// line buffer (docs/DESIGN-config-editor.md §6).
    QString tailAt(int i) const { return i >= 0 && i < tails.size() ? tails.at(i) : QString(); }
    /// 0-based index of the line the cursor is on. nextToken() reads one line per call,
    /// so this is the line a command starts on - which is what the surgical save keys
    /// app-owned rows to.
    int currentLine() const { return line; }

protected:
    int line, col;
    QFile in_file;
    QStringList arr;
    QStringList tails; ///< per-line comment tail, parallel to arr
    /// Per-line massaging before the line enters the buffer. The base keeps lines
    /// verbatim (descriptions in allcmds.csv must survive untouched) and reports no
    /// comment tail; cfgLex overrides both (docs/DESIGN-config-editor.md §6).
    virtual QString preprocess(QString &) { return QString(); }
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

    // A config line is folded to lower case only as far as its command name: the console
    // looks commands up case-insensitively, but an argument is text, so `say "Hello"` and
    // a say-text alias must keep their case or the loaded config no longer matches what
    // the file says. `//` starts a comment only outside a quoted token, the way the game's
    // own tokenizer treats it, so a URL in `alias x "echo http://host/"` survives. (Both
    // rules used to live in Lexer::open, where the case fold also corrupted allcmds.csv
    // descriptions.) The tail is returned raw, never folded, so comment rows re-render
    // verbatim (docs/DESIGN-config-editor.md §6).
    QString preprocess(QString &str) override {
        bool quoted = false;
        int cut = -1;
        for (int i = 0; i < str.size(); ++i) {
            const QChar c = str.at(i);
            if (c == QLatin1Char('"')) {
                quoted = !quoted;
            } else if (!quoted && c == QLatin1Char('/') && i + 1 < str.size()
                       && str.at(i + 1) == QLatin1Char('/')) {
                cut = i;
                break;
            }
        }
        QString tail;
        if (cut >= 0) {
            tail = str.mid(cut);
            str.truncate(cut);
        }

        // The name is the first unquoted word. A line that starts with a quote has no bare
        // command name to fold, and neither has a blank or comment-only line.
        int nameStart = 0;
        while (nameStart < str.size() && str.at(nameStart).isSpace())
            ++nameStart;
        if (nameStart < str.size() && str.at(nameStart) == QLatin1Char('"'))
            return tail;
        for (int i = nameStart; i < str.size() && !str.at(i).isSpace(); ++i)
            str[i] = str.at(i).toLower();
        return tail;
    }

    struct Token {
    public:
        enum TokenClass { TOK_ERR, TOK_EOF, TOK_STR, TOK_END };
        // Non-const members so a Token is movable and assignable (matches cmdLex::Token;
        // audit #4). str is moved on construction rather than copied.
        Token(QString s, TokenClass t)
            : str(std::move(s))
            , type(t) {};
        inline bool isEndToken() const { return type == TOK_END; }
        inline QString const &string() const { return str; }

    private:
        QString str;
        TokenClass type;
    };

    Token nextToken();

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
        // allcmds.csv v5 layout: name, type, default, min, max, flags, docs,
        // input type, platform, context (res/cfg/schema.md).
        Token(const QString n,
              const QString c,
              const QString d,
              const QString mi,
              const QString ma,
              const QString fl,
              const QString docs,
              const QString in,
              const QString pl,
              const QString cx)
            : cmd_name(n)
            , is_cmd(c)
            , def(d)
            , min(mi)
            , max(ma)
            , flags(fl)
            , doc(docs)
            , input(in)
            , platformStr(pl)
            , contextStr(cx)
            , type(TOK_CMD){};
        Token(const TokenClass t)
            : type(t){};
        inline bool isEndToken() const { return type == TOK_EOF; }
        inline QString const &name() const { return cmd_name; }
        inline QString const &defVal() const { return def; }
        inline QString const &minVal() const { return min; }
        inline QString const &maxVal() const { return max; }
        inline QString const &docStr() const { return doc; }
        inline QString const &flagsVal() const { return flags; }
        inline QString const &inputType() const { return input; }
        inline QString const &platform() const { return platformStr; }
        inline QString const &context() const { return contextStr; }
        inline bool isCmd() const { return !is_cmd.isEmpty(); }
        inline bool isBool() const { return input == QLatin1String("bool"); }
        inline bool hasDefault() const { return !def.isEmpty(); };
        /// One-sided bounds: the engine clamps each side independently, so a row with
        /// only a min (or only a max) keeps that side (docs/DESIGN-arg-bounds.md).
        inline bool hasMin() const { return !min.isEmpty(); }
        inline bool hasMax() const { return !max.isEmpty(); }
        inline bool hasRange() const { return hasMin() && hasMax(); }
        inline bool hasUsage() const { return !input.isEmpty() && input != QLatin1String("none"); };
        /// Positional access to the 10 fields, for readers whose layout differs from
        /// allcmds.csv (the argbounds companion file reuses fields 1-5).
        inline QString field(int i) const {
            switch (i) {
            case 0: return cmd_name;
            case 1: return is_cmd;
            case 2: return def;
            case 3: return min;
            case 4: return max;
            case 5: return flags;
            case 6: return doc;
            case 7: return input;
            case 8: return platformStr;
            default: return contextStr;
            }
        }

    private:
        QString cmd_name, is_cmd, def, min, max, flags, doc, input;
        QString platformStr, contextStr;
        TokenClass type;
    };

    Token nextToken();

private:
    // The old DFA errored out on every unquoted field (plain text like
    // "client settings" after a comma), so any row csv.writer had not force-quoted
    // was dropped. Fields are now split by a plain CSV scan instead.
};

} // namespace Lexers
#endif // LEXERS_H
