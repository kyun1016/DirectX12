#include "pch.h"

#include "AppBase.h"
#include "MyApp.h"

using namespace std;

int main(int argc, char* argv[]) {
	SetConsoleOutputCP(CP_UTF8);

	std::unique_ptr<AppBase> app;

	app = make_unique<MyApp>();

	return app->Run();
}