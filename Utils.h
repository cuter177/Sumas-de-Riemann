#ifndef UTILS_H
#define UTILS_H

#include<string>
#include<vector>
#include<iostream>

//const bool IsX (std::string i);

const std::string removeWhiteSpace(const std::string& input);

const std::string sustitucionX(std::string& input,std::string& lim);

const std::string sustitucionE(std::string& input);

const std::string sustitucionPi(std::string& input);

const std::vector<std::string> getMathExpression(std::string& valor);

const std::vector<std::string> getMathExpression(std::string& valor, std::string& lim);

const int getOperatorPrecedance(const std::string& givenOperator);

const bool isOperator(const std::string& givenOperator);

const bool isFunction(const std::string& givenOperator);

// Formatea un double con precisión suficiente. Evita el redondeo a 6 decimales
// de std::to_string, que introducía error en límites y delta_x.
const std::string numeroATexto(double valor);

#endif
