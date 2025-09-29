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
#include <format>

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
	const int ANIM_PLAYER_SLIDING = 2;
	std::vector<Animation> playerAnims;
	const int ANIM_BULLET_MOVING = 0;
	const int ANIM_BULLET_HIT = 1;
	std::vector<Animation> bulletAnims;

	std::vector<SDL_Texture*> textures;
	SDL_Texture* texIdle, * textRun, * texSlide, * texBrick, * texGrass, * texGround, * texPanel, * texBg1, * texBg2, * texBg3, * texBg4,
		* texBullet, * texBulletHit;

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
		playerAnims[ANIM_PLAYER_SLIDING] = Animation(1, 1.0f);
		bulletAnims.resize(2);
		bulletAnims[ANIM_BULLET_MOVING] = Animation(4, 0.05f);
		bulletAnims[ANIM_BULLET_HIT] = Animation(4, 0.15f);

		texIdle = loadTexture(state.renderer, "Shooter/data/idle.png");
		textRun = loadTexture(state.renderer, "Shooter/data/run.png");
		texSlide = loadTexture(state.renderer, "Shooter/data/slide.png");
		texBrick = loadTexture(state.renderer, "Shooter/data/tiles/brick.png");
		texGrass = loadTexture(state.renderer, "Shooter/data/tiles/grass.png");
		texGround = loadTexture(state.renderer, "Shooter/data/tiles/ground.png");
		texPanel = loadTexture(state.renderer, "Shooter/data/tiles/panel.png");
		texBg1 = loadTexture(state.renderer, "Shooter/data/bg/bg_layer1.png");
		texBg2 = loadTexture(state.renderer, "Shooter/data/bg/bg_layer2.png");
		texBg3 = loadTexture(state.renderer, "Shooter/data/bg/bg_layer3.png");
		texBg4 = loadTexture(state.renderer, "Shooter/data/bg/bg_layer4.png");
		texBullet = loadTexture(state.renderer, "Shooter/data/bullet.png");
		texBulletHit = loadTexture(state.renderer, "Shooter/data/bullet_hit.png");
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
	std::vector<GameObject> backgroundTiles;
	std::vector<GameObject> foregroundTiles;
	std::vector<GameObject> bullets;

	int playerIndex;
	SDL_FRect mapViewport;
	float bg2Scroll, bg3Scroll, bg4Scroll;

	GameState(const SDLState &state) {
		playerIndex = -1;
		mapViewport = SDL_FRect{
			.x = 0,
			.y = 0,
			.w = static_cast<float>(state.logW),
			.h = static_cast<float>(state.logH)
		};
		bg2Scroll = bg3Scroll = bg4Scroll = 0;
	};

	GameObject &player() { return layers[LAYER_IDX_CHARACTERS][playerIndex]; }
};

bool initialize(SDLState &state);
void cleanup(SDLState &win);
void drawObject(const SDLState& state, GameState& gameState, GameObject& obj, float width, float height, float deltaTime);
void update(const SDLState& state, GameState& gameStaet, Resources& resources, GameObject& obj, float deltaTime);
void createTiles(const SDLState& state, GameState& gameState, const Resources& resources);
void checkCollissions(const SDLState& state, GameState& gameState, Resources& resources,
	GameObject& a, GameObject& b, float deltaTime);
void handleKeyInput(const SDLState& state, GameState& gs, GameObject& obj, SDL_Scancode key, bool keyDown);
void drawParalaxBackground(SDL_Renderer* renderer, SDL_Texture* texture, float xVelocity, float& scrollPos, float scrollFactor,
	float deltaTime);

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
	GameState gameState(state);
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
				case SDL_EVENT_KEY_DOWN: 
					handleKeyInput(state, gameState, gameState.player(), event.key.scancode, true);
					break;
				
				case SDL_EVENT_KEY_UP: 
					handleKeyInput(state, gameState, gameState.player(), event.key.scancode, false);
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

		//update bullets
		for (GameObject& bullet : gameState.bullets) {

			update(state, gameState, resources, bullet, deltaTime);
			//update the animation
			if (bullet.currentAnimation != -1) {

				bullet.animations[bullet.currentAnimation].step(deltaTime);
			}
		}

		// calculate viewport position
		gameState.mapViewport.x = (gameState.player().position.x + TILE_SIZE / 2) - gameState.mapViewport.w / 2;

		//drawing commands
		SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
		SDL_RenderClear(state.renderer);

		SDL_RenderTexture(state.renderer, resources.texBg1, nullptr, nullptr);
		drawParalaxBackground(state.renderer, resources.texBg4, gameState.player().velocity.x, gameState.bg4Scroll, 0.075f, deltaTime);
		drawParalaxBackground(state.renderer, resources.texBg3, gameState.player().velocity.x, gameState.bg3Scroll, 0.150f, deltaTime);
		drawParalaxBackground(state.renderer, resources.texBg2, gameState.player().velocity.x, gameState.bg2Scroll, 0.3f, deltaTime);

		// draw background tiles
		for (GameObject& obj : gameState.backgroundTiles) {
			SDL_FRect dst{
				.x = obj.position.x - gameState.mapViewport.x,
				.y = obj.position.y,
				.w = static_cast<float>(obj.texture->w),
				.h = static_cast<float>(obj.texture->h)
			};
			SDL_RenderTexture(state.renderer, obj.texture, nullptr, &dst);
		}

		//draw all objects
		for (auto& layer : gameState.layers) {

			for (GameObject& obj : layer) {
				drawObject(state, gameState, obj, TILE_SIZE, TILE_SIZE, deltaTime);
			}
		}

		//draw bullets
		for (GameObject &bullet : gameState.bullets) {
			drawObject(state, gameState, bullet, bullet.collider.w, bullet.collider.h, deltaTime);
		}

		// draw foreground tiles
		for (GameObject& obj : gameState.foregroundTiles) {
			SDL_FRect dst{
				.x = obj.position.x - gameState.mapViewport.x,
				.y = obj.position.y,
				.w = static_cast<float>(obj.texture->w),
				.h = static_cast<float>(obj.texture->h)
			};
			SDL_RenderTexture(state.renderer, obj.texture, nullptr, &dst);
		}

		SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
		SDL_RenderDebugText(state.renderer, 5, 5, 
			std::format("State: {}", static_cast<int>(gameState.player().data.player.state)).c_str());

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

	SDL_SetRenderVSync(state.renderer, 1);

	//configure presentation
	SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
	return initSuccess;
}

void cleanup(SDLState &state ) {
	SDL_DestroyRenderer(state.renderer);
	SDL_DestroyWindow(state.window);
	SDL_Quit();
}

void drawObject(const SDLState& state, GameState& gameState, GameObject& obj, float width, float height, float deltaTime) {

	float sourceX = obj.currentAnimation != -1 ?
		obj.animations[obj.currentAnimation].currentFrame() * width : 0.0f;

	SDL_FRect src{
		.x = sourceX,
		.y = 0,
		.w = width,
		.h = height
	};
	SDL_FRect dst{
		.x = obj.position.x - gameState.mapViewport.x ,
		.y = obj.position.y,
		.w = width,
		.h = height
	};

	SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

	SDL_RenderTextureRotated(state.renderer,obj.texture, &src, &dst, 0, nullptr, flipMode);

}

void update(const SDLState& state, GameState& gameStaet, Resources& resources, GameObject& obj, float deltaTime) {
	
	if (obj.dynamic) {
		//apply some gravity
		obj.velocity += glm::vec2(0, 500) * deltaTime;
	}

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

		Timer& weaponTimer = obj.data.player.weaponTimer;
		weaponTimer.step(deltaTime);

		switch (obj.data.player.state) {

			case PlayerState::idle: {
				// switching to running state
				if (currentDirection) {
					obj.data.player.state = PlayerState::running;
				}
				else {
					if (obj.velocity.x) {
						const float factor = obj.velocity.x > 0 ? -1.5f : 1.5f;
						float amount = factor * obj.acceleration.x * deltaTime;
						if (std::abs(obj.velocity.x) < std::abs(amount)) {
							obj.velocity.x = 0;
						}
						else {
							obj.velocity.x += amount;
						}
					}
				}
				if (state.keys[SDL_SCANCODE_J]) {

					if (weaponTimer.isTimeout()) {
						weaponTimer.reset();

						//spawn some bullets
						GameObject bullet;
						bullet.type = ObjectType::bullet;
						bullet.direction = gameStaet.player().direction;
						bullet.texture = resources.texBullet;
						bullet.currentAnimation = resources.ANIM_BULLET_MOVING;
						bullet.collider = SDL_FRect{
							.x = 0,
							.y = 0,
							.w = static_cast<float>(resources.texBullet->h),
							.h = static_cast<float>(resources.texBullet->h),
						};
						bullet.velocity = glm::vec2(
							obj.velocity.x + 600.0f * obj.direction,
							0
						);
						bullet.animations = resources.bulletAnims;

						const float left = 4;
						const float right = 24;
						const float t = (obj.direction + 1) / 2.0f;
						const float xOffset = left + right * t;

						bullet.position = glm::vec2(obj.position.x + xOffset, obj.position.y + TILE_SIZE / 2 + 1);
						gameStaet.bullets.push_back(bullet);
					}

				}
				obj.texture = resources.texIdle;
				obj.currentAnimation = resources.ANIM_PLAYER_IDLE;
				break;
			}
			
			case PlayerState::running: {
				if (!currentDirection) {
					obj.data.player.state = PlayerState::idle;

				}

				// moving in opposite direction of velocity, sliding !
				if (obj.velocity.x * obj.direction < 0 && obj.grounded) {
					obj.texture = resources.texSlide;
					obj.currentAnimation = resources.ANIM_PLAYER_SLIDING;
				}
				else {
					obj.texture = resources.textRun;
					obj.currentAnimation = resources.ANIM_PLAYER_RUN;
				}
				
				break;
			}

			case PlayerState::jumping: {
				obj.texture = resources.textRun;
				obj.currentAnimation = resources.ANIM_PLAYER_RUN;
			}
		}

		//add acceleration to velocity
		obj.velocity += currentDirection * obj.acceleration * deltaTime;
		if (std::abs(obj.velocity.x) > obj.maxSpeedX) {
			obj.velocity.x = currentDirection * obj.maxSpeedX;
		}
	}
	//add velocity to position
	obj.position += obj.velocity * deltaTime;

	//handle collision detection

	bool foundGround = false;
	for (auto& layer : gameStaet.layers) {
		for (GameObject& objB : layer) {
			if (&obj != &objB) {
				checkCollissions(state, gameStaet, resources, obj, objB, deltaTime);

				//grounded sensor
				SDL_FRect sensor{
					.x = obj.position.x + obj.collider.x,
					.y = obj.position.y + obj.collider.y + obj.collider.h,
					.w = obj.collider.w,
					.h = 1
				};

				SDL_FRect rectB{
					.x = objB.position.x + objB.collider.x,
					.y = objB.position.y + objB.collider.y,
					.w = objB.collider.w,
					.h = objB.collider.h
				};

				if (SDL_HasRectIntersectionFloat(&sensor, &rectB)) {
					foundGround = true;
				}
			}
		}
	}

	if (obj.grounded != foundGround) {
		// switching grounded state
		obj.grounded = foundGround;
		if (foundGround && obj.type == ObjectType::player) {
			obj.data.player.state = PlayerState::running;
		}
	}
}

void collisionResponse(const SDLState& state, GameState& gameState, Resources &resources,
	const SDL_FRect &rectA, const SDL_FRect &rectB, const SDL_FRect &rectC, GameObject& objA, GameObject& objB, float deltaTime) {

	if (objA.type == ObjectType::player) {

		switch (objB.type) {
			case ObjectType::level: {
				if (rectC.w < rectC.h) {
					//horizonal collision
					if (objA.velocity.x > 0) {
						objA.position.x -= rectC.w;
					}
					else if (objA.velocity.x < 0) { //going left
						objA.position.x += rectC.w;
					}
					objA.velocity.x = 0;
				}
				else {
					//vertical collision
					if (objA.velocity.y > 0) {
						objA.position.y -= rectC.h;
					}
					else if (objA.velocity.y < 0) {
						objA.position.y += rectC.h;
					}
					objA.velocity.y = 0;
				}
				break;
			}
		}
	}
}

void checkCollissions(const SDLState& state, GameState& gameState, Resources &resources, 
	GameObject &a, GameObject &b, float deltaTime) {

	SDL_FRect rectA{
		.x = a.position.x + a.collider.x,
		.y = a.position.y + a.collider.y,
		.w = a.collider.w,
		.h = a.collider.h
	};

	SDL_FRect rectB{
	.x = b.position.x + b.collider.x,
	.y = b.position.y + b.collider.y,
	.w = b.collider.w,
	.h = b.collider.h
	};
	SDL_FRect rectC{ 0 };

	if (SDL_GetRectIntersectionFloat(&rectA, &rectB, &rectC)) {
		collisionResponse(state, gameState, resources, rectA, rectB, rectC, a, b, deltaTime);
	}
}

void createTiles(const SDLState& state, GameState& gameState, const Resources& resources) {

	short map[MAP_ROWS][MAP_COLS] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	short foreground[MAP_ROWS][MAP_COLS] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		5, 0, 0, 5, 5, 5, 5, 5, 5, 0, 0, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	short background[MAP_ROWS][MAP_COLS] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		5, 0, 0, 5, 5, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	const auto loadMap = [&state, &gameState, &resources](short layer[MAP_ROWS][MAP_COLS]) {

		const auto createObject = [&state](int r, int c, SDL_Texture* tex, ObjectType type) {
			GameObject o;
			o.type = type;
			o.position = glm::vec2(c * TILE_SIZE, state.logH - (MAP_ROWS - r) * TILE_SIZE);
			o.texture = tex;
			o.collider = { .x = 0, .y = 0 , .w = TILE_SIZE, .h = TILE_SIZE };
			return o;
			};

		for (int r = 0; r < MAP_ROWS; r++) {
			for (int c = 0; c < MAP_COLS; c++) {
				switch (layer[r][c]) {

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
					player.dynamic = true;
					player.collider = {
						.x = 11, .y = 6, .w = 10, .h = 26
					};
					gameState.layers[LAYER_IDX_CHARACTERS].push_back(player);
					gameState.playerIndex = gameState.layers[LAYER_IDX_CHARACTERS].size() - 1;
					break;
				}

				case 5: { //grass
					GameObject o = createObject(r, c, resources.texGrass, ObjectType::level);
					gameState.foregroundTiles.push_back(o);
					break;
				}

				case 6: {
					GameObject o = createObject(r, c, resources.texBrick, ObjectType::level);
					gameState.backgroundTiles.push_back(o);
					break;
				}
				}
			}
		}
	};

	loadMap(map);
	loadMap(background);
	loadMap(foreground);
	assert(gameState.playerIndex != -1);
}

void handleKeyInput(const SDLState &state, GameState &gs, GameObject &obj, SDL_Scancode key, bool keyDown) {
	
	const float JUMP_FORCE = -200.0f;

	if (obj.type == ObjectType::player) {

		switch (obj.data.player.state) {

			case PlayerState::idle: 
				if (key == SDL_SCANCODE_K && keyDown) {
					obj.data.player.state = PlayerState::jumping;
					obj.velocity.y += JUMP_FORCE;
				}
				break;
			

			case PlayerState::running: 
				if (key == SDL_SCANCODE_K && keyDown) {
					obj.data.player.state = PlayerState::jumping;
					obj.velocity.y += JUMP_FORCE;
				}
				break;
			
		}
	}
}

void drawParalaxBackground(SDL_Renderer* renderer, SDL_Texture* texture, float xVelocity, float& scrollPos, float scrollFactor,
	float deltaTime) {
	scrollPos -= xVelocity * scrollFactor * deltaTime;

	if (scrollPos <= -texture->w) {
		scrollPos = 0;
	}

	SDL_FRect dst{
		.x = scrollPos, .y = 30,
		.w = texture->w * 2.0f,
		.h = static_cast<float>(texture->h)
	};

	SDL_RenderTextureTiled(renderer, texture, nullptr, 1, &dst);
}