#include <GLFW\glfw3.h>

class StatsUI {
public:
	void Render();

	bool KeepRunning = false;
private:
	float lastTime;

	float framesPerSecondData[100];
	int currentFramesIdx = 0;
};