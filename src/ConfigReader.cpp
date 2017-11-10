#include "ConfigReader.h"
#include <iostream>
#include <QDir>
#include <QDirIterator>
#include "types.h"
int ConfigReader::openConfig(const QString &filepath)
{
    configFile.close();
    configFile.setFileName(filepath);
    if(!configFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return ErrorCode::CANT_OPEN_CONFIG_FILE;
    }
    xml.setDevice(&configFile);
    return ErrorCode::OK;
}

int ConfigReader::parse()
{
    events.clear();
    states.clear();
    if(xml.device() == NULL)
    {
        return ErrorCode::FILE_NOT_OPENED;
    }
    while (!xml.atEnd())
    {
        xml.readNext();
        if(xml.hasError())
        {
            return ErrorCode::XML_ERROR;
        }
        if (xml.isStartDocument())
        {
            continue;
        }
        if (xml.isStartElement())
        {
            if(xml.name() == "mod.xml")
            {
                QXmlStreamAttributes attributes = xml.attributes();
                for(auto &attr : attributes)
                {
                    if(attr.name()=="version")
                    {
                        version=attr.value().toInt();
                    }
                    else
                    {
                        return ErrorCode::XML_ERROR;
                    }
                }
                continue;
            }
            if(xml.name() == "events")
            {
                auto err = parseEvents();
                if (err != ErrorCode::OK)
                {
                    return err;
                }
            }
            if(xml.name() == "states")
            {
                auto err = parseStates();
                if (err != ErrorCode::OK)
                {
                    return err;
                }
            }
        }

    }
    return ErrorCode::OK;
}
QJsonObject ConfigReader::getDefaultState()
{
    QJsonObject stateObj;
    stateObj["name"]="default (*)";
    stateObj["realName"]="default";
    QJsonArray acceptArr;
    stateObj["accepts"]=acceptArr;

    QJsonArray contentArr;
    QJsonArray valueArr;

    valueArr.append("default (*)");
    valueArr.append("");
    valueArr.append("default (*)");
    valueArr.append("По умолчанию (*)");
    contentArr.append(valueArr);
    valueArr.pop_front();
    stateObj["readable"]=valueArr;
    stateObj["content"]=contentArr;
    stateObj["globallyAppended"] = true;
    return stateObj;
}

QString ConfigReader::createJSON()
{
    QJsonDocument doc;
    QJsonObject obj;
    QJsonArray eventsArr;
    for (size_t i = 0, eSize=events.size(); i < eSize; ++i)
    {
        QJsonObject eventObj;
        eventObj["name"]=events[i].name;

        QJsonArray extIds;
        for(auto &type : events[i].acceptedTypes)
        {
            extIds.append(type.id);
        }
        eventObj["extIds"]=extIds;
        QJsonArray acceptsArray;
        for(auto &accType : events[i].acceptedTypes)
        {
            acceptsArray.append(QJsonValue(accType.name));
        }
        eventObj["accepts"]=acceptsArray;

        QJsonArray selectedChain;
        for (int i = 0; i < 3; ++i)
        {
            QJsonArray arr;
            arr.push_back("default (*)");
            selectedChain.push_back(arr);
        }
        eventObj["selectedChain"]=selectedChain;

        QJsonArray currentChain;
        for (int i = 0; i < 3; ++i)
        {
            QJsonArray arr;
            arr.push_back(QJsonArray());
            currentChain.push_back(arr);
        }
        eventObj["currentChain"]=currentChain;

        QJsonArray statePaths;
        for (int i = 0; i < 3; ++i)
        {
            statePaths.push_back(QJsonArray());
        }
        eventObj["statePaths"]=statePaths;

        QJsonArray filePaths;
        for (int i = 0; i < 3; ++i)
        {
            filePaths.push_back(QJsonArray());
        }

        QJsonArray readable;
        for (int j = 0; j < 3; ++j)
        {
            QJsonArray readableType;
            for(int t = 0; t < 3; ++t)
            {
                readableType.push_back(events[i].acceptedTypes[j].details.readable[t]);
            }
            readable.push_back(readableType);
        }
        QJsonArray description;
        for (int j = 0; j < 3; ++j)
        {
            description.push_back(events[i].details.description[j]);
        }
        eventObj["description"]=description;
        eventObj["readable"]=readable;
        QJsonArray filesExist;
        for (int j = 0; j < 3; ++j)
        {
            filesExist.append(false);
        }
        eventObj["filesExist"]=filesExist;
        eventObj["filePaths"]=filePaths;
        eventObj["index"]=QJsonValue(static_cast<int>(i));
        QJsonArray corrupted;
        for(int j = 0; j < 3; ++j)
        {
            corrupted.append(false);
        }
        eventObj["corrupted"] = corrupted;
        eventsArr.append(eventObj);
    }
    obj["events"]=eventsArr;

    QJsonArray statesArr;
    for (auto &state : states)
    {
        QJsonObject stateObj;
        stateObj["name"] = state.name + " (*)";
        stateObj["realName"] = state.name;
        stateObj["globallyAppended"] = false;
        QJsonArray acceptArr;
        for (auto &val : state.references)
        {
            acceptArr.append(val);
        }
        stateObj["accepts"]=acceptArr;

        QJsonArray contentArr;
        QJsonArray valueArr;

        valueArr.append(state.name + " (*)");
        for (int i = 0; i < 3; ++i)
        {
            valueArr.append(state.details.readable[i]);
        }
        contentArr.append(valueArr);
        valueArr.pop_front();
        stateObj["readable"]=valueArr;
        for (auto &val : state.values)
        {
            QJsonArray valueArr;
            valueArr.append(val.name);
            for (int i = 0; i < 3; ++i)
            {
                valueArr.append(val.details.readable[i]);
            }
            contentArr.append(valueArr);
        }
        stateObj["content"]=contentArr;
        statesArr.append(stateObj);
    }
    statesArr.append(getDefaultState());
    obj["states"]=statesArr;
    obj["version"]=version;
    doc.setObject(obj);
    return doc.toJson(QJsonDocument::JsonFormat::Compact);

}

int getIndexFromName(const QStringRef &str)
{
    auto c=str.constData()[str.length()-1];
    if(c=='i')
    {
        return 0;
    }
    else if(c=='n')
    {
        return 1;
    }
    else if(c=='u')
        return 2;

    return -1;
}

int ConfigReader::parseEvents()
{
    auto err=readNextHeaderName("event");
    if(err!=ErrorCode::OK)
    {
        return err;
    }
    while(err!=ErrorCode::XML_END)
    {
        QXmlStreamAttributes attributes = xml.attributes();
        Event newEvent;
        for(auto &attr : attributes)
        {
            if(attr.name()=="name")
            {
                newEvent.name=attr.value().toString();
            }
            else if (attr.name().indexOf("read")>=0)
            {
                int index=getIndexFromName(attr.name());
                if (index<0) return ErrorCode::XML_ERROR;
                newEvent.details.readable[index]=attr.value().toString();
            }
            else if (attr.name().indexOf("desc")>=0)
            {
                int index=getIndexFromName(attr.name());
                if (index<0) return ErrorCode::XML_ERROR;
                newEvent.details.description[index]=attr.value().toString();
            }
            else
            {
                return ErrorCode::XML_ERROR;
            }
        }
        err=readNextHeaderName("acceptedType");
        if(err != ErrorCode::OK)
        {
            return err;
        }

        while (err != ErrorCode::XML_END)
        {
            QXmlStreamAttributes attributes = xml.attributes();
            ExternalType newType;
            int typeIndex = -1;
            for(auto &attr : attributes)
            {
                if(attr.name()=="name")
                {
                    newType.name=attr.value().toString();
                    if(newType.name[0]=='V')
                        typeIndex = 0;
                    else if(newType.name[0]=='S')
                        typeIndex = 1;
                    else if(newType.name[0]=='L')
                        typeIndex = 2;
                    else
                        typeIndex = -1;

                }
                else if (attr.name() == "externalId")
                {
                    newType.id=attr.value().toString();
                }
                else if (attr.name().indexOf("read")>=0)
                {
                    int index=getIndexFromName(attr.name());
                    if (index<0) return ErrorCode::XML_ERROR;
                    newType.details.readable[index]=attr.value().toString();
                }
                else if (attr.name().indexOf("desc")>=0)
                {
                    int index=getIndexFromName(attr.name());
                    if (index<0) return ErrorCode::XML_ERROR;
                    newType.details.description[index]=attr.value().toString();
                }
                else
                {
                    return ErrorCode::XML_ERROR;
                }
            }
            if(typeIndex > 2 || typeIndex < 0)
            {
                return ErrorCode::XML_ERROR;
            }
            newEvent.acceptedTypes[typeIndex]=newType;
            err=readNextHeaderName("acceptedType");
            if(err != ErrorCode::OK && err != ErrorCode::XML_END)
            {
                return err;
            }
        }
        events.push_back(newEvent);
        err=readNextHeaderName("event");
    }
    return ErrorCode::OK;
}



int ConfigReader::parseStates()
{
    auto err=readNextHeaderName("state");
    if(err!=ErrorCode::OK)
    {
        return err;
    }
    while(err!=ErrorCode::XML_END)
    {
        QXmlStreamAttributes attributes = xml.attributes();
        State newState;
        for(auto &attr : attributes)
        {
            if(attr.name()=="name")
            {
                newState.name=attr.value().toString();
            }
            else if (attr.name().indexOf("read")>=0)
            {
                int index=getIndexFromName(attr.name());
                if (index<0) return ErrorCode::XML_ERROR;
                newState.details.readable[index]=attr.value().toString();
            }
            else if (attr.name().indexOf("desc")>=0)
            {
                int index=getIndexFromName(attr.name());
                if (index<0) return ErrorCode::XML_ERROR;
                newState.details.description[index]=attr.value().toString();
            }
            else
            {
                return ErrorCode::XML_ERROR;
            }
        }
        err=readNextHeaderName("values");
        if(err != ErrorCode::OK)
        {
            return err;
        }
        err=readNextHeaderName("value");
        if(err != ErrorCode::OK)
        {
            return err;
        }
        while (err != ErrorCode::XML_END)
        {
            QXmlStreamAttributes attributes = xml.attributes();
            StateElement newStateElement;
            for(auto &attr : attributes)
            {
                if(attr.name()=="name")
                {
                    newStateElement.name=attr.value().toString();
                }
                else if (attr.name().indexOf("read")>=0)
                {
                    int index=getIndexFromName(attr.name());
                    if (index<0) return ErrorCode::XML_ERROR;
                    newStateElement.details.readable[index]=attr.value().toString();
                }
                else if (attr.name().indexOf("desc")>=0)
                {
                    int index=getIndexFromName(attr.name());
                    if (index<0) return ErrorCode::XML_ERROR;
                    newStateElement.details.description[index]=attr.value().toString();
                }
                else
                {
                    return ErrorCode::XML_ERROR;
                }
            }
            newState.values.push_back(newStateElement);
            err=readNextHeaderName("value");
            if(err != ErrorCode::OK && err != ErrorCode::XML_END)
            {
                return err;
            }
        }
        err=readNextHeaderName("references");
        if(err != ErrorCode::OK)
        {
            return err;
        }
        err=readNextHeaderName("event");
        if(err != ErrorCode::OK && err != ErrorCode::XML_END)
        {
            return err;
        }
        while (err != ErrorCode::XML_END)
        {
            auto attributes = xml.attributes();
            if (attributes.hasAttribute("name"))
            {
                newState.references.push_back(attributes.value("name").toString());
            }
            else
            {
                return ErrorCode::XML_ERROR;
            }
            err=readNextHeaderName("event");
            if(err != ErrorCode::OK && err != ErrorCode::XML_END)
            {
                return err;
            }
        }
        states.push_back(newState);
        err=readNextHeaderName("state");
    }
    return ErrorCode::OK;
}


int ConfigReader::readNextHeaderName(const char *name)
{
    bool res=xml.readNextStartElement();
    while(res==false)
    {
        if(xml.hasError())
        {
            return ErrorCode::XML_ERROR;
        }
        if(xml.isEndElement() && xml.name() != name)
        {
            return ErrorCode::XML_END;
        }
        res=xml.readNextStartElement();
    }
#ifdef TRACE
    auto str=xml.qualifiedName();

    printf("%s",str.toUtf8().data());
    fflush(stdout);
#endif
    return ErrorCode::OK;
}

auto hasUnicode(const QString &str)
{
    return std::any_of(str.begin(),str.end(),
                       [](QChar c)->bool {return c.unicode()>127;});
}
void OldConfig::openModFolder(const QString &dirPath, const ConfigReader &reader, Generator &gen)
{
    using namespace std;
    QFileInfo fi(dirPath);
    modName = fi.fileName();
    cout<<"Opening mod. Name = `"<<modName.toUtf8().data()<<"`."<<endl;
    QDirIterator it(dirPath);
    while(it.hasNext())
    {
        QFileInfo typeDir(it.next());
        QString typeName = typeDir.fileName();

        if(typeName == "Voice")
        {
            modTypes[VOICE] = 1;
            cout<<"----------------VOICE-------------------"<<endl;
            cout<<"Found Voice mods info. Analysing."<<endl;
            openModTypeFolder(typeDir.absoluteFilePath(),reader,gen, VOICE);
            cout<<"----------------VOICE-------------------"<<endl;
        }
        else if(typeName == "SFX")
        {
            modTypes[SFX] = 1;
            cout<<"-----------------SFX--------------------"<<endl;
            cout<<"Found SFX mods info. Analysing."<<endl;
            openModTypeFolder(typeDir.absoluteFilePath(),reader,gen, SFX);
            cout<<"-----------------SFX--------------------"<<endl;
        }
        else if(typeName == "Loop")
        {
            modTypes[LOOP] = 1;
            cout<<"-----------------LOOP-------------------"<<endl;
            cout<<"Found Loop mods info. Analysing."<<endl;
            openModTypeFolder(typeDir.absoluteFilePath(),reader,gen, LOOP);
            cout<<"-----------------LOOP-------------------"<<endl;
        }
    }
    if(all_of(modTypes.begin(),modTypes.end(),[](int m)->bool{return m == 0;}))
    {
        cout<<"ERROR. No voice/sfx/loop dir"<<endl;
    }
}

void OldConfig::openModTypeFolder(const QString &dirPath, const ConfigReader &reader, Generator &gen, ModType type)
{
    using namespace std;
    QDirIterator it(dirPath, QStringList() << "*.wem", QDir::Files, QDirIterator::Subdirectories);
    int startIndex = dirPath.length();
    vector<QString> files;
    printf("Found 0 *.wem files.");
    int count = 0;
    while (it.hasNext()) {
        QString fileName = it.next();
        files.push_back(fileName.right(fileName.length()-startIndex));
        count+=1;
        printf("\rFound %d *.wem files.", count);
        //cout<< " - " << files[files.size()-1].toUtf8().data() << endl;
    }
    cout<<endl;
    printf("\rProcessing : 0/%d", count);
    int kk = 0;
    for(auto &path : files) {
        kk++;
        printf("\rProcessing : %d/%d", kk,count);
        QStringList list = path.split('/');        
        list.pop_front();
        if(list.size()>1)
        {
            auto& events = reader.events;
            QString eventName = "Play_"+list[0];
            auto found = find_if(events.begin(),events.end(),[&eventName] (const Event &ev) -> bool { return ev.name == eventName;});
            if(found != events.end())
            {
                auto& genEvents = gen.serializedEvents;
                auto found = find_if(genEvents.begin(),genEvents.end(),
                                     [&eventName] (const ExternalEvent &ev) -> bool { return ev.eventName == eventName;});
                if(found == genEvents.end())
                {
                    ExternalEvent event;
                    ExtendedEvent chain;
                    for(int i = 1, iEnd = list.size()-1; i < iEnd; ++i)
                    {
                        QString stateName = "!";
                        QString stateValue = list[i];
                        if(list[i].contains("__"))
                        {
                            stateName = list[i].left(list[i].indexOf("__"));
                            stateValue = list[i].right(list[i].length() - list[i].indexOf("__")-2);
                        }
                        chain.stateList.push_back(StateValue{stateName, stateValue});
                    }
                    auto str = dirPath + path;
                    QFileInfo fi(str);
                    if(hasUnicode(fi.fileName()))
                    {
                        QString newName = QString::number(qHash(str))+'.'+fi.completeSuffix();
                        chain.latinPathList.push_back(newName);
                        gen.latinFilesList.push_back(newName);

                    }
                    else
                    {
                        chain.latinPathList.push_back("");
                        gen.latinFilesList.push_back("");
                    }
                    chain.pathList.push_back(fi.fileName());
                    event.events[type].stateChains.push_back(chain);
                    gen.filesList.push_back(str);
                    event.eventName = eventName;
                    gen.serializedEvents.push_back(event);
                }
                else
                {
                    ExternalEvent& event = *found;
                    ExternalEventContainer& cont = event.events[type];
                    ExtendedEvent chain;
                    for(int i = 1, iEnd = list.size()-1; i < iEnd; ++i)
                    {
                        QString stateName = "!";
                        QString stateValue = list[i];
                        if(list[i].contains("__"))
                        {
                            stateName = list[i].left(list[i].indexOf("__"));
                            stateValue = list[i].right(list[i].length() - list[i].indexOf("__")-2);
                        }
                        chain.stateList.push_back(StateValue{stateName, stateValue});
                    }
                    auto str = dirPath + path;
                    QFileInfo fi(str);
                    if(hasUnicode(fi.fileName()))
                    {
                        QString newName = QString::number(qHash(str))+'.'+fi.completeSuffix();
                        chain.latinPathList.push_back(newName);
                        gen.latinFilesList.push_back(newName);

                    }
                    else
                    {
                        chain.latinPathList.push_back("");
                        gen.latinFilesList.push_back("");
                    }
                    chain.pathList.push_back(fi.fileName());
                    event.events[type].stateChains.push_back(chain);
                    gen.filesList.push_back(str);
                }
            }
            else
            {
                cout<<endl<<"Error. Event `"<<eventName.toUtf8().data()<<"` not found."<<endl;
            }
        }
        else if(list.size() == 1)
        {
            cout<<endl<<"Error. File placed in root directory. Can't process file: `"<<list[0].toUtf8().data()<<"`."<<endl;
        }
    }
    cout<<endl;
}
void OldConfig::mergeAndAlignAll(const ConfigReader &reader, Generator &gen)
{
    using namespace std;
    cout<<"Mod Building. Found " << gen.serializedEvents.size()<<" events."<<endl;
    for(auto &extEv : gen.serializedEvents)
    {
        for(int i = 0; i<3; ++i)
        {
            auto &extEvTyped = extEv.events[i];
            char sType = 'V';
            if(i==1)
                sType='S';
            else if(i==2)
                sType='L';

            extEvTyped.extID = sType + extEv.eventName.right(extEv.eventName.length()-5);
            for(auto &chain: extEvTyped.stateChains)
            {
                for(auto &state: chain.stateList)
                {
                    QString potentialStateName;
                    if(state.stateName!="!")
                        continue;
                    auto found = find_if(reader.states.begin(),reader.states.end(),[&state,&extEv](const State &realState)->bool{
                        return any_of(realState.values.begin(),realState.values.end(),[&state](const StateElement &elem) -> bool {
                                    return state.stateValue == elem.name;
                        }) && any_of(realState.references.begin(),realState.references.end(), [&extEv](const QString& evName)->bool{
                            return evName == extEv.eventName;
                        });
                      });
                    if(found != reader.states.end())
                    {
                        state.stateName = found->name;
                    }
                    else
                    {
                        cout<<"Error. Can't find state value `"<<state.stateValue.toUtf8().data()<<"` assosiated with any state. ";
                        if(chain.pathList.size()>0)
                        {
                            auto found = std::find_if(gen.filesList.begin(),
                                                      gen.filesList.end(),
                                                      [&chain](const QString &file)->bool { return file.contains(chain.pathList[0]);});
                            if(found!=gen.filesList.end())
                            {
                                cout<<"File `"<<QFileInfo(chain.pathList[0]).fileName().toUtf8().data()<<"` will not be copied.";
                                gen.filesList.erase(found);
                            }
                            found = std::find_if(gen.latinFilesList.begin(),
                                              gen.latinFilesList.end(),
                                              [&chain](const QString &file)->bool { return file.contains(chain.pathList[0]);});
                            if(found!=gen.latinFilesList.end())
                            {
                                gen.latinFilesList.erase(found);
                            }
                        }
                        cout<<endl;
                    }
                }
                sort(chain.stateList.begin(),chain.stateList.end());
            }
        }
    }
    for(auto &extEv : gen.serializedEvents)
    {
        for(auto &extEvTyped : extEv.events)
        {
            set<ExtendedEvent, PathComparator> merge;
            for(auto &chain : extEvTyped.stateChains)
            {
                auto res = merge.insert(chain);
                if(!res.second)
                {
                    res.first->pathList.insert(res.first->pathList.end(), chain.pathList.begin(), chain.pathList.end());
                    res.first->latinPathList.insert(res.first->latinPathList.end(), chain.latinPathList.begin(), chain.latinPathList.end());
                }
            }
            extEvTyped.stateChains.clear();
            extEvTyped.stateChains.insert(extEvTyped.stateChains.begin(),merge.begin(),merge.end());
        }
    }
}


