// Shooter.cpp : Defines the entry point for the application.
//

#include "Shooter.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include "gameobject.h"
#include <vector>
#include <glm/glm.hpp>
#include <array>

using namespace std;

struct SDLState {
	SDL_Window* window;
	SDL_Renderer* renderer;
	int width, height, logW, logH;
	const bool* keys;

	SDLState() : keys(SDL_GetKeyboardState(nullptr)) {

	}
};

struct Resources {
	const int ANIM_PLAYER_IDLE = 0;
	const int ANIM_PLAYER_RUN = 1;
	std::vector<Animation> playerAnims;

	std::vector<SDL_Texture*> textures;
	SDL_Texture *texIdle, *textRun, *texBrick, *texGrass, *texGround, *texPanel;

	SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& filepath) {

		SDL_Texture* tex = IMG_LoadTexture(renderer, filepath.c_str());
		SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
		textures.push_back(tex);
		return tex;
	}

	void load(SDLState& state) {
		playerAnims.resize(5);
		playerAnims[ANIM_PLAYER_IDLE] = Animation(8, 1.6f);
		playerAnims[ANIM_PLAYER_RUN] = Animation(4, 0.5f);

		texIdle = loadTexture(state.renderer, "Shooter/data/idle.png");
		textRun = loadTexture(state.renderer, "Shooter/data/run.png");
		texBrick = loadTexture(state.renderer, "Shooter/data/tiles/brick.png");
		texGrass = loadTexture(state.renderer, "Shooter/data/tiles/grass.png");
		texGround = loadTexture(state.renderer, "Shooter/data/tiles/ground.png");
		texPanel = loadTexture(state.renderer, "Shooter/data/tiles/panel.png");
	}

	void unload() {
		for (SDL_Texture* tex : textures) {
			SDL_DestroyTexture(tex);
		}
	}
};

const size_t LAYER_IDX_LEVEL = 0;
const size_t LAYER_IDX_CHARACTERS = 1;
const int MAP_ROWS = 5;
const int MAP_COLS = 50;
const int TILE_SIZE = 32;

struct GameState {
	std::array<std::vector<GameObject>, 2> layers;
	int playerIndex;

	GameState() {
		playerIndex = 0;
	}
};

bool initialize(SDLState &state);
void cleanup(SDLState &win);
void drawObject(const SDLState& state, GameState& gameState, GameObject& obj, float deltaTime);
void update(const SDLState& state, GameState& gameStaet, Resources& resources, GameObject& obj, float deltaTime);
void createTiles(const SDLState& state, GameState& gameState, const Resources& resources);

int main(int argc, char *argv[])
{
	SDLState state;
	state.width = 1600;
	state.height = 900;
	state.logW = 640;
	state.logH = 320;

	if (!initialize(state)) {
		return 1;
	}
	
	//load game assets
	Resources resources;
	resources.load(state);

	//setup game data
	GameState gameState;
	createTiles(state, gameState, resources);

	uint64_t prevTime = SDL_GetTicks();

	//main loop
	bool running = true;
	while (running) {
		uint64_t nowTime = SDL_GetTicks();
		float deltaTime = (nowTime - prevTime) / 1000.0f;
		SDL_Event event{ 0 };
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_EVENT_QUIT:
					running = false;
					break;
				case SDL_EVENT_WINDOW_RESIZED:
					state.width = event.window.data1;
					state.height = event.window.data2;
					break;
			}
		}

		//update all objects
		for (auto& layer : gameState.layers) {

			for (GameObject& obj : layer) {

				update(state, gameState, resources, obj, deltaTime);
				//update the animation
				if (obj.currentAnimation != -1) {
					obj.animations[obj.currentAnimation].step(deltaTime);
				}
			}
		}

		//drawing commands
		SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
		SDL_RenderClear(state.renderer);

		//draw all objects
		for (auto& layer : gameState.layers) {

			for (GameObject& obj : layer) {
				drawObject(state, gameState, obj, deltaTime);
			}
		}

		// swap buffers and present
		SDL_RenderPresent(state.renderer);
		prevTime = nowTime;
	}

	resources.unload();
	SDL_Quit();
	return 0;
}

bool initialize(SDLState& state) {

	bool initSuccess = true;
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Failed to initialize SDL", nullptr);
		initSuccess = false;
	}

	state.window = SDL_CreateWindow("SDL3 Demo", state.width, state.height, SDL_WINDOW_RESIZABLE);
	if (!state.window) {

		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Failed to create window", nullptr);
		cleanup(state);
		initSuccess = false;
	}

	//create the renderer
	state.renderer = SDL_CreateRenderer(state.window, nullptr);
	if (!state.renderer) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating renderer", state.window);
		cleanup(state);
		initSuccess = false;
	}

	//configure presentation
	SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
	return initSuccess;
}

void cleanup(SDLState &state ) {
	SDL_DestroyRenderer(state.renderer);
	SDL_DestroyWindow(state.window);
	SDL_Quit();
}

void drawObject(const SDLState& state, GameState& gameState, GameObject& obj, float deltaTime) {
	const float spritSize = 32;
	float sourceX = obj.currentAnimation != -1 ?
		obj.animations[obj.currentAnimation].currentFrame() * spritSize : 0.0f;
	SDL_FRect src{
		.x = sourceX,
		.y = 0,
		.w = spritSize,
		.h = spritSize
	};
	SDL_FRect dst{
		.x = obj.position.x,
		.y = obj.position.y,
		.w = 32,
		.h = 32
	};

	SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

	SDL_RenderTextureRotated(state.renderer,obj.texture, &src, &dst, 0, nullptr, flipMode);

}

void update(const SDLState& state, GameState& gameStaet, Resources& resources, GameObject& obj, float deltaTime) {
	if (obj.type == ObjectType::player) {
		float currentDirection = 0;

		if (state.keys[SDL_SCANCODE_A]) {
			currentDirection += -1;
		}

		if (state.keys[SDL_SCANCODE_D]) {
			currentDirection += 1;
		}

		if (currentDirection) {
			obj.direction = currentDirection;
		}

		switch (obj.data.player.state) {
			case PlayerState::idle: {
				if (currentDirection) {
					obj.data.player.state = PlayerState::running;
					obj.texture = resources.textRun;
					obj.currentAnimation = resources.ANIM_PLAYER_RUN;
				}
				else {
					if (obj.velocity.x) {
						const float factor = obj.velocity.x > 0 ? -1.5F : 1.5f;
						float amount = factor * obj.acceleration.x * deltaTime;
						if (std::abs(obj.velocity.x) < std::abs(amount)) {
							obj.velocity.x = 0;
						}
						else {
							obj.velocity.x += amount;
						}
					}
				}
				break;
			}
			
			case PlayerState::running: {
				if (!currentDirection) {
					obj.data.player.state = PlayerState::idle;
					obj.texture = resources.texIdle;
					obj.currentAnimation = resources.ANIM_PLAYER_IDLE;
				}
				break;
			}
		}

		//add acceleration to velocity
		obj.velocity += currentDirection * obj.acceleration * deltaTime;
		if (std::abs(obj.velocity.x) > obj.maxSpeedX) {
			obj.velocity.x = currentDirection * obj.maxSpeedX;
		}

		//add velocity to position
		obj.position += obj.velocity * deltaTime;
	}
}

void createTiles(const SDLState& state, GameState& gameState, const Resources& resources) {

	short map[MAP_ROWS][MAP_COLS] = {
		4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	const auto createObject = [&state](int r, int c, SDL_Texture* tex, ObjectType type) {
		GameObject o;
		o.type = type;
		o.position = glm::vec2(c * TILE_SIZE, state.logH - (MAP_ROWS - r) * TILE_SIZE);
		o.texture = tex;
		return o;
	};

	for (int r = 0; r < MAP_ROWS; r++) {
		for (int c = 0; c < MAP_COLS; c++) {
			switch (map[r][c]) {

			case 1: { // ground
				GameObject o = createObject(r, c, resources.texGround, ObjectType::level);
				gameState.layers[LAYER_IDX_LEVEL].push_back(o);
				break;
			}

			case 2: { // panel
				GameObject o = createObject(r, c, resources.texPanel, ObjectType::level);
				gameState.layers[LAYER_IDX_LEVEL].push_back(o);
				break;
			}


			case 4: {

				// create our player
				GameObject player = createObject(r, c, resources.texIdle, ObjectType::player);
				player.data.player = PlayerData();
				player.animations = resources.playerAnims;
				player.currentAnimation = resources.ANIM_PLAYER_IDLE;
				player.acceleration = glm::vec2(300, 0);
				player.maxSpeedX = 100;
				gameState.layers[LAYER_IDX_CHARACTERS].push_back(player);
				break;
			}

			}
		}
	}
}