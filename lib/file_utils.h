#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>

using std::string;

/**
 * @brief Сохраняет текстовые представления двух полиномов (A и B) в файл.
 * 
 * @param filepath Путь к файлу для сохранения.
 * @param polyA Строковое представление полинома A.
 * @param polyB Строковое представление полинома B.
 * @return true Если файл успешно сохранен.
 * @return false Если произошла ошибка при записи в файл.
 */
bool save_polynomials(const string& filepath, const string& polyA, const string& polyB);

/**
 * @brief Загружает текстовые представления двух полиномов (A и B) из файла.
 * 
 * @param filepath Путь к файлу для загрузки.
 * @param polyA Ссылка для записи считанного полинома A.
 * @param polyB Ссылка для записи считанного полинома B.
 * @return true Если файл успешно прочитан.
 * @return false Если произошла ошибка открытия или чтения файла.
 */
bool load_polynomials(const string& filepath, string& polyA, string& polyB);

/**
 * @brief Вызывает стандартное диалоговое окно Windows для выбора файла на открытие.
 * 
 * @param hwnd Указатель на дескриптор родительского окна (кастуется к HWND). Можно передать nullptr.
 * @return string Путь к выбранному файлу или пустая строка, если пользователь отменил выбор.
 */
string open_file_dialog(void* hwnd);

/**
 * @brief Вызывает стандартное диалоговое окно Windows для выбора пути сохранения файла.
 * 
 * @param hwnd Указатель на дескриптор родительского окна (кастуется к HWND). Можно передать nullptr.
 * @return string Путь к выбранному файлу или пустая строка, если пользователь отменил выбор.
 */
string save_file_dialog(void* hwnd);

#endif // FILE_UTILS_H
