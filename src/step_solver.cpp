#include "step_solver.h"
#include <cmath>
#include <algorithm>

using namespace std;

// Пошаговое сложение
string StepByStepSolver::add(const Polynomial& a, const Polynomial& b, Polynomial& result) {
    result = a + b;
    string trace = ""; // Строка для накопления подробных шагов сложения (трассировка)
    trace += "Сложение\n";
    trace += "Дано:\n";
    trace += "  A(x) = " + a.to_string() + "\n";
    trace += "  B(x) = " + b.to_string() + "\n\n";

    int max_deg = max(a.degree(), b.degree()); // Максимальная степень среди двух слагаемых многочленов
    if (max_deg < 0) {
        trace += "Оба многочлена равны нулю. Результат: 0\n";
        return trace;
    }

    trace += "Алгоритм: Складываем коэффициенты при соответствующих степенях x.\n";
    for (int deg = max_deg; deg >= 0; --deg) {
        double coeffA = 0.0; // Коэффициент при степени deg в многочлене A
        double coeffB = 0.0; // Коэффициент при степени deg в многочлене B
        
        if (deg >= 0 && deg < (int)a.coefficients().size()) {
            coeffA = a.coefficients()[deg];
        }
        if (deg >= 0 && deg < (int)b.coefficients().size()) {
            coeffB = b.coefficients()[deg];
        }

        double sum = coeffA + coeffB; // Результат сложения коэффициентов для степени deg
        trace += "  Степень x^" + to_string(deg) + ": ";
        trace += "(" + Polynomial::double_to_string(coeffA) + ") + (" + Polynomial::double_to_string(coeffB) + ") = " + Polynomial::double_to_string(sum) + "\n";
    }

    trace += "\nИтог сложения после упрощения: " + result.to_string() + "\n";
    return trace;
}

// Пошаговое вычитание
string StepByStepSolver::subtract(const Polynomial& a, const Polynomial& b, Polynomial& result) {
    result = a - b;
    string trace = ""; // Строка для накопления подробных шагов вычитания (трассировка)
    trace += "Вычитание\n";
    trace += "Дано:\n";
    trace += "  A(x) = " + a.to_string() + "\n";
    trace += "  B(x) = " + b.to_string() + "\n\n";

    int max_deg = max(a.degree(), b.degree()); // Максимальная степень среди двух многочленов
    if (max_deg < 0) {
        trace += "Оба многочлена равны нулю. Результат: 0\n";
        return trace;
    }

    trace += "Алгоритм: Вычитаем коэффициенты при соответствующих степенях x.\n";
    for (int deg = max_deg; deg >= 0; --deg) {
        double coeffA = 0.0; // Коэффициент при степени deg в уменьшаемом A
        double coeffB = 0.0; // Коэффициент при степени deg в вычитаемом B
        
        if (deg >= 0 && deg < (int)a.coefficients().size()) {
            coeffA = a.coefficients()[deg];
        }
        if (deg >= 0 && deg < (int)b.coefficients().size()) {
            coeffB = b.coefficients()[deg];
        }

        double diff = coeffA - coeffB; // Результат вычитания коэффициентов для степени deg
        trace += "  Степень x^" + to_string(deg) + ": ";
        trace += "(" + Polynomial::double_to_string(coeffA) + ") - (" + Polynomial::double_to_string(coeffB) + ") = " + Polynomial::double_to_string(diff) + "\n";
    }

    trace += "\nИтог вычитания после упрощения: " + result.to_string() + "\n";
    return trace;
}

// Пошаговое умножение
string StepByStepSolver::multiply(const Polynomial& a, const Polynomial& b, Polynomial& result) {
    result = a * b;
    string trace = ""; // Строка для накопления шагов умножения (трассировка)
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
    vector<double> res_coeffs(a.coefficients().size() + b.coefficients().size() - 1, 0.0); // Вектор коэффициентов результирующего многочлена
    
    for (int i = a.degree(); i >= 0; --i) {
        double ca = a.coefficients()[i]; // Коэффициент текущего члена первого полинома (при x^i)
        if (abs(ca) < 1e-9) continue;
        
        for (int j = b.degree(); j >= 0; --j) {
            double cb = b.coefficients()[j]; // Коэффициент текущего члена второго полинома (при x^j)
            if (abs(cb) < 1e-9) continue;
            
            double prod = ca * cb; // Произведение коэффициентов
            int deg = i + j; // Степень результирующего члена (сумма степеней i и j)
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

    vector<double> q_coeffs(dividend.degree() - divisor.degree() + 1, 0.0); // Вектор коэффициентов частного
    Polynomial cur_remainder = dividend; // Переменная для текущего делящегося остатка (начинается с делимого)
    int step = 1; // Счетчик шагов деления

    trace += "Алгоритм: на каждом шаге делим старший член остатка на старший член делителя, записываем результат в частное, умножаем полученный член на делитель и вычитаем его из остатка.\n\n";

    while (cur_remainder.degree() >= divisor.degree()) {
        trace += "Шаг " + to_string(step++) + ":\n";
        trace += "  Текущий делимый остаток: " + cur_remainder.to_string() + "\n";

        int deg_diff = cur_remainder.degree() - divisor.degree(); // Разность степеней старших членов текущего остатка и делителя
        double lead_dividend = cur_remainder.leading_coefficient(); // Старший коэффициент текущего остатка
        double lead_divisor = divisor.leading_coefficient(); // Старший коэффициент делителя
        double q_coeff = lead_dividend / lead_divisor; // Коэффициент нового члена частного

        q_coeffs[deg_diff] = q_coeff;

        trace += "  1. Делим старшие члены: (" + Polynomial::double_to_string(lead_dividend) + "x^" + to_string(cur_remainder.degree()) + ") / ("
           + Polynomial::double_to_string(lead_divisor) + "x^" + to_string(divisor.degree()) + ") = " + Polynomial::double_to_string(q_coeff) + "x^" + to_string(deg_diff) + "\n";

        // Формируем промежуточный вычитаемый полином: q_coeff * x^deg_diff * divisor
        vector<double> term_coeffs(deg_diff + divisor.coefficients().size(), 0.0); // Вектор для коэффициентов вычитаемого полинома
        for (size_t i = 0; i < divisor.coefficients().size(); ++i) {
            term_coeffs[i + deg_diff] = divisor.coefficients()[i] * q_coeff;
        }
        Polynomial term_poly(term_coeffs); // Временный полином, вычитаемый из текущего остатка
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
    Polynomial quotient; // Временная переменная для хранения частного (не выводится)
    string steps = divide(dividend, divisor, quotient, remainder); // Строка с подробной трассировкой процесса деления
    string trace = steps; // Строка для результирующего отчета
    trace += "\nИтог операции остатка от деления (%): " + (remainder.degree() == -1 ? "0" : remainder.to_string()) + "\n";
    return trace;
}

// Пошаговое возведение в степень
string StepByStepSolver::power(const Polynomial& base, int exponent, Polynomial& result) {
    if (exponent < 0) {
        throw invalid_argument("Степень должна быть неотрицательной!");
    }

    string loc_string;
    loc_string += "Возведение в степень\n";
    loc_string += "Основание: P(x) = " + base.to_string() + "\n";
    loc_string += "Показатель степени: n = " + to_string(exponent) + "\n\n";
    loc_string += "Используется алгоритм бинарного возведения в степень за O(log n) умножений многочленов.\n\n";

    Polynomial res({1.0}); // Накопленный результат возведения (начинается с 1 - единичного полинома)
    Polynomial cur_base = base; // Текущее основание степени, возводимое в квадрат на каждом шаге
    int exp = exponent; // Рабочая копия показателя степени для пошагового деления на 2
    int step = 1; // Счетчик шагов бинарного возведения в степень

    while (exp > 0) {
        loc_string += "Шаг " + to_string(step++) + ":\n";
        loc_string += "  Текущая степень для разбора: " + to_string(exp) + "\n";
        loc_string += "  Текущее основание: " + cur_base.to_string() + "\n";
        loc_string += "  Накопленный результат: " + res.to_string() + "\n";

        if (exp % 2 == 1) {
            loc_string += "  -> Показатель степени НЕЧЕТНЫЙ. Умножаем накопленный результат на основание:\n";
            loc_string += "     Результат = (" + res.to_string() + ") * (" + cur_base.to_string() + ")\n";
            res = res * cur_base;
            loc_string += "     = " + res.to_string() + "\n";
        } else {
            loc_string += "  -> Показатель степени ЧЕТНЫЙ. Накопленный результат не изменяется.\n";
        }

        if (exp > 1) {
            loc_string += "  -> Возводим основание в квадрат для следующего шага:\n";
            loc_string += "     Новое основание = (" + cur_base.to_string() + ")^2\n";
            cur_base = cur_base * cur_base;
            loc_string += "     = " + cur_base.to_string() + "\n";
        }
        exp /= 2;
        loc_string += "\n";
    }

    loc_string += "Возведение завершено.\n";
    loc_string += "Итог: " + res.to_string() + "\n";
    return loc_string;
}
