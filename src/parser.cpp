#include "parser.h"

Parser::Parser(TimeTabler* timeTabler) {
    this->timeTabler = timeTabler;
}

void Parser::parseFields(std::string &file) {
    YAML::Node config = YAML::LoadFile(file);

    YAML::Node instructorsConfig = config["instructors"];
    for (YAML::Node& instructorNode : instructorsConfig) {
        Instructor instructor(instructorNode.as<std::string>());
        timeTabler->data.instructors.push_back(instructor);
    }

    YAML::Node classroomsConfig = config["classrooms"];
    for (YAML::Node& classroomNode : classroomsConfig) {
        Classroom classroom(classroomNode["number"].as<std::string>(), classroomNode["size"].as<unsigned>());
        timeTabler->data.classrooms.push_back(classroom);
    }

    YAML::Node segmentsConfig = config["segments"];
    unsigned segmentStart = segmentsConfig["start"].as<unsigned>();
    unsigned segmentEnd = segmentsConfig["end"].as<unsigned>();
    for (unsigned i = segmentStart; i<segmentEnd; ++i) {
        for (unsigned j = i; j<segmentEnd; ++j) {
            timeTabler->data.segments.push_back(Segment(i, j));
        }
    }

    YAML::Node slotsConfig = config["slots"];
    for (YAML::Node& slotNode : slotsConfig) {
        std::vector<SlotElement> slotElements;
        IsMinor isMinor = (slotNode["is_minor"].as<bool>()) ? IsMinor::isMinorCourse : IsMinor::isNotMinorCourse;
        for (YAML::Node& periodNode : slotElements["time_periods"]) {
            Time start(periodNode["start"].as<std::string>());
            Time end(periodNode["end"].as<std::string>())
            Day day = getDayFromString(periodNode["day"].as<std::string>());
            slotElements.push_back(SlotElement(start, end, day));
        }
        timeTabler->data.slots.push_back(Slot(slotNode["name"].as<std::string>(), isMinor, slotElements));
    }

    timeTabler->data.isMinors.push_back(IsMinor(MinorType::isMinorCourse));
    timeTabler->data.isMinors.push_back(IsMinor(MinorType::isNotMinorCourse));

    YAML::Node programsConfig = config["programs"];
    for (YAML::Node& programNode : programsConfig) {
        std::string name = programNode.as<std::string>();
        programsConfig.push_back(Program(name, CourseType::core));
        programsConfig.push_back(Program(name, CourseType::elective));
    }

}

Day Parser::getDayFromString(std::string &day) {
    if (day == "Monday") return Day::Monday;
    if (day == "Tuesday") return Day::Tuesday;
    if (day == "Wednesday") return Day::Wednesday;
    if (day == "Thursady") return Day::Thursady;
    if (day == "Friday") return Day::Friday;
    if (day == "Saturday") return Day::Saturday;
    if (day == "Sunday") return Day::Sunday;
    assert(false && "Incorrect day");
    return Day::Monday;
}

void Parser::parseInput(std::string &file) {
    csv::Parser parser(file);
    for (unsigned i=0; i<parser.rowCount; ++i) {
        std::string name = parser[i]["name"];
        std::string classSizeStr = parser[i]["class_size"];
        unsigned classSize = unsigned(std::stoi(classSizeStr));
        std::string instructorStr = parser[i]["instructor"];
        Instructor *instructor = null_ptr;
        for (unsigned i = 0; i < timeTabler->data.instructors.size(); i++) {
            if (timeTabler->data.instructors[i].getName() == instructorStr) {
                instructor = &(timeTabler->data.instructors[i]);
                break;
            }
        }
        if (instructor == null_ptr) {
            // TODO Error and exit
        }
        std::string segmentStr = parser[i]["segment"];
        Segment *segment = null_ptr;
        for (unsigned i = 0; i < timeTabler->data.segments.size(); i++) {
            if (timeTabler->data.segments[i].toString() == segmentStr) {
                segment = &(timeTabler->data.segments[i]);
                break;
            }
        }
        if (segment == null_ptr) {
            // TODO Error and exit
        }
        std::string segment(std::stoi(segmentStr[0]), std::stoi(segmentStr[1]));
        std::string isMinorStr = parser[i]["is_minor"];
        IsMinor *isMinor = null_ptr;
        if (isMinorStr == "Yes") {
            isMinor = timeTabler->data.isMinors[0];
        } else if (isMinorStr == "No") {
            isMinor = timeTabler->data.IsMinors[1];
        } else {
            // TODO Error and exit
        }
        Course course(name, classSize, instructor, segment, isMinor);

        for (unsigned j = 0; j < timeTabler->data.programs.size(); j+=2) {
            std::string s = timeTabler->data.programs[j].getName();
            if (parser[i][s] == "Core") {
                course.addProgram(&(timeTabler->data.programs[j]));
            } else if (parser[i][s] == "Elective") {
                course.addProgram(&(timeTabler->data.programs[j+1]));
            } else {
                // TODO Error and exit
            }
        }

        timeTabler->data.courses.push_back(course);
    }
}

void Praser::addVars() {
    for (Course c : timeTabler->data.courses) {
        std::vector<std::vector<Lit>> courseVars;
        courseVars.resize(Global::FIELD_COUNT);
        for (Classroom cr : timeTabler->data.classrooms) {
            Lit v = timeTabler->solver.newLiteral();// TODO create vars using solver
            courseVars[FieldType::classroom].push_back(v);
        }
        for (Instructor i : timeTabler->data.instructor) {
            Lit v = timeTabler->solver.newLiteral()
            courseVars[FieldType::instructor].push_back(v);
        }
        for (IsMinor i : timeTabler->data.isMinors) {
            Lit v = timeTabler->solver.newLiteral()
            courseVars[FieldType::isMinor].push_back(v);
        }
        for (Program p : timeTabler->data.programs) {
            Lit v = timeTabler->solver.newLiteral()
            courseVars[FieldType::program].push_back(v);
        }
        for (Segment s : timeTabler->data.segments) {
            Lit v = timeTabler->solver.newLiteral()
            courseVars[FieldType::segment].push_back(v);
        }
        for (Slot s : timeTabler->data.slots) {
            Lit v = timeTabler->solver.newLiteral()
            courseVars[FieldType::slot].push_back(v);
        }
        timeTabler->data.fieldValueVars.push_back(courseVars);

        std::vector<Lit> highLevelCourseVars;
        for (unsigned i = 0; i < Global::FIELD_COUNT; ++i) {
            Lit v = timeTabler->solver.newLiteral()
            highLevelCourseVars.push_back(v);
        }
        timeTabler->data.highLevelVars.push_back(highLevelCourseVars)
    }
}