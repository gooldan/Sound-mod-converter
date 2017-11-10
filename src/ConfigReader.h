#ifndef CONFIGREADER_H
#define CONFIGREADER_H
#include <QString>
#include <memory>
#include <QFile>
#include <QXmlStreamReader>
#include <vector>
#include <map>
#include <array>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "types.h"
#include "Generator.h"
//#define TRACE
class ConfigReader;
struct OldConfig
{
    QString modName;
    std::array<int,3> modTypes{0,0,0};
    void openModFolder(const QString &dirPath, const ConfigReader &reader, Generator &gen);
    void openModTypeFolder(const QString &dirPath, const ConfigReader &reader, Generator &gen, ModType type);
    void mergeAndAlignAll(const ConfigReader &reader, Generator &gen);
};

class ConfigReader
{
public:

    enum ErrorCode{
        OK,
        XML_END,
        CANT_OPEN_CONFIG_FILE,
        FILE_NOT_OPENED,
        XML_ERROR
    };

    friend struct OldConfig;
    int openConfig(const QString &filepath);
    int parse();
    QString createJSON();
    int parseStates();
    int parseEvents();
    int readNextHeaderName(const char *name="");
    QJsonObject getDefaultState();
    QXmlStreamReader xml;
    QFile configFile;
    std::vector<Event> events;
    std::vector<State> states;
    int version;
};

#endif // CONFIGREADER_H
