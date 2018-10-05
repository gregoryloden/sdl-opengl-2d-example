#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <gl\GL.h>
#include <thread>

int refreshRate = 60;
int ring1X = 175;
int ring1Y = 145;
int ring2X = 130;
int ring2Y = 130;
int dotX = 180;
int dotY = 180;
SDL_Window* window = nullptr;
SDL_GLContext glContext = nullptr;
int updatesPerSecond = 48;
int initialScreenWidth = 400;
int initialScreenHeight = 400;
bool maintainWindowResolutionOnResize = true;

class Sprite {
public:
	GLuint textureId;
	int width;
	int height;
	Sprite(const char* imagePath)
	: textureId(0)
	, width(1)
	, height(1) {
		glGenTextures(1, &textureId);

		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		SDL_Surface* surface = IMG_Load(imagePath);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
		width = surface->w;
		height = surface->w;
		SDL_FreeSurface(surface);
	}
	~Sprite() {}
	void render(int x, int y) {
		int maxX = x + width;
		int maxY = y + height;

		glBindTexture(GL_TEXTURE_2D, textureId);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2i(x, y);
		glTexCoord2f(1.0f, 0.0f);
		glVertex2i(maxX, y);
		glTexCoord2f(1.0f, 1.0f);
		glVertex2i(maxX, maxY);
		glTexCoord2f(0.0f, 1.0f);
		glVertex2i(x, maxY);
		glEnd();
	}
};

//forward declarations instead of a header file
void renderLoop();
void update();

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char *argv[]) {
	//Initialize SDL
	int initResult = SDL_Init(SDL_INIT_EVERYTHING);
	if (initResult < 0)
		return initResult;
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

	//Initialize a window
	window = SDL_CreateWindow(
		"Simple SDL OpenGL 2D Example",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		initialScreenWidth,
		initialScreenHeight,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

	//Get the screen refresh rate
	SDL_DisplayMode displayMode;
	SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &displayMode);
	if (displayMode.refresh_rate > 0)
		refreshRate = displayMode.refresh_rate;

	//Start rendering on a separate thread
	std::thread renderLoopThread (renderLoop);

	//Begin our update loop
	int updateDelay = -1;
	Uint32 startTime = 0;
	int updateNum = 0;
	while (true) {
		//If we missed an update or haven't begun the loop, reset the update number and time
		if (updateDelay <= 0) {
			updateNum = 0;
			startTime = SDL_GetTicks();
		} else
			SDL_Delay((Uint32)updateDelay);
		update();
		updateNum++;
		updateDelay = 1000 * updateNum / updatesPerSecond - (int)(SDL_GetTicks() - startTime);
	}
}
void renderLoop() {
	//In the event v-sync doesn't work we still want to rate-limit based on the screen's refresh rate
	int minMsPerFrame = 1000 / refreshRate;

	//Set up OpenGL for our window
	glContext = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glOrtho(0, (GLdouble)initialScreenWidth, (GLdouble)initialScreenHeight, 0, -1, 1);

	//Initialize textures
	Sprite dot ("dot.png");
	Sprite ring ("ring.png");

	//Begin our render loop
	int lastWindowWidth = 0;
	int lastWindowHeight = 0;
	while (true) {
		Uint32 preRenderTicks = SDL_GetTicks();

		//Scale the contents of the screen to match window resizes, if specified
		if (maintainWindowResolutionOnResize) {
			int windowWidth = 0;
			int windowHeight = 0;
			SDL_GetWindowSize(window, &windowWidth, &windowHeight);
			if (windowWidth != lastWindowWidth || windowHeight != lastWindowHeight) {
				glViewport(0, 0, windowWidth, windowHeight);
				lastWindowWidth = windowWidth;
				lastWindowHeight = windowHeight;
			}
		}

		//Clear the screen and render
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_BLEND);
		dot.render(dotX, dotY);
		ring.render(ring2X, ring2Y);
		glEnable(GL_BLEND);
		ring.render(ring1X, ring1Y);
		glFlush();
		SDL_GL_SwapWindow(window);

		//sleep if we don't expect to render for at least 2 more milliseconds
		int remainingDelay = minMsPerFrame - (int)(SDL_GetTicks() - preRenderTicks);
		if (remainingDelay >= 2)
			SDL_Delay(remainingDelay);
	}
}
void update() {
	//Handle any queued updates
	SDL_Event gameEvent;
	while (SDL_PollEvent(&gameEvent) != 0) {
		switch (gameEvent.type) {
			case SDL_QUIT:
				SDL_GL_DeleteContext(glContext);
				SDL_DestroyWindow(window);
				SDL_Quit();
				std::exit(0);
			default:
				break;
		}
	}
	const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
	//ESDF to move the opaque ring
	if (keyboardState[SDL_SCANCODE_E] != 0) ring2Y -= 1;
	if (keyboardState[SDL_SCANCODE_S] != 0) ring2X -= 1;
	if (keyboardState[SDL_SCANCODE_D] != 0) ring2Y += 1;
	if (keyboardState[SDL_SCANCODE_F] != 0) ring2X += 1;
	//IJKL to move the transparent ring
	if (keyboardState[SDL_SCANCODE_I] != 0) ring1Y -= 1;
	if (keyboardState[SDL_SCANCODE_J] != 0) ring1X -= 1;
	if (keyboardState[SDL_SCANCODE_K] != 0) ring1Y += 1;
	if (keyboardState[SDL_SCANCODE_L] != 0) ring1X += 1;
	//up/down/left/right to move the opaque dot
	if (keyboardState[SDL_SCANCODE_UP] != 0) dotY -= 1;
	if (keyboardState[SDL_SCANCODE_LEFT] != 0) dotX -= 1;
	if (keyboardState[SDL_SCANCODE_DOWN] != 0) dotY += 1;
	if (keyboardState[SDL_SCANCODE_RIGHT] != 0) dotX += 1;
}