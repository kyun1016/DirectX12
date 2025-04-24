#include "pch.h"

#include "AppBase.h"
#include "MyApp.h"

using namespace std;

int main(int argc, char* argv[]) {
	SetConsoleOutputCP(CP_UTF8);

	MyApp::GetInstance()->Run();

	MyApp::GetInstance()->Release();
	return 0;
}