#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <stdexcept>
#include "polynomial.h"
#include "file_utils.h"    // Утилиты для работы с файлами и диалоговыми окнами Windows
#include "step_solver.h"    // Пошаговый отладчик математических операций

using namespace std;

// Глобальные объекты Direct3D 11, необходимые для отрисовки графики через видеокарту
static ID3D11Device*            g_pd3dDevice = nullptr;          // Интерфейс виртуального графического адаптера (создает ресурсы)
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;   // Контекст устройства (выполняет команды отрисовки)
static IDXGISwapChain*          g_pSwapChain = nullptr;          // Двойной буфер для плавной смены кадров на экране (Swap Chain)
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0; // Переменные для отслеживания изменения размеров окна
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr; // Представление буфера кадра, куда мы рисуем

// Предварительные объявления вспомогательных функций инициализации Direct3D и обработки системных сообщений Windows
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Цветовая палитра для премиального темного оформления интерфейса
namespace Theme {
    const ImVec4 Primary = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);      // Спокойный синий акцент
    const ImVec4 Secondary = ImVec4(0.18f, 0.80f, 0.44f, 1.00f);    // Насыщенный зеленый для успешных действий
    const ImVec4 Warning = ImVec4(0.90f, 0.30f, 0.26f, 1.00f);      // Пастельно-красный для вывода ошибок
    const ImVec4 Background = ImVec4(0.07f, 0.08f, 0.10f, 1.00f);   // Темно-серый/грифельный фон главного окна
    const ImVec4 CardBg = ImVec4(0.12f, 0.13f, 0.16f, 1.00f);       // Цвет фона карточек и панелей управления
    const ImVec4 Gold = ImVec4(0.95f, 0.77f, 0.06f, 1.00f);         // Золотой для выделения результатов вычислений
}

// Главная точка входа в программу (консольное приложение под капотом, но окно консоли скрыто благодаря флагам линкера)
int main(int, char**)
{
    // 1. Регистрация класса окна Windows
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"PolySolverWinClass", nullptr };
    ::RegisterClassExW(&wc);
    
    // Создаем системное окно Windows
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"PolySolver", WS_OVERLAPPEDWINDOW, 100, 100, 1024, 768, nullptr, nullptr, wc.hInstance, nullptr);

    // 2. Инициализация Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Отображаем окно на экране
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // 3. Инициализация контекста библиотеки Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Включаем управление с клавиатуры

    // Загрузка шрифта с поддержкой кириллицы (Cyrillic) для корректного отображения русских символов
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 16.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    if (font == nullptr) {
        // Резервный вариант, если Arial не доступен
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\tahoma.ttf", 16.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    }

    // 4. Стилизация интерфейса (округления и границы окон/кнопок)
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(10, 10);
    style.ItemInnerSpacing = ImVec2(6, 6);

    // Настройка цветов темы ImGui
    style.Colors[ImGuiCol_WindowBg] = Theme::Background;
    style.Colors[ImGuiCol_ChildBg] = Theme::CardBg;
    style.Colors[ImGuiCol_PopupBg] = Theme::CardBg;
    style.Colors[ImGuiCol_Border] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.19f, 0.24f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.26f, 0.32f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.30f, 0.38f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.15f, 0.19f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_Button] = Theme::Primary;
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.58f, 0.94f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.46f, 0.82f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.24f, 0.30f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.38f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.36f, 0.46f, 0.60f, 1.00f);

    // 5. Инициализация бэкендов ImGui для Win32 API и DirectX 11
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Буферы для ввода текста формул пользователем
    char polyA_buf[256] = "3x^2 - 2x + 1";
    char polyB_buf[256] = "x - 1";
    
    // Внутренние объекты многочленов, с которыми мы работаем
    Polynomial polyA({1.0, -2.0, 3.0});
    Polynomial polyB({-1.0, 1.0});
    Polynomial polyRes;
    Polynomial polyRem; // Объект для хранения остатка от деления
    
    // Переменные состояния UI для вывода результатов вычислений
    string result_title = "Нет данных";
    string result_poly_str = "";
    string error_message = "";
    string debug_steps = "";    // Лог пошаговых этапов решения
    bool debug_mode = false;         // Флаг включения режима отладки

    // Переменные для вычисления значения многочлена в точке
    double eval_x = 2.0;
    double eval_val_A = 0.0;
    double eval_val_B = 0.0;
    double eval_val_Res = 0.0;
    bool evaluated = false;
    
    // Степень для возведения
    int power_exp = 2;
    int is_power_of_A = 1; // 1 — возводим A(x), 0 — возводим B(x)

    // Параметры для отрисовки графика
    float plot_x_min = -5.0f;
    float plot_x_max = 5.0f;
    const int plot_points = 200;
    vector<float> plot_data_x(plot_points);
    vector<float> plot_data_A(plot_points);
    vector<float> plot_data_B(plot_points);
    vector<float> plot_data_Res(plot_points);
    bool plot_show_A = true;
    bool plot_show_B = true;
    bool plot_show_Res = false;

    // Лямбда-функция для пересчета точек графиков при изменении масштаба или самих формул
    auto update_plots = [&]() {
        float step = (plot_x_max - plot_x_min) / (plot_points - 1);
        for (int i = 0; i < plot_points; ++i) {
            float x = plot_x_min + i * step;
            plot_data_x[i] = x;
            plot_data_A[i] = static_cast<float>(polyA.evaluate(x));
            plot_data_B[i] = static_cast<float>(polyB.evaluate(x));
            plot_data_Res[i] = static_cast<float>(polyRes.evaluate(x));
        }
    };

    // Первоначальный расчет точек для графика
    update_plots();

    // 6. Главный цикл обработки сообщений и рендеринга GUI
    bool done = false;
    while (!done)
    {
        MSG msg;
        // Обрабатываем системную очередь сообщений Windows
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Если пользователь изменил размер окна, пересоздаем буфер рендеринга
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Начинаем отрисовку нового кадра ImGui
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Создаем окно на весь экран приложения
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("PolySolverPanel", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

        // Верхний колонтитул приложения
        ImGui::TextColored(Theme::Primary, "PolySolver");
        ImGui::SameLine();
        ImGui::TextDisabled("| Арифметика многочленов, импорт/экспорт и трассировка решений");
        
        // Кнопка Справка в правой части заголовочной панели (выравнивание по правому краю контента)
        ImGui::SameLine(ImGui::GetContentRegionMax().x - 85.0f);
        if (ImGui::Button("Справка")) {
            ImGui::OpenPopup("О программе PolySolver");
        }

        // Описание модального окна "Справка"
        if (ImGui::BeginPopupModal("О программе PolySolver", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(Theme::Primary, "PolySolver v1.1.0");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("PolySolver — это интерактивный калькулятор многочленов.");
            ImGui::Text("Программа предназначена для вычислений и обучения алгебре.");
            ImGui::Spacing();
            ImGui::TextColored(Theme::Gold, "Доступный функционал:");
            ImGui::BulletText("Арифметика: сложение, вычитание, умножение, деление уголком и остаток.");
            ImGui::BulletText("Степени: возведение многочленов в степень до 20 через бинарный алгоритм.");
            ImGui::BulletText("Отладка: пошаговый разбор математических решений на русском языке.");
            ImGui::BulletText("Графики: интерактивный плоттер с авто-масштабированием по осям X и Y.");
            ImGui::BulletText("Работа с файлами: импорт и экспорт уравнений через проводник Windows.");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Среда: C++20 + CLion + MinGW (GCC 13) + Dear ImGui (DirectX 11)");
            ImGui::Spacing();
            if (ImGui::Button("Закрыть", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();
        ImGui::Spacing();

        // Левая панель: Ввод данных, выбор шаблонов и математические операции
        ImGui::BeginChild("ControlPanel", ImVec2(ImGui::GetContentRegionAvail().x * 0.45f, 0), true);
        {
            // Секция 1: Ввод многочленов и файловые диалоги
            ImGui::TextColored(Theme::Primary, "1. Ввод многочленов");
            ImGui::Spacing();
            
            // Кнопки для Импорта и Экспорта (симметрично занимают всю ширину)
            float file_btn_w = (ImGui::GetContentRegionAvail().x - 10.0f) / 2.0f;
            if (ImGui::Button("Импорт из файла", ImVec2(file_btn_w, 30))) {
                string filepath = open_file_dialog(hwnd);
                if (!filepath.empty()) {
                    string loadedA, loadedB;
                    if (load_polynomials(filepath, loadedA, loadedB)) {
                        strcpy_s(polyA_buf, loadedA.c_str());
                        strcpy_s(polyB_buf, loadedB.c_str());
                        try {
                            polyA = Polynomial::from_string(polyA_buf);
                            polyB = Polynomial::from_string(polyB_buf);
                            error_message = "";
                            debug_steps = "Файлы успешно импортированы.\nA(x) = " + loadedA + "\nB(x) = " + loadedB + "\n";
                            update_plots();
                        } catch (const exception& e) {
                            error_message = string("Ошибка разбора импортированных строк: ") + e.what();
                        }
                    } else {
                        error_message = "Ошибка: не удалось прочитать многочлены из файла.";
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Экспорт в файл", ImVec2(file_btn_w, 30))) {
                string filepath = save_file_dialog(hwnd);
                if (!filepath.empty()) {
                    if (save_polynomials(filepath, polyA_buf, polyB_buf)) {
                        debug_steps = "Многочлены успешно сохранены в файл:\n" + filepath;
                    } else {
                        error_message = "Ошибка: не удалось записать многочлены в файл.";
                    }
                }
            }
            
            ImGui::Separator();
            ImGui::Spacing();

            // Ввод формулы для A(x) (выравнивание по ширине и симметрия)
            ImGui::Text("A(x):");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 200.0f);
            if (ImGui::InputText("##inputA", polyA_buf, IM_ARRAYSIZE(polyA_buf))) {
                try {
                    polyA = Polynomial::from_string(polyA_buf);
                    error_message = "";
                    update_plots();
                } catch (const exception& e) {
                    error_message = string("Ошибка в A(x): ") + e.what();
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Формат ввода: '3x^2 - 2x + 1', '-x^4 + 2.5x', '5'");
            }
            ImGui::SameLine();
            if (ImGui::Button("Шаблон##A", ImVec2(100, 0))) {
                strcpy_s(polyA_buf, "x^2 - 4x + 3");
                polyA = Polynomial::from_string(polyA_buf);
                update_plots();
            }
            ImGui::SameLine();
            if (ImGui::Button("Очистить##A", ImVec2(80, 0))) {
                polyA_buf[0] = '\0';
                polyA = Polynomial();
                update_plots();
            }

            ImGui::Spacing();

            // Ввод формулы для B(x)
            ImGui::Text("B(x):");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 200.0f);
            if (ImGui::InputText("##inputB", polyB_buf, IM_ARRAYSIZE(polyB_buf))) {
                try {
                    polyB = Polynomial::from_string(polyB_buf);
                    error_message = "";
                    update_plots();
                } catch (const exception& e) {
                    error_message = string("Ошибка в B(x): ") + e.what();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Шаблон##B", ImVec2(100, 0))) {
                strcpy_s(polyB_buf, "x - 3");
                polyB = Polynomial::from_string(polyB_buf);
                update_plots();
            }
            ImGui::SameLine();
            if (ImGui::Button("Очистить##B", ImVec2(80, 0))) {
                polyB_buf[0] = '\0';
                polyB = Polynomial();
                update_plots();
            }

            ImGui::Spacing();
            ImGui::Spacing();

            // Секция 2: Операции и настройки режима отладки
            ImGui::TextColored(Theme::Primary, "2. Математические операции");
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 140.0f);
            ImGui::Checkbox("Отладка (шаги)", &debug_mode);
            
            ImGui::Separator();
            ImGui::Spacing();

            // Сетка из кнопок арифметических операций
            float btn_w = (ImGui::GetContentRegionAvail().x - 20.0f) / 3.0f;
            
            // Операция сложения
            if (ImGui::Button("Сложить (A+B)", ImVec2(btn_w, 40))) {
                try {
                    polyA = Polynomial::from_string(polyA_buf);
                    polyB = Polynomial::from_string(polyB_buf);
                    if (debug_mode) {
                        debug_steps = StepByStepSolver::add(polyA, polyB, polyRes);
                    } else {
                        polyRes = polyA + polyB;
                        debug_steps = "";
                    }
                    result_title = "Сложение: A(x) + B(x)";
                    result_poly_str = polyRes.to_string();
                    plot_show_Res = true;
                    error_message = "";
                    update_plots();
                } catch (const exception& e) {
                    error_message = e.what();
                }
            }
            ImGui::SameLine();
            
            // Операция вычитания
            if (ImGui::Button("Вычесть (A-B)", ImVec2(btn_w, 40))) {
                try {
                    polyA = Polynomial::from_string(polyA_buf);
                    polyB = Polynomial::from_string(polyB_buf);
                    if (debug_mode) {
                        debug_steps = StepByStepSolver::subtract(polyA, polyB, polyRes);
                    } else {
                        polyRes = polyA - polyB;
                        debug_steps = "";
                    }
                    result_title = "Вычитание: A(x) - B(x)";
                    result_poly_str = polyRes.to_string();
                    plot_show_Res = true;
                    error_message = "";
                    update_plots();
                } catch (const exception& e) {
                    error_message = e.what();
                }
            }
            ImGui::SameLine();
            
            // Операция умножения
            if (ImGui::Button("Умножить (A*B)", ImVec2(btn_w, 40))) {
                try {
                    polyA = Polynomial::from_string(polyA_buf);
                    polyB = Polynomial::from_string(polyB_buf);
                    if (debug_mode) {
                        debug_steps = StepByStepSolver::multiply(polyA, polyB, polyRes);
                    } else {
                        polyRes = polyA * polyB;
                        debug_steps = "";
                    }
                    result_title = "Умножение: A(x) * B(x)";
                    result_poly_str = polyRes.to_string();
                    plot_show_Res = true;
                    error_message = "";
                    update_plots();
                } catch (const exception& e) {
                    error_message = e.what();
                }
            }

            // Операция деления уголком
            if (ImGui::Button("Разделить (A/B)", ImVec2(btn_w, 40))) {
                try {
                    polyA = Polynomial::from_string(polyA_buf);
                    polyB = Polynomial::from_string(polyB_buf);
                    if (debug_mode) {
                        debug_steps = StepByStepSolver::divide(polyA, polyB, polyRes, polyRem);
                    } else {
                        auto [q, r] = Polynomial::divide(polyA, polyB);
                        polyRes = q;
                        polyRem = r;
                        debug_steps = "";
                    }
                    result_title = "Деление: A(x) / B(x)";
                    result_poly_str = "Частное: " + polyRes.to_string() + "\nОстаток: " + polyRem.to_string();
                    plot_show_Res = true;
                    error_message = "";
                    update_plots();
                } catch (const exception& e) {
                    error_message = e.what();
                }
            }
            ImGui::SameLine();
            
            // Операция получения остатка от деления
            if (ImGui::Button("Остаток (A%B)", ImVec2(btn_w, 40))) {
                try {
                    polyA = Polynomial::from_string(polyA_buf);
                    polyB = Polynomial::from_string(polyB_buf);
                    if (debug_mode) {
                        debug_steps = StepByStepSolver::modulo(polyA, polyB, polyRes);
                    } else {
                        polyRes = polyA % polyB;
                        debug_steps = "";
                    }
                    result_title = "Остаток от деления: A(x) % B(x)";
                    result_poly_str = polyRes.to_string();
                    plot_show_Res = true;
                    error_message = "";
                    update_plots();
                } catch (const exception& e) {
                    error_message = e.what();
                }
            }
            ImGui::SameLine();
            
            // Операция сравнения многочленов (делает сетку кнопок 3х2 симметричной и завершенной)
            if (ImGui::Button("Сравнить (A==B)", ImVec2(btn_w, 40))) {
                try {
                    polyA = Polynomial::from_string(polyA_buf);
                    polyB = Polynomial::from_string(polyB_buf);
                    if (debug_mode) {
                        debug_steps = "Сравнение многочленов на равенство:\n";
                        debug_steps += "A(x) = " + polyA.to_string() + "\n";
                        debug_steps += "B(x) = " + polyB.to_string() + "\n\n";
                        if (polyA == polyB) {
                            debug_steps += "Коэффициенты многочленов полностью совпадают.\nВывод: A(x) == B(x)\n";
                        } else {
                            debug_steps += "Коэффициенты многочленов различаются.\nВывод: A(x) != B(x)\n";
                        }
                    } else {
                        debug_steps = "";
                    }
                    result_title = "Сравнение: A(x) и B(x)";
                    if (polyA == polyB) {
                        result_poly_str = "Многочлены равны:\nA(x) == B(x)";
                    } else {
                        result_poly_str = "Многочлены не равны:\nA(x) != B(x)";
                    }
                    plot_show_Res = false; // Нет результирующего многочлена для вывода на графике
                    error_message = "";
                    update_plots();
                } catch (const exception& e) {
                    error_message = e.what();
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Возведение выбранного полинома в целую степень (выравнивание в линию)
            ImGui::Text("Возведение в степень:");
            ImGui::SameLine();
            ImGui::RadioButton("A(x)", &is_power_of_A, 1); ImGui::SameLine();
            ImGui::RadioButton("B(x)", &is_power_of_A, 0);
            
            ImGui::SetNextItemWidth(120);
            ImGui::InputInt("Степень##exp", &power_exp);
            if (power_exp < 0) power_exp = 0;
            if (power_exp > 20) power_exp = 20; // Ограничиваем степень 20 для избежания перегрузки памяти при перемножении векторов
            
            ImGui::SameLine();
            if (ImGui::Button("Вычислить степень", ImVec2(160, 0))) {
                try {
                    if (is_power_of_A == 1) {
                        polyA = Polynomial::from_string(polyA_buf);
                        if (debug_mode) {
                            debug_steps = StepByStepSolver::power(polyA, power_exp, polyRes);
                        } else {
                            polyRes = polyA.pow(power_exp);
                            debug_steps = "";
                        }
                        result_title = "Результат: A(x)^" + to_string(power_exp);
                    } else {
                        polyB = Polynomial::from_string(polyB_buf);
                        if (debug_mode) {
                            debug_steps = StepByStepSolver::power(polyB, power_exp, polyRes);
                        } else {
                            polyRes = polyB.pow(power_exp);
                            debug_steps = "";
                        }
                        result_title = "Результат: B(x)^" + to_string(power_exp);
                    }
                    result_poly_str = polyRes.to_string();
                    plot_show_Res = true;
                    error_message = "";
                    update_plots();
                } catch (const exception& e) {
                    error_message = e.what();
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Вычисление значения многочленов при конкретном значении x (компактное выравнивание)
            ImGui::Text("Вычислить в точке x:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            ImGui::InputDouble("##eval_val", &eval_x, 0.1, 1.0, "%.3f");
            ImGui::SameLine();
            if (ImGui::Button("Вычислить##btn", ImVec2(160, 0))) {
                try {
                    polyA = Polynomial::from_string(polyA_buf);
                    polyB = Polynomial::from_string(polyB_buf);
                    eval_val_A = polyA.evaluate(eval_x);
                    eval_val_B = polyB.evaluate(eval_x);
                    eval_val_Res = polyRes.evaluate(eval_x);
                    evaluated = true;
                    error_message = "";
                } catch (const exception& e) {
                    error_message = e.what();
                }
            }

            // Вывод просчитанных значений на экран
            if (evaluated) {
                ImGui::Indent(10.0f);
                ImGui::TextColored(Theme::Secondary, "A(%.3f) = %.4f", eval_x, eval_val_A);
                ImGui::TextColored(Theme::Secondary, "B(%.3f) = %.4f", eval_x, eval_val_B);
                if (plot_show_Res) {
                    ImGui::TextColored(Theme::Gold, "Результат(%.3f) = %.4f", eval_x, eval_val_Res);
                }
                ImGui::Unindent(10.0f);
            }
        }
        ImGui::EndChild();

        // Правая панель: Вывод карточки результатов и графический плоттер
        ImGui::SameLine();
        ImGui::BeginChild("ResultPanel", ImVec2(0, 0), true);
        {
            // Блок вывода ошибок
            if (!error_message.empty()) {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.20f, 0.08f, 0.08f, 1.00f));
                ImGui::BeginChild("ErrorCard", ImVec2(0, 60), true);
                ImGui::TextColored(Theme::Warning, "ОШИБКА:");
                ImGui::TextWrapped("%s", error_message.c_str());
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::Spacing();
            }

            // Карточка вывода результатов последней математической операции
            ImGui::TextColored(Theme::Primary, "3. Результаты вычислений");
            ImGui::Separator();
            ImGui::Spacing();

            float result_card_h = (debug_mode && !debug_steps.empty()) ? 80.0f : 100.0f;
            ImGui::BeginChild("ResultCard", ImVec2(0, result_card_h), true);
            {
                ImGui::TextColored(Theme::Gold, "%s", result_title.c_str());
                ImGui::Separator();
                ImGui::Spacing();
                if (!result_poly_str.empty()) {
                    ImGui::TextWrapped("%s", result_poly_str.c_str());
                } else {
                    ImGui::TextDisabled("Выполните операцию, чтобы увидеть результат.");
                }
            }
            ImGui::EndChild();

            // Если включен режим отладки и накоплены пошаговые инструкции, выводим их в лог
            if (debug_mode && !debug_steps.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(Theme::Primary, "Пошаговый разбор решения (Отладка):");
                ImGui::BeginChild("DebugStepsCard", ImVec2(0, 160), true);
                ImGui::TextUnformatted(debug_steps.c_str());
                ImGui::EndChild();
            }

            ImGui::Spacing();
            ImGui::Spacing();

            // Графический просмотрщик полиномов
            ImGui::TextColored(Theme::Primary, "4. Графический просмотр");
            ImGui::Separator();
            ImGui::Spacing();

            // Переключатели слоев отображения на графике
            ImGui::Checkbox("Показать A(x) [Синий]", &plot_show_A); ImGui::SameLine();
            ImGui::Checkbox("Показать B(x) [Зеленый]", &plot_show_B); ImGui::SameLine();
            ImGui::Checkbox("Показать Результат [Желтый]", &plot_show_Res);

            // Настройка границ оси абсцисс (X)
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Границы X: Мин"); ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            if (ImGui::InputFloat("##xmin", &plot_x_min, 1.0f, 5.0f, "%.1f")) {
                if (plot_x_min >= plot_x_max) plot_x_min = plot_x_max - 1.0f;
                update_plots();
            }
            ImGui::SameLine();
            ImGui::Text("Макс"); ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            if (ImGui::InputFloat("##xmax", &plot_x_max, 1.0f, 5.0f, "%.1f")) {
                if (plot_x_max <= plot_x_min) plot_x_max = plot_x_min + 1.0f;
                update_plots();
            }

            ImGui::Spacing();

            // Отрисовка холста для рисования осей и кривых графиков
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_size = ImGui::GetContentRegionAvail();
            if (canvas_size.y < 160.0f) canvas_size.y = 160.0f; // Задаем минимальную высоту холста

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            // Заливаем холст темно-серым фоном
            draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(24, 25, 30, 255), 4.0f);
            draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(50, 52, 60, 255), 4.0f);

            // Определение масштабирования по оси ординат (Y)
            float y_min = -10.0f;
            float y_max = 10.0f;

            // Находим минимальные и максимальные значения функций на выбранном интервале, чтобы график масштабировался красиво
            bool first_val = true;
            auto check_y_bounds = [&](const vector<float>& y_data, bool show) {
                if (!show) return;
                for (float val : y_data) {
                    if (isnan(val) || isinf(val)) continue;
                    // Ограничиваем экстремальные пиковые значения, чтобы график не сжимался в линию
                    if (val > 1000.0f) val = 1000.0f;
                    if (val < -1000.0f) val = -1000.0f;
                    if (first_val) {
                        y_min = val;
                        y_max = val;
                        first_val = false;
                    } else {
                        if (val < y_min) y_min = val;
                        if (val > y_max) y_max = val;
                    }
                }
            };

            check_y_bounds(plot_data_A, plot_show_A);
            check_y_bounds(plot_data_B, plot_show_B);
            check_y_bounds(plot_data_Res, plot_show_Res);

            // Добавляем отступы по краям графика, чтобы вершины кривых не упирались в рамку
            if (y_max == y_min) {
                y_max += 1.0f;
                y_min -= 1.0f;
            } else {
                float padding = (y_max - y_min) * 0.1f;
                y_max += padding;
                y_min -= padding;
            }

            // Функция преобразования математических координат в экранные пиксели холста
            auto get_screen_coords = [&](float x, float y) -> ImVec2 {
                float t_x = (x - plot_x_min) / (plot_x_max - plot_x_min);
                float t_y = (y - y_min) / (y_max - y_min);
                return ImVec2(
                    canvas_pos.x + t_x * canvas_size.x,
                    canvas_pos.y + (1.0f - t_y) * canvas_size.y // Инвертируем Y, так как в пикселях 0 находится вверху
                );
            };

            // Рисуем осевые линии X = 0 и Y = 0, если они попадают в область видимости
            if (plot_x_min <= 0.0f && plot_x_max >= 0.0f) {
                ImVec2 p0 = get_screen_coords(0.0f, y_min);
                ImVec2 p1 = get_screen_coords(0.0f, y_max);
                draw_list->AddLine(p0, p1, IM_COL32(80, 80, 90, 255), 1.5f);
            }
            if (y_min <= 0.0f && y_max >= 0.0f) {
                ImVec2 p0 = get_screen_coords(plot_x_min, 0.0f);
                ImVec2 p1 = get_screen_coords(plot_x_max, 0.0f);
                draw_list->AddLine(p0, p1, IM_COL32(80, 80, 90, 255), 1.5f);
            }

            // Функция построения ломаной линии графика по точкам
            auto draw_plot_line = [&](const vector<float>& y_data, ImU32 color) {
                vector<ImVec2> points;
                points.reserve(plot_points);
                for (int i = 0; i < plot_points; ++i) {
                    float x = plot_data_x[i];
                    float y = y_data[i];
                    if (isnan(y) || isinf(y)) continue;
                    // Ограничиваем границы вывода точек, чтобы линии не уходили далеко за рамки холста
                    float clamped_y = max(y_min, min(y_max, y));
                    points.push_back(get_screen_coords(x, clamped_y));
                }
                for (size_t i = 1; i < points.size(); ++i) {
                    draw_list->AddLine(points[i - 1], points[i], color, 2.0f);
                }
            };

            // Отрисовываем графики соответствующих функций
            if (plot_show_A) draw_plot_line(plot_data_A, IM_COL32(61, 133, 224, 255)); // Синяя линия для A(x)
            if (plot_show_B) draw_plot_line(plot_data_B, IM_COL32(46, 204, 113, 255)); // Зеленая линия для B(x)
            if (plot_show_Res) draw_plot_line(plot_data_Res, IM_COL32(241, 196, 15, 255)); // Желтая линия для результата

            // Вывод числовых меток осей в углах холста
            char min_txt[32], max_txt[32], y_min_txt[32], y_max_txt[32];
            sprintf_s(min_txt, "X: %.1f", plot_x_min);
            sprintf_s(max_txt, "X: %.1f", plot_x_max);
            sprintf_s(y_min_txt, "Y: %.1f", y_min);
            sprintf_s(y_max_txt, "Y: %.1f", y_max);

            draw_list->AddText(ImVec2(canvas_pos.x + 5, canvas_pos.y + canvas_size.y - 18), IM_COL32(180, 180, 180, 255), min_txt);
            draw_list->AddText(ImVec2(canvas_pos.x + canvas_size.x - 70, canvas_pos.y + canvas_size.y - 18), IM_COL32(180, 180, 180, 255), max_txt);
            draw_list->AddText(ImVec2(canvas_pos.x + 5, canvas_pos.y + canvas_size.y - 35), IM_COL32(180, 180, 180, 255), y_min_txt);
            draw_list->AddText(ImVec2(canvas_pos.x + 5, canvas_pos.y + 5), IM_COL32(180, 180, 180, 255), y_max_txt);

            // Сдвигаем курсор вывода ImGui за пределы нарисованного графика
            ImGui::SetCursorScreenPos(ImVec2(canvas_pos.x, canvas_pos.y + canvas_size.y + 10.0f));
        }
        ImGui::EndChild();

        ImGui::End();

        // 7. Очистка экрана и рендеринг буферов Direct3D 11
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.10f, 0.11f, 0.13f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Смена буферов с включенной вертикальной синхронизацией (V-Sync)
        g_pSwapChain->Present(1, 0);
    }

    // 8. Корректное освобождение ресурсов при закрытии программы
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Вспомогательная функция создания графического устройства Direct3D 11 и Swap Chain
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

// Освобождение интерфейсов D3D
void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

// Создание Target View для связывания буфера рисования DirectX с экраном
void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

// Удаление Target View
void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Объявляем обработчик событий мыши и клавиатуры, поставляемый библиотекой ImGui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Функция обратного вызова (Window Procedure) для обработки системных событий окна
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Сначала передаем события в ImGui (чтобы перехватывать клики по кнопкам интерфейса)
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        // Обработка сворачивания и изменения размеров окна
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            g_ResizeWidth = (UINT)LOWORD(lParam);
            g_ResizeHeight = (UINT)HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU) // Блокируем вызов системного меню по клавише ALT
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0); // Отправляем сигнал завершения работы
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
