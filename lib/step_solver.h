#ifndef STEP_SOLVER_H
#define STEP_SOLVER_H

#include "polynomial.h"
#include <string>

using std::string;

// Класс StepByStepSolver реализует пошаговое подробное решение
// математических операций над многочленами с комментариями на русском языке.
class StepByStepSolver {
public:
    // Пошаговое сложение A(x) + B(x)
    static string add(const Polynomial& a, const Polynomial& b, Polynomial& result);

    // Пошаговое вычитание A(x) - B(x)
    static string subtract(const Polynomial& a, const Polynomial& b, Polynomial& result);

    // Пошаговое умножение A(x) * B(x)
    static string multiply(const Polynomial& a, const Polynomial& b, Polynomial& result);

    // Пошаговое деление P(x) / Q(x) с получением частного и остатка
    static string divide(const Polynomial& dividend, const Polynomial& divisor, Polynomial& quotient, Polynomial& remainder);

    // Пошаговое взятие остатка P(x) % Q(x)
    static string modulo(const Polynomial& dividend, const Polynomial& divisor, Polynomial& remainder);

    // Пошаговое возведение многочлена в степень P(x)^n через бинарное возведение
    static string power(const Polynomial& base, int exponent, Polynomial& result);
};

#endif // STEP_SOLVER_H
