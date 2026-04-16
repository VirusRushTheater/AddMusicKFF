#ifndef SONGLIST_H
#define SONGLIST_H

#include <vector>
#include <string>
#include <QString>

class SongList
{
public:
    std::vector<QString> fileNames;
    QString name;

    static SongList parseSongs(const QString& str, const QString& labelname);
    QString toString() const;
};

#endif // SONGLIST_H