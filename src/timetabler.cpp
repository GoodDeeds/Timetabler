#include "timetabler.h"

#include "Encoder.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
#include "MaxSATFormula.h"
#include "cclause.h"
#include "clauses.h"
#include "core/SolverTypes.h"
#include "mtl/Vec.h"
#include "tsolver.h"
#include "utils.h"

using namespace NSPACE;

/**
 * @brief      Constructs the Timetabler object.
 */
Timetabler::Timetabler() {
  solver = new TSolver(1, _CARD_TOTALIZER_);
  formula = new MaxSATFormula();
  formula->setProblemType(_WEIGHTED_);
}

/**
 * @brief      Adds clauses to the solver with specified weights.
 *
 * A negative weight implies that the clauses are hard, and a zero weight
 * implies that the clauses are not added to the solver.
 *
 * @param[in]  clauses  The clauses
 * @param[in]  weight   The weight
 */
void Timetabler::addClauses(const std::vector<CClause> &clauses, int weight) {
  for (unsigned i = 0; i < clauses.size(); i++) {
    vec<Lit> clauseVec;
    std::vector<Lit> clauseVector = clauses[i].getLits();
    for (unsigned j = 0; j < clauseVector.size(); j++) {
      clauseVec.push(clauseVector[j]);
    }
    addToFormula(clauseVec, weight);
  }
}

/**
 * @brief      Adds unit soft clauses for the high level variables to the
 * solver.
 */
void Timetabler::addHighLevelClauses() {
  for (unsigned i = 0; i < Global::FIELD_COUNT; i++) {
    for (unsigned j = 0; j < data.highLevelVars.size(); j++) {
      vec<Lit> highLevelClause;
      highLevelClause.clear();
      highLevelClause.push(mkLit(data.highLevelVars[j][i], false));
      addToFormula(highLevelClause, data.highLevelVarWeights[i]);
    }
  }
}

/**
 * @brief      Adds high level clauses for predefined constraints.
 *
 * For each predefined constraint C, a variable x is created, and a hard clause
 * x->C is added to the solver. A unit clause with x is added as a soft clause
 * with weight as specified for the constraint.
 *
 * @param[in]  clauseType  The clause type
 * @param[in]  course      The corresponding course index (-1 for if there is no
 * corresponding course)
 */
void Timetabler::addHighLevelConstraintClauses(PredefinedClauses clauseType,
                                               const int course) {
  if (course == -1) {
    assert(data.predefinedConstraintVars[clauseType].size() == 1);
    Lit l = mkLit(data.predefinedConstraintVars[clauseType][0], false);
    if (data.predefinedClausesWeights[clauseType] != 0) {
      addToFormula(l, data.predefinedClausesWeights[clauseType]);
    } else {
      addToFormula(l, -1);
    }
  } else {
    assert(data.predefinedConstraintVars[clauseType].size() ==
           data.courses.size());
    Lit l = mkLit(data.predefinedConstraintVars[clauseType][course], false);
    if (data.predefinedClausesWeights[clauseType] != 0) {
      addToFormula(l, data.predefinedClausesWeights[clauseType]);
    } else {
      addToFormula(l, -1);
    }
  }
}

/**
 * @brief      Adds high level custom constraint clauses.
 *
 * For each custom constraint C, a variable x is created, and a hard clause x->C
 * is added to the solver. A unit clause with x is added as a soft clause with
 * weight as specified for the constraint.
 *
 *
 * @param[in]  index   The index of the custom constraint variable
 * @param[in]  weight  The weight of the custom constraint
 */
void Timetabler::addHighLevelCustomConstraintClauses(int index, int weight) {
  Lit l = mkLit(data.customConstraintVars[index], false);
  if (weight != 0) {
    addToFormula(l, weight);
  } else {
    addToFormula(l, -1);
  }
}

/**
 * @brief      Adds unit clauses corresponding to existing assignments given in
 * the input to the solver.
 */
void Timetabler::addExistingAssignments() {
  for (unsigned i = 0; i < data.existingAssignmentVars.size(); i++) {
    for (unsigned j = 0; j < data.existingAssignmentVars[i].size(); j++) {
      for (unsigned k = 0; k < data.existingAssignmentVars[i][j].size(); k++) {
        if (data.existingAssignmentVars[i][j][k] == l_Undef) {
          continue;
        }
        vec<Lit> clause;
        clause.clear();
        if (data.existingAssignmentVars[i][j][k] == l_True) {
          clause.push(mkLit(data.fieldValueVars[i][j][k]));
        } else {
          clause.push(~mkLit(data.fieldValueVars[i][j][k]));
        }
        addToFormula(clause, data.existingAssignmentWeights[j]);
      }
    }
  }
}

/**
 * @brief      Add a given vec of literals with the given weight to the formula.
 *
 * A negative weight implies that the clauses are had, and a zero weight implies
 * that the clauses are not added to the solver.
 *
 * @param      input   The input
 * @param[in]  weight  The weight
 */
void Timetabler::addToFormula(vec<Lit> &input, int weight) {
  if (weight < 0) {
    formula->addHardClause(input);
  } else if (weight > 0) {
    formula->addSoftClause(weight, input);
  }
}

/**
 * @brief      Add single literal to the formula.
 *
 * @param[in]  input   The input
 * @param[in]  weight  The weight
 */
void Timetabler::addToFormula(Lit input, int weight) {
  vec<Lit> inputLits;
  inputLits.push(input);
  addToFormula(inputLits, weight);
}

/**
 * @brief      Adds clauses to the solver with specified weights.
 *
 * A negative weight implies that the clauses are had, and a zero weight implies
 * that the clauses are not added to the solver.
 *
 * @param[in]  clauses  The clauses
 * @param[in]  weight   The weight
 */
void Timetabler::addClauses(const Clauses &clauses, int weight) {
  addClauses(clauses.getClauses(), weight);
}

/**
 * @brief      Calls the solver to solve for the constraints.
 *
 * @return     True, if all high level variables were satisfied, False otherwise
 */
SolverStatus Timetabler::solve() {
  solver->loadFormula(formula);
  model = solver->tSearch();
  if (model.size() == 0) {
    return SolverStatus::Unsolved;
  }
  if (checkAllTrue(Utils::flattenVector<Var>(data.highLevelVars)) &&
      checkAllTrue(data.predefinedConstraintVars) &&
      checkAllTrue(data.customConstraintVars)) {
    return SolverStatus::Solved;
  }
  return SolverStatus::HighLevelFailed;
}

/**
 * @brief      Checks if a given set of variables are true in the model returned
 * by the solver.
 *
 * @param[in]  inputs  The input variables
 *
 * @return     True, if all variables are True, False otherwise
 */
bool Timetabler::checkAllTrue(const std::vector<Var> &inputs) {
  if (model.size() == 0) {
    return false;
  }
  for (unsigned i = 0; i < inputs.size(); i++) {
    if (model[inputs[i]] == l_False) {
      return false;
    }
  }
  return true;
}

/**
 * @brief      Checks if a given set of variables (in 2D vector) are true in the
 * model returned by the solver.
 *
 * @param[in]  inputs  The input variables
 *
 * @return     True, if all variables are True, False otherwise
 */
bool Timetabler::checkAllTrue(const std::vector<std::vector<Var>> &inputs) {
  if (model.size() == 0) {
    return false;
  }
  for (auto &i : inputs) {
    for (auto &j : i) {
      if (model[j] == l_False) return false;
    }
  }
  return true;
}

/**
 * @brief      Determines if a given variable is true in the model returned by
 * the solver.
 *
 * @param[in]  v     The variable to be checked
 *
 * @return     True if variable true, False otherwise
 */
bool Timetabler::isVarTrue(const Var &v) {
  if (model.size() == 0) {
    return false;
  }
  if (model[v] == l_False) {
    return false;
  }
  return true;
}

/**
 * @brief      Calls the formula to issue a new variable and returns it.
 *
 * @return     The new Var added to the formula
 */
Var Timetabler::newVar() {
  Var var = formula->nVars();
  formula->newVar();
  return var;
}

/**
 * @brief      Calls the formula to issue a new literal and returns it.
 *
 * @param[in]  sign  The sign, whether the literal contains a sign with the
 * variable
 *
 * @return     The Lit corresponding to the new Var added to the formula
 */
Lit Timetabler::newLiteral(bool sign) {
  Lit p = mkLit(formula->nVars(), sign);
  formula->newVar();
  return p;
}

/**
 * @brief      Prints the result of the problem.
 */
void Timetabler::printResult(SolverStatus status) {
  if (status == SolverStatus::Solved) {
    LOG(INFO) << "All high level clauses were satisfied";
    displayChangesInGivenAssignment();
    displayTimeTable();
  } else if (status == SolverStatus::HighLevelFailed) {
    LOG(WARNING) << "Some high level clauses were not satisfied";
    displayUnsatisfiedOutputReasons();
    displayChangesInGivenAssignment();
    displayTimeTable();
  } else {
    LOG(WARNING) << "Not Solved";
  }
}

/**
 * @brief      Displays  the changes that have been made to the default
 * assignment given by the user as input by the solver
 */
void Timetabler::displayChangesInGivenAssignment() {
  for (unsigned i = 0; i < data.existingAssignmentVars.size(); i++) {
    for (unsigned j = 0; j < data.existingAssignmentVars[i].size(); j++) {
      for (unsigned k = 0; k < data.existingAssignmentVars[i][j].size(); k++) {
        if (data.existingAssignmentVars[i][j][k] == l_True &&
            model[data.fieldValueVars[i][j][k]] == l_False) {
          LOG(WARNING) << "Value of field "
                       << Utils::getFieldTypeName(FieldType(j)) << " "
                       << Utils::getFieldName(FieldType(j), k, data)
                       << " for course " << data.courses[i].getName()
                       << " changed from 'True' to 'False'";
        } else if (data.existingAssignmentVars[i][j][k] == l_False &&
                   model[data.fieldValueVars[i][j][k]] == l_True) {
          LOG(WARNING) << "Value of field "
                       << Utils::getFieldTypeName(FieldType(j)) << " "
                       << Utils::getFieldName(FieldType(j), k, data)
                       << " for course " << data.courses[i].getName()
                       << " changed from 'False' to 'True'";
        }
      }
    }
  }
}

/**
 * @brief      Displays the generated time table.
 */
void Timetabler::displayTimeTable() {
  for (unsigned i = 0; i < data.courses.size(); i++) {
    LOG(INFO) << "Course : " << data.courses[i].getName();
    for (unsigned j = 0; j < data.fieldValueVars[i][FieldType::slot].size();
         j++) {
      if (isVarTrue(data.fieldValueVars[i][FieldType::slot][j])) {
        LOG(INFO) << "Slot : " << data.slots[j].getName();
      }
    }
    for (unsigned j = 0;
         j < data.fieldValueVars[i][FieldType::instructor].size(); j++) {
      if (isVarTrue(data.fieldValueVars[i][FieldType::instructor][j])) {
        LOG(INFO) << "Instructor : " << data.instructors[j].getName();
      }
    }
    for (unsigned j = 0;
         j < data.fieldValueVars[i][FieldType::classroom].size(); j++) {
      if (isVarTrue(data.fieldValueVars[i][FieldType::classroom][j])) {
        LOG(INFO) << "Classroom : " << data.classrooms[j].getName();
      }
    }
    for (unsigned j = 0; j < data.fieldValueVars[i][FieldType::segment].size();
         j++) {
      if (isVarTrue(data.fieldValueVars[i][FieldType::segment][j])) {
        LOG(INFO) << "Segment : " << data.segments[j].getName();
      }
    }
    for (unsigned j = 0; j < data.fieldValueVars[i][FieldType::isMinor].size();
         j++) {
      if (isVarTrue(data.fieldValueVars[i][FieldType::isMinor][j])) {
        LOG(INFO) << "Is Minor : " << data.isMinors[j].getName();
      }
    }
    for (unsigned j = 0; j < data.fieldValueVars[i][FieldType::program].size();
         j++) {
      if (isVarTrue(data.fieldValueVars[i][FieldType::program][j])) {
        LOG(INFO) << "Program : " << data.programs[j].getNameWithType();
      }
    }
    LOG(INFO) << "";
  }
}

/**
 * @brief      Writes the generated time table to a CSV file.
 *
 * @param[in]  fileName  The file path of the output CSV file
 */
void Timetabler::writeOutput(std::string fileName) {
  std::ofstream fileObject;
  fileObject.open(fileName);
  fileObject << "name,class_size,instructor,segment,is_minor,";
  for (unsigned i = 0; i < data.programs.size(); i += 2) {
    fileObject << data.programs[i].getName() << ",";
  }
  fileObject << "classroom,slot" << std::endl;
  for (unsigned i = 0; i < data.courses.size(); i++) {
    fileObject << data.courses[i].getName() << ","
               << data.courses[i].getClassSize() << ",";
    for (unsigned j = 0;
         j < data.fieldValueVars[i][FieldType::instructor].size(); j++) {
      if (isVarTrue(data.fieldValueVars[i][FieldType::instructor][j])) {
        fileObject << data.instructors[j].getName();
      }
    }
    fileObject << ",";
    for (unsigned j = 0; j < data.fieldValueVars[i][FieldType::segment].size();
         j++) {
      if (isVarTrue(data.fieldValueVars[i][FieldType::segment][j])) {
        fileObject << data.segments[j].getName();
      }
    }
    fileObject << ",";
    for (unsigned j = 0; j < data.fieldValueVars[i][FieldType::isMinor].size();
         j++) {
      if (isVarTrue(data.fieldValueVars[i][FieldType::isMinor][j])) {
        fileObject << data.isMinors[j].getName();
      }
    }
    fileObject << ",";
    for (unsigned j = 0; j < data.fieldValueVars[i][FieldType::program].size();
         j += 2) {
      if (isVarTrue(data.fieldValueVars[i][FieldType::program][j])) {
        fileObject << data.programs[j].getCourseTypeName() << ",";
      } else if (isVarTrue(data.fieldValueVars[i][FieldType::program][j + 1])) {
        fileObject << data.programs[j + 1].getCourseTypeName() << ",";
      } else {
        fileObject << "No,";
      }
    }
    for (unsigned j = 0;
         j < data.fieldValueVars[i][FieldType::classroom].size(); j++) {
      if (isVarTrue(data.fieldValueVars[i][FieldType::classroom][j])) {
        fileObject << data.classrooms[j].getName();
      }
    }
    fileObject << ",";
    for (unsigned j = 0; j < data.fieldValueVars[i][FieldType::slot].size();
         j++) {
      if (isVarTrue(data.fieldValueVars[i][FieldType::slot][j])) {
        fileObject << data.slots[j].getName();
      }
    }
    fileObject << std::endl;
  }
  fileObject.close();
}

/**
 * @brief      Displays the reasons due to which the formula could not be
 *             satisfied, if applicable.
 */
void Timetabler::displayUnsatisfiedOutputReasons() {
  for (unsigned i = 0; i < data.highLevelVars.size(); i++) {
    for (unsigned j = 0; j < data.highLevelVars[i].size(); j++) {
      if (!isVarTrue(data.highLevelVars[i][j])) {
        LOG(WARNING) << "Field : " << Utils::getFieldTypeName(FieldType(j))
                     << " of Course : " << data.courses[i].getName()
                     << " could not be satisfied";
      }
    }
  }
  for (unsigned i = 0; i < data.predefinedConstraintVars.size(); i++) {
    if (!checkAllTrue(data.predefinedConstraintVars[i]) &&
        data.predefinedClausesWeights[i] != 0) {
      LOG(WARNING) << "Predefined Constraint : "
                   << Utils::getPredefinedConstraintName(PredefinedClauses(i))
                   << " could not be satisfied";
    }
  }
  for (unsigned i = 0; i < data.customConstraintVars.size(); i++) {
    if (!isVarTrue(data.customConstraintVars[i])) {
      LOG(WARNING) << "Custom Constraint : " << i + 1 << " for course "
                   << data.courses[data.customMap[i + 1]].getName()
                   << " could not be satisfied";
    }
  }
}

Clauses Timetabler::generateAtMostKTotalizerEncoding(
    const std::vector<Var> &relaxationVars, int64_t rhs) {
    vec<Lit> encodingLits;
    vec<Lit> encodingAssumptions;
    for (auto &v : relaxationVars) {
        encodingLits.push(mkLit(v, false));
    }
    TTotalizer totalizerEncoder;
    totalizerEncoder.build(formula, encodingLits, rhs);
    totalizerEncoder.update(formula, rhs, encodingLits, encodingAssumptions);
    return Clauses(encodingAssumptions);
}

/**
 * @brief      Destroys the object, and deletes the solver.
 */
Timetabler::~Timetabler() { delete solver; }
