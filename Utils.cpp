#include"Utils.h"


#include <string>

const std::string sustitucionPi(std::string& input)
{
    {
        std::string p_expression{};
        std::string pi = "3.14159265358979323846";
        for (char character : input)
        {
            if (character == 'p')
            {
                p_expression += pi;
            }
            else
            {
                p_expression += character;
            }
        }
        return p_expression;
    }
}

const std::string sustitucionE(std::string& input)
{
    {
        std::string e_expression{};
        std::string euler = "2.71828182845904523536";
        for (char character : input)
        {
            if (character == 'e')
            {
                e_expression += euler;
            }
            else
            {
                e_expression += character;
            }
        }
        return e_expression;
    }
}

const std::string sustitucionX(std::string& input,std::string& lim)
{
    std::string m_expression{};
    std::string limP = lim;
    //std::string euler = "2";
    for (char character : input)
    {
        if (character != 'x')
        {
            m_expression += character;
        }
        else
        {
            m_expression += limP; //woooooow!
        }
        //if (character == 'e') {
        //       m_expression += euler;
        //  }
    }
    return m_expression;
}

const std::string removeWhiteSpace(std::string& input)
{
    std::string s_expression{};
    for (char character : input)
    {
        if (character != ' ')
            s_expression += character;
    }
    return s_expression;
}

const std::vector<std::string> getMathExpression(std::string& valor, std::string& lim)
{
    //Variables
    std::vector<std::string> mathExpression;
    std::string buildString = "";
    std::string input = valor;
    std::string limP = lim;
    //std::string limP;
    /*
    //Get expression
    std::cin.ignore();
    std::cout << "Introduce la expresion matematica: ";
    std::getline(std::cin, input);
    */

    input = removeWhiteSpace(input);
    input = sustitucionX(input,limP); //limP
    input = sustitucionE(input);
    input = sustitucionPi(input);

    for (unsigned int i = 0; i < input.size(); i++)//Loop through each character in expression
    {
        char character = input[i];//get the current character

        if ((i == 0 || //Start Scope
                (input.size() > 0 && input[i - 1] == '(')) && //Open bracket scope
                character == '-')//Negative deceleration
            buildString += '-';//add a minus to the token so the following number can be build onto it

        else if (isdigit(character) || character == '.')//if character is a number
            buildString += character;//add to current token

        else if (character == '^' ||//if character is an operator
                 character == '*' ||
                 character == '/' ||
                 character == '+' ||
                 character == '-' ||
                 character == '(' ||
                 character == ')')
        {
            //Check if the operator is defining a negative
            if ((i >= 1 && character == '-') &&
                    (input[i - 1] == '^' ||//if character is an operator
                     input[i - 1] == '*' ||
                     input[i - 1] == '/' ||
                     input[i - 1] == '+' ||
                     input[i - 1] == '-')
               )

                buildString += "-";//define the negative

            else//define a binary operator
            {
                if (buildString.size() > 0)//Add token to expression if there is one
                {
                    mathExpression.push_back(buildString);//push it to expression
                    buildString = "";//reset token
                }
                buildString += character;//put operator into token (to convert to string)
                mathExpression.push_back(buildString);//push token into expression
                buildString = "";//reset token
            }
        }
        else//if not a operand or operator then we must be taking in a function name
            buildString += character;//add the character of the function to the token
    }

    //once loop is complete
    if (buildString.size() > 0)//Push the remaining token into the expression
        mathExpression.push_back(buildString);


    //loop through each token
    for (size_t i = 0ull; i < mathExpression.size(); i++)
    {
        //if we cant read the next 3 tokens from i then stop loop
        if ((mathExpression.size() - i) < 3)
            break;

        const std::string&
        token_one = mathExpression[i],
        token_two = mathExpression[i + 1],
        token_three = mathExpression[i + 2];

        if (isOperator(token_one) && token_two == "-" && token_three == "(")
        {
            //remove the inverse of the bracket "-"
            mathExpression.erase(mathExpression.begin() + i + 1);

            //insert a "-1" after the bracket
            mathExpression.insert(mathExpression.begin() + i + 2, "-1");

            mathExpression.insert(mathExpression.begin() + i + 3, "*");

            mathExpression.insert(mathExpression.begin() + i + 4, "(");

            int bracket_alt = 0;

            for (size_t j = i + 5; j < mathExpression.size(); j++)
            {
                if (mathExpression[j] == "(") bracket_alt += 1;
                else if (mathExpression[j] == ")") bracket_alt -= 1;

                if (bracket_alt == -1)
                {
                    mathExpression.insert(mathExpression.begin() + j, ")");
                    break;
                }

            }
        }

    }

    return mathExpression;
}


const std::vector<std::string> getMathExpression(std::string& valor)
{
    //Variables
    std::vector<std::string> mathExpression;
    std::string buildString = "";
    std::string input = valor;

    input = removeWhiteSpace(input);
    input = sustitucionE(input);
    input = sustitucionPi(input);

    for (unsigned int i = 0; i < input.size(); i++)//Loop through each character in expression
    {
        char character = input[i];//get the current character

        if ((i == 0 || //Start Scope
                (input.size() > 0 && input[i - 1] == '(')) && //Open bracket scope
                character == '-')//Negative deceleration
            buildString += '-';//add a minus to the token so the following number can be build onto it

        else if (isdigit(character) || character == '.')//if character is a number
            buildString += character;//add to current token

        else if (character == '^' ||//if character is an operator
                 character == '*' ||
                 character == '/' ||
                 character == '+' ||
                 character == '-' ||
                 character == '(' ||
                 character == ')')
        {
            //Check if the operator is defining a negative
            if ((i >= 1 && character == '-') &&
                    (input[i - 1] == '^' ||//if character is an operator
                     input[i - 1] == '*' ||
                     input[i - 1] == '/' ||
                     input[i - 1] == '+' ||
                     input[i - 1] == '-')
               )

                buildString += "-";//define the negative

            else//define a binary operator
            {
                if (buildString.size() > 0)//Add token to expression if there is one
                {
                    mathExpression.push_back(buildString);//push it to expression
                    buildString = "";//reset token
                }
                buildString += character;//put operator into token (to convert to string)
                mathExpression.push_back(buildString);//push token into expression
                buildString = "";//reset token
            }
        }
        else//if not a operand or operator then we must be taking in a function name
            buildString += character;//add the character of the function to the token
    }

    //once loop is complete
    if (buildString.size() > 0)//Push the remaining token into the expression
        mathExpression.push_back(buildString);


    //loop through each token
    for (size_t i = 0ull; i < mathExpression.size(); i++)
    {
        //if we cant read the next 3 tokens from i then stop loop
        if ((mathExpression.size() - i) < 3)
            break;

        const std::string&
        token_one = mathExpression[i],
        token_two = mathExpression[i + 1],
        token_three = mathExpression[i + 2];

        if (isOperator(token_one) && token_two == "-" && token_three == "(")
        {
            //remove the inverse of the bracket "-"
            mathExpression.erase(mathExpression.begin() + i + 1);

            //insert a "-1" after the bracket
            mathExpression.insert(mathExpression.begin() + i + 2, "-1");

            mathExpression.insert(mathExpression.begin() + i + 3, "*");

            mathExpression.insert(mathExpression.begin() + i + 4, "(");

            int bracket_alt = 0;

            for (size_t j = i + 5; j < mathExpression.size(); j++)
            {
                if (mathExpression[j] == "(") bracket_alt += 1;
                else if (mathExpression[j] == ")") bracket_alt -= 1;

                if (bracket_alt == -1)
                {
                    mathExpression.insert(mathExpression.begin() + j, ")");
                    break;
                }

            }
        }

    }

    return mathExpression;
}


const int getOperatorPrecedance(const std::string& givenOperator)
{
    //get the precedance of the token
    if (givenOperator == "^")
        return 4;
    if (givenOperator == "*")
        return 3;
    if (givenOperator == "/")
        return 3;
    if (givenOperator == "+")
        return 2;
    if (givenOperator == "-")
        return 2;
    if (givenOperator == "(")
        return 1;
    if (givenOperator == ")")
        return 1;
    if (givenOperator == "cos")
        return 5;
    if (givenOperator == "tan")
        return 5;
    if (givenOperator == "sin")
        return 5;
    if (givenOperator == "sen")
        return 5;
    if (givenOperator == "cosh")
        return 5;
    if (givenOperator == "tanh")
        return 5;
    if (givenOperator == "sinh")
        return 5;
    if (givenOperator == "ln")
        return 5;
    if (givenOperator == "abs")
        return 5;
    if (givenOperator == "E")
        return 5;
    if (givenOperator == "arcsin")
        return 5;
    if (givenOperator == "arccos")
        return 5;
    if (givenOperator == "arctan")
        return 5;
    if (givenOperator == "r")
        return 5;
    if (givenOperator == "c")
        return 5;
    else
        return 0;

}

const bool isOperator(const std::string& givenOperator)
{
    //determine if the token is an operator
    if (givenOperator == "^")
        return true;
    if (givenOperator == "*")
        return true;
    if (givenOperator == "/")
        return true;
    if (givenOperator == "+")
        return true;
    if (givenOperator == "-")
        return true;
    if (givenOperator == "(")
        return true;
    if (givenOperator == ")")
        return true;
    return false;
}

const bool isFunction(const std::string& givenOperator)
{
    //determine if the token is a function
    if (givenOperator == "cos")
        return true;
    if (givenOperator == "tan")
        return true;
    if (givenOperator == "sin")
        return true;
    if (givenOperator == "sen")
        return true;
    if (givenOperator == "cosh")
        return true;
    if (givenOperator == "tanh")
        return true;
    if (givenOperator == "sinh")
        return true;
    if (givenOperator == "ln")
        return true;
    if (givenOperator == "abs")
        return true;
    if (givenOperator == "E")
        return true;
    if (givenOperator == "arcsin")
        return true;
    if (givenOperator == "arccos")
        return true;
    if (givenOperator == "arctan")
        return true;
    if (givenOperator == "r")
        return true;
    if (givenOperator == "c")
        return true;
    return false;
}
