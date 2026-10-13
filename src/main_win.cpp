#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include "catalog.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr int kSearch = 101, kList = 102, kDetails = 103, kOpen = 104;
HWND g_search{}, g_list{}, g_details{}, g_open{};
std::vector<isya::Entry> g_catalog;
std::vector<std::size_t> g_filtered;

std::wstring wide(std::string_view text) {
    if (text.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (len <= 0) return L"[UTF-8 Fehler]";
    std::wstring out(static_cast<std::size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), out.data(), len);
    return out;
}
std::filesystem::path data_path() {
    wchar_t path[32768]{};
    const auto count = GetModuleFileNameW(nullptr, path, 32768);
    return std::filesystem::path(std::wstring(path, count)).parent_path() / L"data" / L"resources.tsv";
}
void selected(HWND window) {
    const int pos = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
    if (pos < 0 || static_cast<std::size_t>(pos) >= g_filtered.size()) {
        SetWindowTextW(g_details, L"Waehle links einen Eintrag. Wiki-Angaben sind nicht live verifiziert.");
        EnableWindow(g_open, FALSE); return;
    }
    const auto& x = g_catalog[g_filtered[static_cast<std::size_t>(pos)]];
    const auto str = wide(x.name) + L"  [" + wide(x.category) + L"]\r\n" +
        L"Fundort / Bezug: " + wide(x.source_place) + L"\r\n" +
        L"Verwendung: " + wide(x.purpose) + L"\r\n" +
        L"Datenqualitaet: " + wide(x.quality) + L"\r\n" +
        L"Quelle: " + wide(x.source_url);
    SetWindowTextW(g_details, str.c_str()); EnableWindow(g_open, !x.source_url.empty());
    (void)window;
}
void refresh(HWND window) {
    wchar_t search[256]{};
    GetWindowTextW(g_search, search, 256);
    const std::wstring query(search);
    SendMessageW(g_list, LB_RESETCONTENT, 0, 0);
    g_filtered.clear();
    for (std::size_t i = 0; i < g_catalog.size(); ++i) {
        const auto& e = g_catalog[i];
        const auto name = wide(e.name);
        const auto place = wide(e.source_place);
        const auto category = wide(e.category);
        if (!isya::contains_normalized(name, query) &&
            !isya::contains_normalized(place, query) &&
            !isya::contains_normalized(category, query)) continue;
        g_filtered.push_back(i);
        const auto line = name + L"   |   " + category;
        SendMessageW(g_list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
    }
    if (!g_filtered.empty()) SendMessageW(g_list, LB_SETCURSEL, 0, 0);
    selected(window);
}
void layout(HWND window) {
    RECT r{}; GetClientRect(window, &r);
    const int w = r.right - r.left, h = r.bottom-r.top;
    MoveWindow(g_search, 15, 14, std::max(100, w - 30), 27, TRUE);
    const int left = std::max(230, w*42/100);
    MoveWindow(g_list, 15, 53, std::max(100,left-24), std::max(90,h-68), TRUE);
    MoveWindow(g_details, left, 53, std::max(100,w-left-15), std::max(100,h-110), TRUE);
    MoveWindow(g_open, left, std::max(53,h-44), 182, 28, TRUE);
}
LRESULT CALLBACK window_proc(HWND window, UINT msg, WPARAM w, LPARAM l) {
    switch(msg) {
        case WM_CREATE: {
            auto make = [&](const wchar_t* cls, const wchar_t* text, DWORD style, int id) {
                return CreateWindowExW(WS_EX_CLIENTEDGE, cls, text, style | WS_CHILD | WS_VISIBLE,
                    0, 0, 100, 30, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                    GetModuleHandleW(nullptr), nullptr);
            };
            g_search = make(L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, kSearch);
            g_list = make(L"LISTBOX", L"", WS_VSCROLL | LBS_NOTIFY | WS_TABSTOP | LBS_NOINTEGRALHEIGHT, kList);
            g_details = make(L"EDIT", L"", ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, kDetails);
            g_open = make(L"BUTTON", L"Wiki-Quelle oeffnen", BS_PUSHBUTTON | WS_TABSTOP, kOpen);
            const auto font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            for(auto control : {g_search,g_list,g_details,g_open}) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            try { g_catalog = isya::load_catalog(data_path()); }
            catch (const std::exception&) {
                MessageBoxW(window, L"data\\resources.tsv fehlt oder ist beschaedigt. Bitte das komplette Actions-ZIP entpacken.",
                    L"NextGen-Helfer - Daten fehlen", MB_OK | MB_ICONWARNING);
            }
            refresh(window); layout(window);
            return 0;
        }
        case WM_SIZE: layout(window); return 0;
        case WM_COMMAND: {
            const int id = LOWORD(w), code = HIWORD(w);
            if (id == kSearch && code == EN_CHANGE) refresh(window);
            if (id == kList && code == LBN_SELCHANGE) selected(window);
            if (id == kOpen && code == BN_CLICKED) {
                const int pos = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
                if (pos >= 0 && static_cast<std::size_t>(pos) < g_filtered.size()) {
                    const auto url = wide(g_catalog[g_filtered[static_cast<std::size_t>(pos)]].source_url);
                    if (url.rfind(L"https://fiesta-online.fandom.com/", 0) == 0)
                        ShellExecuteW(window, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                }
            }
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(window, msg, w, l);
}
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDPIAware();
    const wchar_t* cls = L"NextGenHelferWikiCatalogWindow";
    WNDCLASSEXW wc{}; wc.cbSize = sizeof(wc); wc.hInstance=instance; wc.lpfnWndProc=window_proc;
    wc.lpszClassName=cls; wc.hCursor=LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
    if(!RegisterClassExW(&wc)) return 1;
    HWND window = CreateWindowExW(0, cls, L"NextGen-Helfer - Fiesta Wiki Datenkatalog (Prototyp)",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 930, 580, nullptr, nullptr, instance, nullptr);
    if(!window) return 2;
    ShowWindow(window,show); UpdateWindow(window);
    MSG msg{}; while(GetMessageW(&msg,nullptr,0,0)>0) {TranslateMessage(&msg); DispatchMessageW(&msg);}
    return static_cast<int>(msg.wParam);
}
