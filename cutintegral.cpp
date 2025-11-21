#include<iostream>

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <iomanip>


#include"node.h"
#include"ExpressionParser.h"
#include"toPostFix.h"
#include"Utils.h"
#include"cutIntegral.h"

using namespace std;

 //setters
    void cortarIntegral::setArg(std::string valor) {
        arg = valor;
    }
    void cortarIntegral::setLimI(std::string valor) {

        std::string input = valor;

        toPostFix x(getMathExpression(input));
            Expression_Parser myTree(x.getPostFixExpression());
            std::shared_ptr<node> tree = myTree.toTree();

        limI = std::to_string(myTree.evaluateExpressionTree(tree));
    }
    void cortarIntegral::setLimS(std::string valor) {

        std::string input = valor;

        toPostFix x(getMathExpression(input));
            Expression_Parser myTree(x.getPostFixExpression());
            std::shared_ptr<node> tree = myTree.toTree();

        limS = std::to_string(myTree.evaluateExpressionTree(tree));
    }
    void cortarIntegral::setLimI_mostrar(std::string valor) {
        limI_mostrar = valor;
    }
    void cortarIntegral::setLimS_mostrar(std::string valor) {
        limS_mostrar = valor;
    }
    void cortarIntegral::setDeltaX(std::string valor) {
        double ls = stod(getLimS());
        double li = stod(getLimI());
        double tempN = stod(valor); // convertir valor a double

        if (tempN <= 0) {
            throw std::runtime_error("El número de rectángulos debe ser mayor que 0");
        }
        n = static_cast<int>(tempN); // ahora sí lo guardas como entero;
        deltaX = std::to_string((ls - li) / tempN);
    }


 //getters
    const string cortarIntegral::getArg() {
        return arg;
    }
    const string cortarIntegral::getLimI() {
        return limI;
    }
    const string cortarIntegral::getLimS() {
        return limS;
    }
    const string cortarIntegral::getLimI_mostrar() {
        return limI_mostrar;
    }
    const string cortarIntegral::getLimS_mostrar() {
        return limS_mostrar;
    }
    const string cortarIntegral::getDeltaX() {
        return deltaX;
    }
    int cortarIntegral::getN() const {
        return n;
    }


    void cortarIntegral::cortar(string input) {
        string cad = input;

        int pos1 = cad.find("(");
        int pos2 = cad.rfind(")");

        string cad2 = cad.substr(pos1+1,((pos2-2)-pos1+1));             //x,yum,t,z
        string cad3 = cad2.substr(cad2.find(",")+1,cad2.length());      //yum,t,z
        string cad4 = cad3.substr(cad3.find(",")+1,cad3.length());      //t,z
        string cad5 = cad4.substr(cad4.find(",")+1,cad4.length());      //z

        setArg(cad2.substr(0,cad2.find(",")));
        setLimI(cad3.substr(0,cad3.find(",")));
        setLimS(cad4.substr(0,cad4.find(",")));
        setLimI_mostrar(cad3.substr(0,cad3.find(",")));
        setLimS_mostrar(cad4.substr(0,cad4.find(",")));
        setDeltaX(cad5.substr(0,cad5.length()));
    }

    void cortarIntegral::mostrarDatos() {
        cout << "Funcion:\t\t"<<getArg()<< endl;
        cout << "Limite inferior:\t"<<getLimI_mostrar()<<endl;
        cout << "Limite superior:\t"<<getLimS_mostrar()<<endl;
        cout << "Delta de x:\t\t"<<getDeltaX()<<endl;
        //cout << "Delta de x:\t\t"<<getDeltaX()<<endl;
        cout << "" <<endl;
    }

    const string cortarIntegral::esConstante(string valor) {

        string pi = "3.14159265358979323846";
        string euler = "2.71828182845904523536";

        if (valor == "e") {
            valor.swap(euler);
        }
        if (valor == "pi") {
            valor.swap(pi);
        }

        return valor;
    }
