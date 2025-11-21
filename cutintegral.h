#ifndef CUTINTEGRAL_H_INCLUDED
#define CUTINTEGRAL_H_INCLUDED

class cortarIntegral{

public:
    //funciones miembro

    //establecer
    void setArg(std::string);
    void setLimI(std::string);
    void setLimS(std::string);
    void setLimI_mostrar(std::string);
    void setLimS_mostrar(std::string);
    void setDeltaX(std::string);

    //obtener
    const std::string getArg();
    const std::string getLimI();
    const std::string getLimS();
    const std::string getLimI_mostrar();
    const std::string getLimS_mostrar();
    const std::string getDeltaX();

    int getN() const;

    //principal
    void cortar(std::string);

    //mostrartodo
    void mostrarDatos();

    //es e o p en limites
    const std::string esConstante(std::string);

private:
    //datos miembro
    std::string cad;

    std::string arg;
    std::string limI;
    std::string limS;
    std::string limI_mostrar;
    std::string limS_mostrar;
    std::string deltaX;
    int n;


};

#endif // CUTINTEGRAL_H_INCLUDED