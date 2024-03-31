#include <iostream>
#include <string>
#include <thread>
#include <WS2tcpip.h>
#include <wincrypt.h>
#include <cryptuiapi.h>
#pragma comment(lib, "ws2_32.lib")

// Глобальные переменные
HWND hInputField;
HWND hOutputField;
HWND hSendButton;

HWND ConnectWnd;
HWND ConnectField;
HWND IPLaible;
HWND IPField;
HWND PortLaible;
HWND PortField;
HWND IPButton;
HWND CloseButton;

HMENU root_menu;
HMENU low_menu;
#define menu1 1
#define menu2 2
#define menu3 3

SOCKET listening = NULL;
SOCKET client_socket = NULL;

const int len_send = 4000;
//char buf_char[len_send];


// Прототипы функций
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ConnectProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void AddTextToOutputField(const wchar_t* text, int parm, bool endline);
void SendWString(SOCKET clientSocket, const wchar_t* wstr);
void receWString(SOCKET sock);
void MainMenu(HWND);
int HostConnect(SOCKET& csock, SOCKET& lis);
int ClientConnect();



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Регистрация главного класса окна
    WNDCLASS wndClass = {};
    wndClass.lpfnWndProc = WndProc;
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = L"MyAppClass";
    // Регистрация окна для подключения
    WNDCLASS ConnectClass = {};
    ConnectClass.lpfnWndProc = ConnectProc;
    ConnectClass.hInstance = hInstance;
    ConnectClass.lpszClassName = L"MyConnectClass";

    RegisterClass(&wndClass);
    RegisterClass(&ConnectClass);

    // Создание окна подключения
    ConnectWnd = CreateWindow(ConnectClass.lpszClassName, L"Connect to IP", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 340, 150, NULL, NULL, hInstance, NULL);

    // Создание окна
    HWND hWnd = CreateWindow(wndClass.lpszClassName, L"Project Crypto", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 400, NULL, NULL, hInstance, NULL);
    // Создание поля вывода текста
    hOutputField = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY, 10, 10, 770, 200, hWnd, (HMENU)2, hInstance, NULL);
    // Создание поля для ввода текста
    hInputField = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOVSCROLL | ES_MULTILINE, 10, 220, 640, 100, hWnd, NULL, hInstance, NULL);
    // Создание кнопки отправки
    hSendButton = CreateWindow(L"BUTTON", L"Send", WS_VISIBLE | WS_CHILD, 660, 220, 110, 100, hWnd, NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    //ShowWindow(ConnectWnd, nCmdShow);

    //SOCKET tmp_sock = client_socket;
    //int tmp_len = len_send;
    //char tmp_buf[len_send];
    //int bytes_received;

    MSG msg = {};
    std::thread th;
    while (GetMessage(&msg, nullptr, 0, 0)) {

        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (client_socket > 0) {
            std::thread th(receWString, client_socket);
            //receWString(client_socket);
            th.detach();
        }
            
    }

    // Закрытие соединения
    closesocket(client_socket);

    // Очистка Winsock
    WSACleanup();

    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        MainMenu(hWnd);
        break;

    case WM_COMMAND:
        if (wParam == menu1) {
            ShowWindow(ConnectWnd, SW_SHOWNORMAL);
        }
        if (wParam == menu2) {
            AddTextToOutputField(L"Start hosting.", 0, true);
            SOCKET tmp1 = client_socket;
            SOCKET tmp2 = listening;
            //std::thread th([&tmp1, &tmp2]() {HostConnect(tmp1, tmp2); });
            std::thread th(HostConnect, std::ref(client_socket), std::ref(listening));
            //HostConnect(client_socket,listening);
            th.detach();
        }
        if (wParam == menu3) {
            AddTextToOutputField(L"Session is stopped.", 0, true);
            client_socket = NULL;
            listening = NULL;
            closesocket(listening);
            closesocket(client_socket);
            WSACleanup();
        }
        if (HWND(lParam) == hSendButton && HIWORD(wParam) == BN_CLICKED) {
            int textLength = GetWindowTextLength(hInputField);
            if (textLength > 0) {
                wchar_t* buffer = new wchar_t[textLength + 1];
                
                GetWindowText(hInputField, buffer, textLength + 1);
                SendWString(client_socket, buffer);

                delete[] buffer;
            }
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

LRESULT CALLBACK ConnectProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message)
    {
    case WM_CREATE:
        IPLaible = CreateWindow(L"STATIC", L"IP: ", WS_VISIBLE | WS_CHILD, 10, 10, 130, 20, hWnd, NULL, NULL, NULL); // Окно ввода IP
        IPField = CreateWindow(L"EDIT", L"255.255.255.255", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOVSCROLL | ES_MULTILINE, 30, 10, 130, 20, hWnd, NULL, NULL, NULL);

        PortLaible = CreateWindow(L"STATIC", L"PORT(optional): ", WS_VISIBLE | WS_CHILD, 10, 40, 150, 20, hWnd, NULL, NULL, NULL); // Окно вввода порта
        PortField = CreateWindow(L"EDIT", L"54000", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOVSCROLL | ES_MULTILINE, 115, 40, 45, 20, hWnd, NULL, NULL, NULL);

        IPButton = CreateWindow(L"BUTTON", L"Connect", WS_VISIBLE | WS_CHILD, 200, 10, 110, 40, hWnd, NULL, NULL, NULL);
        CloseButton = CreateWindow(L"BUTTON", L"Close", WS_VISIBLE | WS_CHILD, 200, 60, 110, 40, hWnd, NULL, NULL, NULL);
        return 0;
    case WM_COMMAND:
        if (HWND(lParam) == IPButton && HIWORD(wParam) == BN_CLICKED) {
            AddTextToOutputField(L"Connecting.", 0, true);
            if (ClientConnect() == 0) AddTextToOutputField(L"Connection successful.", 0, true);
            ShowWindow(ConnectWnd, SW_HIDE);
            return 0;
        }
        if (HWND(lParam) == CloseButton && HIWORD(wParam) == BN_CLICKED) {
            ShowWindow(ConnectWnd, SW_HIDE);
            return 0;
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void AddTextToOutputField(const wchar_t* text, int parm, bool endline) {  // Добавление текста в поле вывода
    int textLength = GetWindowTextLength(hOutputField);
    //SendMessage(hOutputField, EM_SETSEL, textLength, textLength);
    switch (parm) {
    case 0: break;
    case 1: SendMessage(hOutputField, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(L"You: ")); break;
    case 2: SendMessage(hOutputField, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(L"Client: ")); break;
    }
    
    SendMessage(hOutputField, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(text));
    if (endline) SendMessage(hOutputField, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(L"\r\n"));

    // Прокрутка вниз при добавлении нового текста
    //SendMessage(hOutputField, EM_SETSEL, 0, -1);
    //SendMessage(hOutputField, EM_SCROLLCARET, 0, 0);
    return;
}

void SendWString(SOCKET sock, const wchar_t* wstr) {
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* buffer = new char[bufferSize];
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buffer, bufferSize, NULL, NULL);
    
    int bytesSent = send(sock, buffer, bufferSize, 0);
    AddTextToOutputField(wstr, 1, true);
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "Failed to send data." << std::endl;
        AddTextToOutputField(L"Failed to send data.", 0, true);
    }

    delete[] buffer;
    return;
}

void receWString(SOCKET sock) {
    char buf_char[len_send];
    ZeroMemory(buf_char, len_send);

    int bytes_recv = recv(sock, buf_char, len_send, 0);
    if (bytes_recv == SOCKET_ERROR) {
        //AddTextToOutputField(L"Connection lost.", 0, true);
        return;
    }
    if (bytes_recv > 0) {
        buf_char[bytes_recv] = '\0';
        int wc_len = MultiByteToWideChar(CP_UTF8, 0, buf_char, -1, NULL, 0);
        wchar_t* buf_wchar = new wchar_t[wc_len];
        MultiByteToWideChar(CP_UTF8, 0, buf_char, -1, buf_wchar, wc_len);
        AddTextToOutputField(buf_wchar, 2, true);
    }
    return;
}

void MainMenu(HWND hWnd) {
    root_menu = CreateMenu();
    low_menu = CreateMenu();
    AppendMenu(root_menu, MF_POPUP, (UINT_PTR)low_menu, L"Connection");
    AppendMenu(low_menu, MF_STRING, menu1, L"Connect to IP");
    AppendMenu(low_menu, MF_STRING, menu2, L"Host session");
    AppendMenu(low_menu, MF_STRING, menu3, L"Stop session");

    SetMenu(hWnd, root_menu);
    return;
}

int ClientConnect()
{
    // Инициализация Winsock
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);
    int wsOK = WSAStartup(ver, &wsData);
    if (wsOK != 0) {
        std::cerr << "Can't initialize Winsock! Quitting" << std::endl;
        AddTextToOutputField(L"Can't initialize Winsock! Quitting.", 0, true);
        return -1;
    }

    // Создание сокета для клиента
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Can't create a socket! Quitting" << std::endl;
        AddTextToOutputField(L"Can't create a socket! Quitting.", 0, true);
        return -1;
    }

    // Связываем сокет с IP и портом сервера
    int text_len = GetWindowTextLength(IPField);
    wchar_t* buffer = new wchar_t[text_len + 1];
    GetWindowText(IPField, buffer, text_len + 1);

    char* server_ip = new char[text_len];
    int server_port = 54000;

    WideCharToMultiByte(CP_UTF8, 0, buffer, -1, server_ip, text_len, NULL, NULL);

    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &hint.sin_addr);

    // Подключение к серверу
    int connRes = connect(client_socket, (sockaddr*)&hint, sizeof(hint));

    delete[] buffer;
    delete[] server_ip;

    if (connRes == SOCKET_ERROR) {
        std::cerr << "Can't connect to server! Quitting." << std::endl;
        AddTextToOutputField(L"Can't connect to server! Quitting.", 0, true);
        closesocket(client_socket);
        WSACleanup();
        return -1;
    }

    return 0;
}

int HostConnect(SOCKET& csock, SOCKET& lis)
{
    // Инициализация Winsock
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);
    int wsOK = WSAStartup(ver, &wsData);
    if (wsOK != 0) {
        std::cerr << "Can't initialize Winsock! Quitting" << std::endl;
        AddTextToOutputField(L"Can't initialize Winsock! Quitting.", 0, true);
        return -1;
    }

    // Создание сокета для сервера
    lis = socket(AF_INET, SOCK_STREAM, 0);
    if (lis == INVALID_SOCKET) {
        std::cerr << "Can't create a socket! Quitting" << std::endl;
        AddTextToOutputField(L"Can't create a socket! Quitting.", 0, true);
        return -1;
    }

    // Привязка сокета к IP и порту
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(54000);
    hint.sin_addr.s_addr = INADDR_ANY;

    bind(lis, (sockaddr*)&hint, sizeof(hint));

    // Прослушивание порта
    listen(lis, SOMAXCONN);

    // Ожидание соединения с клиентом
    sockaddr_in client;
    int clientSize = sizeof(client);

    csock = accept(lis, (sockaddr*)&client, &clientSize);

    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    ZeroMemory(host, NI_MAXHOST);
    ZeroMemory(service, NI_MAXSERV);

    if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
        //std::cout << host << " connected on port " << service << std::endl;
        wchar_t* wc = new wchar_t[sizeof(host)];
        MultiByteToWideChar(CP_UTF8, 0, host, -1, wc, sizeof(host));
        AddTextToOutputField(wc, 0, false);
        AddTextToOutputField(L" connected on port ", 0, false);
        MultiByteToWideChar(CP_UTF8, 0, service, -1, wc, sizeof(host));
        AddTextToOutputField(wc, 0, true);
        closesocket(lis);
        delete[] wc;
        return 0;
    }
    else {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
        std::cout << host << " connected on port " << ntohs(client.sin_port) << std::endl;
        closesocket(lis);
        return 0;
    }

    return -1;
}