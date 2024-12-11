#include <memory>
#include <Windows.h>
#include "AppBase.h"

using namespace std;

int main(int argc, char* argv[]) {
	SetConsoleOutputCP(CP_UTF8);

	std::unique_ptr<kyun::AppBase> app;
	cout << "안녕!" << endl;

	return 0;
}