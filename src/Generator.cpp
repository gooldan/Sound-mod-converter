#include "Generator.h"
#include <FileIO.h>
#include <QTextStream>

bool Generator::setOutputDir(const QString &dirFilepath)
{
    if(!FileIO::fileExists(dirFilepath))
    {
        return false;
    }
    outputDir=dirFilepath;
    return true;
}

bool Generator::serialize(const QString &jsString)
{
    serializedEvents.clear();
    filesList.clear();
    latinFilesList.clear();
    notFoundFiles.clear();

    QJsonDocument doc;
    //printf("%s",jsString.toUtf8().data());
    //fflush(stdout);
    doc=QJsonDocument::fromJson(jsString.toUtf8());
    auto mainObj = doc.object();
    auto eventObj = mainObj["events"];
    auto stateObj = mainObj["states"];
    QHash<QString,State> stateList;
    for(auto obj : stateObj.toArray())
    {
        State curState;
        auto stateObj=obj.toObject();
        curState.name=stateObj["realName"].toString();
        auto stateContent=stateObj["content"].toArray();
        for(int i = 1, arrE=stateContent.size(); i < arrE; ++i)
        {
            StateElement el;
            el.name=stateContent[i].toArray()[0].toString();
            curState.values.push_back(el);
        }
        stateList.insert(curState.name,curState);
    }
    for(auto objRef : eventObj.toArray())
    {
        auto obj=objRef.toObject();
        ExternalEvent event;
        for(int t = 0; t < 3; ++t)
        {
            ExternalEventContainer cont;

            auto stateArr=obj["selectedChain"].toArray()[t].toArray();
            std::vector<std::pair<QString,State>> chainsObjects;
            for(auto stateName : stateArr)
            {
                auto str=stateName.toString();
                str.chop(4);
                auto it=stateList.find(str);
                if(it==stateList.end())
                {
                    return false;
                }
                chainsObjects.emplace_back(it.key(),it.value());
            }
            auto selectedArr=obj["statePaths"].toArray()[t].toArray();

            auto pathArr=obj["filePaths"].toArray()[t].toArray();

            for(int i = 0, iEnd=selectedArr.size(); i < iEnd; ++i)
            {
                ExtendedEvent newStateChain;
                auto stateIndexes = selectedArr[i].toString().split(",");
                for(int j =0, jEnd=stateIndexes.size(); j < jEnd; ++j)
                {
                    StateValue value;
                    value.stateName=chainsObjects[j].first;
                    QString temp=stateIndexes[j];
                    int index=stateIndexes[j].toInt();
                    if(index > 0)
                    {
                        value.stateValue=chainsObjects[j].second.values[index-1].name;
                        newStateChain.stateList.push_back(value);
                    }
                }
                auto pathArrObj = pathArr[i].toObject();
                if(!pathArrObj["filesNotFound"].toBool())
                {
                    auto currentChainPath=pathArrObj["paths"].toArray();
                    for(auto singlePath : currentChainPath)
                    {
                        auto str = singlePath.toString();
                        if(str=="Muted")
                        {
                            str="TISHINA.wav";
                        }
                        auto hasUnicode = [](const QString &str)->bool{
                            return std::any_of(str.begin(),str.end(),
                                               [](QChar c)->bool {return c.unicode()>127;});
                            };
                        QFileInfo fi(str);
                        if(hasUnicode(fi.fileName()))
                        {
                            QString newName = QString::number(qHash(str))+'.'+fi.completeSuffix();
                            newStateChain.latinPathList.push_back(newName);
                            latinFilesList.push_back(newName);

                        }
                        else
                        {
                            newStateChain.latinPathList.push_back("");
                            latinFilesList.push_back("");
                        }
                        newStateChain.pathList.push_back(fi.fileName());
                        filesList.push_back(str);
                    }
                }
                cont.stateChains.push_back(newStateChain);
            }
            auto extEventName= obj["extIds"].toArray()[t].toString();
            cont.extID=extEventName;
            event.events[t]=std::move(cont);
        }
        event.eventName=obj["name"].toString();
        serializedEvents.push_back(event);
    }
    return  true;

}

QString Generator::generate(bool copy, QString currentModName)
{
    QDir d(outputDir);
    d.mkdir(currentModName);
    outputDir = d.absoluteFilePath(currentModName);
    printf("Copying files. Output dir is: `%s`.\n", outputDir.toUtf8().data());
    copyFiles();
    QString xmlFileStr;
    QXmlStreamWriter wr(&xmlFileStr);
    wr.setCodec("UTF-8");
    wr.setAutoFormatting(true);
    wr.writeStartDocument();
    wr.writeStartElement("AudioModification.xml");
    wr.writeStartElement("AudioModification");
    QString modName= QDir(outputDir).dirName();
    wr.writeTextElement("Name",modName);
    for (auto &ee : serializedEvents)
    {

        if(std::any_of(ee.events.begin(),ee.events.end(),
                       [](ExternalEventContainer val)->bool{return val.stateChains.size()>0;}))
        {
            wr.writeStartElement("ExternalEvent");
            wr.writeTextElement("Name", ee.eventName);
            for(int t = 0; t < 3; ++t)
            {
                    if(ee.events[t].stateChains.empty())
                    {
                        continue;
                    }
                    wr.writeStartElement("Container");
                    wr.writeTextElement("Name",ee.names[t]);
                    wr.writeTextElement("ExternalId",ee.events[t].extID);
                    for (auto &chain : ee.events[t].stateChains)
                    {

                        if(chain.pathList.empty() ||
                                std::any_of(chain.stateList.begin(),
                                            chain.stateList.end(),
                                            [](const StateValue &v)->bool{return v.stateName == "!";}))
                        {
                            continue;
                        }
                        wr.writeStartElement("Path");
                        wr.writeStartElement("StateList");
                        for (auto &chainElement : chain.stateList)
                        {
                            wr.writeStartElement("State");
                            wr.writeTextElement("Name",chainElement.stateName);
                            wr.writeTextElement("Value",chainElement.stateValue);
                            wr.writeEndElement();
                        }
                        wr.writeEndElement();
                        wr.writeStartElement("FilesList");
                        for (size_t i = 0,iEnd = chain.pathList.size(); i < iEnd; ++i)
                        {
                            wr.writeStartElement("File");
                            if(chain.latinPathList[i].length() > 0)
                            {
                                QFileInfo fi(chain.latinPathList[i]);
                                if(fi.suffix()=="wav")
                                {
                                    wr.writeTextElement("Name",fi.completeBaseName()+".wem");
                                }
                                else
                                {
                                    wr.writeTextElement("Name",chain.latinPathList[i]);
                                }
                            }
                            else
                            {
                                QFileInfo fi(chain.pathList[i]);
                                if(fi.suffix()=="wav")
                                {
                                    wr.writeTextElement("Name",fi.completeBaseName()+".wem");
                                }
                                else
                                {
                                    wr.writeTextElement("Name",chain.pathList[i]);
                                }
                            }
                            wr.writeEndElement();
                        }
                        wr.writeEndElement();
                        wr.writeEndElement();
                    }
                    wr.writeEndElement();

            }
            wr.writeEndElement();
        }
    }
    wr.writeEndElement();
    wr.writeEndElement();
    wr.writeEndDocument();
    QFile outXML(outputDir+"/mod.xml");
    if (!outXML.open(QFile::WriteOnly | QFile::Truncate))
        return "{\"answer\":\"BAD\"}";

    QTextStream out(&outXML);
    out.setCodec("UTF-8");
    out << xmlFileStr;
    outXML.close();

    xmlFileStr.clear();


    emit generationDone();
    QJsonDocument doc;
    QJsonObject obj;
    obj["answer"]="OK";
    QJsonArray arr;
    for (auto &f : notFoundFiles)
    {
        arr.append(QJsonValue(f));
    }
    obj["notFound"]=arr;
    doc.setObject(obj);
    return doc.toJson(QJsonDocument::JsonFormat::Compact);
}

void Generator::fileCopyProgress(int progress)
{
    printf("\rCopied: %d %%...",progress);
}

void Generator::allFilesCopied()
{
    printf("\nAll files are copied. Check output dir for your mod.\n");
}

void Generator::generationDone()
{

}

void Generator::printProgress(int progress)
{

}

void Generator::wwiseOutput()
{

}

void Generator::copyFiles()
{
    wavFiles.clear();
    for(size_t i = 0, iEnd=filesList.size(); i < iEnd; ++i)
    {

        QFileInfo file(filesList[i]);
        QString newName = outputDir+"/"+file.fileName();
        if(latinFilesList[i].length() > 0)
        {
            newName = outputDir+"/"+latinFilesList[i];
        }
        if(file.exists())
        {
            QFile::copy(filesList[i],newName);
            QFileInfo newNameFile(newName);
            if (newNameFile.suffix() == "wav")
            {
                wavFiles.push_back(newName);
            }
        }
        else
        {
            notFoundFiles.push_back(filesList[i]);
        }
        fileCopyProgress(i*100/iEnd);

    }
    printf("\rCopied: 100 %%...");
    allFilesCopied();
}
