#ifndef CONVARS_H
#define CONVARS_H

#include <QString>
#include <QStringList>

namespace cmd {
typedef long long int i64;
enum CmdType { CMD_IVAR, CMD_FVAR, CMD_SVAR, CMD_BVAR, CMD_CMD, CMD_BIND, CMD_ALIAS, CMD_UNKWN };

struct Entry {
    QStringList args;
    CmdType type;
    Entry()
        : type(CMD_UNKWN){};
};

/**
 * One entry of allcmds.csv (res/cfg/schema.md). The metadata members (flags,
 * platform, context) describe where and how the console entry can be used; they
 * drive the Build-A-Bind filters. Instances are stored as owning pointers in
 * GameCfg (value storage would slice the typed subclasses).
 */
class ConVar {
public:
    ConVar(const QString &nm, const QString &doc, CmdType t, const QString &usg,
           const QString &fl, const QString &pl, const QString &cx)
        : name(nm)
        , docs(doc)
        , usage(usg)
        , flags(fl)
        , platform(pl)
        , context(cx)
        , m_type(t){};
    virtual ~ConVar() = default;
    QString const &get_name() const { return name; };
    QString const &get_docs() const { return docs; };
    QString const &get_usage() const { return usage; };
    QString const &get_flags() const { return flags; };
    QString const &get_platform() const { return platform; };
    QString const &get_context() const { return context; };
    /// The command needs sv_cheats 1 (or a singleplayer server) to run.
    bool isCheat() const { return flags.split(' ', Qt::SkipEmptyParts).contains(QLatin1String("cheat")); };
    virtual CmdType get_type() const { return m_type; };
    /// Range bounds for the Build-A-Bind argument editors; doubles so int and flt
    /// cvars feed the same spin box. Empty strings = no bound known.
    virtual bool hasRange() const { return false; };
    virtual double minVal() const { return 0.0; };
    virtual double maxVal() const { return 0.0; };
    virtual QString defaultStr() const { return {}; };

private:
    QString name, docs, usage, flags, platform, context;
    CmdType m_type;
};

class IntVar : public ConVar {
public:
    struct Range {
        const i64 min, max;
        Range(i64 minval, i64 maxval)
            : min(minval)
            , max(maxval){};
    };

    IntVar(const QString &nm, const QString &doc, i64 def, i64 min, i64 max, const QString &fl,
           const QString &pl, const QString &cx)
        : ConVar(nm, doc, CMD_IVAR, "int", fl, pl, cx)
        , defVal(def)
        , r(min, max)
        , minMax(min != max){};
    IntVar(const QString &nm, const QString &doc, i64 def, const QString &fl, const QString &pl,
           const QString &cx)
        : IntVar(nm, doc, def, 0, 0, fl, pl, cx){};

    auto getDefaultVal() const { return defVal; }
    auto getMinVal() const { return minMax ? r.min : 0; }
    auto getMaxVal() const { return minMax ? r.max : 0; }
    bool hasRange() const override { return minMax; }
    double minVal() const override { return static_cast<double>(r.min); }
    double maxVal() const override { return static_cast<double>(r.max); }
    QString defaultStr() const override { return QString::number(defVal); }
    CmdType get_type() const override { return CMD_IVAR; }

private:
    i64 defVal;
    Range r;
    bool minMax;
};

class FltVar : public ConVar {
public:
    struct Range {
        const double min, max;
        Range(double minval, double maxval)
            : min(minval)
            , max(maxval){};
    };
    FltVar(const QString &nm, const QString &doc, double def, double min, double max,
           const QString &fl, const QString &pl, const QString &cx)
        : ConVar(nm, doc, CMD_FVAR, "flt", fl, pl, cx)
        , defVal(def)
        , r(min, max)
        , minMax(min != max){};
    FltVar(const QString &nm, const QString &doc, double def, const QString &fl,
           const QString &pl, const QString &cx)
        : FltVar(nm, doc, def, 0.0, 0.0, fl, pl, cx){};

    auto getDefaultVal() const { return defVal; }
    auto getMinVal() const { return minMax ? r.min : 0.0; }
    auto getMaxVal() const { return minMax ? r.max : 0.0; }
    bool hasRange() const override { return minMax; }
    double minVal() const override { return r.min; }
    double maxVal() const override { return r.max; }
    QString defaultStr() const override { return QString::number(defVal); }
    CmdType get_type() const override { return CMD_FVAR; }

private:
    double defVal;
    Range r;
    bool minMax;
};

class StrVar : public ConVar {
public:
    StrVar(const QString &nm, const QString &doc, const QString &def, const QString &fl,
           const QString &pl, const QString &cx)
        : ConVar(nm, doc, CMD_SVAR, "str", fl, pl, cx)
        , defStr(def){};

    QString const &getDefaultStr() const { return defStr; };
    QString defaultStr() const override { return defStr; }
    CmdType get_type() const override { return CMD_SVAR; }

private:
    QString defStr;
};

class BoolVar : public ConVar {
public:
    BoolVar(const QString &nm, const QString &doc, bool def, const QString &fl,
            const QString &pl, const QString &cx)
        : ConVar(nm, doc, CMD_BVAR, "bool", fl, pl, cx)
        , defVal(def){};

    bool getDefaultVal() const { return defVal; }
    QString defaultStr() const override { return defVal ? "1" : "0"; }
    CmdType get_type() const override { return CMD_BVAR; }

private:
    bool defVal;
};

class ConCmd : public ConVar {
public:
    ConCmd(const QString &nm, const QString &doc, const QString &usg, const QString &fl,
           const QString &pl, const QString &cx)
        : ConVar(nm, doc, CMD_CMD, usg, fl, pl, cx) {}

    CmdType get_type() const override { return CMD_CMD; }
};

} // namespace cmd
#endif // CONVARS_H
