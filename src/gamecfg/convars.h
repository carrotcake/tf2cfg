#ifndef CONVARS_H
#define CONVARS_H

#include <QString>
#include <QStringList>

namespace cmd {
enum CmdType { CMD_IVAR, CMD_FVAR, CMD_SVAR, CMD_CMD, CMD_BIND, CMD_ALIAS, CMD_UNKWN };

struct Entry {
    QStringList args;
    CmdType type;
    Entry()
        : type(CMD_UNKWN){};
};
class ConVar {
public:
    ConVar(const QString nm, const QString doc)
        : name(nm)
        , docs(doc){};
    QString const& get_name() const { return name; };
    QString const& get_docs() const { return docs; };
    virtual CmdType get_type() const;

private:
    const QString name, docs;
};

class IntVar : public ConVar {
public:
    typedef long long int i64;
    struct Range {
        const i64 min, max;
        Range(i64 minval, i64 maxval)
            : min(minval)
            , max(maxval){};
    };

    IntVar(const QString nm, const QString doc, i64 def, i64 min, int max)
        : ConVar(nm, doc)
        , defVal(def)
        , r(min, max)
        , minMax(min != max){};
    IntVar(const QString nm, const QString doc, int def)
        : IntVar(nm, doc, def, 0, 0){};

    auto getDefaultVal() const { return defVal; }
    auto getMinVal() const { return minMax ? r.min : 0; }
    auto getMaxVal() const { return minMax ? r.max : 0; }
    auto hasRange() const { return minMax; }
    CmdType get_type() const { return CMD_IVAR; }

private:
    const i64 defVal;
    const Range r;
    const bool minMax;
};

class FltVar : public ConVar {
public:
    struct Range {
        const double min, max;
        Range(double minval, double maxval)
            : min(minval)
            , max(maxval){};
    };
    FltVar(const QString& nm, const QString& doc, double def, double min, double max)
        : ConVar(nm, doc)
        , defVal(def)
        , r(min, max)
        , minMax(min != max){};
    FltVar(const QString& nm, const QString& doc, double def)
        : FltVar(nm, doc, def, 0.0, 0.0){};

    auto getDefaultVal() const { return defVal; }
    auto getMinVal() const { return minMax ? r.min : 0; }
    auto getMaxVal() const { return minMax ? r.max : 0; }
    auto hasRange() const { return minMax; }
    CmdType get_type() const { return CMD_FVAR; }

private:
    const double defVal;
    Range r;
    const bool minMax;
};

class StrVar : public ConVar {
public:
    StrVar(const QString& nm, const QString& doc, const QString& def)
        : ConVar(nm, doc)
        , defStr(def){};

    QString const& getDefaultStr() const { return defStr; };
    CmdType get_type() const { return CMD_SVAR; }

private:
    const QString defStr;
};

class ConCmd : public ConVar {
    ConCmd(const QString& nm, const QString& doc, const QString& usg)
        : ConVar(nm, doc)
        , usage(usg) {}

    QString const& getUsage() const { return usage; }
    CmdType get_type() const { return CMD_CMD; }

private:
    const QString usage;
};

} // namespace cmd
#endif // CONVARS_H
