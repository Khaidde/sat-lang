#include "src/sat_syntax_tree.hpp"
// take arbitrary formula of AND, NOT, OR, and literals, and convert to CNF form using Tseitin transformation

int main()
{
    std::unordered_map<SAT_Expression, int> expressionMap;

    // Example usage
    SAT_Expression expression1(0);
    SAT_Expression *expression2 = new SAT_Expression(Operator::AND, new SAT_Expression(1), new SAT_Expression(2));
    SAT_Expression expression_combined2(Operator::AND, new SAT_Expression(0), expression2);
    SAT_Expression *expression3 = new SAT_Expression(Operator::AND, new SAT_Expression(0), new SAT_Expression(1));
    SAT_Expression expression_combined3(Operator::AND, expression3, new SAT_Expression(2));
    SAT_Expression *expression3_duplicate = new SAT_Expression(Operator::AND, new SAT_Expression(0), new SAT_Expression(1));
    SAT_Expression expression_combined3_duplicate(Operator::AND, expression3_duplicate, new SAT_Expression(2));

    expressionMap[expression1] = 1;
    expressionMap[expression_combined2] = 2;
    expressionMap[expression_combined3] = 3;

    // print expressionMap
    for (auto &x : expressionMap)
    {
        x.first.display();
        std::cout << " : " << x.second << std::endl;
    }

    return 0;
}
