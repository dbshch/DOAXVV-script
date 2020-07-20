#include "windows.h"
#include "captcha.h"
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <tlhelp32.h>
#include <tchar.h>
using namespace std;
using namespace cv;

int cap_flg;

int type = 0;
int region = 0;
int position_x = 0, position_y = 0;
int fp_drink = -1;

void clickPosition(int x, int y) {
    x += position_x;
    y += position_y;

    SetCursorPos(x, y);
    INPUT Inputs[2] = { 0 };

    Inputs[0].type = INPUT_MOUSE;
    Inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    Inputs[1].type = INPUT_MOUSE;
    Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(2, Inputs, sizeof(INPUT));
}

DWORD FindProcessId()
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    DWORD result = NULL;

    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hProcessSnap) return(FALSE);

    pe32.dwSize = sizeof(PROCESSENTRY32); // <----- IMPORTANT

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if (!Process32First(hProcessSnap, &pe32))
    {
        CloseHandle(hProcessSnap);          // clean the snapshot object
        printf("!!! Failed to gather information on system processes! \n");
        return(NULL);
    }

    do
    {
        //printf("Checking process %ls\n", pe32.szExeFile);
        std::string strTo(260, 0);
        WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, &strTo[0], 260, NULL, NULL);
        if (strTo[0] == 'D' && strTo[1] == 'O' && strTo[2] == 'A')
        {
            result = pe32.th32ProcessID;
            break;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);

    return result;
}

Mat hwnd2mat(HWND hwnd) {

    HDC hwindowDC, hwindowCompatibleDC;

    int height, width, srcheight, srcwidth;
    HBITMAP hbwindow;
    Mat src;
    BITMAPINFOHEADER  bi;

    hwindowDC = GetDC(hwnd);
    hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
    SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

    RECT windowsize;    // get the height and width of the screen
    GetClientRect(hwnd, &windowsize);

    srcheight = windowsize.bottom;
    srcwidth = windowsize.right;
    height = windowsize.bottom / 2;  //change this to whatever size you want to resize to
    width = windowsize.right / 2;

    src.create(height, width, CV_8UC4);

    // create a bitmap
    hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
    bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
    bi.biWidth = width;
    bi.biHeight = -height;  //this is the line that makes it draw upside down or not
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    // use the previously created device context with the bitmap
    SelectObject(hwindowCompatibleDC, hbwindow);
    // copy from the window device context to the bitmap device context
    StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
    GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow

    // avoid memory leak
    DeleteObject(hbwindow); DeleteDC(hwindowCompatibleDC); ReleaseDC(hwnd, hwindowDC);

    return src;
}

Mat s2mat(POINT a, POINT b) {

    HDC hwindowDC, hwindowCompatibleDC;

    int height, width, srcheight, srcwidth;
    HBITMAP hbwindow;
    Mat src;
    BITMAPINFOHEADER  bi;

    hwindowDC = GetDC(NULL);
    hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
    //HBITMAP hBitmap = CreateCompatibleBitmap(hwindowDC, abs(b.x - a.x), abs(b.y - a.y));
    //HGDIOBJ old_obj = SelectObject(hwindowCompatibleDC, hBitmap);
    //BOOL    bRet = BitBlt(hwindowCompatibleDC, 0, 0, abs(b.x - a.x), abs(b.y - a.y), hwindowDC, a.x, a.y, SRCCOPY);

    SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

    srcheight = a.y;
    srcwidth = a.x;
    height = srcheight;  //change this to whatever size you want to resize to
    width = srcwidth;

    src.create(height, width, CV_8UC4);

    // create a bitmap
    hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
    bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
    bi.biWidth = width;
    bi.biHeight = -height;  //this is the line that makes it draw upside down or not
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    // use the previously created device context with the bitmap
    SelectObject(hwindowCompatibleDC, hbwindow);
    // copy from the window device context to the bitmap device context
    StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, position_x, position_y, srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
    GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow

    // avoid memory leak
    DeleteObject(hbwindow); DeleteDC(hwindowCompatibleDC); ReleaseDC(NULL, hwindowDC);

    return src;
}

void screenshot(POINT a, POINT b)
{
    // copy screen to bitmap
    HDC     hScreen = GetDC(NULL);
    HDC     hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, abs(b.x - a.x), abs(b.y - a.y));
    HGDIOBJ old_obj = SelectObject(hDC, hBitmap);
    BOOL    bRet = BitBlt(hDC, 0, 0, abs(b.x - a.x), abs(b.y - a.y), hScreen, a.x, a.y, SRCCOPY);

    // save bitmap to clipboard
    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_BITMAP, hBitmap);
    CloseClipboard();

    // clean up
    SelectObject(hDC, old_obj);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);
    DeleteObject(hBitmap);
}

struct state {
    //state* next_state, *next2;
    std::vector<state*> next_state;
    int (*check_next)(Mat input);
    void (*action)();
};

state init, prep, wait1, wait3, FP, game, scaptcha;

void waitTime(int r = 3000) {
    int t = rand() % r;
    waitKey(t+500);
}

void run_machine() {
    state now = init;
    POINT a, b;
    a.x = 970;
    a.y = 570;
    b.x = 970;
    b.y = 570;
    Mat input, gray;
    while (1) {
        now.action();
        if (fp_drink == -2)
            return;
        if (cap_flg == 0) {
            input = s2mat(a, b);
            cv::cvtColor(input, gray, COLOR_BGR2GRAY);
            cv::threshold(gray, gray, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
            int r = now.check_next(gray);
            now = *(now.next_state[r]);
            cv::waitKey(300);
        }
        else {
            now = *(now.next_state[0]);
        }
    }
}

int c_init(Mat input) {
    Mat ma = region == 1 ? imread("init.png") : imread("init_jp.png");
    cv::cvtColor(ma, ma, COLOR_BGR2GRAY);
    cv::Rect myROI(0, 50, 100, 100);
    cv::Mat img1 = input(myROI);
    //imwrite("init_jp.png", img1);
    Mat diff;
    cv::compare(img1, ma, diff, cv::CMP_NE);
    int nz = cv::countNonZero(diff);
    printf("init %d\n",nz);
    while (nz > 2000) {
        POINT a, b;
        a.x = 970;
        a.y = 570;
        Mat input, gray;
        b.x = 970;
        b.y = 570;
        input = s2mat(a, b);
        cv::cvtColor(input, input, COLOR_BGR2GRAY);
        cv::threshold(input, input, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
        cv::Mat img1 = input(myROI);
        cv::compare(img1, ma, diff, cv::CMP_NE);
        nz = cv::countNonZero(diff);
        printf("init %d\n", nz);
        waitKey(1000);
    }
    return 0;
}

int c_prep(Mat input) {
    Mat ma = region == 1 ? imread("prep.png") : imread("prep_jp.png");
    cv::cvtColor(ma, ma, COLOR_BGR2GRAY);
    cv::Rect myROI(680, 510, 100, 60);
    cv::Mat img1 = input(myROI);
    //imwrite("prep_jp.png", img1);
    Mat diff;
    cv::compare(img1, ma, diff, cv::CMP_NE);
    int nz = cv::countNonZero(diff);
    cout << "prep " << nz << endl;
    SetCursorPos(position_x+500, position_y+340);
    int cnt = 0;
    while (nz > 1000) {
        POINT a, b;
        a.x = 970;
        a.y = 570;
        b.x = 970;
        b.y = 570;
        Mat input, gray;
        input = s2mat(a, b);
        cv::cvtColor(input, input, COLOR_BGR2GRAY);
        cv::threshold(input, input, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
        cv::Mat img1 = input(myROI);
        cv::compare(img1, ma, diff, cv::CMP_NE);
        nz = cv::countNonZero(diff);
        cout << "prep " << nz << endl;
        //imshow("i", img1);
        ++cnt;
        waitKey(500);
        if (cnt == 10) {
            return 1;
        }
    }
    return 0;
}

int c_wait1(Mat input) {
    int flg = 0;
    Mat ma = imread("wait_jp.png");
    cv::cvtColor(ma, ma, COLOR_BGR2GRAY);
    Mat fp = imread("fp.png");
    cv::cvtColor(fp, fp, COLOR_BGR2GRAY);
    cv::Rect myROI(40, 50, 250, 100);
    cv::Mat img1 = input(myROI);
    //imwrite("wait_jp.png", img1);

    Mat diff;
    cv::compare(img1, ma, diff, cv::CMP_NE);
    int nz = cv::countNonZero(diff);
    cv::compare(img1, fp, diff, cv::CMP_NE);
    int nz2 = cv::countNonZero(diff);
    printf("wait %d %d\n", nz , nz2);
    while (1) {
        if (nz2 < 100)
            flg += 1;
        if (flg > 2)
            return 1;
        if (nz < 4000)
            return 0;
        POINT a, b;
        a.x = 970;
        a.y = 570;
        b.x = 970;
        b.y = 570;
        Mat input, gray;
        input = s2mat(a, b);
        cv::cvtColor(input, input, COLOR_BGR2GRAY);
        cv::threshold(input, input, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
        cv::Mat img1 = input(myROI);
        cv::compare(img1, ma, diff, cv::CMP_NE);
        nz = cv::countNonZero(diff);
        cv::compare(img1, fp, diff, cv::CMP_NE);
        nz2 = cv::countNonZero(diff);
        printf("wait %d %\n", nz, nz2);
        //imshow("i", img1);
        waitKey(500);
    }
}

void a_init() {
    //waitTime();
    clickPosition(670, 210);
    waitTime(500);
    if (type == 1)
        clickPosition(698 + rand() % 235, 368);//398 +rand() % 26);
    else if (type == 2)
        clickPosition(698 + rand() % 235, 398);//398 +rand() % 26);
    else if (type == 3)
        clickPosition(698 + rand() % 235, 464);//398 +rand() % 26);
}

void a_prep() {
    waitTime(3000);
    clickPosition(663+rand()%181, 522+rand()%32);
}

void a_captcha() {
    cap_flg = 1;
    while (1) {
        int r = 0;
        POINT a, b;
        a.x = 970;
        a.y = 570;
        b.x = 970;
        b.y = 570;
        int cn = 0;
        while (r == 0) {
            r = captcha(s2mat(a, b));
            if (cn == 50) r = 111111;
        }
        printf("%d\n", r);
        INPUT input[12];
        int k = 10;
        for (int i = 0; i < 6; ++i) {

            WORD vkey = 0x30 + r % 10; // see link below
            r /= 10;
            input[(5 - i) * 2].type = INPUT_KEYBOARD;
            input[(5 - i) * 2].ki.wScan = MapVirtualKey(vkey, MAPVK_VK_TO_VSC);
            input[(5 - i) * 2].ki.time = 0;
            input[(5 - i) * 2].ki.dwExtraInfo = 0;
            input[(5 - i) * 2].ki.wVk = vkey;
            input[(5 - i) * 2].ki.dwFlags = 0; // there is no KEYEVENTF_KEYDOWN

            input[(5 - i) * 2 + 1].ki.dwFlags = KEYEVENTF_KEYUP;
        }
        SendInput(12, input, sizeof(INPUT));

        clickPosition(550, 467);
        waitKey(6000);
        cv::Rect myROI(350, 150, 270, 70);
        Mat sc, gray;
        sc = s2mat(a, b);
        cv::cvtColor(sc, sc, COLOR_BGR2GRAY);
        cv::threshold(sc, sc, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
        cv::Mat img1 = sc(myROI);
       // imshow("i", img1);
        Mat cap = region == 1 ? imread("cap.png") : imread("cap_jp.png");
        cv::cvtColor(cap, cap, COLOR_BGR2GRAY);
        Mat diff;
        cv::compare(img1, cap, diff, cv::CMP_NE);
        int nz = cv::countNonZero(diff);
        if (nz > 1000) {
            return;
        }
    }
}

void a_FP() {
    waitTime(1000);
    if (fp_drink == 0) {
        cout << "no fp" << endl;
        waitKey(0);
        fp_drink = -2;
        return;
    }
    if (fp_drink > 0) --fp_drink;
    while (1) {
        cv::Rect myROI(350, 150, 270, 70);
        POINT a, b;
        a.x = 970;
        a.y = 570;
        b.x = 970;
        b.y = 570;
        Mat input, gray;
        input = s2mat(a, b);
        cv::cvtColor(input, input, COLOR_BGR2GRAY);
        cv::threshold(input, input, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
        cv::Mat img1 = input(myROI);
        //imwrite("cap_fp_jp.png", img1);
        //imshow("i", img1);
        Mat cap = region == 1 ? imread("cap.png") : imread("cap_jp.png");
        cv::cvtColor(cap, cap, COLOR_BGR2GRAY);
        Mat diff;
        cv::compare(img1, cap, diff, cv::CMP_NE);
        int nz = cv::countNonZero(diff);
        printf("cap %d\n", nz);
        if (nz < 1500) {
            a_captcha();
            return;
        }
        Mat ma = region==1? imread("cap_fp.png") : imread("cap_fp_jp.png");
        cv::cvtColor(ma, ma, COLOR_BGR2GRAY);
        cv::compare(img1, ma, diff, cv::CMP_NE);
        nz = cv::countNonZero(diff);
        if (nz < 2000)
            break;
        printf("fp %d\n", nz);
        waitKey(500);
    }
    if (region == 2) {
        clickPosition(449, 389);
        waitKey(1000);
    }
    clickPosition(550, 467);
}

void a_game() {
    waitTime(3000);
    clickPosition(734+rand()%56, 526+rand()%41);
    waitTime(3000);
    SetCursorPos(position_x + 503 + rand() % 69, position_y + 440 + rand() % 61);
    while (1) {
        INPUT Inputs[2] = { 0 };

        Inputs[0].type = INPUT_MOUSE;
        Inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

        Inputs[1].type = INPUT_MOUSE;
        Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

        SendInput(2, Inputs, sizeof(INPUT));

        Mat ma = imread("wait_jp.png");
        cv::cvtColor(ma, ma, COLOR_BGR2GRAY);
        cv::Rect myROI(40, 50, 250, 100);
        POINT a, b;
        a.x = 970;
        a.y = 570;
        b.x = 970;
        b.y = 570;
        Mat input, gray;
        input = s2mat(a, b);
        cv::cvtColor(input, input, COLOR_BGR2GRAY);
        cv::threshold(input, input, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
        cv::Mat img1 = input(myROI);
        Mat diff;
        cv::compare(img1, ma, diff, cv::CMP_NE);
        int nz = cv::countNonZero(diff);
        //cout << "game " << nz << endl;
        if (nz < 4000) break;
        //imshow("i", img1);
        waitTime(200);
    }
}

void a_wait() {
    cap_flg = 0;
    return;
}

int c_game(Mat input) {
    Mat ma = imread("game_jp.png");
    cv::cvtColor(ma, ma, COLOR_BGR2GRAY);
    cv::Rect myROI(740, 526, 40, 40);
    cv::Mat img1 = input(myROI);
    //imwrite("game_jp.png", img1);
    Mat diff;
    cv::compare(img1, ma, diff, cv::CMP_NE);
    int nz = cv::countNonZero(diff);
    cout << "game " << nz << endl;
    SetCursorPos(position_x + 530+rand()%50, position_y + 360+rand()%50);
    while (nz > 400) {
        INPUT Inputs[2] = { 0 };

        Inputs[0].type = INPUT_MOUSE;
        Inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

        Inputs[1].type = INPUT_MOUSE;
        Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(2, Inputs, sizeof(INPUT));

        POINT a, b;
        a.x = 970;
        a.y = 570;
        b.x = 970;
        b.y = 570;
        Mat input, gray;
        input = s2mat(a, b);
        cv::cvtColor(input, input, COLOR_BGR2GRAY);
        cv::threshold(input, input, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
        cv::Mat img1 = input(myROI);
        cv::compare(img1, ma, diff, cv::CMP_NE);
        nz = cv::countNonZero(diff);
        cout << "game " << nz << endl;
        //imshow("i", img1);
        waitTime(100);
    }
    return 0;
}

int c_wait3(Mat input) {
    return 0;
}

int c_captcha(Mat input) {
    Mat ma = imread("cap.png");
    cv::cvtColor(ma, ma, COLOR_BGR2GRAY);
    cv::Rect myROI(350, 150, 270, 70);
    cv::Mat img1 = input(myROI);
    Mat diff;
    cv::compare(img1, ma, diff, cv::CMP_NE);
    int nz = cv::countNonZero(diff);
    //cout << "prep " << nz << endl;
    SetCursorPos(position_x + 500, position_y + 340);
    while (nz > 10) {
        POINT a, b;
        a.x = 970;
        a.y = 570;
        b.x = 970;
        b.y = 570;
        Mat input, gray;
        input = s2mat(a, b);
        cv::cvtColor(input, input, COLOR_BGR2GRAY);
        cv::threshold(input, input, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
        cv::Mat img1 = input(myROI);
       // imshow("i", img1);
        Mat cap = imread("cap.png");
        cv::cvtColor(cap, cap, COLOR_BGR2GRAY);
        Mat diff;
        cv::compare(img1, cap, diff, cv::CMP_NE);
        int nz = cv::countNonZero(diff);
        if (nz < 1000)
            return 1;
        ma = region == 1 ? imread("cap_fp.png") : imread("cap_fp_jp.png");
        cv::cvtColor(ma, ma, COLOR_BGR2GRAY);
        cv::compare(img1, ma, diff, cv::CMP_NE);
        nz = cv::countNonZero(diff);
        if (nz < 1000)
            return 0;
        waitKey(500);
    }
    return 0;
}

int main()
{
    HWND hwnd = NULL;
    HWND hwnd2 = NULL;
    int cnt = 0;
    DWORD dwProcessID = FindProcessId();
    cout << dwProcessID << endl;
    DWORD pidwin;
    do {
        hwnd = FindWindowEx(NULL, hwnd, NULL, NULL);
        GetWindowThreadProcessId(hwnd, &pidwin);
        if (pidwin == dwProcessID) {
            ++cnt;
            if (cnt == 3)
                hwnd2 = hwnd;
            RECT rect;
            GetWindowRect(hwnd, &rect);
            cout << rect.left <<" "<< rect.right << " " << rect.top << " " << rect.bottom << endl;
            //MoveWindow
        }
    } while (hwnd != NULL);
    RECT rect;
    GetWindowRect(hwnd2, &rect);
    position_x = rect.left + 8;
    position_y = rect.top;
    //cout << rect.left <<" "<< rect.right << " " << rect.top << " " << rect.bottom << endl;
    //MoveWindow(hwnd2, -8, 0, 976, 579, false);
    cout << "1:steam 2: jp" << endl;
    cin >> region;
    while (region == 3) {
        POINT p;
        GetCursorPos(&p);
        cout << p.x << " " << p.y << endl;
        waitKey(1000);
    }
    cout << "use FP drink. -1 for infinite" << endl;
    cin >> fp_drink;
    cout << "event no." << endl;
    cin >> type;
    cap_flg = 0;
    srand(time(NULL));
    init.next_state.push_back(&prep);
    init.next_state.push_back(&init);
    init.action = a_init;
    init.check_next = c_prep;

    prep.next_state.push_back(&wait1);
    prep.next_state.push_back(&FP);
    prep.action = a_prep;
    prep.check_next = c_wait1;

    FP.next_state.push_back(&prep);
    init.next_state.push_back(&init);
    //FP.next_state.push_back(&scaptcha);
    FP.action = a_FP;
    FP.check_next = c_prep;

    //scaptcha.next_state.push_back(&prep);
    //scaptcha.action = a_captcha;
    //scaptcha.check_next = c_prep;

    wait1.next_state.push_back(&game);
    wait1.action = a_wait;
    wait1.check_next = c_game;

    game.next_state.push_back(&wait3);
    game.action = a_game;
    game.check_next = c_wait3;

    wait3.next_state.push_back(&init);
    wait3.action = a_wait;
    wait3.check_next = c_init;

    run_machine();
    return 0;
}