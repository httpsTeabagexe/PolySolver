#include "polynomial.h"
#include <algorithm>
#include <cctype>

using namespace std;

// Метод нормализации: удаляет нулевые коэффициенты при старших степенях.
// Мы используем epsilon-окрестность (1e-9) для сравнения вещественных чисел с нулем,
// так как из-за точности double число может быть не ровно 0, а очень близким к нему (например, 1e-16).
void Polynomial::normalize() {
    while (!coeffs_.empty() && abs(coeffs_.back()) < 1e-9) {
        coeffs_.pop_back();
    }
}

// Конструктор, принимающий вектор коэффициентов. После сохранения вызывается нормализация.
Polynomial::Polynomial(const vector<double>& coeffs) : coeffs_(coeffs) {
    normalize();
}

// Степень многочлена — это индекс последнего элемента в векторе (после нормализации).
// Если вектор пуст (многочлен равен 0), степень считается равной -1.
int Polynomial::degree() const {
    return coeffs_.empty() ? -1 : static_cast<int>(coeffs_.size()) - 1;
}

// Возвращает вектор коэффициентов.
const vector<double>& Polynomial::coefficients() const {
    return coeffs_;
}

// Коэффициент при старшей степени (последний элемент вектора).
double Polynomial::leading_coefficient() const {
    if (coeffs_.empty()) return 0.0;
    return coeffs_.back();
}

// Вычисление значения многочлена в точке x с помощью схемы Горнера.
// Схема Горнера позволяет вычислить значение за O(N) умножений и сложений:
// P(x) = a0 + x*(a1 + x*(a2 + ... + x*an)...)
double Polynomial::evaluate(double x) const {
    if (coeffs_.empty()) return 0.0;
    double result = coeffs_.back();
    for (int i = static_cast<int>(coeffs_.size()) - 2; i >= 0; --i) {
        result = result * x + coeffs_[i];
    }
    return result;
}

// Сложение многочленов: коэффициенты при одинаковых степенях просто складываются.
Polynomial Polynomial::operator+(const Polynomial& other) const {
    size_t max_size = max(coeffs_.size(), other.coeffs_.size());
    vector<double> result_coeffs(max_size, 0.0);
    for (size_t i = 0; i < max_size; ++i) {
        double a = (i < coeffs_.size()) ? coeffs_[i] : 0.0;
        double b = (i < other.coeffs_.size()) ? other.coeffs_[i] : 0.0;
        result_coeffs[i] = a + b;
    }
    return Polynomial(result_coeffs);
}

// Вычитание многочленов: коэффициенты вычитаются друг из друга.
Polynomial Polynomial::operator-(const Polynomial& other) const {
    size_t max_size = max(coeffs_.size(), other.coeffs_.size());
    vector<double> result_coeffs(max_size, 0.0);
    for (size_t i = 0; i < max_size; ++i) {
        double a = (i < coeffs_.size()) ? coeffs_[i] : 0.0;
        double b = (i < other.coeffs_.size()) ? other.coeffs_[i] : 0.0;
        result_coeffs[i] = a - b;
    }
    return Polynomial(result_coeffs);
}

// Умножение многочленов: каждый член первого многочлена умножается на каждый член второго.
// Новая степень члена равна сумме степеней перемножаемых членов: (a_i * x^i) * (b_j * x^j) = (a_i * b_j) * x^(i+j)
Polynomial Polynomial::operator*(const Polynomial& other) const {
    if (coeffs_.empty() || other.coeffs_.empty()) {
        return Polynomial(); // Умножение на 0 дает 0
    }
    vector<double> result_coeffs(coeffs_.size() + other.coeffs_.size() - 1, 0.0);
    for (size_t i = 0; i < coeffs_.size(); ++i) {
        for (size_t j = 0; j < other.coeffs_.size(); ++j) {
            result_coeffs[i + j] += coeffs_[i] * other.coeffs_[j];
        }
    }
    return Polynomial(result_coeffs);
}

// Деление многочленов «уголком» (деление с остатком).
// На каждом шаге мы делим старший член текущего остатка на старший член делителя.
// Полученный член частного умножается на делитель и вычитается из остатка.
// Процесс продолжается, пока степень остатка не станет меньше степени делителя.
pair<Polynomial, Polynomial> Polynomial::divide(const Polynomial& dividend, const Polynomial& divisor) {
    if (divisor.degree() == -1) {
        throw invalid_argument("Деление на нулевой многочлен невозможно!");
    }

    // Если степень делимого меньше степени делителя, то частное = 0, а остаток = делимое.
    if (dividend.degree() < divisor.degree()) {
        return { Polynomial(), dividend };
    }

    vector<double> q_coeffs(dividend.degree() - divisor.degree() + 1, 0.0);
    Polynomial remainder = dividend;

    while (remainder.degree() >= divisor.degree()) {
        int deg_diff = remainder.degree() - divisor.degree();
        double coeff = remainder.leading_coefficient() / divisor.leading_coefficient();

        q_coeffs[deg_diff] = coeff;

        // Создаем вычитаемый член: coeff * x^deg_diff * divisor
        vector<double> subtrahend_coeffs(deg_diff + divisor.coeffs_.size(), 0.0);
        for (size_t i = 0; i < divisor.coeffs_.size(); ++i) {
            subtrahend_coeffs[i + deg_diff] = divisor.coeffs_[i] * coeff;
        }

        remainder = remainder - Polynomial(subtrahend_coeffs);
    }

    return { Polynomial(q_coeffs), remainder };
}

// Оператор деления / возвращает только частное.
Polynomial Polynomial::operator/(const Polynomial& other) const {
    return divide(*this, other).first;
}

// Оператор взятия остатка % возвращает только остаток.
Polynomial Polynomial::operator%(const Polynomial& other) const {
    return divide(*this, other).second;
}

// Алгоритм быстрого возведения в степень (бинарное возведение) за O(log N) умножений.
// Вместо умножения полинома N раз на себя, мы используем свойства степеней:
// P^k = (P^(k/2))^2 для четных k, и P * P^(k-1) для нечетных.
Polynomial Polynomial::pow(int exponent) const {
    if (exponent < 0) {
        throw invalid_argument("Степень должна быть неотрицательной!");
    }
    Polynomial result({1.0}); // P^0 = 1
    Polynomial base = *this;
    while (exponent > 0) {
        if (exponent % 2 == 1) {
            result = result * base;
        }
        base = base * base;
        exponent /= 2;
    }
    return result;
}

// Перегрузка оператора ^ как синоним для метода pow.
Polynomial Polynomial::operator^(int exponent) const {
    return pow(exponent);
}

// Вспомогательный метод для форматирования вещественных чисел без лишних нулей в конце.
// Например, 3.500000 -> 3.5, а 5.000000 -> 5.
string Polynomial::double_to_string(double val) {
    string s = std::to_string(val);
    size_t dot_pos = s.find('.');
    if (dot_pos != string::npos) {
        s.erase(s.find_last_not_of('0') + 1, string::npos);
        if (s.back() == '.') {
            s.pop_back();
        }
    }
    return s;
}

// Перевод многочлена в красивую текстовую строку.
// Обрабатывает знаки, пропуск нулевых коэффициентов, единичные степени и коэффициенты.
string Polynomial::to_string() const {
    if (coeffs_.empty()) {
        return "0";
    }

    // Дополнительная проверка на случай, если все коэффициенты стали очень близкими к нулю
    bool all_zero = true;
    for (double c : coeffs_) {
        if (abs(c) >= 1e-9) {
            all_zero = false;
            break;
        }
    }
    if (all_zero) {
        return "0";
    }

    string res = "";
    bool is_first = true;
    for (int deg = static_cast<int>(coeffs_.size()) - 1; deg >= 0; --deg) {
        double val = coeffs_[deg];
        if (abs(val) < 1e-9) {
            continue; // Пропускаем нулевые слагаемые
        }

        if (is_first) {
            if (val < 0) {
                res += "-";
                val = -val;
            }
            is_first = false;
        } else {
            if (val < 0) {
                res += " - ";
                val = -val;
            } else {
                res += " + ";
            }
        }

        if (deg == 0) {
            res += double_to_string(val); // Свободный член (без x)
        } else {
            // Не пишем коэффициент, если он равен 1 (например, пишем x^2 вместо 1x^2)
            if (abs(val - 1.0) >= 1e-9) {
                res += double_to_string(val);
            }
            res += "x";
            if (deg > 1) {
                res += "^" + std::to_string(deg); // Степень пишется только если она больше 1
            }
        }
    }
    return res;
}

// --- НАЧАЛО НОВОГО ПАРСЕРА С ПОДДЕРЖКОЙ СКОБОК И ПРИОРИТЕТОВ ---

enum class TokenType {
    NUMBER, X, PLUS, MINUS, MUL, DIV, MOD, POWER, LPAREN, RPAREN, END
};

struct Token {
    TokenType type;
    double val;
    string text;
};

static vector<Token> tokenize(const string& str) {
    vector<Token> tokens;
    size_t i = 0;
    while (i < str.length()) {
        char c = str[i];
        if (isspace(static_cast<unsigned char>(c))) {
            i++;
            continue;
        }
        if (c == 'x' || c == 'X') {
            tokens.push_back({TokenType::X, 0.0, "x"});
            i++;
        } else if (c == '+') {
            tokens.push_back({TokenType::PLUS, 0.0, "+"});
            i++;
        } else if (c == '-') {
            tokens.push_back({TokenType::MINUS, 0.0, "-"});
            i++;
        } else if (c == '*') {
            tokens.push_back({TokenType::MUL, 0.0, "*"});
            i++;
        } else if (c == '/') {
            tokens.push_back({TokenType::DIV, 0.0, "/"});
            i++;
        } else if (c == '%') {
            tokens.push_back({TokenType::MOD, 0.0, "%"});
            i++;
        } else if (c == '^') {
            tokens.push_back({TokenType::POWER, 0.0, "^"});
            i++;
        } else if (c == '(') {
            tokens.push_back({TokenType::LPAREN, 0.0, "("});
            i++;
        } else if (c == ')') {
            tokens.push_back({TokenType::RPAREN, 0.0, ")"});
            i++;
        } else if (isdigit(static_cast<unsigned char>(c)) || c == '.') {
            size_t start = i;
            bool has_dot = (c == '.');
            i++;
            while (i < str.length() && (isdigit(static_cast<unsigned char>(str[i])) || (!has_dot && str[i] == '.'))) {
                if (str[i] == '.') has_dot = true;
                i++;
            }
            string num_str = str.substr(start, i - start);
            double val = stod(num_str);
            tokens.push_back({TokenType::NUMBER, val, num_str});
        } else {
            throw invalid_argument(string("Неизвестный символ в выражении: ") + c);
        }
    }
    tokens.push_back({TokenType::END, 0.0, ""});
    return tokens;
}

static vector<Token> insert_implicit_multiplication(const vector<Token>& tokens) {
    vector<Token> result;
    if (tokens.empty()) return result;
    
    result.push_back(tokens[0]);
    for (size_t i = 1; i < tokens.size(); ++i) {
        Token prev = tokens[i - 1];
        Token cur = tokens[i];
        
        bool need_mul = false;
        
        if (prev.type == TokenType::NUMBER && (cur.type == TokenType::X || cur.type == TokenType::LPAREN)) {
            need_mul = true;
        }
        if (prev.type == TokenType::X && cur.type == TokenType::LPAREN) {
            need_mul = true;
        }
        if (prev.type == TokenType::RPAREN && (cur.type == TokenType::X || cur.type == TokenType::NUMBER || cur.type == TokenType::LPAREN)) {
            need_mul = true;
        }
        
        if (need_mul) {
            result.push_back({TokenType::MUL, 0.0, "*"});
        }
        result.push_back(cur);
    }
    return result;
}

class ExpressionParser {
private:
    vector<Token> tokens;
    size_t pos = 0;

    Token peek() {
        return tokens[pos];
    }

    Token consume() {
        return tokens[pos++];
    }

    void match(TokenType type) {
        if (peek().type == type) {
            consume();
        } else {
            throw invalid_argument("Неожиданный символ: '" + peek().text + "', ожидалось '" + (type == TokenType::RPAREN ? ")" : "(") + "'");
        }
    }

    // Методы рекурсивного спуска для разбора выражения с правильными приоритетами:
    Polynomial expression(); // Сложение (+) и вычитание (-)
    Polynomial term();       // Умножение (*), деление (/) и остаток (%)
    Polynomial unary();      // Унарные операторы: плюс (+) и минус (-)
    Polynomial factor();     // Возведение в степень (^)
    Polynomial primary();    // Базовые элементы: числа, переменная x, выражения в скобках ()

public:
    explicit ExpressionParser(vector<Token> t) : tokens(move(t)) {}

    Polynomial parse() {
        Polynomial res = expression();
        if (peek().type != TokenType::END) {
            throw invalid_argument("Лишние символы в конце выражения: '" + peek().text + "'");
        }
        return res;
    }
};

// Выражение: терм { (+|-) терм }
Polynomial ExpressionParser::expression() {
    Polynomial res = term();
    while (peek().type == TokenType::PLUS || peek().type == TokenType::MINUS) {
        TokenType op = consume().type;
        Polynomial rhs = term();
        if (op == TokenType::PLUS) {
            res = res + rhs;
        } else {
            res = res - rhs;
        }
    }
    return res;
}

// Терм: унарный_член { (*|/|%) унарный_член }
Polynomial ExpressionParser::term() {
    Polynomial res = unary();
    while (peek().type == TokenType::MUL || peek().type == TokenType::DIV || peek().type == TokenType::MOD) {
        TokenType op = consume().type;
        Polynomial rhs = unary();
        if (op == TokenType::MUL) {
            res = res * rhs;
        } else if (op == TokenType::DIV) {
            res = res / rhs;
        } else {
            res = res % rhs;
        }
    }
    return res;
}

// Унарный член: (+|-) унарный_член | фактор
Polynomial ExpressionParser::unary() {
    Token t = peek();
    if (t.type == TokenType::PLUS) {
        consume();
        return unary(); // Унарный плюс просто передает управление дальше
    } else if (t.type == TokenType::MINUS) {
        consume();
        return Polynomial() - unary(); // Унарный минус вычитает значение из нуля
    } else {
        return factor();
    }
}

// Фактор: первичный_член [ ^ первичный_член ]
Polynomial ExpressionParser::factor() {
    Polynomial res = primary();
    if (peek().type == TokenType::POWER) {
        consume();
        Polynomial exp_poly = primary();
        if (exp_poly.degree() > 0 || exp_poly.coefficients().empty()) {
            throw invalid_argument("Показатель степени должен быть числовым значением!");
        }
        double exp_val = exp_poly.coefficients()[0];
        if (exp_val < 0.0 || floor(exp_val) != exp_val) {
            throw invalid_argument("Показатель степени должен быть целым неотрицательным числом: " + std::to_string(exp_val));
        }
        res = res.pow(static_cast<int>(exp_val));
    }
    return res;
}

// Первичный член: число | x | ( выражение )
Polynomial ExpressionParser::primary() {
    Token t = peek();
    if (t.type == TokenType::NUMBER) {
        consume();
        return Polynomial({t.val});
    } else if (t.type == TokenType::X) {
        consume();
        return Polynomial({0.0, 1.0});
    } else if (t.type == TokenType::LPAREN) {
        consume();
        Polynomial res = expression();
        match(TokenType::RPAREN);
        return res;
    } else {
        throw invalid_argument("Неожиданный символ: '" + (t.text.empty() ? "конец строки" : t.text) + "'");
    }
}

Polynomial Polynomial::from_string(const string& str) {
    if (str.empty()) {
        return Polynomial();
    }
    auto tokens = tokenize(str);
    auto implicit_tokens = insert_implicit_multiplication(tokens);
    ExpressionParser parser(implicit_tokens);
    return parser.parse();
}

// Сравнение на равенство с учетом погрешности вещественных чисел double.
bool Polynomial::operator==(const Polynomial& other) const {
    if (coeffs_.size() != other.coeffs_.size()) {
        return false;
    }
    for (size_t i = 0; i < coeffs_.size(); ++i) {
        if (abs(coeffs_[i] - other.coeffs_[i]) >= 1e-9) {
            return false;
        }
    }
    return true;
}

// Сравнение на неравенство.
bool Polynomial::operator!=(const Polynomial& other) const {
    return !(*this == other);
}
