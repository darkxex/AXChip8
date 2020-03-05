#ifdef __SWITCH__
#include <unistd.h>
#include <switch.h>
#include <dirent.h>
#endif 
#ifndef __SWITCH__
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <array>
#include <cstdint>
#include <iostream>
#include "chip8.h"
#include <string.h>
#include <stdio.h>
constexpr int WIDTH = 64;
constexpr int HEIGHT = 32;
constexpr int SCALE = 10;
constexpr int FPS = 60;
constexpr int TICKS_PER_FRAME = 1000 / FPS;
constexpr int INSTRUCTIONS_PER_STEP = 10;

constexpr std::array<SDL_Keycode, 16> keymap{
	SDLK_x, SDLK_1, SDLK_2, SDLK_3,   // 0 1 2 3
	SDLK_q, SDLK_w, SDLK_e, SDLK_a,   // 4 5 6 7
	SDLK_s, SDLK_d, SDLK_z, SDLK_c,   // 8 9 A B
	SDLK_4, SDLK_r, SDLK_f, SDLK_v };  // C D E F

void sdl_error() {
	std::cerr << "SDL has encountered an error: ";
	std::cerr << SDL_GetError() << "\n";
	SDL_Quit();
	exit(1);
}
//Texture wrapper class
class LTexture
{
public:
	//Initializes variables
	LTexture();

	//Deallocates memory
	~LTexture();

	//Loads image at specified path
	bool loadFromFile(std::string path);

	//Creates image from font string
	bool loadFromRenderedText(std::string textureText, SDL_Color textColor);

	//Deallocates texture
	void free();

	//Set color modulation
	void setColor(Uint8 red, Uint8 green, Uint8 blue);

	//Set blending
	void setBlendMode(SDL_BlendMode blending);

	//Set alpha modulation
	void setAlpha(Uint8 alpha);

	//Renders texture at given point
	void render(int x, int y, SDL_Rect* clip = NULL, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);

	//Gets image dimensions
	int getWidth();
	int getHeight();

private:
	//The actual hardware texture
	SDL_Texture* mTexture;

	//Image dimensions
	int mWidth;
	int mHeight;
};
void init_sdl(SDL_Window*& window, SDL_Texture*& texture, SDL_Renderer*& renderer) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		sdl_error();
	}
	
	
#ifdef __SWITCH__
	window = SDL_CreateWindow("chip8", 0, 0, 1280, 720, 0);
#else
	window = SDL_CreateWindow("chip8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN);
#endif // SWITCH
	if (window == nullptr) {
		sdl_error();
	}

	renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == nullptr) {
		sdl_error();
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
	if (texture == nullptr) {
		sdl_error();
	}
}

void init_audio(Mix_Chunk*& chunk) {
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		
		std::cerr << "SDL_mixer has encountered an error: ";
		std::cerr << Mix_GetError() << "\n";
		SDL_Quit();
		Mix_Quit();
		exit(1);
	}
	
#ifdef __SWITCH__
	
	chunk = Mix_LoadWAV("romfs:/beep.wav");

#else
	chunk = Mix_LoadWAV("C:\\respaldo2017\\C++\\Chip8\\Debug\\beep.wav");

#endif // SWITCH

	Mix_VolumeChunk(chunk, MIX_MAX_VOLUME / 8);
	if (chunk == nullptr) {
		std::cerr << "SDL_mixer has encountered an error: ";
		std::cerr << Mix_GetError() << "\n";
		SDL_Quit();
		Mix_Quit();
		exit(1);
	}
}
#ifdef __SWITCH__
int getInd(char* curFile, int curIndex) {
	DIR* dir;
	struct dirent* ent;

	if (curIndex < 0)
		curIndex = 0;

	dir = opendir("/roms/chip8");//Open current-working-directory.
	if (dir == NULL)
	{
		sprintf(curFile, "Failed to open dir!");
		return curIndex;
	}
	else
	{
		int i;
		for (i = 0; i <= curIndex; i++) {
			ent = readdir(dir);
		}
		if (ent)
			sprintf(curFile, "/roms/chip8/%s", ent->d_name);
		else
			curIndex--;
		closedir(dir);
	}

	return curIndex;
}
#endif
SDL_Window* window = nullptr;
SDL_Texture* texture = nullptr;
SDL_Renderer* renderer = nullptr;
Mix_Chunk* chunk = nullptr;
//Globally used font
TTF_Font *gFont = NULL;

//Rendered texture
LTexture gTextTexture;
LTexture::LTexture()
{
	//Initialize
	mTexture = NULL;
	mWidth = 0;
	mHeight = 0;
}

LTexture::~LTexture()
{
	//Deallocate
	free();
}

bool LTexture::loadFromRenderedText(std::string textureText, SDL_Color textColor)
{
	//Get rid of preexisting texture
	free();

	//Render text surface
	SDL_Surface* textSurface = TTF_RenderText_Solid(gFont, textureText.c_str(), textColor);
	if (textSurface == NULL)
	{
		printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
	}
	else
	{
		//Create texture from surface pixels
		mTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
		if (mTexture == NULL)
		{
			printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
		}
		else
		{
			//Get image dimensions
			mWidth = textSurface->w;
			mHeight = textSurface->h;
		}

		//Get rid of old surface
		SDL_FreeSurface(textSurface);
	}

	//Return success
	return mTexture != NULL;
}

void LTexture::free()
{
	//Free texture if it exists
	if (mTexture != NULL)
	{
		SDL_DestroyTexture(mTexture);
		mTexture = NULL;
		mWidth = 0;
		mHeight = 0;
	}
}

void LTexture::setColor(Uint8 red, Uint8 green, Uint8 blue)
{
	//Modulate texture rgb
	SDL_SetTextureColorMod(mTexture, red, green, blue);
}

void LTexture::setBlendMode(SDL_BlendMode blending)
{
	//Set blending function
	SDL_SetTextureBlendMode(mTexture, blending);
}

void LTexture::setAlpha(Uint8 alpha)
{
	//Modulate texture alpha
	SDL_SetTextureAlphaMod(mTexture, alpha);
}

void LTexture::render(int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip)
{
	//Set rendering space and render to screen
	SDL_Rect renderQuad = { x, y, mWidth, mHeight };

	//Set clip rendering dimensions
	if (clip != NULL)
	{
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}

	//Render to screen
	SDL_RenderCopyEx(renderer, mTexture, clip, &renderQuad, angle, center, flip);
}

int LTexture::getWidth()
{
	return mWidth;
}

int LTexture::getHeight()
{
	return mHeight;
}

int main(int argc, char* argv[]) {
#ifdef __SWITCH__
	romfsInit();
	socketInitializeDefault();
	nxlinkStdio();
	struct stat st = { 0 };

	if (stat("sdmc:/roms", &st) == -1) {
		mkdir("sdmc:/roms", 0777);
	}
	if (stat("sdmc:/roms/chip8", &st) == -1) {
		mkdir("sdmc:/roms/chip8", 0777);
	}
#endif 
	
	SDL_Event event;

	init_sdl(window, texture, renderer);
	init_audio(chunk);
	TTF_Init();
	Chip8 chip8;
	SDL_Color textColor = { 0, 0, 0 };
	bool quit2 = false;
#ifdef __SWITCH__
	gFont = TTF_OpenFont("romfs:/lazy.ttf", 28);
	int curIndex = 0;
	char curFile[255];
	curIndex = getInd(curFile, curIndex);
	
	bool quit = false;
	while (!quit && appletMainLoop())
	{

		

		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
		//hidKeysHeld returns information about which buttons have are held down in this frame
		u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);
		//hidKeysUp returns information about which buttons have been just released
		u64 kUp = hidKeysUp(CONTROLLER_P1_AUTO);


		

		if (kDown & KEY_DOWN || kDown & KEY_DDOWN) {
			
			
			curIndex++;
			curIndex = getInd(curFile, curIndex);
			printf("\x1b[18;10H%s", curFile);
}

		if (kDown & KEY_UP || kDown & KEY_DUP) {
		
		
			curIndex--;
			curIndex = getInd(curFile, curIndex);
			printf("\x1b[18;10H%s", curFile);
		}

		if (kDown & KEY_A)
		{
			quit = true;
			chip8.load_rom(curFile);
		}
		if (kDown & KEY_PLUS)
		{
			quit = true;
			quit2 = true;
		}

		//Clear screen
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderClear(renderer);
		
		gTextTexture.loadFromRenderedText(curFile, textColor);

		//Render current frame
		gTextTexture.render((1280 - gTextTexture.getWidth()) / 2, (720 - gTextTexture.getHeight()) / 2);
		gTextTexture.loadFromRenderedText("Select Rom with DPad.", textColor);

		//Render current frame
		gTextTexture.render((1280 - gTextTexture.getWidth()) / 2, (720 - gTextTexture.getHeight()) / 2 - 30);
		//Update screen
		SDL_RenderPresent(renderer);
		}
	

#else
	chip8.load_rom("C:\\respaldo2017\\C++\\Chip8\\Debug\\PONG2");

#endif // SWITCH
	std::uint32_t start_time;
	std::uint32_t delta_time;
	
	bool color = false;
#ifdef __SWITCH__
	while (appletMainLoop() && !quit2)
#else
	while (!quit2)
#endif // SWITCH
	{
#ifdef __SWITCH__
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
		//hidKeysHeld returns information about which buttons have are held down in this frame
		u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);
		//hidKeysUp returns information about which buttons have been just released
		u64 kUp = hidKeysUp(CONTROLLER_P1_AUTO);


		if (kDown & KEY_PLUS)
		{
			quit2 = true;

		}
		if (kDown & KEY_MINUS) {
			color = !color;
		}
		if (kDown & KEY_DUP) {
			chip8.press_key(0);
		}
		if (kUp & KEY_DUP) {
			chip8.release_key(0);
		}

		if (kDown & KEY_DDOWN) {
			chip8.press_key(1);
		}
		if (kUp & KEY_DDOWN) {
			chip8.release_key(1);
		}
		if (kDown & KEY_DLEFT) {
			chip8.press_key(2);
		}
		if (kUp & KEY_DLEFT) {
			chip8.release_key(2);
		}
		if (kDown & KEY_DRIGHT) {
			chip8.press_key(3);
		}
		if (kUp & KEY_DRIGHT) {
			chip8.release_key(3);
		}
		if (kDown & KEY_A) {
			chip8.press_key(4);
		}
		if (kUp & KEY_A) {
			chip8.release_key(4);
		}
		if (kDown & KEY_B) {
			chip8.press_key(5);
		}
		if (kUp & KEY_B) {
			chip8.release_key(5);
		}
		if (kDown & KEY_Y) {
			chip8.press_key(6);
	}
		if (kUp & KEY_Y) {
			chip8.release_key(6);
		}
		if (kDown & KEY_X) {
			chip8.press_key(7);
		}
		if (kUp & KEY_X) {
			chip8.release_key(7);
		}
		if (kDown & KEY_L) {
			chip8.press_key(7);
	}
		if (kUp & KEY_L) {
			chip8.release_key(7);
		}
		if (kDown & KEY_R) {
			chip8.press_key(8);
		}
		if (kUp & KEY_R) {
			chip8.release_key(8);
		}
		if (kDown & KEY_ZL) {
			chip8.press_key(9);
	}
		if (kUp & KEY_ZL) {
			chip8.release_key(9);
		}
		if (kDown & KEY_ZR) {
			chip8.press_key(10);
		}
		if (kUp & KEY_ZR) {
			chip8.release_key(10);
		}
		if (kDown & KEY_LSTICK_LEFT) {
			chip8.press_key(11);
	}
		if (kUp & KEY_LSTICK_LEFT) {
			chip8.release_key(11);
		}
		if (kDown & KEY_LSTICK_UP) {
			chip8.press_key(12);
		}
		if (kUp & KEY_LSTICK_UP) {
			chip8.release_key(12);
		}
		if (kDown & KEY_LSTICK_RIGHT) {
			chip8.press_key(13);
		}
		if (kUp & KEY_LSTICK_RIGHT) {
			chip8.release_key(13);
		}
		if (kDown & KEY_LSTICK_DOWN) {
			chip8.press_key(14);
		}
		if (kUp & KEY_LSTICK_DOWN) {
			chip8.release_key(14);
		}
		if (kDown & KEY_RSTICK_DOWN) {
			chip8.press_key(15);
		}
		if (kUp & KEY_RSTICK_DOWN) {
			chip8.release_key(15);
		}

#endif // SWITCH
	

		start_time = SDL_GetTicks();
		for (int i = 0; i < INSTRUCTIONS_PER_STEP; i++) {
			chip8.emulate_cycle();
		}
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				quit2 = true;
				break;
			case SDL_KEYDOWN:
				for (int i = 0; i < keymap.size(); i++) {
					if (event.key.keysym.sym == keymap[i]) {
						chip8.press_key(i);
					}
				}
				break;
			case SDL_KEYUP:
				for (int i = 0; i < keymap.size(); i++) {
					if (event.key.keysym.sym == keymap[i]) {
						chip8.release_key(i);
					}
				}
				break;
			}
		}

		if (chip8.get_sound_timer() > 0) {
			Mix_PlayChannel(-1, chunk, 0);
		}

		if (chip8.get_draw_flag()) {
			chip8.reset_draw_flag();
			std::uint32_t* pixels = nullptr;
			int pitch;
			SDL_LockTexture(texture, nullptr, reinterpret_cast<void**>(&pixels), &pitch);
			for (int i = 0; i < WIDTH * HEIGHT; i++) {
				if (color == false)
				pixels[i] = (chip8.get_pixel_data(i) == 0) ? 0x000000FF : 0xFFFFFFFF;
				else
					pixels[i] = (chip8.get_pixel_data(i) == 0) ? 0x000000FF : 0x64DC64FF;
			}
			SDL_UnlockTexture(texture);
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, nullptr, nullptr);
			SDL_RenderPresent(renderer);
		}
		delta_time = SDL_GetTicks() - start_time;
		if (TICKS_PER_FRAME > delta_time) {
			chip8.step_timers();
			SDL_Delay(TICKS_PER_FRAME - delta_time);
		}
	}
#ifdef __SWITCH__
	socketExit();
	romfsExit();
#endif // SWITCH
	Mix_FreeChunk(chunk);
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	Mix_Quit();
	SDL_Quit();
	return 0;
}