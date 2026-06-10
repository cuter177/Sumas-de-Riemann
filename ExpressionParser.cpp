#include "ExpressionParser.h"

#include<iostream>
#include<stdexcept>
#include <math.h>
#include <cmath>

Expression_Parser::Expression_Parser(const std::vector<std::string>& input)
{
    tokens = input;//take in the expression
};



std::shared_ptr<node> Expression_Parser::toTree()
{
    try {
        //Loop through all tokens in expression
        for (unsigned int i = 0; i < tokens.size(); i++)
        {
            //if the token is not an operator or function
            if (!(tokens[i] == "+" ||
                tokens[i] == "-" ||
                tokens[i] == "/" ||
                tokens[i] == "*" ||
                tokens[i] == "^" ||
                tokens[i] == "cosh" ||
                tokens[i] == "tanh" ||
                tokens[i] == "sinh" ||
                tokens[i] == "arccos" ||
                tokens[i] == "arctan" ||
                tokens[i] == "arcsin" ||
                tokens[i] == "ln" ||
                tokens[i] == "abs" ||
                tokens[i] == "E" ||
                tokens[i] == "r" ||
                tokens[i] == "c" ||
                tokens[i] == "cos" ||
                tokens[i] == "tan" ||
                tokens[i] == "sen" ||
                tokens[i] == "sin"))

                expression_tree.emplace_back(new node(tokens[i]));//add the token to the expression tree
            else
            {
                if (tokens[i] == "cos" ||//if the token is a function
                    tokens[i] == "tan" ||
                    tokens[i] == "sin" ||
                    tokens[i] == "sen" ||
                    tokens[i] == "cosh" ||
                    tokens[i] == "tanh" ||
                    tokens[i] == "sinh" ||
                    tokens[i] == "arccos" ||
                    tokens[i] == "arctan" ||
                    tokens[i] == "arcsin" ||
                    tokens[i] == "r" ||
                    tokens[i] == "c" ||
                    tokens[i] == "ln" ||
                    tokens[i] == "E" ||
                    tokens[i] == "abs") {


                    temp = std::make_shared<node>((tokens[i]));//create a new node
                    temp->right = expression_tree[expression_tree.size() - 1];//have the right tree contain the furthest token in the vector
                    expression_tree.pop_back();//take the furthest token in the vector away
                    expression_tree.push_back(temp);//put the new token in the vector
                }

                else//if the token is an operator
                {
                    temp = std::make_shared<node>(tokens[i]);//create a new node
                    temp->right = (expression_tree[expression_tree.size() - 1]);//have the right tree set to the furthest token in the vector
                    expression_tree.pop_back();//take the furthest token in the vector away

                    if (expression_tree.size() > 0)
                    {
                        temp->left = (expression_tree[expression_tree.size() - 1]);//have the left tree set to the furthest token in the vector
                        expression_tree.pop_back();//take the furthest token in the vector away
                    }

                    expression_tree.push_back(temp);//put the new token in the vector
                }
            }

        }
        temp = expression_tree.size() > 0 ?  expression_tree[expression_tree.size() - 1] : nullptr;//get the root node of the expression tree
        expression_tree.resize(0);//reset the expression tree
        return temp;//return the root node of the expression tree
    }


    catch (std::out_of_range)
    {
        std::cout << "ERROR : OUT OF BOUNDS" << std::endl;
        return nullptr;
    }
}

long double Expression_Parser::exponencial(const long double valor){
    long double exp = 2.71828182845904523536*valor;
    return exp;
}

long double Expression_Parser::evaluateExpressionTree(const std::shared_ptr<node>& root)
{
    if (root == nullptr) return 0;
    //Evaluate each token as they should be evaluated using a recursive approach. This will result in a final answer
    if (root->data == "+")
        return evaluateExpressionTree(root->left) + evaluateExpressionTree(root->right);
    if (root->data == "-")
    {
        if(root->left == nullptr)
            return -evaluateExpressionTree(root->right);
        else
            return evaluateExpressionTree(root->left) - evaluateExpressionTree(root->right);
    }

    if (root->data == "*")
        return evaluateExpressionTree(root->left) * evaluateExpressionTree(root->right);
    if (root->data == "/")
        return evaluateExpressionTree(root->left) / evaluateExpressionTree(root->right);
    if (root->data == "^")
    {
        long double left = evaluateExpressionTree(root->left);
        long double right = evaluateExpressionTree(root->right);
        long double ans;

        // si la potencia es negativa, entonces se debe retornar 1/pow(left, abs(right))
        if (root->right->data.size() > 1 && root->right->data[0] == '-')
        {
            return (1 / (pow(left, abs(right))));
        }
        //si el token izquierdo es negativo y el derecho es un entero, maneje el signo según la paridad de la derecha
        else if (left < 0 && floor(right) == right)
        {
            if (static_cast<int>(right) % 2 != 0) {
                ans = -(pow(abs(left), right));
            }
            else {
                ans = pow(abs(left), right);
            }
        }
        else {
            ans = pow(left, right);
        }
        return ans;
    }

    if (root->data == "cos")
        return cos(evaluateExpressionTree(root->right));
    if (root->data == "sin")
        return sin(evaluateExpressionTree(root->right));
    if (root->data == "sen")
        return sin(evaluateExpressionTree(root->right));
    if (root->data == "tan")
        return tan(evaluateExpressionTree(root->right));

    if (root->data == "tanh")
        return tanh(evaluateExpressionTree(root->right));
    if (root->data == "cosh")
        return cosh(evaluateExpressionTree(root->right));
    if (root->data == "sinh")
        return sinh(evaluateExpressionTree(root->right));

    if (root->data == "arctan")
        return atan(evaluateExpressionTree(root->right));
    if (root->data == "arccos")
        return acos(evaluateExpressionTree(root->right));
    if (root->data == "arcsin")
        return asin(evaluateExpressionTree(root->right));
    if (root->data == "ln")
        return log(evaluateExpressionTree(root->right));
    if (root->data == "abs")
        return fabsl(evaluateExpressionTree(root->right));
    if (root->data == "E")
        return exp(evaluateExpressionTree(root->right));
    if (root->data == "r")
        return sqrt(evaluateExpressionTree(root->right));
    if (root->data == "c")
        return cbrt(evaluateExpressionTree(root->right));



    try
    {
        std::string nodeData = root->data;
        double data = stold(nodeData);
        if (data == -0)
            return 0;
        else
            return data;
    }
    catch (std::invalid_argument)
    {
        Error = true;
        //std::cout << "ERROR : INVALID ARGUMENT" << std::endl;
        return 0;
    }
    //if not a function or operator then return the data in the node
};
