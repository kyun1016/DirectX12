#include <memory>
#include <Windows.h>
#include "AppBase.h"
#include "ShapesApp.h"
#include <WinUser.h>
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
	SetConsoleOutputCP(CP_UTF8);

	std::unique_ptr<AppBase> app;

	app = make_unique<ShapesApp>();

	return app->Run();
}