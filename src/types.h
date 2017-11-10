#ifndef TYPES_H
#define TYPES_H
#include <array>
#include <QString>
#include <vector>
#include <set>
#include <algorithm>
enum ModType {
    VOICE,
    SFX,
    LOOP,
    COUNT
};
struct ElementDescription
{

    std::array<QString,3> readable;
    std::array<QString,3> description;
};

struct ExternalType
{
    QString name;
    QString id;
    ElementDescription details;
};

struct Event
{
    QString name;
    std::array<ExternalType,3> acceptedTypes;
    ElementDescription details;
};

struct StateElement
{
    QString name;
    ElementDescription details;
};

struct State
{
    QString name;
    QString realName;
    std::vector<QString> references;
    std::vector<StateElement> values;
    ElementDescription details;
};



struct StateValue{
    QString stateName;
    QString stateValue;
    bool operator <(const StateValue &right) const
    {
        return this->stateValue < right.stateValue;
    }
};

struct ExtendedEvent:Event{
    std::vector<StateValue> stateList;
    mutable std::vector<QString> pathList;
    mutable std::vector<QString> latinPathList;
};
struct PathComparator{
    bool operator() (const ExtendedEvent &lhs,
                     const ExtendedEvent &rhs) const
    {
        return std::lexicographical_compare(lhs.stateList.begin(),lhs.stateList.end(),
                                     rhs.stateList.begin(),rhs.stateList.end());
    }
};
struct ExternalEventContainer{
    std::vector<ExtendedEvent> stateChains;
    QString extID;
};
struct ExternalEvent{
    std::array<ExternalEventContainer,3> events;
    std::vector<QString> names={"Voice","SFX","Loop"};
    std::vector<QString> extIds={"","",""};
    QString eventName;
};

#endif // TYPES_H
