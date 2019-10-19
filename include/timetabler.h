/** @file */

#ifndef TIMETABLER_H
#define TIMETABLER_H

#include <vector>
#include "MaxSATFormula.h"
#include "cclause.h"
#include "core/SolverTypes.h"
#include "data.h"
#include "mtl/Vec.h"
#include "totalizer.h"
#include "tsolver.h"

using namespace NSPACE;

/**
 * @brief      Enum to store the solver status
 */
enum SolverStatus {
  /**
   * A timetable could be generated with the given constraints
   */
  Solved,
  /**
   * Some high level constraints could not be satisfied
   */
  HighLevelFailed,
  /**
   * Some hard clauses could not be satisfied
   */
  Unsolved
};

/**
 * @brief      Class for time tabler.
 *
 * This class is responsible for handling the solver.
 * This accepts constraints, adds it to the MaxSATFormula,
 * calls the solver, and interprets and displays the result.
 * It is also responsible for writing the output to a CSV,
 * and creating new literals or variables in the solver when
 * requested and returning them.
 */
class Timetabler {
 private:
  /**
   * A pointer to the MaxSAT solver object
   */
  TSolver *solver;
  /**
   * A pointer to the MaxSAT formula object
   */
  MaxSATFormula *formula;
  /**
   * Stores the values of each solver variable to be checked after solving
   */
  std::vector<lbool> model;

 public:
  /**
   * Stores all the field and input data obtained by the parser
   */
  Data data;
  Timetabler();
  ~Timetabler();
  void addClauses(const std::vector<CClause> &, int);
  void addClauses(const Clauses &, int);
  bool checkAllTrue(const std::vector<Var> &);
  bool checkAllTrue(const std::vector<std::vector<Var>> &);
  bool isVarTrue(const Var &);
  SolverStatus solve();
  Var newVar();
  Lit newLiteral(bool sign = false);
  void printResult(SolverStatus);
  void displayTimeTable();
  void displayUnsatisfiedOutputReasons();
  void addHighLevelClauses();
  void addHighLevelConstraintClauses(PredefinedClauses, const int course);
  void addHighLevelCustomConstraintClauses(int, int);
  void writeOutput(std::string);
  void addExistingAssignments();
  void addToFormula(vec<Lit> &, int);
  void addToFormula(Lit, int);
  void displayChangesInGivenAssignment();
  Clauses generateAtMostKTotalizerEncoding(const std::vector<Var> &,
                                             int64_t rhs = 1);
};

#endif
