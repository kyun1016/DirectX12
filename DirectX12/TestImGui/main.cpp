#include "pch.h"

#include "AppBase.h"
#include "LandAndWavesApp.h"

using namespace std;

int main(int argc, char* argv[]) {
	SetConsoleOutputCP(CP_UTF8);

	std::unique_ptr<AppBase> app;

	app = make_unique<LandAndWavesApp>();

	return app->Run();
}