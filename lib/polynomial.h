#ifndef POLYNOMIAL_H
#define POLYNOMIAL_H

#include <vector>
#include <string>
#include <utility>
#include <stdexcept>
#include <cmath>

using std::vector;
using std::string;
using std::pair;

// Класс Polynomial представляет многочлен (полином) произвольной степени.
// Коэффициенты хранятся в векторе, где индекс элемента соответствует степени x.
class Polynomial {
private:
    // coeffs_[i] хранит коэффициент при x^i.
    // Например, для многочлена 3x^2 - 2x + 5 вектор будет выглядеть так: {5.0, -2.0, 3.0}
    vector<double> coeffs_; 

    // Вспомогательный метод для удаления нулевых старших коэффициентов.
    // Это нужно, чтобы степень многочлена определялась правильно (например, чтобы 0x^2 + x + 1 стал просто x + 1).
    void normalize();

public:
    // Конструкторы класса
    Polynomial() = default; // Создает нулевой многочлен (0)
    explicit Polynomial(const vector<double>& coeffs); // Создает многочлен из вектора коэффициентов

    // Методы доступа (геттеры)
    int degree() const; // Возвращает степень многочлена (для нуля возвращает -1)
    const vector<double>& coefficients() const; // Возвращает ссылку на вектор коэффициентов
    double leading_coefficient() const; // Возвращает коэффициент при старшей степени

    // Вычисление значения многочлена при заданном x: P(x)
    double evaluate(double x) const;

    // Базовые арифметические операции (перегрузка операторов)
    Polynomial operator+(const Polynomial& other) const; // Сложение двух многочленов
    Polynomial operator-(const Polynomial& other) const; // Вычитание
    Polynomial operator*(const Polynomial& other) const; // Умножение
    
    // Деление многочленов «уголком» (возвращает пару {Частное, Остаток})
    static pair<Polynomial, Polynomial> divide(const Polynomial& dividend, const Polynomial& divisor);
    
    Polynomial operator/(const Polynomial& other) const; // Получение только частного
    Polynomial operator%(const Polynomial& other) const; // Получение только остатка от деления

    // Возведение многочлена в целую неотрицательную степень
    Polynomial pow(int exponent) const;
    Polynomial operator^(int exponent) const; // Перегрузка оператора ^ для удобства возведения в степень

    // Ввод-вывод: перевод многочлена в строку и чтение из строки
    string to_string() const; // Перевод в читаемый вид, например: "3x^2 - 2x + 1"
    static Polynomial from_string(const string& str); // Парсинг строки пользователя в объект класса
    static string double_to_string(double val); // Преобразование вещественного числа в строку без лишних нулей в конце

    // Операторы сравнения (нужны для тестов и проверок на равенство)
    bool operator==(const Polynomial& other) const;
    bool operator!=(const Polynomial& other) const;
};

#endif // POLYNOMIAL_H
