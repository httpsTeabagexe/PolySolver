#include "polynomial.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace std;

// Макрос для сравнения вещественных чисел с заданной погрешностью
#define ASSERT_NEAR(a, b, tol) assert(abs((a) - (b)) < (tol))

// Тест конструкторов класса
void test_constructors() {
    Polynomial p1;
    assert(p1.degree() == -1); // Конструктор по умолчанию должен создавать нулевой полином со степенью -1
    assert(p1.coefficients().empty());

    Polynomial p2({1.0, 2.0, 3.0}); // Многочлен 3x^2 + 2x + 1
    assert(p2.degree() == 2);
    assert(p2.coefficients().size() == 3);
    assert(p2.coefficients()[0] == 1.0);
    assert(p2.coefficients()[1] == 2.0);
    assert(p2.coefficients()[2] == 3.0);

    // Проверяем нормализацию: лишние нули в конце вектора должны удаляться
    Polynomial p3({1.0, 0.0, 0.0});
    assert(p3.degree() == 0); // Многочлен вида 1 + 0x + 0x^2 эквивалентен константе 1 (степень 0)
    assert(p3.coefficients().size() == 1);
}

// Тест вычисления значения многочлена в точке
void test_evaluation() {
    // P(x) = 3x^2 + 2x + 1
    Polynomial p({1.0, 2.0, 3.0});
    ASSERT_NEAR(p.evaluate(0.0), 1.0, 1e-9);   // P(0) = 1
    ASSERT_NEAR(p.evaluate(1.0), 6.0, 1e-9);   // P(1) = 3 + 2 + 1 = 6
    ASSERT_NEAR(p.evaluate(-1.0), 2.0, 1e-9);  // P(-1) = 3 - 2 + 1 = 2
    ASSERT_NEAR(p.evaluate(2.0), 17.0, 1e-9);  // P(2) = 12 + 4 + 1 = 17
}

// Тест сложения
void test_addition() {
    Polynomial p1({1.0, 2.0}); // 2x + 1
    Polynomial p2({3.0, 4.0, 5.0}); // 5x^2 + 4x + 3
    Polynomial sum = p1 + p2; // Результирующий многочлен: 5x^2 + 6x + 4
    assert(sum.degree() == 2);
    assert(sum.coefficients()[0] == 4.0);
    assert(sum.coefficients()[1] == 6.0);
    assert(sum.coefficients()[2] == 5.0);
}

// Тест вычитания
void test_subtraction() {
    Polynomial p1({1.0, 2.0}); // 2x + 1
    Polynomial p2({3.0, 4.0, 5.0}); // 5x^2 + 4x + 3
    Polynomial diff = p2 - p1; // Результирующий многочлен: 5x^2 + 2x + 2
    assert(diff.degree() == 2);
    assert(diff.coefficients()[0] == 2.0);
    assert(diff.coefficients()[1] == 2.0);
    assert(diff.coefficients()[2] == 5.0);
}

// Тест умножения
void test_multiplication() {
    Polynomial p1({1.0, 2.0}); // 2x + 1
    Polynomial p2({3.0, 4.0}); // 4x + 3
    Polynomial prod = p1 * p2; // (2x+1)(4x+3) = 8x^2 + 10x + 3
    assert(prod.degree() == 2);
    assert(prod.coefficients()[0] == 3.0);
    assert(prod.coefficients()[1] == 10.0);
    assert(prod.coefficients()[2] == 8.0);
}

// Тест деления уголком
void test_error_division() {
    // (x^2 - 1) / (x - 1) = x + 1, остаток 0
    Polynomial dividend({-1.0, 0.0, 1.0}); // x^2 - 1
    Polynomial divisor({-1.0, 1.0}); // x - 1
    auto [q, r] = Polynomial::divide(dividend, divisor);
    assert(q == Polynomial({1.0, 1.0})); // Частное: x + 1
    assert(r == Polynomial()); // Остаток: 0

    // Проверяем более сложный случай:
    // (3x^3 + x^2 + 2x + 5) / (x^2 + x + 1)
    // 3x^3 + x^2 + 2x + 5 = (3x - 2)(x^2 + x + 1) + (x + 7)
    Polynomial pA({5.0, 2.0, 1.0, 3.0});
    Polynomial pB({1.0, 1.0, 1.0});
    auto [q2, r2] = Polynomial::divide(pA, pB);
    assert(q2 == Polynomial({-2.0, 3.0})); // Частное: 3x - 2
    assert(r2 == Polynomial({7.0, 1.0})); // Остаток: x + 7
}

// Тест возведения в степень
void test_power() {
    Polynomial p({1.0, 1.0}); // x + 1
    Polynomial p_sq = p.pow(2); // (x+1)^2 = x^2 + 2x + 1
    assert(p_sq == Polynomial({1.0, 2.0, 1.0}));

    Polynomial p_cube = p ^ 3; // (x+1)^3 = x^3 + 3x^2 + 3x + 1
    assert(p_cube == Polynomial({1.0, 3.0, 3.0, 1.0}));
}

// Тест вывода в строку (форматирование)
void test_formatting() {
    assert(Polynomial().to_string() == "0");
    assert(Polynomial({5.0}).to_string() == "5");
    assert(Polynomial({-3.2}).to_string() == "-3.2");
    assert(Polynomial({0.0, 1.0}).to_string() == "x");
    assert(Polynomial({0.0, -1.0}).to_string() == "-x");
    assert(Polynomial({1.0, 2.0, 3.0}).to_string() == "3x^2 + 2x + 1");
    assert(Polynomial({-1.0, 0.0, -5.5}).to_string() == "-5.5x^2 - 1");
}

// Тест парсинга строк в объекты Polynomial
void test_parsing() {
    assert(Polynomial::from_string("0") == Polynomial());
    assert(Polynomial::from_string("5") == Polynomial({5.0}));
    assert(Polynomial::from_string("-3.2") == Polynomial({-3.2}));
    assert(Polynomial::from_string("x") == Polynomial({0.0, 1.0}));
    assert(Polynomial::from_string("-x") == Polynomial({0.0, -1.0}));
    assert(Polynomial::from_string("3x^2+2x+1") == Polynomial({1.0, 2.0, 3.0}));
    assert(Polynomial::from_string("-5.5x^2-1") == Polynomial({-1.0, 0.0, -5.5}));
    assert(Polynomial::from_string("2x^3 - x + 5") == Polynomial({5.0, -1.0, 0.0, 2.0}));

    // Проверка поддержки скобок и приоритетов арифметических операций
    assert(Polynomial::from_string("(x + 1) * (x - 1)") == Polynomial({-1.0, 0.0, 1.0})); // x^2 - 1
    assert(Polynomial::from_string("2(x + 1)") == Polynomial({2.0, 2.0})); // 2x + 2
    assert(Polynomial::from_string("(x + 1)^2 - x^2") == Polynomial({1.0, 2.0})); // 2x + 1
    assert(Polynomial::from_string("x + x * x") == Polynomial({0.0, 1.0, 1.0})); // x^2 + x
    assert(Polynomial::from_string("1 + 2x^2") == Polynomial({1.0, 0.0, 2.0})); // 2x^2 + 1
    assert(Polynomial::from_string("-x^2") == Polynomial({0.0, 0.0, -1.0})); // -x^2 (унарный минус имеет меньший приоритет, чем степень)
    assert(Polynomial::from_string("-(x+1)^2") == Polynomial({-1.0, -2.0, -1.0})); // -(x+1)^2
    assert(Polynomial::from_string("-2(x+1)^2") == Polynomial({-2.0, -4.0, -2.0})); // -2(x+1)^2
    assert(Polynomial::from_string("-x(x+1)") == Polynomial({0.0, -1.0, -1.0})); // -x(x+1) -> -x^2 - x

    
    // Проверка обработки некорректных строк
    try {
        Polynomial::from_string("(x + 1");
        assert(false); // Ожидалось исключение: незакрытая скобка
    } catch (const invalid_argument&) {}

    try {
        Polynomial::from_string("x^1.5");
        assert(false); // Ожидалось исключение: нецелая степень
    } catch (const invalid_argument&) {}
}

int main() {
    test_constructors();
    test_evaluation();
    test_addition();
    test_subtraction();
    test_multiplication();
    test_error_division();
    test_power();
    test_formatting();
    test_parsing();

    cout << "Все юнит-тесты класса Polynomial успешно пройдены!" << endl;
    return 0;
}
