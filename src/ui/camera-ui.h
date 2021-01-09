#include <camera.h>

class CameraUI {
public:
	CameraUI(Camera*);

	void Render();
private:
	Camera* camera;
};