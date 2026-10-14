#ifndef CONVARS_H
#define CONVARS_H

#include <QString>
#include <QStringList>
#include <QVector>

namespace cmd {
typedef long long int i64;
enum CmdType { CMD_IVAR, CMD_FVAR, CMD_SVAR, CMD_BVAR, CMD_CMD, CMD_BIND, CMD_ALIAS, CMD_UNKWN,
               CMD_COMMENT };

/**
 * Authoring bounds for one argument of a console command (docs/DESIGN-arg-bounds.md).
 * The engine passes command arguments as strings and never clamps them; these exist
 * only to shape the Build-A-Bind editors. One-sided bounds are explicit flags, the
 * same way ConVar clamps each side independently.
 */
struct ArgSpec {
    int index = 0;      ///< position in the usage signature
    bool isFloat = false; ///< int vs flt editor mode
    bool hasMin = false;
    double min = 0.0;
    bool hasMax = false;
    double max = 0.0;
};


struct Entry {
    QStringList args;
    CmdType type;
    /// File line (0-based) the row was parsed from; -1 for a row the app created
    /// (setBind), which is what makes the surgical save regenerate exactly these
    /// lines and copy every other one verbatim (docs/legacy/ui-qml-rewrite.md 10.1).
    int sourceLine = -1;
    /// True when the row was written by the app rather than parsed. Parsed rows stay
    /// false even when they are binds: the file remains their source of truth until
    /// the user edits them.
    bool appOwned = false;
    /// Save anchor for a session row (sourceLine < 0): insert after this file line.
    /// -1 keeps the legacy bind rule (after the last bind line), AnchorEof appends at
    /// end of file (docs/DESIGN-config-editor.md §9).
    int anchorLine = -1;
    /// Comment tail (`// ...` raw text, never case-folded) the source line carried
    /// after the command part (docs/DESIGN-config-editor.md §6). A standalone comment
    /// line is a CMD_COMMENT row whose args hold the tail text instead.
    QString commentTail;
    /// Session compound identity (docs/DESIGN-config-editor.md §5): rows sharing a
    /// nonzero groupId render as ONE `;`-joined line at save time. 0 = not grouped;
    /// parsed rows group by sourceLine and never need this.
    int groupId = 0;
    Entry()
        : type(CMD_UNKWN){};
};

/// Sentinel anchorLine: the session row is appended at end of file.
constexpr int ANCHOR_EOF = 2147483647;

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
    /// cvars feed the same spin box. Each side stands alone, matching the engine's
    /// independent m_bHasMin/m_bHasMax clamps; empty = no bound on that side.
    virtual bool hasRange() const { return false; };
    virtual bool hasMinBound() const { return false; };
    virtual bool hasMaxBound() const { return false; };
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
        const bool hasMin, hasMax;
        Range(i64 minval, i64 maxval, bool hasmn, bool hasmx)
            : min(minval)
            , max(maxval)
            , hasMin(hasmn)
            , hasMax(hasmx){};
    };

    IntVar(const QString &nm, const QString &doc, i64 def, i64 min, i64 max, const QString &fl,
           const QString &pl, const QString &cx)
        : ConVar(nm, doc, CMD_IVAR, "int", fl, pl, cx)
        , defVal(def)
        , r(min, max, true, true){};
    IntVar(const QString &nm, const QString &doc, i64 def, const QString &fl, const QString &pl,
           const QString &cx)
        : IntVar(nm, doc, def, 0, 0, fl, pl, cx){};
    /// Full-control ctor: each bound side is present or absent on its own.
    IntVar(const QString &nm, const QString &doc, i64 def, i64 min, i64 max, bool hasmn,
           bool hasmx, const QString &fl, const QString &pl, const QString &cx)
        : ConVar(nm, doc, CMD_IVAR, "int", fl, pl, cx)
        , defVal(def)
        , r(min, max, hasmn, hasmx){};

    auto getDefaultVal() const { return defVal; }
    auto getMinVal() const { return r.min; }
    auto getMaxVal() const { return r.max; }
    bool hasMinBound() const override { return r.hasMin; }
    bool hasMaxBound() const override { return r.hasMax; }
    bool hasRange() const override { return r.hasMin || r.hasMax; }
    double minVal() const override { return static_cast<double>(r.min); }
    double maxVal() const override { return static_cast<double>(r.max); }
    QString defaultStr() const override { return QString::number(defVal); }
    CmdType get_type() const override { return CMD_IVAR; }

private:
    i64 defVal;
    Range r;
};

class FltVar : public ConVar {
public:
    struct Range {
        const double min, max;
        const bool hasMin, hasMax;
        Range(double minval, double maxval, bool hasmn, bool hasmx)
            : min(minval)
            , max(maxval)
            , hasMin(hasmn)
            , hasMax(hasmx){};
    };
    FltVar(const QString &nm, const QString &doc, double def, double min, double max,
           const QString &fl, const QString &pl, const QString &cx)
        : ConVar(nm, doc, CMD_FVAR, "flt", fl, pl, cx)
        , defVal(def)
        , r(min, max, true, true){};
    FltVar(const QString &nm, const QString &doc, double def, const QString &fl,
           const QString &pl, const QString &cx)
        : FltVar(nm, doc, def, 0.0, 0.0, fl, pl, cx){};
    /// Full-control ctor: each bound side is present or absent on its own.
    FltVar(const QString &nm, const QString &doc, double def, double min, double max,
           bool hasmn, bool hasmx, const QString &fl, const QString &pl, const QString &cx)
        : ConVar(nm, doc, CMD_FVAR, "flt", fl, pl, cx)
        , defVal(def)
        , r(min, max, hasmn, hasmx){};

    auto getDefaultVal() const { return defVal; }
    auto getMinVal() const { return r.min; }
    auto getMaxVal() const { return r.max; }
    bool hasMinBound() const override { return r.hasMin; }
    bool hasMaxBound() const override { return r.hasMax; }
    bool hasRange() const override { return r.hasMin || r.hasMax; }
    double minVal() const override { return r.min; }
    double maxVal() const override { return r.max; }
    QString defaultStr() const override { return QString::number(defVal); }
    CmdType get_type() const override { return CMD_FVAR; }

private:
    double defVal;
    Range r;
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

    /// Per-argument authoring bounds (docs/DESIGN-arg-bounds.md). Attached by
    /// GameCfg::loadArgBounds() after the catalog loads; empty unless the companion
    /// file bounds this command.
    void setArgSpecs(const QVector<ArgSpec> &specs) { m_argSpecs = specs; }
    const QVector<ArgSpec> &argSpecs() const { return m_argSpecs; }

    CmdType get_type() const override { return CMD_CMD; }

private:
    QVector<ArgSpec> m_argSpecs;
};

} // namespace cmd
#endif // CONVARS_H
