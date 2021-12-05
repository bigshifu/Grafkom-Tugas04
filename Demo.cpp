#include "Demo.h"



Demo::Demo() {

}


Demo::~Demo() {
}



void Demo::Init() {
	// build and compile our shader program
	// ------------------------------------
	//shadowmapShader = BuildShader("multipleLight.vert", "multipleLight.frag", nullptr);

	BuildShaders();
	BuildDepthMap();
	BuildTexturedCube();
	BuildTexturedPlane();

	InitCamera();
}

void Demo::DeInit() {
	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &cubeEBO);
	glDeleteVertexArrays(1, &planeVAO);
	glDeleteBuffers(1, &planeVBO);
	glDeleteBuffers(1, &planeEBO);
	glDeleteBuffers(1, &depthMapFBO);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void Demo::ProcessInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	// zoom camera
	// -----------
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		if (fovy < 90) {
			fovy += 0.0001f;
		}
	}

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (fovy > 0) {
			fovy -= 0.0001f;
		}
	}

	// update camera movement 
	// -------------
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		MoveCamera(CAMERA_SPEED);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		MoveCamera(-CAMERA_SPEED);
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		StrafeCamera(-CAMERA_SPEED);
	}

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		StrafeCamera(CAMERA_SPEED);
	}

	// update camera rotation
	// ----------------------
	double mouseX, mouseY;
	double midX = screenWidth / 2;
	double midY = screenHeight / 2;
	float angleY = 0.0f;
	float angleZ = 0.0f;

	// Get mouse position
	glfwGetCursorPos(window, &mouseX, &mouseY);
	if ((mouseX == midX) && (mouseY == midY)) {
		return;
	}

	// Set mouse position
	glfwSetCursorPos(window, midX, midY);

	// Get the direction from the mouse cursor, set a resonable maneuvering speed
	angleY = (float)((midX - mouseX)) / 1000;
	angleZ = (float)((midY - mouseY)) / 1000;

	// The higher the value is the faster the camera looks around.
	viewCamY += angleZ * 2;

	// limit the rotation around the x-axis
	if ((viewCamY - posCamY) > 8) {
		viewCamY = posCamY + 8;
	}
	if ((viewCamY - posCamY) < -8) {
		viewCamY = posCamY - 8;
	}
	RotateCamera(-angleY);
}

void Demo::Update(double deltaTime) {
	angle += (float)((deltaTime * 1.0f) / 1000);
}

void Demo::Render() {

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	// Step 1 Render depth of scene to texture
	// ----------------------------------------
	glm::mat4 lightProjection, lightView;
	glm::mat4 lightSpaceMatrix;
	float near_plane = 1.0f, far_plane = 10.0f;
	lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
	lightView = glm::lookAt(glm::vec3(3.0f, 4.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0, 1.0, 0.0));
	lightSpaceMatrix = lightProjection * lightView;
	// render scene from light's point of view
	UseShader(this->depthmapShader);
	glUniformMatrix4fv(glGetUniformLocation(this->depthmapShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
	glViewport(0, 0, this->SHADOW_WIDTH, this->SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	DrawTexturedCube(this->depthmapShader);
	DrawTexturedPlane(this->depthmapShader);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);



	// Step 2 Render scene normally using generated depth map
	// ------------------------------------------------------
	glViewport(0, 0, this->screenWidth, this->screenHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Pass perspective projection matrix
	UseShader(this->shadowmapShader);
	glm::mat4 projection = glm::perspective(45.0f, (GLfloat)this->screenWidth / (GLfloat)this->screenHeight, 0.1f, 100.0f);
	GLint projLoc = glGetUniformLocation(this->shadowmapShader, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// LookAt camera (position, target/direction, up)
	glm::mat4 view = glm::lookAt(glm::vec3(posCamX, posCamY, posCamZ), glm::vec3(viewCamX, viewCamY, viewCamZ), glm::vec3(upCamX, upCamY, upCamZ));
	GLint viewLoc = glGetUniformLocation(this->shadowmapShader, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	// Setting Light Attributes
	glUniformMatrix4fv(glGetUniformLocation(this->shadowmapShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
	glUniform3f(glGetUniformLocation(this->shadowmapShader, "viewPos"), posCamX, posCamY, posCamZ);
	glUniform3f(glGetUniformLocation(this->shadowmapShader, "lightPos"), 3.0f, 4.0f, -1.0f);

	// Configure Shaders
	glUniform1i(glGetUniformLocation(this->shadowmapShader, "diffuseTexture"), 0);
	glUniform1i(glGetUniformLocation(this->shadowmapShader, "shadowMap"), 1);

	// Render floor
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, plane_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	DrawTexturedPlane(this->shadowmapShader);

	// Render cube
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cube_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	DrawTexturedCube(this->shadowmapShader);

	//// set lighting attributes
	//GLint viewPosLoc = glGetUniformLocation(this->shadowmapShader, "viewPos");
	//glUniform3f(viewPosLoc, posCamX, posCamY, posCamZ);

	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "dirLight.direction"), 0.0f, -1.0f, -1.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "dirLight.ambient"), 0.1f, 0.1f, 0.1f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "dirLight.diffuse"), 1.0f, 0.0f, 0.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "dirLight.specular"), 0.1f, 0.1f, 0.1f);
	//// point light 1
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[0].position"), 0.0f, 3.0f, 0.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[0].ambient"), 1.0f, 0.0f, 1.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[0].diffuse"), 1.0f, 0.0f, 0.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[0].specular"), 1.0f, 0.0f, 0.0f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "pointLights[0].constant"), 1.0f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "pointLights[0].linear"), 0.09f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "pointLights[0].quadratic"), 0.032f);
	//// point light 2
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[1].position"), -2.0f, 3.0f, 0.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[1].ambient"), 0.0f, 1.0f, 0.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[1].diffuse"), 0.0f, 1.0f, 0.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[1].specular"), 0.0f, 1.0f, 0.0f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "pointLights[1].constant"), 1.0f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "pointLights[1].linear"), 0.09f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "pointLights[1].quadratic"), 0.032f);
	//// point light 3
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[2].position"), 2.0f, 3.0f, 0.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[2].ambient"), 0.0f, 0.0f, 1.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[2].diffuse"), 0.0f, 0.0f, 1.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[2].specular"), 0.0f, 0.0f, 1.0f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "pointLights[2].constant"), 1.0f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "pointLights[2].linear"), 0.09f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "pointLights[2].quadratic"), 0.032f);
	//// point light 4
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[3].position"), 0.0f, 3.0f, 2.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[3].ambient"), 0.0f, 1.0f, 1.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[3].diffuse"), 0.0f, 1.0f, 1.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "pointLights[3].specular"), 0.0f, 1.0f, 1.0f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "pointLights[3].constant"), 1.0f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "pointLights[3].linear"), 0.09f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "pointLights[3].quadratic"), 0.032f);
	//// spotLight
	//glUniform3fv(glGetUniformLocation(this->shadowmapShader, "spotLight.position"), 1, &posCamX);
	//glUniform3fv(glGetUniformLocation(this->shadowmapShader, "spotLight.direction"), 1, &viewCamX);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "spotLight.ambient"), 1.0f, 0.0f, 1.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "spotLight.diffuse"), 1.0f, 0.0f, 1.0f);
	//glUniform3f(glGetUniformLocation(this->shadowmapShader, "spotLight.specular"), 1.0f, 0.0f, 1.0f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "spotLight.constant"), 1.0f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "spotLight.linear"), 0.09f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "spotLight.quadratic"), 0.032f);
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "spotLight.cutOff"), glm::cos(glm::radians(12.5f)));
	//glUniform1f(glGetUniformLocation(this->shadowmapShader, "spotLight.outerCutOff"), glm::cos(glm::radians(15.0f)));

	//DrawTexturedCube();
	//DrawTexturedPlane();

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
}

void Demo::BuildTexturedCube() {
	// load image into texture memory
	// ------------------------------
	// Load and create a texture 
	glGenTextures(1, &cube_texture);
	glBindTexture(GL_TEXTURE_2D, cube_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width, height;
	unsigned char* image = SOIL_load_image("motif kayu.png", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	/*glGenTextures(1, &stexture);
	glBindTexture(GL_TEXTURE_2D, stexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	image = SOIL_load_image("motif kayu_specularmap.png", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);*/

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float vertices[] = {
		// format position, tex coords
		// front
		-1, -1, 1, 0, 0,			0.0f,  0.0f,  1.0f,		// 0
		-0.7, -1, 1, 0.15, 0,		0.0f,  0.0f,  1.0f,		// 1
		-0.7,  0.7, 1, 0.15, 1,		0.0f,  0.0f,  1.0f,		// 2
		-1,  0.7, 1, 0, 1,			0.0f,  0.0f,  1.0f,		// 3
		0.7, -1, 1, 0.85, 0,		0.0f,  0.0f,  1.0f,		// 4
		1, -1, 1, 1, 0,				0.0f,  0.0f,  1.0f,		// 5
		1,  0.7, 1, 1, 1,			0.0f,  0.0f,  1.0f,		// 6
		0.7,  0.7, 1, 0.85, 1,		0.0f,  0.0f,  1.0f,		// 7
		1,  1, 1, 1, 0.15,			0.0f,  0.0f,  1.0f,		// 8
		-1,  1, 1, 0, 0.15,			0.0f,  0.0f,  1.0f,		// 9
		-1, -1, -0.7, 0, 0,			0.0f,  0.0f,  1.0f,		// 10
		-0.7, -1, -0.7, 0.15, 0,	0.0f,  0.0f,  1.0f,		// 11
		-0.7,  0.7, -0.7, 0.15, 1,	0.0f,  0.0f,  1.0f,		// 12
		-1,  0.7, -0.7, 0, 1,		0.0f,  0.0f,  1.0f,		// 13
		0.7, -1, -0.7, 0.85, 0,		0.0f,  0.0f,  1.0f,		// 14
		1, -1, -0.7, 1, 0,			0.0f,  0.0f,  1.0f,		// 15
		1,  0.7, -0.7, 1, 1,		0.0f,  0.0f,  1.0f,		// 16
		0.7,  0.7, -0.7, 0.85, 1,	0.0f,  0.0f,  1.0f,		// 17

		// right
		1, -1, 1, 0, 0,				1.0f,  0.0f,  0.0f,// 18
		1, -1, 0.7, 0.15, 0,		1.0f,  0.0f,  0.0f,// 19
		1,  0.7, 0.7, 0.15, 1,		1.0f,  0.0f,  0.0f,// 20
		1,  0.7, 1, 0, 1,			1.0f,  0.0f,  0.0f,// 21
		1, -1, -0.7, 0.85, 0,		1.0f,  0.0f,  0.0f,// 22
		1, -1, -1, 1, 0,			1.0f,  0.0f,  0.0f,// 23
		1,  0.7, -1, 1, 1,			1.0f,  0.0f,  0.0f,// 24
		1,  0.7, -0.7, 0.85, 1,		1.0f,  0.0f,  0.0f,// 25
		1,  1, -1, 1, 1,			1.0f,  0.0f,  0.0f,// 26
		1,  1, 1, 0, 1,				1.0f,  0.0f,  0.0f,// 27
		-0.7, -1, 1, 0, 0,			1.0f,  0.0f,  0.0f,// 28
		-0.7, -1, 0.7, 0.15, 0,		1.0f,  0.0f,  0.0f,// 29
		-0.7,  0.7, 0.7, 0.15, 1,	1.0f,  0.0f,  0.0f,// 30
		-0.7,  0.7, 1, 0, 1,		1.0f,  0.0f,  0.0f,// 31
		-0.7, -1, -0.7, 0.85, 0,	1.0f,  0.0f,  0.0f,// 32
		-0.7, -1, -1, 1, 0,			1.0f,  0.0f,  0.0f,// 33
		-0.7,  0.7, -1, 1, 1,		1.0f,  0.0f,  0.0f,// 34
		-0.7,  0.7, -0.7, 0.85, 1,	1.0f,  0.0f,  0.0f,// 35

		// back
		1, -1, -1, 0, 0,			0.0f,  0.0f,  -1.0f,// 36
		0.7, -1, -1, 0.15, 0,		0.0f,  0.0f,  -1.0f,// 37
		0.7,  0.7, -1, 0.15, 1,		0.0f,  0.0f,  -1.0f,// 38
		1,  0.7, -1, 0, 1,			0.0f,  0.0f,  -1.0f,// 39
		-0.7, -1, -1, 0.85, 0,		0.0f,  0.0f,  -1.0f,// 40
		-1, -1, -1, 1, 0,			0.0f,  0.0f,  -1.0f,// 41
		-1,  0.7, -1, 1, 1,			0.0f,  0.0f,  -1.0f,// 42
		-0.7,  0.7, -1, 0.85, 1,	0.0f,  0.0f,  -1.0f,// 43
		-1,  1, -1, 1, 1,			0.0f,  0.0f,  -1.0f,// 44
		1,  1, -1, 0, 1,			0.0f,  0.0f,  -1.0f,// 45
		1, -1, 0.7, 0, 0,			0.0f,  0.0f,  -1.0f,// 46
		0.7, -1, 0.7, 0.15, 0,		0.0f,  0.0f,  -1.0f,// 47
		0.7,  0.7, 0.7, 0.15, 1,	0.0f,  0.0f,  -1.0f,// 48
		1,  0.7, 0.7, 0, 1,			0.0f,  0.0f,  -1.0f,// 49
		-0.7, -1, 0.7, 0.85, 0,		0.0f,  0.0f,  -1.0f,// 50
		-1, -1, 0.7, 1, 0,			0.0f,  0.0f,  -1.0f,// 51
		-1,  0.7, 0.7, 1, 1,		0.0f,  0.0f,  -1.0f,// 52
		-0.7,  0.7, 0.7, 0.85, 1,	0.0f,  0.0f,  -1.0f,// 53

		// left
		-1, -1, -1, 0, 0,			-1.0f,  0.0f,  0.0f,// 54
		-1, -1, -0.7, 0.15, 0,		-1.0f,  0.0f,  0.0f,// 55
		-1,  0.7, -0.7, 0.15, 1,	-1.0f,  0.0f,  0.0f,// 56
		-1,  0.7, -1, 0, 1,			-1.0f,  0.0f,  0.0f,// 57
		-1, -1, 0.7, 0.85, 0,		-1.0f,  0.0f,  0.0f,// 58
		-1, -1, 1, 1, 0,			-1.0f,  0.0f,  0.0f,// 59
		-1,  0.7, 1, 1, 1,			-1.0f,  0.0f,  0.0f,// 60
		-1,  0.7, 0.7, 0.85, 1,		-1.0f,  0.0f,  0.0f,// 61
		-1,  1, 1, 1, 1,			-1.0f,  0.0f,  0.0f,// 62
		-1,  1, -1, 0, 1,			-1.0f,  0.0f,  0.0f,// 63
		0.7, -1, -1, 0, 0,			-1.0f,  0.0f,  0.0f,// 64
		0.7, -1, -0.7, 0.15, 0,		-1.0f,  0.0f,  0.0f,// 65
		0.7,  0.7, -0.7, 0.15, 1,	-1.0f,  0.0f,  0.0f,// 66
		0.7,  0.7, -1, 0, 1,		-1.0f,  0.0f,  0.0f,// 67
		0.7, -1, 0.7, 0.85, 0,		-1.0f,  0.0f,  0.0f,// 68
		0.7, -1, 1, 1, 0,			-1.0f,  0.0f,  0.0f,// 69
		0.7,  0.7, 1, 1, 1,			-1.0f,  0.0f,  0.0f,// 70
		0.7,  0.7, 0.7, 0.85, 1,	-1.0f,  0.0f,  0.0f,// 71

		// top
		-1, 1, 1, 0, 0,				0.0f,  1.0f,  0.0f,// 72
		1, 1, 1, 1, 0,				0.0f,  1.0f,  0.0f,// 73
		1,  1, -1, 1, 1,			0.0f,  1.0f,  0.0f,// 74
		-1,  1, -1, 0, 1,			0.0f,  1.0f,  0.0f,// 75

		// bottom
		-1, 0.7, 1, 0, 0,			0.0f,  -1.0f,  0.0f,// 76
		1, 0.7, 1, 1, 0,			0.0f,  -1.0f,  0.0f,// 77
		1,  0.7, -1, 1, 1,			0.0f,  -1.0f,  0.0f,// 78
		-1,  0.7, -1, 0, 1,			0.0f,  -1.0f,  0.0f// 79
	};

	unsigned int indices[] = {
		0,  1,  2,  0,  2,  3,			// front 1
		4,  5,  6,  4,  6,  7,			// front 2
		3,  6,  8,  3,  8,  9,			// front 3
		10,  11,  12,  10,  12,  13,	// front 4
		14,  15,  16,  14,  16,  17,	// front 5
		18,  19,  20,  18,  20,  21,	// right 1
		22,  23,  24,  22,  24,  25,	// right 2
		21,  24,  26,  21,  26,  27,	// right 3
		28,  29,  30,  28,  30,  31,	// right 4
		32,  33,  34,  32,  34,  35,	// right 5
		36,  37,  38,  36,  38,  39,	// back 1
		40,  41,  42,  40,  42,  43,	// back 2
		39,  42,  44,  39,  44,  45,	// back 3
		46,  47,  48,  46,  48,  49,	// back 4
		50,  51,  52,  50,  52,  53,	// back 5
		54,  55,  56,  54,  56,  57,	// left 1
		58,  59,  60,  58,  60,  61,	// left 2
		57,  60,  62,  57,  62,  63,	// left 3
		64,  65,  66,  64,  66,  67,	// left 4
		68,  69,  70,  68,  70,  71,	// left 5
		72,  73,  74,  72,  74,  75,	// top
		78,  77,  76,  79,  78,  76		// bottom
	};

	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glGenBuffers(1, &cubeEBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(cubeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// define position pointer layout 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(0 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);

	// define texcoord pointer layout 1
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// define normal pointer layout 2
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void Demo::DrawTexturedCube(GLuint shader)
{
	/*glUseProgram(this->shadowmapShader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cube_texture);
	glUniform1i(glGetUniformLocation(this->shadowmapShader, "material.diffuse"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, stexture);
	glUniform1i(glGetUniformLocation(this->shadowmapShader, "material.specular"), 1);

	GLint shininessMatLoc = glGetUniformLocation(this->shadowmapShader, "material.shininess");
	glUniform1f(shininessMatLoc, 0.4f);*/

	UseShader(shader);
	glBindVertexArray(cubeVAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

	glm::mat4 model;
	model = glm::translate(model, glm::vec3(0, 0.5, 0));
	model = glm::rotate(model, angle, glm::vec3(0, 1, 0));
	model = glm::scale(model, glm::vec3(2, 2, 2));

	//GLint modelLoc = glGetUniformLocation(this->shadowmapShader, "model");
	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, 6 * 22, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::BuildTexturedPlane()
{
	// Load and create a texture 
	glGenTextures(1, &plane_texture);
	glBindTexture(GL_TEXTURE_2D, plane_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height;
	unsigned char* image = SOIL_load_image("wood.png", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	/*glGenTextures(1, &stexture2);
	glBindTexture(GL_TEXTURE_2D, stexture2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	image = SOIL_load_image("marble_specularmap.png", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);*/


	// Build geometry
	GLfloat vertices[] = {
		// format position, tex coords
		// bottom
		-50.0, -0.5, -50.0,  0,  0, 0.0f,  1.0f,  0.0f,
		50.0, -0.5, -50.0, 50,  0, 0.0f,  1.0f,  0.0f,
		50.0, -0.5,  50.0, 50, 50, 0.0f,  1.0f,  0.0f,
		-50.0, -0.5,  50.0,  0, 50, 0.0f,  1.0f,  0.0f,
	};

	GLuint indices[] = { 0,  2,  1,  0,  3,  2 };

	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glGenBuffers(1, &planeEBO);

	glBindVertexArray(planeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(0);
	// TexCoord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	// Normal attribute
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0); // Unbind VAO
}



void Demo::DrawTexturedPlane(GLuint shader)
{
	//UseShader(this->shadowmapShader);

	/*glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, plane_texture);
	glUniform1i(glGetUniformLocation(this->shadowmapShader, "material.diffuse"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, stexture2);
	glUniform1i(glGetUniformLocation(this->shadowmapShader, "material.specular"), 3);

	GLint shininessMatLoc = glGetUniformLocation(this->shadowmapShader, "material.shininess");
	glUniform1f(shininessMatLoc, 0.4f);*/

	UseShader(shader);
	glBindVertexArray(planeVAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

	glm::mat4 model;
	//GLint modelLoc = glGetUniformLocation(this->shadowmapShader, "model");
	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::BuildDepthMap() {
	// configure depth map FBO
	// -----------------------
	glGenFramebuffers(1, &depthMapFBO);
	// create depth texture
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, this->SHADOW_WIDTH, this->SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Demo::BuildShaders()
{
	// build and compile our shader program
	// ------------------------------------
	shadowmapShader = BuildShader("shadowMapping.vert", "shadowMapping.frag", nullptr);
	//shadowmapShader = BuildShader("vertexShader.vert", "fragmentShader.frag", nullptr);
	depthmapShader = BuildShader("depthMap.vert", "depthMap.frag", nullptr);
}

void Demo::InitCamera()
{
	posCamX = 0.0f;
	posCamY = 4.0f;
	posCamZ = 8.0f;
	viewCamX = 0.0f;
	viewCamY = 1.0f;
	viewCamZ = 0.0f;
	upCamX = 0.0f;
	upCamY = 1.0f;
	upCamZ = 0.0f;
	CAMERA_SPEED = 0.001f;
	fovy = 45.0f;
	glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}


void Demo::MoveCamera(float speed)
{
	float x = viewCamX - posCamX;
	float z = viewCamZ - posCamZ;
	// forward positive cameraspeed and backward negative -cameraspeed.
	posCamX = posCamX + x * speed;
	posCamZ = posCamZ + z * speed;
	viewCamX = viewCamX + x * speed;
	viewCamZ = viewCamZ + z * speed;
}

void Demo::StrafeCamera(float speed)
{
	float x = viewCamX - posCamX;
	float z = viewCamZ - posCamZ;
	float orthoX = -z;
	float orthoZ = x;

	// left positive cameraspeed and right negative -cameraspeed.
	posCamX = posCamX + orthoX * speed;
	posCamZ = posCamZ + orthoZ * speed;
	viewCamX = viewCamX + orthoX * speed;
	viewCamZ = viewCamZ + orthoZ * speed;
}

void Demo::RotateCamera(float speed)
{
	float x = viewCamX - posCamX;
	float z = viewCamZ - posCamZ;
	viewCamZ = (float)(posCamZ + glm::sin(speed) * x + glm::cos(speed) * z);
	viewCamX = (float)(posCamX + glm::cos(speed) * x - glm::sin(speed) * z);
}


int main(int argc, char** argv) {
	RenderEngine& app = Demo();
	app.Start("Tugas 4 : 185150201111063", 800, 600, false, true);
}