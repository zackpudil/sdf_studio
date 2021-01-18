#include "stats-ui.h"
#include <imgui.h>
#include <string>

void StatsUI::Render() {

	float currentTime = glfwGetTime();
	float delta = currentTime - lastTime;

	if (delta > 0) {
		framesPerSecondData[currentFramesIdx] = 1.0 / delta;
		currentFramesIdx = (currentFramesIdx + 1) % IM_ARRAYSIZE(framesPerSecondData);
		lastTime = currentTime;
	}

	float avg = 0.0;
	for (int i = 0; i < 100; i++) {
		avg += framesPerSecondData[i];
	}
	avg /= 100;

	ImGui::Begin("Stats");
	ImGui::PlotLines(
		"FPS", 
		framesPerSecondData, 
		IM_ARRAYSIZE(framesPerSecondData), 
		currentFramesIdx, 
		std::to_string(framesPerSecondData[currentFramesIdx]).c_str(), 
		0.0, 80, 
		ImVec2(400, 100)
	);
	ImGui::Text(std::string("Current FPS: " + std::to_string(framesPerSecondData[currentFramesIdx])).c_str());
	ImGui::Text(std::string("AVG FPS: " + std::to_string(avg)).c_str());

	ImGui::Checkbox("Keep Running", &KeepRunning);
	ImGui::End();

}
