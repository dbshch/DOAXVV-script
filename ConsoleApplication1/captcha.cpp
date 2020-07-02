#include "captcha.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>      /* printf, scanf, puts, NULL */
#include <cstdlib>     /* srand, rand */
#include <time.h>
#include <vector>
#include <cstring>
#include <algorithm>
#include "base64.h"
#include "Winhttp.h"
using namespace std;
using namespace cv;
using namespace cv::ml;

string url_encode(const string& value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << uppercase;
        escaped << '%' << setw(2) << int((unsigned char)c);
        escaped << nouppercase;
    }

    return escaped.str();
}

int captcha(cv::Mat input) {
    cv::cvtColor(input, input, COLOR_BGR2GRAY);
    cv::Rect myROI(300, 230, 360, 70);
    cv::Mat img1 = input(myROI);
    cv::threshold(img1, img1, 130, 255, THRESH_BINARY_INV);
    imwrite("a.png", img1);

    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    findContours(img1, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    Mat drawing = Mat::zeros(img1.size(), CV_8UC3);
    if (contours.size() != 6)
        return 0;
    int x[6];
    for (int i = 0; i < 6; ++i) {
        vector<Point> pi = contours.at(i);
        int x_min = 210000, y_min = 210000, x_max = 0, y_max = 0;
        for (auto p : pi) {
            if (p.x < x_min)
                x_min = p.x;
            if (p.x > x_max)
                x_max = p.x;
            if (p.y < y_min)
                y_min = p.y;
            if (p.y > y_max)
                y_max = p.y;
        }
        x[i] = x_min;
    }
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 5; ++j) {
            if (x[j + 1] < x[j]) {
                int temp = x[j];
                x[j] = x[j + 1];
                x[j + 1] = temp;
            }
        }
    }
    if (x[5] > 320)
        return 0;
    for (int i = 0; i < 5; ++i) {
        if (abs(x[i + 1] - x[i]) < 10)
            return 0;
    }

    string line;
    ifstream file("a.png", ios::in | ios::binary | ios::ate);
    int size = file.tellg();
    char* memblock = new char[size];
    file.seekg(0, ios::beg);
    file.read(memblock, size);
    file.close();
    std::string outs = base64_encode(reinterpret_cast<const unsigned char*>(memblock), size);
    outs = url_encode(outs);

    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    BOOL  bResults = FALSE;
    HINTERNET  hSession = NULL,
        hConnect = NULL,
        hRequest = NULL;
    hSession = WinHttpOpen(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.132 Safari/537.36",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    // Specify an HTTP server.
    if (hSession)
        hConnect = WinHttpConnect(hSession, L"aip.baidubce.com",
            INTERNET_DEFAULT_HTTPS_PORT, 0);

    // Create an HTTP request handle.
    if (hConnect)
        hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/rest/2.0/ocr/v1/general_basic?access_token=24.9a5156e6baf525313fb1dafaf4a730a0.2592000.1596205839.282335-18888816",
            NULL, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);

    // Send a request.
    string str = "image=" + outs;
    LPSTR  data = const_cast<char*>(str.c_str());
    LPCWSTR  header = L"content-type: application/x-www-form-urlencoded";
    LPVOID lpBuffer = (LPVOID)str.c_str();
    DWORD data_len = strlen(data);
    DWORD header_len = wcslen(header);
    if (hRequest)
        bResults = WinHttpSendRequest(hRequest,
            header, header_len,
            (LPVOID)data, data_len,
            data_len, 0);


    // End the request.
    if (bResults)
        bResults = WinHttpReceiveResponse(hRequest, NULL);

    // Keep checking for data until there is nothing left.
    string result;
    if (bResults)
    {
        do
        {
            // Check for available data.
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
                printf("Error %u in WinHttpQueryDataAvailable.\n",
                    GetLastError());

            // Allocate space for the buffer.
            pszOutBuffer = new char[dwSize + 1];
            if (!pszOutBuffer)
            {
                printf("Out of memory\n");
                dwSize = 0;
            }
            else
            {
                if (dwSize == 0) break;
                // Read the data.
                ZeroMemory(pszOutBuffer, dwSize + 1);

                if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer,
                    dwSize, &dwDownloaded))
                    printf("Error %u in WinHttpReadData.\n", GetLastError());
                else
                    result = string(pszOutBuffer);

                // Free the memory allocated to the buffer.
                delete[] pszOutBuffer;
            }
        } while (dwSize > 0);
    }
    int n = -1;
    for (int i = 0; i < 9; ++i) {
        n = result.find('\"', n + 1);
    }
    int n1 = result.find('\"', n + 1);
    result = result.substr(n + 1, n1 - n - 1);
    if (n1 - n - 1 < 6)
        return 0;
    for (int i = 0; i < 6; ++i) {
        switch (result[i])
        {
        case 'A':
            result[i] = '4';
            break;
        case 'O':
            result[i] = '0';
            break;
        case 'o':
            result[i] = '0';
            break;
        case 's':
            result[i] = '5';
            break;
        case 'S':
            result[i] = '5';
            break;
        case 'q':
            result[i] = '4';
            break;
        case '>':
        case ')':
            result[i] = '7';
            break;
        case ' ':
            result = result.substr(0, i) + result.substr(i + 1);
            --i;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            break;
        default:
            result[i] = '4';
            break;
        }
        if (result[i] - '0' > 9 || result[i] - '0' < 0) {
            cout << result << endl;
            return 0;
        }
        if (result.length() < 6) return 0;
    }
    printf("%s\n", result.c_str());
    //imshow("a", img1);
    waitKey(10);
    if (!bResults)
        printf("Error %d has occurred.\n", GetLastError());

    // Close any open handles.
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    //for (int i = 0; i < 6; ++i) {
    //    for (int j = 0; j < 5; ++j) {
    //        if (x[j + 1] < x[j]) {
    //            int temp = x[j];
    //            x[j] = x[j + 1];
    //            x[j + 1] = temp;
    //            Mat mt = nu[j];
    //            nu[j] = nu[j + 1];
    //            nu[j + 1] = mt;
    //        }
    //    }
    //}
    //for (int i = 0; i < 6; ++i) {
    //    imshow(std::to_string(i), nu[i]);
    //    printf("%d", predict(nu[i]));
    //}
    return stoi(result);
}
