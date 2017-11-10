#ifndef GENERATOR_H
#define GENERATOR_H
#include "types.h"
#include <QObject>
#include <QFile>
#include <QDir>
#include <thread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QObject>
#include <QHash>
#include "QXmlStreamWriter"
#include <functional>
#include <algorithm>
#include <thread>
#include <QProcess>

struct OldConfig;
class Generator
{
    friend struct OldConfig;
public:
    bool setOutputDir(const QString &dirFilepath);
    bool serialize(const QString &jsString);
    QString generate(bool copy, QString currentModName);
    void mainProgress(int progress);
    void fileCopyProgress(int progress);
    void allFilesCopied();
    void generationDone();
    void wwiseGenEnd(int errorCode);
    void wwiseMessage(QString message);
    void printProgress(int progress);
    void wwiseOutput();
    bool checkForExistance();
    void copyFiles();
    std::vector<QString> filesList;
    std::vector<QString> latinFilesList;
    std::vector<QString> notFoundFiles;
    std::vector<QString> wavFiles;
    std::vector<ExternalEvent> serializedEvents;
    QString outputDir;
    QProcess *proc;
};

#endif // GENERATOR_H
