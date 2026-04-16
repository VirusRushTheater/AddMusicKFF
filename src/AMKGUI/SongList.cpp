#include "SongList.h"
#include <QRegularExpression>
#include <algorithm>

SongList SongList::parseSongs(const QString& str, const QString& labelname)
{
    SongList ret;
    QStringList lines = str.split('\n');
    bool foundList = false;

    for (const QString& line : lines) {
        if (line.startsWith(labelname + ":")) {
            foundList = true;
            continue;
        } else if (line.contains(":")) {
            foundList = false;
        }

        if (foundList && line.length() > 2) {
            // Parse hex number and filename
            QString trimmed = line.trimmed();
            if (trimmed.length() >= 4) {
                QString hexPart = trimmed.left(2);
                QString filePath = trimmed.mid(2).trimmed();

                bool ok;
                int number = hexPart.toInt(&ok, 16);
                if (ok) {
                    if (ret.fileNames.size() <= static_cast<size_t>(number)) {
                        ret.fileNames.resize(number + 1, "");
                    }
                    ret.fileNames[number] = filePath;
                }
            }
        }
    }
    ret.name = labelname;
    return ret;
}

QString SongList::toString() const
{
    QString ret = name + ":\n";
    for (size_t i = 0; i < fileNames.size(); ++i) {
        if (!fileNames[i].isEmpty()) {
            ret += QString("%1  %2\n").arg(i, 2, 16, QChar('0')).arg(fileNames[i]);
        }
    }
    return ret;
}