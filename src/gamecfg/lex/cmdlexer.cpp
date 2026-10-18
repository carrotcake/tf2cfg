#include "lexers.h"

using namespace Lexers;

cmdLex::Token cmdLex::nextToken() {
    if (line >= arr.size()) {
        return Token(Token::TOK_EOF);
    }
    const QString str = arr.at(line);

    // Plain CSV field scan: fields are quoted when they contain commas, quotes or
    // newlines; "" inside a quoted field is an escaped quote. Trailing empty fields
    // are tolerated so hand-edited rows do not kill the load.
    QStringList fields;
    QString temp;
    bool inQuotes = false;
    for (int i = 0; i <= str.size(); ++i) {
        // sentinel comma one past the end closes the final field
        const QChar c = i < str.size() ? str.at(i) : QChar(',');
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < str.size() && str.at(i + 1) == '"') {
                    temp.append('"');
                    i++;
                } else {
                    inQuotes = false;
                }
            } else {
                temp.append(c);
            }
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == ',') {
            fields.push_back(temp);
            temp.clear();
        } else {
            temp.append(c);
        }
    }

    // One token per line: skip past it in the shared cursor (advanceOneChar would
    // only step col by one and re-scan this line on the next call).
    col = 0;
    line++;
    while (fields.size() < 10) {
        fields.push_back(QString());
    }
    return Token(fields[0], fields[1], fields[2], fields[3], fields[4], fields[5], fields[6],
                 fields[7], fields[8], fields[9]);
}