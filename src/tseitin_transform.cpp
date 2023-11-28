#include "tseitin_transform.hpp"

#include "sat_syntax_tree.hpp"
#include <fstream>
#include <iostream>
#include <set>
#include <vector>
// take arbitrary formula of AND, NOT, OR, and literals, and convert to CNF form using Tseitin transformation

namespace slang {

// function takes (expression, unordered_map of SAT_Expression to int (int is the new proposition we add), set of all
// int props ) this function's job is to simply recurse and populate expressionMap, it's simple to construct CNF from
//  expressionMap and expression, do that with outer function
// If expressionmap is empty that means its just a literal, so that's in CNF already
// returns the proposition we map this expression to.
int convert_to_prop(SAT_Expression *expression, std::unordered_map<SAT_Expression, int> &expressionMap,
                    //    std::unordered_map<int, SAT_Expression> &newExpressions,
                    int &nextUnusedProp) {
  if (expression == nullptr) {
    return -1;
  } else if (expression->isLiteral()) {
    return expression->literal;
  } else {
    int leftProp  = convert_to_prop(expression->leftChild, expressionMap, nextUnusedProp);
    int rightProp = convert_to_prop(expression->rightChild, expressionMap, nextUnusedProp);
    // now expressionMap should have mappings for our left and right

    // mutate expression:
    if (leftProp != -1) {
      expression->leftChild = new SAT_Expression(leftProp);
    }
    expression->rightChild = new SAT_Expression(rightProp);

    auto it = expressionMap.find(*expression);
    if (it != expressionMap.end()) {
      // already have a mapping
      int value = it->second;
      return value;
    } else {
      expressionMap[*expression] = nextUnusedProp;
      int ret                    = nextUnusedProp++;
      return ret;
    }
  }
}

std::set<int> set_of_props(SAT_Expression *expression) {
  std::set<int> props;
  if (expression == nullptr) {
    return props;
  } else if (expression->isLiteral()) {
    props.insert(expression->literal);
    return props;
  } else {
    std::set<int> leftProps  = set_of_props(expression->leftChild);
    std::set<int> rightProps = set_of_props(expression->rightChild);
    props.insert(leftProps.begin(), leftProps.end());
    props.insert(rightProps.begin(), rightProps.end());
    return props;
  }
}

// Function that takes a SAT_Expression and the int representing the proposition we mapped it to and returns a vector of
// vector of ints for the clauses
//  that represent the CNF form of the expression
std::vector<std::vector<int>> clauses_representing_biconditional_mapping(const SAT_Expression *expression, int prop) {
  std::vector<std::vector<int>> clauses;
  switch (expression->op) {
  case Operator::AND: {
    // (prop <-> a ^ b) = (-a v -b v prop) ^ (a v -prop) ^ (b v -prop)
    std::vector<int> clause1;
    clause1.push_back(prop);
    clause1.push_back(-expression->leftChild->literal);
    clause1.push_back(-expression->rightChild->literal);
    clauses.push_back(clause1);

    std::vector<int> clause2;
    clause2.push_back(-prop);
    clause2.push_back(expression->leftChild->literal);
    clauses.push_back(clause2);

    std::vector<int> clause3;
    clause3.push_back(-prop);
    clause3.push_back(expression->rightChild->literal);
    clauses.push_back(clause3);
    break;
  } // end case AND
  case Operator::OR: {
    // (prop <-> a v b) = (a v b v -prop) ^ (-a v prop) ^ (-b v prop)
    std::vector<int> clause1;
    clause1.push_back(-prop);
    clause1.push_back(expression->leftChild->literal);
    clause1.push_back(expression->rightChild->literal);
    clauses.push_back(clause1);

    std::vector<int> clause2;
    clause2.push_back(prop);
    clause2.push_back(-expression->leftChild->literal);
    clauses.push_back(clause2);

    std::vector<int> clause3;
    clause3.push_back(prop);
    clause3.push_back(-expression->rightChild->literal);
    clauses.push_back(clause3);
    break;
  } // end case OR
  case Operator::NOT: {
    // (prop <-> -b) = (prop v b) ^ (-prop v -b)
    std::vector<int> clause1;
    clause1.push_back(prop);
    clause1.push_back(expression->rightChild->literal);
    clauses.push_back(clause1);
    std::vector<int> clause2;
    clause2.push_back(-prop);
    clause2.push_back(-expression->rightChild->literal);
    clauses.push_back(clause2);
    break;
  } // end case NOT
  default: assert(!"Unreachable"); break;
  }
  return clauses;
}

// list of clauses, where each clause is a list of ints OR'd together
std::vector<std::vector<int>> to_cnf(SAT_Expression *expression) {
  std::vector<std::vector<int>> cnf;
  std::set<int> all_props = set_of_props(expression);
  // get max of set_of_props
  int lastUsedProp   = *all_props.rbegin();
  int nextUnusedProp = lastUsedProp + 1;
  std::unordered_map<SAT_Expression, int> expressionMap;
  int overall_prop = convert_to_prop(expression, expressionMap, nextUnusedProp);
  // add overall_prop to cnf
  std::vector<int> overall_prop_clause;
  overall_prop_clause.push_back(overall_prop);
  cnf.push_back(overall_prop_clause);

  // iterate over all expressions in expressionMap and add the clauses they generate to cnf
  for (auto &x : expressionMap) {
    std::vector<std::vector<int>> clauses = clauses_representing_biconditional_mapping(&x.first, x.second);
    cnf.insert(cnf.end(), clauses.begin(), clauses.end());
  }
  return cnf;
}

// Helper function to calculate the number of variables
int getNumberOfVariables(const std::vector<std::vector<int>> &cnf) {
  int maxVariable = 0;

  for (const auto &clause : cnf) {
    for (const auto &literal : clause) {
      int variable = std::abs(literal);
      assert(variable != 0); // 0 is not a valid variable
      maxVariable = std::max(maxVariable, variable);
    }
  }

  return maxVariable;
}

void output_dimacs(const std::vector<std::vector<int>> &cnf, std::string filename) {
  std::ofstream outFile(filename);

  if (!outFile) {
    std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
    return;
  }

  // Write the DIMACS header
  outFile << "p cnf " << getNumberOfVariables(cnf) << " " << cnf.size() << std::endl;

  // Write each clause to the file
  for (const auto &clause : cnf) {
    for (const auto &literal : clause) {
      outFile << literal << " ";
    }
    outFile << "0\n"; // Terminate the clause with '0'
  }

  outFile.close();
}

} // namespace slang
