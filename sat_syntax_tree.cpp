#include <iostream>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cassert>

enum Operator
{
    Literal,
    AND,
    OR,
    NOT
};

// Define a custom struct
struct SAT_Expression
{
    Operator op;
    int literal;
    SAT_Expression *leftChild;
    SAT_Expression *rightChild; // ONLY rightChild exists if op is NOT!!!!

    // Constructor for a literal node
    SAT_Expression(const int &literalProp) : op(Operator::Literal), literal(literalProp), leftChild(nullptr), rightChild(nullptr) {}

    // Constructor for a non-literal node
    SAT_Expression(Operator operatorType, SAT_Expression *left, SAT_Expression *right)
        : op(operatorType), literal(-1), leftChild(left), rightChild(right)
    {
        // assert if op is NOT that left child is null
        if (op == Operator::NOT)
        {
            assert(leftChild == nullptr);
        }
    }

    // considers (a v b) v c differently from a v (b v c)
    bool operator==(const SAT_Expression &other) const
    {
        if (op != other.op || literal != other.literal)
        {
            return false;
        }
        else if (op == Operator::Literal)
        {
            return literal == other.literal;
        }

        // Compare left and right children recursively
        if (leftChild && other.leftChild)
        {
            if (!(*leftChild == *other.leftChild))
            {
                return false;
            }
        }
        else if (leftChild || other.leftChild)
        {
            return false; // One has a child, the other doesn't
        }

        if (rightChild && other.rightChild)
        {
            return *rightChild == *other.rightChild;
        }
        else
        {
            return !(rightChild || other.rightChild); // One has a child, the other doesn't
        }
    }

    bool isLiteral() const
    {
        return op == Operator::Literal;
    }

    void display() const
    {
        if (isLiteral())
        {
            std::cout << literal;
        }
        else
        {
            std::cout << "(";
            if (leftChild)
            {
                leftChild->display();
                std::cout << " ";
            }
            else
            {
                std::cout << "";
            }

            switch (op)
            {
            case Operator::AND:
                std::cout << "AND ";
                break;
            case Operator::OR:
                std::cout << "OR ";
                break;
            case Operator::NOT:
                std::cout << "NOT ";
                break;
            default:
                std::cout << "Unknown ";
            }

            if (rightChild)
            {
                rightChild->display();
            }
            else
            {
                std::cout << "";
            }
            std::cout << ")";
        }
    }
};

namespace std
{
    template <>
    struct hash<SAT_Expression>
    {
        std::size_t operator()(const SAT_Expression &expression) const
        {
            std::size_t hash = std::hash<int>()(static_cast<int>(expression.op));
            hash_combine(hash, expression.literal);

            // Hash left and right children recursively
            if (expression.leftChild)
            {
                hash_combine(hash, std::hash<SAT_Expression>()(*expression.leftChild));
            }

            if (expression.rightChild)
            {
                hash_combine(hash, std::hash<SAT_Expression>()(*expression.rightChild));
            }

            return hash;
        }

        // Helper function to combine hashes
        template <class T>
        void hash_combine(std::size_t &seed, const T &v) const
        {
            std::hash<T> hasher;
            seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
    };
}

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