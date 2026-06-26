#include "step_solver.h"
#include <cmath>
#include <algorithm>

using namespace std;

// Пошаговое сложение
string StepByStepSolver::add(const Polynomial& a, const Polynomial& b, Polynomial& result) {
    result = a + b;
    string trace = "";
    trace += "Сложение\n";
    trace += "Дано:\n";
    trace += "  A(x) = " + a.to_string() + "\n";
    trace += "  B(x) = " + b.to_string() + "\n\n";

    int max_deg = max(a.degree(), b.degree());
    if (max_deg < 0) {
        trace += "Оба многочлена равны нулю. Результат: 0\n";
        return trace;
    }

    trace += "Алгоритм: Складываем коэффициенты при соответствующих степенях x.\n";
    for (int deg = max_deg; deg >= 0; --deg) {
        double coeffA = 0.0;
        double coeffB = 0.0;
        
        if (deg >= 0 && deg < (int)a.coefficients().size()) {
            coeffA = a.coefficients()[deg];
        }
        if (deg >= 0 && deg < (int)b.coefficients().size()) {
            coeffB = b.coefficients()[deg];
        }

        double sum = coeffA + coeffB;
        trace += "  Степень x^" + to_string(deg) + ": ";
        trace += "(" + Polynomial::double_to_string(coeffA) + ") + (" + Polynomial::double_to_string(coeffB) + ") = " + Polynomial::double_to_string(sum) + "\n";
    }

    trace += "\nИтог сложения после упрощения: " + result.to_string() + "\n";
    return trace;
}

// Пошаговое вычитание
string StepByStepSolver::subtract(const Polynomial& a, const Polynomial& b, Polynomial& result) {
    result = a - b;
    string trace = "";
    trace += "Вычитание\n";
    trace += "Дано:\n";
    trace += "  A(x) = " + a.to_string() + "\n";
    trace += "  B(x) = " + b.to_string() + "\n\n";

    int max_deg = max(a.degree(), b.degree());
    if (max_deg < 0) {
        trace += "Оба многочлена равны нулю. Результат: 0\n";
        return trace;
    }

    trace += "Алгоритм: Вычитаем коэффициенты при соответствующих степенях x.\n";
    for (int deg = max_deg; deg >= 0; --deg) {
        double coeffA = 0.0;
        double coeffB = 0.0;
        
        if (deg >= 0 && deg < (int)a.coefficients().size()) {
            coeffA = a.coefficients()[deg];
        }
        if (deg >= 0 && deg < (int)b.coefficients().size()) {
            coeffB = b.coefficients()[deg];
        }

        double diff = coeffA - coeffB;
        trace += "  Степень x^" + to_string(deg) + ": ";
        trace += "(" + Polynomial::double_to_string(coeffA) + ") - (" + Polynomial::double_to_string(coeffB) + ") = " + Polynomial::double_to_string(diff) + "\n";
    }

    trace += "\nИтог вычитания после упрощения: " + result.to_string() + "\n";
    return trace;
}

// Пошаговое умножение
string StepByStepSolver::multiply(const Polynomial& a, const Polynomial& b, Polynomial& result) {
    result = a * b;
    string trace = "";
    trace += "Умножение\n";
    trace += "Дано:\n";
    trace += "  A(x) = (" + a.to_string() + ")\n";
    trace += "  B(x) = (" + b.to_string() + ")\n\n";

    if (a.degree() == -1 || b.degree() == -1) {
        trace += "Один из сомножителей равен 0. Результат умножения: 0\n";
        return trace;
    }

    trace += "Алгоритм: Перемножаем каждое слагаемое первого многочлена на каждое слагаемое второго.\n";
    trace += "При этом коэффициенты перемножаются, а показатели степеней x складываются: (a * x^i) * (b * x^j) = (a * b) * x^(i+j).\n\n";

    trace += "Шаг 1: Промежуточные произведения членов:\n";
    vector<double> res_coeffs(a.coefficients().size() + b.coefficients().size() - 1, 0.0);
    
    for (int i = a.degree(); i >= 0; --i) {
        double ca = a.coefficients()[i];
        if (abs(ca) < 1e-9) continue;
        
        for (int j = b.degree(); j >= 0; --j) {
            double cb = b.coefficients()[j];
            if (abs(cb) < 1e-9) continue;
            
            double prod = ca * cb;
            int deg = i + j;
            res_coeffs[deg] += prod;
            
            trace += "  (" + Polynomial::double_to_string(ca) + "x^" + to_string(i) + ") * (" + Polynomial::double_to_string(cb) + "x^" + to_string(j) + ") = " + Polynomial::double_to_string(prod) + "x^" + to_string(deg) + "\n";
        }
    }

    trace += "\nШаг 2: Группировка подобных членов (сложение коэффициентов при одинаковых степенях):\n";
    for (int deg = static_cast<int>(res_coeffs.size()) - 1; deg >= 0; --deg) {
        if (abs(res_coeffs[deg]) < 1e-9) continue;
        trace += "  Коэффициент при x^" + to_string(deg) + " = " + Polynomial::double_to_string(res_coeffs[deg]) + "\n";
    }

    trace += "\nИтог умножения: " + result.to_string() + "\n";
    return trace;
}

// Пошаговое деление столбиком (уголком)
string StepByStepSolver::divide(const Polynomial& dividend, const Polynomial& divisor, Polynomial& quotient, Polynomial& remainder) {
    if (divisor.degree() == -1) {
        throw invalid_argument("Деление на нулевой многочлен невозможно!");
    }

    string trace = "";
    trace += "Деление уголком\n";
    trace += "Делимое: P(x) = " + dividend.to_string() + "\n";
    trace += "Делитель: Q(x) = " + divisor.to_string() + "\n\n";

    if (dividend.degree() < divisor.degree()) {
        quotient = Polynomial();
        remainder = dividend;
        trace += "Степень делимого меньше степени делителя. Процесс деления не требуется.\n";
        trace += "Частное = 0\n";
        trace += "Остаток = " + remainder.to_string() + "\n";
        return trace;
    }

    vector<double> q_coeffs(dividend.degree() - divisor.degree() + 1, 0.0);
    Polynomial cur_remainder = dividend;
    int step = 1;

    trace += "Алгоритм: на каждом шаге делим старший член остатка на старший член делителя, записываем результат в частное, умножаем полученный член на делитель и вычитаем его из остатка.\n\n";

    while (cur_remainder.degree() >= divisor.degree()) {
        trace += "Шаг " + to_string(step++) + ":\n";
        trace += "  Текущий делимый остаток: " + cur_remainder.to_string() + "\n";

        int deg_diff = cur_remainder.degree() - divisor.degree();
        double lead_dividend = cur_remainder.leading_coefficient();
        double lead_divisor = divisor.leading_coefficient();
        double q_coeff = lead_dividend / lead_divisor;

        q_coeffs[deg_diff] = q_coeff;

        trace += "  1. Делим старшие члены: (" + Polynomial::double_to_string(lead_dividend) + "x^" + to_string(cur_remainder.degree()) + ") / ("
           + Polynomial::double_to_string(lead_divisor) + "x^" + to_string(divisor.degree()) + ") = " + Polynomial::double_to_string(q_coeff) + "x^" + to_string(deg_diff) + "\n";

        // Формируем промежуточный вычитаемый полином: q_coeff * x^deg_diff * divisor
        vector<double> term_coeffs(deg_diff + divisor.coefficients().size(), 0.0);
        for (size_t i = 0; i < divisor.coefficients().size(); ++i) {
            term_coeffs[i + deg_diff] = divisor.coefficients()[i] * q_coeff;
        }
        Polynomial term_poly(term_coeffs);
        trace += "  2. Умножаем этот член на делитель: " + Polynomial::double_to_string(q_coeff) + "x^" + to_string(deg_diff) + " * (" + divisor.to_string() + ") = " + term_poly.to_string() + "\n";

        // Вычитаем
        cur_remainder = cur_remainder - term_poly;
        trace += "  3. Вычитаем из остатка и получаем новый остаток: " + (cur_remainder.degree() == -1 ? "0" : cur_remainder.to_string()) + "\n\n";
    }

    quotient = Polynomial(q_coeffs);
    remainder = cur_remainder;

    trace += "Деление завершено.\n";
    trace += "Частное (результат): " + quotient.to_string() + "\n";
    trace += "Остаток: " + (remainder.degree() == -1 ? "0" : remainder.to_string()) + "\n";
    return trace;
}

// Пошаговое получение остатка
string StepByStepSolver::modulo(const Polynomial& dividend, const Polynomial& divisor, Polynomial& remainder) {
    Polynomial quotient;
    string steps = divide(dividend, divisor, quotient, remainder);
    string trace = steps;
    trace += "\nИтог операции остатка от деления (%): " + (remainder.degree() == -1 ? "0" : remainder.to_string()) + "\n";
    return trace;
}

// Пошаговое возведение в степень
string StepByStepSolver::power(const Polynomial& base, int exponent, Polynomial& result) {
    if (exponent < 0) {
        throw invalid_argument("Степень должна быть неотрицательной!");
    }

    string trace = "";
    trace += "Возведение в степень\n";
    trace += "Основание: P(x) = " + base.to_string() + "\n";
    trace += "Показатель степени: n = " + to_string(exponent) + "\n\n";
    trace += "Используется алгоритм бинарного возведения в степень за O(log n) умножений многочленов.\n\n";

    Polynomial res({1.0}); // Начальный результат = 1 (полином степени 0)
    Polynomial cur_base = base;
    int exp = exponent;
    int step = 1;

    while (exp > 0) {
        trace += "Шаг " + to_string(step++) + ":\n";
        trace += "  Текущая степень для разбора: " + to_string(exp) + "\n";
        trace += "  Текущее основание: " + cur_base.to_string() + "\n";
        trace += "  Накопленный результат: " + res.to_string() + "\n";

        if (exp % 2 == 1) {
            trace += "  -> Показатель степени НЕЧЕТНЫЙ. Умножаем накопленный результат на основание:\n";
            trace += "     Результат = (" + res.to_string() + ") * (" + cur_base.to_string() + ")\n";
            res = res * cur_base;
            trace += "     = " + res.to_string() + "\n";
        } else {
            trace += "  -> Показатель степени ЧЕТНЫЙ. Накопленный результат не изменяется.\n";
        }

        if (exp > 1) {
            trace += "  -> Возводим основание в квадрат для следующего шага:\n";
            trace += "     Новое основание = (" + cur_base.to_string() + ")^2\n";
            cur_base = cur_base * cur_base;
            trace += "     = " + cur_base.to_string() + "\n";
        }
        exp /= 2;
        trace += "\n";
    }

    result = res;
    trace += "Возведение завершено.\n";
    trace += "Итог: " + result.to_string() + "\n";
    return trace;
}
