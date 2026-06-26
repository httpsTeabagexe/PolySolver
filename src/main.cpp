// ====================================================================================
// ПРИМЕЧАНИЕ: Данный файл использует готовые решения из открытого доступа (boilerplate):
// Инициализация Win32 окна, настройка Direct3D 11, функции CreateDeviceD3D, CleanupDeviceD3D,
// WndProc и основной цикл рендеринга основаны на официальном открытом примере Dear ImGui:
// https://github.com/ocornut/imgui/tree/master/examples/example_win32_directx11
// Лицензия: MIT
// ====================================================================================

#include "imgui.h"
#include "implot.h"
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
    const ImVec4 Primary = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);      // Синий акцент
    const ImVec4 Secondary = ImVec4(0.18f, 0.80f, 0.44f, 1.00f);    // Зеленый для успешных действий
    const ImVec4 Warning = ImVec4(0.90f, 0.30f, 0.26f, 1.00f);      // Красный для вывода ошибок
    const ImVec4 Background = ImVec4(0.07f, 0.08f, 0.10f, 1.00f);   // Темно-серый фон главного окна
    const ImVec4 CardBg = ImVec4(0.12f, 0.13f, 0.16f, 1.00f);       // Темно-синий фон "карточек"
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
    ImGui::CreateContext();
    ImPlot::CreateContext();

    // Стилизация ImPlot для бесшовной интеграции с темной темой
    ImPlotStyle& implot_style = ImPlot::GetStyle();
    implot_style.Colors[ImPlotCol_FrameBg] = Theme::CardBg;
    implot_style.Colors[ImPlotCol_PlotBg] = Theme::Background;
    implot_style.Colors[ImPlotCol_PlotBorder] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    implot_style.Colors[ImPlotCol_AxisGrid] = ImVec4(0.20f, 0.22f, 0.27f, 0.50f);

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Включаем управление с клавиатуры

    // Загрузка шрифта с поддержкой кириллицы (Cyrillic) для корректного отображения русских символов
    ImFont* font = io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\arial.ttf)", 16.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    if (font == nullptr) {
        // Резервный вариант, если Arial не доступен
        io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\tahoma.ttf)", 16.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
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
    float plot_y_min = -10.0f;
    float plot_y_max = 10.0f;
    bool plot_auto_y = true; // Автоматическое выравнивание по оси Y по умолчанию
    const int plot_points = 200;
    vector<float> plot_data_x(plot_points);
    vector<float> plot_data_A(plot_points);
    vector<float> plot_data_B(plot_points);
    vector<float> plot_data_Res(plot_points);
    bool plot_show_A = true;
    bool plot_show_B = true;
    bool plot_show_Res = false;
    bool reset_limits = true; // Флаг для первоначальной настройки лимитов графика при запуске или сбросе

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

            // Высота карточки результата увеличена, чтобы помещались длинные уравнения
            float result_card_h = (debug_mode && !debug_steps.empty()) ? 95.0f : 120.0f;
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

            // Если включен режим отладки и накоплены пошаговые инструкции, выводим их в лог (высота увеличена до 200px)
            if (debug_mode && !debug_steps.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(Theme::Primary, "Пошаговый разбор решения (Отладка):");
                ImGui::BeginChild("DebugStepsCard", ImVec2(0, 200), true);
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

            // Кнопки сброса масштаба и включения автовыравнивания по Y
            if (ImGui::Button("Домой (Сбросить вид)")) {
                plot_x_min = -5.0f;
                plot_x_max = 5.0f;
                plot_y_min = -10.0f;
                plot_y_max = 10.0f;
                plot_auto_y = true;
                reset_limits = true;
                update_plots();
            }
            ImGui::SameLine();
            ImGui::Checkbox("Авто-масштаб по Y", &plot_auto_y);

            ImGui::Spacing();

            // Определение размера холста
            ImVec2 canvas_size = ImGui::GetContentRegionAvail(); // Доступные размеры холста
            if (canvas_size.y < 350.0f) canvas_size.y = 350.0f; // Задаем комфортную высоту холста

            // Если активен режим авто-масштабирования по Y, рассчитываем границы Y автоматически
            if (plot_auto_y) {
                bool first_val = true; // Флаг для первой добавленной точки при поиске границ
                float temp_y_min = -10.0f; // Временный минимум Y для авто-масштабирования
                float temp_y_max = 10.0f; // Временный максимум Y для авто-масштабирования
                
                auto check_y_bounds = [&](const vector<float>& y_data, bool show) {
                    if (!show) return;
                    for (float val : y_data) {
                        if (isnan(val) || isinf(val)) continue;
                        if (val > 1000.0f) val = 1000.0f;
                        if (val < -1000.0f) val = -1000.0f;
                        if (first_val) {
                            temp_y_min = val;
                            temp_y_max = val;
                            first_val = false;
                        } else {
                            if (val < temp_y_min) temp_y_min = val;
                            if (val > temp_y_max) temp_y_max = val;
                        }
                    }
                };

                check_y_bounds(plot_data_A, plot_show_A);
                check_y_bounds(plot_data_B, plot_show_B);
                check_y_bounds(plot_data_Res, plot_show_Res);

                if (temp_y_max == temp_y_min) {
                    temp_y_max += 1.0f;
                    temp_y_min -= 1.0f;
                } else {
                    float padding = (temp_y_max - temp_y_min) * 0.1f; // 10% отступ от краев
                    temp_y_max += padding;
                    temp_y_min -= padding;
                }
                
                plot_y_min = temp_y_min;
                plot_y_max = temp_y_max;
            }

            // Отрисовка интерактивного графика через ImPlot
            if (ImPlot::BeginPlot("##PolynomialPlot", canvas_size, ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText)) {
                // Настраиваем оси координат и масштаб
                ImPlot::SetupAxes("x", "y");
                
                if (reset_limits) {
                    ImPlot::SetupAxisLimits(ImAxis_X1, -5.0f, 5.0f, ImPlotCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, -10.0f, 10.0f, ImPlotCond_Always);
                    plot_x_min = -5.0f;
                    plot_x_max = 5.0f;
                    plot_y_min = -10.0f;
                    plot_y_max = 10.0f;
                    reset_limits = false;
                } else {
                    ImPlot::SetupAxisLimits(ImAxis_X1, plot_x_min, plot_x_max, ImPlotCond_Always);
                    if (plot_auto_y) {
                        ImPlot::SetupAxisLimits(ImAxis_Y1, plot_y_min, plot_y_max, ImPlotCond_Always);
                    } else {
                        ImPlot::SetupAxisLimits(ImAxis_Y1, plot_y_min, plot_y_max, ImPlotCond_Once);
                    }
                }

                // Считываем текущие границы осей, измененные пользователем при панорамировании или зуме
                ImPlotRect limits = ImPlot::GetPlotLimits();
                float current_x_min = static_cast<float>(limits.X.Min);   // Текущая левая граница графика по оси X
                float current_x_max = static_cast<float>(limits.X.Max);   // Текущая правая граница графика по оси X
                float current_y_min = static_cast<float>(limits.Y.Min);   // Текущая нижняя граница графика по оси Y
                float current_y_max = static_cast<float>(limits.Y.Max);   // Текущая верхняя граница графика по оси Y

                // Если границы по оси X изменились, обновляем расчет точек кривых для плавности отображения
                if (abs(plot_x_min - current_x_min) > 1e-4f || abs(plot_x_max - current_x_max) > 1e-4f) {
                    plot_x_min = current_x_min;
                    plot_x_max = current_x_max;
                    update_plots();
                }

                // Если авто-масштабирование по Y включено, проверяем, пытается ли пользователь изменить масштаб по Y вручную
                if (plot_auto_y) {
                    if (abs(plot_y_min - current_y_min) > 1e-3f || abs(plot_y_max - current_y_max) > 1e-3f) {
                        if (ImPlot::IsPlotHovered() && (ImGui::GetIO().MouseWheel != 0.0f || ImGui::IsMouseDragging(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Right))) {
                            plot_auto_y = false;
                        }
                    }
                } else {
                    plot_y_min = current_y_min;
                    plot_y_max = current_y_max;
                }

                // Отображение самих линий графиков с соответствующими цветами темы
                if (plot_show_A) {
                    ImPlot::SetNextLineStyle(ImVec4(61.0f/255.0f, 133.0f/255.0f, 224.0f/255.0f, 1.0f), 2.0f); // Синий акцент A(x)
                    ImPlot::PlotLine("A(x)", plot_data_x.data(), plot_data_A.data(), plot_points);
                }
                if (plot_show_B) {
                    ImPlot::SetNextLineStyle(ImVec4(46.0f/255.0f, 204.0f/255.0f, 113.0f/255.0f, 1.0f), 2.0f); // Зеленый B(x)
                    ImPlot::PlotLine("B(x)", plot_data_x.data(), plot_data_B.data(), plot_points);
                }
                if (plot_show_Res) {
                    ImPlot::SetNextLineStyle(ImVec4(241.0f/255.0f, 196.0f/255.0f, 15.0f/255.0f, 1.0f), 2.0f); // Желтый/Золотой результат
                    ImPlot::PlotLine("Результат", plot_data_x.data(), plot_data_Res.data(), plot_points);
                }

                // Интерактивный режим Desmos: перекрестие и маркеры значений при наведении мыши
                if (ImPlot::IsPlotHovered()) {
                    ImPlotPoint mouse_math = ImPlot::GetPlotMousePos(); // Координаты мыши в математической системе координат
                    double m_x = mouse_math.x;                          // Координата X мыши

                    // Получаем экранные координаты и рисуем линии перекрестия внутри окна графика
                    ImDrawList* plot_draw_list = ImPlot::GetPlotDrawList();

                    // Осевые линии по умолчанию
                    ImVec2 line_v_top = ImPlot::PlotToPixels(m_x, plot_y_max);          // Верхняя точка вертикальной линии перекрестия
                    ImVec2 line_v_bottom = ImPlot::PlotToPixels(m_x, plot_y_min);       // Нижняя точка вертикальной линии перекрестия
                    ImVec2 line_h_left = ImPlot::PlotToPixels(plot_x_min, mouse_math.y); // Левая точка горизонтальной линии перекрестия
                    ImVec2 line_h_right = ImPlot::PlotToPixels(plot_x_max, mouse_math.y);// Правая точка горизонтальной линии перекрестия

                    // Тонкие линии перекрестия
                    plot_draw_list->AddLine(line_v_bottom, line_v_top, IM_COL32(200, 200, 200, 50), 1.0f);
                    plot_draw_list->AddLine(line_h_left, line_h_right, IM_COL32(200, 200, 200, 50), 1.0f);

                    // Собираем текст всплывающей подсказки
                    string tooltip_str = "X: " + std::to_string(m_x).substr(0, 7) + "\n"; // Строка всплывающего тултипа

                    auto draw_hover_marker = [&](double val, bool show, ImU32 color, const string& label) {
                        if (!show || isnan(val) || isinf(val)) return;
                        // Проверяем, находится ли значение в разумных пределах отображения
                        if (val >= plot_y_min - (plot_y_max - plot_y_min) * 0.1 && val <= plot_y_max + (plot_y_max - plot_y_min) * 0.1) {
                            ImVec2 marker_pos = ImPlot::PlotToPixels(m_x, val); // Экранные координаты маркера на кривой
                            plot_draw_list->AddCircleFilled(marker_pos, 4.0f, color);
                            plot_draw_list->AddCircle(marker_pos, 6.0f, IM_COL32(255, 255, 255, 200), 0, 1.0f);
                            tooltip_str += label + "(x) = " + std::to_string(val).substr(0, 7) + "\n";
                        }
                    };

                    draw_hover_marker(polyA.evaluate(m_x), plot_show_A, IM_COL32(61, 133, 224, 255), "A");
                    draw_hover_marker(polyB.evaluate(m_x), plot_show_B, IM_COL32(46, 204, 113, 255), "B");
                    if (plot_show_Res) {
                        draw_hover_marker(polyRes.evaluate(m_x), plot_show_Res, IM_COL32(241, 196, 15, 255), "Рез");
                    }

                    if (!tooltip_str.empty() && tooltip_str.back() == '\n') {
                        tooltip_str.pop_back();
                    }

                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(tooltip_str.c_str());
                    ImGui::EndTooltip();
                }

                ImPlot::EndPlot();
            }
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
    ImPlot::DestroyContext();
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
