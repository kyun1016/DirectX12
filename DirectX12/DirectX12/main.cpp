#include <memory>
#include <Windows.h>
#include "AppBase.h"
#include "AppSimple.h"
#include <WinUser.h>
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
	SetConsoleOutputCP(CP_UTF8);

	std::unique_ptr<kyun::AppBase> app;

	app = make_unique<kyun::AppSimple>();
	return app->Run();
}