//emerald-ide-dist\win-unpacked
#include <bits/stdc++.h>
#include <windows.h>
#define TARGET_POOL "start EmeraldIDE.exe"

int main(){
	std::cout << "正在打开IDE..." << std::endl; 
	system("cd emerald-ide-dist\\win-unpacked");
	system("where EmeraldIDE.exe");
	system(TARGET_POOL);
	return 0;
}
