#ifndef SLOT_H
#define SLOT_H

#include <vector>
#include <string>
#include "fields/field.h"
#include "fields/is_minor.h"
#include "global.h"

enum class Day {
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday
};

class Time {
private:
    unsigned hours;
    unsigned minutes;
public:
    Time(unsigned, unsigned);
    Time(std::string);
    Time& operator=(const Time&);
    bool operator==(const Time&);
    bool operator<(const Time&);
    bool operator<=(const Time&);  
    bool operator>=(const Time&);
    bool operator>(const Time&);
    std::string getTimeString();
    bool isMorningTime();
};

class SlotElement {
private:
    Time startTime, endTime;
    Day day;
public:
    SlotElement(Time&, Time&, Day);
    bool isIntersecting(SlotElement &other);
    bool isMorningSlotElement();
};

class Slot : public Field {
private:
    std::string name;
    IsMinor isMinor;
    std::vector<SlotElement> slotElements;
public:
    Slot(std::string, IsMinor, std::vector<SlotElement>);
    bool operator==(const Slot &other);
    bool isIntersecting(Slot &other);
    void addSlotElements(SlotElement);
    bool isMinorSlot();
    FieldType getType();
    std::string getTypeName();
    std::string getName();
    bool isMorningSlot();
};

#endif