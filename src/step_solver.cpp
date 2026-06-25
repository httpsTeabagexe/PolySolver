#include "step_solver.h"
#include <cmath>
#include <algorithm>
#include <iomanip>

using namespace std;

// Пошаговое сложение
string StepByStepSolver::add(const Polynomial& a, const Polynomial& b, Polynomial& result) {
    result = a + b;
    string ss = "";
    ss += "Сложение\n";
    ss += "Дано:\n";
    ss += "  A(x) = " + a.to_string() + "\n";
    ss += "  B(x) = " + b.to_string() + "\n\n";

    int max_deg = max(a.degree(), b.degree());
    if (max_deg < 0) {
        ss += "Оба многочлена равны нулю. Результат: 0\n";
        return ss;
    }

    ss += "Алгоритм: Складываем коэффициенты при соответствующих степенях x.\n";
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
        ss += "  Степень x^" + to_string(deg) + ": ";
        ss += "(" + Polynomial::double_to_string(coeffA) + ") + (" + Polynomial::double_to_string(coeffB) + ") = " + Polynomial::double_to_string(sum) + "\n";
    }

    ss += "\nИтог сложения после упрощения: " + result.to_string() + "\n";
    return ss;
}

// Пошаговое вычитание
string StepByStepSolver::subtract(const Polynomial& a, const Polynomial& b, Polynomial& result) {
    result = a - b;
    string ss = "";
    ss += "Вычитание\n";
    ss += "Дано:\n";
    ss += "  A(x) = " + a.to_string() + "\n";
    ss += "  B(x) = " + b.to_string() + "\n\n";

    int max_deg = max(a.degree(), b.degree());
    if (max_deg < 0) {
        ss += "Оба многочлена равны нулю. Результат: 0\n";
        return ss;
    }

    ss += "Алгоритм: Вычитаем коэффициенты при соответствующих степенях x.\n";
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
        ss += "  Степень x^" + to_string(deg) + ": ";
        ss += "(" + Polynomial::double_to_string(coeffA) + ") - (" + Polynomial::double_to_string(coeffB) + ") = " + Polynomial::double_to_string(diff) + "\n";
    }

    ss += "\nИтог вычитания после упрощения: " + result.to_string() + "\n";
    return ss;
}

// Пошаговое умножение
string StepByStepSolver::multiply(const Polynomial& a, const Polynomial& b, Polynomial& result) {
    result = a * b;
    string ss = "";
    ss += "Умножение\n";
    ss += "Дано:\n";
    ss += "  A(x) = (" + a.to_string() + ")\n";
    ss += "  B(x) = (" + b.to_string() + ")\n\n";

    if (a.degree() == -1 || b.degree() == -1) {
        ss += "Один из сомножителей равен 0. Результат умножения: 0\n";
        return ss;
    }

    ss += "Алгоритм: Перемножаем каждое слагаемое первого многочлена на каждое слагаемое второго.\n";
    ss += "При этом коэффициенты перемножаются, а показатели степеней x складываются: (a * x^i) * (b * x^j) = (a * b) * x^(i+j).\n\n";

    ss += "Шаг 1: Промежуточные произведения членов:\n";
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
            
            ss += "  (" + Polynomial::double_to_string(ca) + "x^" + to_string(i) + ") * (" + Polynomial::double_to_string(cb) + "x^" + to_string(j) + ") = " + Polynomial::double_to_string(prod) + "x^" + to_string(deg) + "\n";
        }
    }

    ss += "\nШаг 2: Группировка подобных членов (сложение коэффициентов при одинаковых степенях):\n";
    for (int deg = static_cast<int>(res_coeffs.size()) - 1; deg >= 0; --deg) {
        if (abs(res_coeffs[deg]) < 1e-9) continue;
        ss += "  Коэффициент при x^" + to_string(deg) + " = " + Polynomial::double_to_string(res_coeffs[deg]) + "\n";
    }

    ss += "\nИтог умножения: " + result.to_string() + "\n";
    return ss;
}

// Пошаговое деление столбиком (уголком)
string StepByStepSolver::divide(const Polynomial& dividend, const Polynomial& divisor, Polynomial& quotient, Polynomial& remainder) {
    if (divisor.degree() == -1) {
        throw invalid_argument("Деление на нулевой многочлен невозможно!");
    }

    string ss = "";
    ss += "Деление уголком\n";
    ss += "Делимое: P(x) = " + dividend.to_string() + "\n";
    ss += "Делитель: Q(x) = " + divisor.to_string() + "\n\n";

    if (dividend.degree() < divisor.degree()) {
        quotient = Polynomial();
        remainder = dividend;
        ss += "Степень делимого меньше степени делителя. Процесс деления не требуется.\n";
        ss += "Частное = 0\n";
        ss += "Остаток = " + remainder.to_string() + "\n";
        return ss;
    }

    vector<double> q_coeffs(dividend.degree() - divisor.degree() + 1, 0.0);
    Polynomial cur_remainder = dividend;
    int step = 1;

    ss += "Алгоритм: на каждом шаге делим старший член остатка на старший член делителя, записываем результат в частное, умножаем полученный член на делитель и вычитаем его из остатка.\n\n";

    while (cur_remainder.degree() >= divisor.degree()) {
        ss += "Шаг " + to_string(step++) + ":\n";
        ss += "  Текущий делимый остаток: " + cur_remainder.to_string() + "\n";

        int deg_diff = cur_remainder.degree() - divisor.degree();
        double lead_dividend = cur_remainder.leading_coefficient();
        double lead_divisor = divisor.leading_coefficient();
        double q_coeff = lead_dividend / lead_divisor;

        q_coeffs[deg_diff] = q_coeff;

        ss += "  1. Делим старшие члены: (" + Polynomial::double_to_string(lead_dividend) + "x^" + to_string(cur_remainder.degree()) + ") / ("
           + Polynomial::double_to_string(lead_divisor) + "x^" + to_string(divisor.degree()) + ") = " + Polynomial::double_to_string(q_coeff) + "x^" + to_string(deg_diff) + "\n";

        // Формируем промежуточный вычитаемый полином: q_coeff * x^deg_diff * divisor
        vector<double> term_coeffs(deg_diff + divisor.coefficients().size(), 0.0);
        for (size_t i = 0; i < divisor.coefficients().size(); ++i) {
            term_coeffs[i + deg_diff] = divisor.coefficients()[i] * q_coeff;
        }
        Polynomial term_poly(term_coeffs);
        ss += "  2. Умножаем этот член на делитель: " + Polynomial::double_to_string(q_coeff) + "x^" + to_string(deg_diff) + " * (" + divisor.to_string() + ") = " + term_poly.to_string() + "\n";

        // Вычитаем
        cur_remainder = cur_remainder - term_poly;
        ss += "  3. Вычитаем из остатка и получаем новый остаток: " + (cur_remainder.degree() == -1 ? "0" : cur_remainder.to_string()) + "\n\n";
    }

    quotient = Polynomial(q_coeffs);
    remainder = cur_remainder;

    ss += "Деление завершено.\n";
    ss += "Частное (результат): " + quotient.to_string() + "\n";
    ss += "Остаток: " + (remainder.degree() == -1 ? "0" : remainder.to_string()) + "\n";
    return ss;
}

// Пошаговое получение остатка
string StepByStepSolver::modulo(const Polynomial& dividend, const Polynomial& divisor, Polynomial& remainder) {
    Polynomial quotient;
    string steps = divide(dividend, divisor, quotient, remainder);
    string ss = steps;
    ss += "\nИтог операции остатка от деления (%): " + (remainder.degree() == -1 ? "0" : remainder.to_string()) + "\n";
    return ss;
}

// Пошаговое возведение в степень
string StepByStepSolver::power(const Polynomial& base, int exponent, Polynomial& result) {
    if (exponent < 0) {
        throw invalid_argument("Степень должна быть неотрицательной!");
    }

    string ss = "";
    ss += "Возведение в степень\n";
    ss += "Основание: P(x) = " + base.to_string() + "\n";
    ss += "Показатель степени: n = " + to_string(exponent) + "\n\n";
    ss += "Используется алгоритм бинарного возведения в степень за O(log n) умножений многочленов.\n\n";

    Polynomial res({1.0}); // Начальный результат = 1 (полином степени 0)
    Polynomial cur_base = base;
    int exp = exponent;
    int step = 1;

    while (exp > 0) {
        ss += "Шаг " + to_string(step++) + ":\n";
        ss += "  Текущая степень для разбора: " + to_string(exp) + "\n";
        ss += "  Текущее основание: " + cur_base.to_string() + "\n";
        ss += "  Накопленный результат: " + res.to_string() + "\n";

        if (exp % 2 == 1) {
            ss += "  -> Показатель степени НЕЧЕТНЫЙ. Умножаем накопленный результат на основание:\n";
            ss += "     Результат = (" + res.to_string() + ") * (" + cur_base.to_string() + ")\n";
            res = res * cur_base;
            ss += "     = " + res.to_string() + "\n";
        } else {
            ss += "  -> Показатель степени ЧЕТНЫЙ. Накопленный результат не изменяется.\n";
        }

        if (exp > 1) {
            ss += "  -> Возводим основание в квадрат для следующего шага:\n";
            ss += "     Новое основание = (" + cur_base.to_string() + ")^2\n";
            cur_base = cur_base * cur_base;
            ss += "     = " + cur_base.to_string() + "\n";
        }
        exp /= 2;
        ss += "\n";
    }

    result = res;
    ss += "Возведение завершено.\n";
    ss += "Итог: " + result.to_string() + "\n";
    return ss;
}
