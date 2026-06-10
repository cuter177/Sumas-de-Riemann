#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include<vector>
#include<string>
#include<iomanip>
#include"node.h"

class Expression_Parser
{
private:
    bool Error = false;
public:
    bool getError() const { return Error; }
    Expression_Parser(const std::vector<std::string>& input);
    ~Expression_Parser() = default;

    std::shared_ptr<node> toTree();

    long double evaluateExpressionTree(const std::shared_ptr<node>& root);
    long double exponencial(const long double valor);

    std::shared_ptr<node> temp;;//General pointer used for manipulation
    std::vector<std::shared_ptr<node>> expression_tree;//will result in one element pointing to the root of the tree
    std::vector<std::string> tokens;//A list of each postfix expression token
};

#endif
