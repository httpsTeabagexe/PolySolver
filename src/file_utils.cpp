#include <windows.h>
#include <commdlg.h>
#include <fstream>
#include "file_utils.h"

using namespace std;

// Функция для сохранения полиномов в файл
bool save_polynomials(const string& filepath, const string& polyA, const string& polyB) {
    // Открываем файл для записи (ofstream).
    // Если файла нет, он будет создан. Если есть - перезаписан.
    ofstream out_file(filepath);
    
    // Проверяем, удалось ли открыть файл (например, если путь некорректный или нет прав)
    if (!out_file.is_open()) {
        return false;
    }
    
    // Записываем полином A на первую строчку
    out_file << polyA << "\n";
    
    // Записываем полином B на вторую строчку
    out_file << polyB << "\n";
    
    // Закрываем файл вручную (хотя деструктор out_file сделает это автоматически)
    out_file.close();
    
    // Возвращаем true, показывая, что сохранение прошло успешно
    return true;
}

// Функция для загрузки полиномов из файла
bool load_polynomials(const string& filepath, string& polyA, string& polyB) {
    // Открываем файл для чтения (ifstream)
    ifstream in_file(filepath);
    
    // Если файл не существует или не может быть открыт, возвращаем false
    if (!in_file.is_open()) {
        return false;
    }
    
    // Пытаемся считать первую строку в polyA и вторую в polyB.
    // Если в файле меньше двух строк, getline вернет ошибку.
    if (!getline(in_file, polyA)) {
        return false;
    }
    if (!getline(in_file, polyB)) {
        return false;
    }
    
    in_file.close();
    return true;
}

// Вспомогательная функция для открытия стандартного диалогового окна Windows
string open_file_dialog(void* hwnd) {
    // Буфер для сохранения пути выбранного файла.
    // MAX_PATH в Windows обычно равен 260 символам.
    char szFile[MAX_PATH] = { 0 };
    
    // Структура OPENFILENAMEA содержит параметры для системного диалога открытия файла
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn)); // Заполняем структуру нулями
    
    ofn.lStructSize = sizeof(ofn);
    // Приводим void* hwnd к типу HWND (дескриптор окна) для привязки диалога к основному окну
    ofn.hwndOwner = reinterpret_cast<HWND>(hwnd);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    
    // Задаем фильтры для расширений файлов. 
    // Формат строки фильтра: "Описание\0*.расширение\0..." заканчивается двумя \0.
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    
    // Флаги:
    // OFN_PATHMUSTEXIST - пользователь может выбрать файл только в существующей папке.
    // OFN_FILEMUSTEXIST - пользователь не может ввести имя несуществующего файла.
    // OFN_NOCHANGEDIR - предотвращает изменение текущей рабочей директории программы.
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    // Вызываем системную функцию GetOpenFileNameA.
    // Если пользователь выбрал файл и нажал "Открыть", функция вернет TRUE.
    if (GetOpenFileNameA(&ofn) == TRUE) {
        return string(szFile);
    }
    
    // Если пользователь нажал "Отмена" или закрыл окно, возвращаем пустую строку
    return "";
}

// Вспомогательная функция для открытия диалога сохранения файла Windows
string save_file_dialog(void* hwnd) {
    char szFile[MAX_PATH] = { 0 };
    
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = reinterpret_cast<HWND>(hwnd);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    
    // Флаги:
    // OFN_PATHMUSTEXIST - путь должен существовать.
    // OFN_OVERWRITEPROMPT - если файл уже существует, спросить пользователя о перезаписи.
    // OFN_NOCHANGEDIR - предотвращает изменение текущей рабочей директории программы.
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    
    // Вызываем системную функцию GetSaveFileNameA
    if (GetSaveFileNameA(&ofn) == TRUE) {
        return string(szFile);
    }
    
    return "";
}
