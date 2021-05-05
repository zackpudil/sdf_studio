#include <project.h>
#include <ui\scene-ui.h>
#include <ui\environment-ui.h>
#include <ui\camera-ui.h>

class ProjectUI {
public:
	ProjectUI(Project* p, SceneUI* s, EnvironmentUI* e, CameraUI* c);

	void Render();

	bool Offline = false;

private:
	Project* project;
	SceneUI* sceneUI;
	EnvironmentUI* environmentUI;
	CameraUI* cameraUI;
};