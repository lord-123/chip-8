#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <stdint.h>
#include <SDL2/SDL.h>

#define W 64
#define H 32
#define SCALE 12

#define STACK_SIZE 0x10

uint8_t memory[0x1000]= {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};
uint16_t stack[STACK_SIZE];
uint8_t display[H][W/8];

// registers
uint16_t pc = 0x200, I;
uint8_t delay_timer, sound_timer, V[0x10];

// SDL stuff
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Event event;
char running;

int init_sdl()
{
	if (SDL_Init(SDL_INIT_VIDEO))
	{
		printf("SDL init failed: %s\n", SDL_GetError());
		return -1;
	}

	window = SDL_CreateWindow("chip-8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W*SCALE, H*SCALE, SDL_WINDOW_SHOWN);

	if (!window)
	{
		printf("Couldn't make window: %s\n", SDL_GetError());
		return -1;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	if (!renderer)
	{
		printf("Couldn't get renderer: %s\n", SDL_GetError());
		return -1;
	}

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);

	return 0;
}

void cls()
{
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(renderer);
}

void draw_display()
{
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	int x, y;
	for (x = 0; x < W; ++x)
	{
		for (y = 0; y < H; ++y)
		{
			if (display[y][x/8] & (128 >> x%8)){
				SDL_FRect r = {x*SCALE, y*SCALE, SCALE, SCALE};
				SDL_RenderDrawRectF(renderer, &r);
			}
		}
	}
}

void update_screen()
{
		cls();

		draw_display();

		SDL_RenderPresent(renderer);
}

void set_pixel(uint8_t x, uint8_t y)
{
	display[y][x/8] ^= (128 >> x%8);
}

#define LIST_INSTRUCTIONS(o)						\
	o(CLS, FFFF, 00E0, cls())						\
		o(JMP, F000, 1000, pc = nnn)				\
		o(LDA, F000, 6000, V[x] = nn)				\
		o(ADD, F000, 7000, V[x] += nn)				\
		o(STI, F000, A000, I = nnn)					\
		o(DRAW, F000, D000,							\
		  uint8_t x1 = V[x] & 63;					\
		  uint8_t y1 = V[y] & 31;					\
		  V[0xF] = 0;										\
		  for (int row = 0; row < n; ++row) {				\
			  uint8_t sprite = memory[I+row];				\
			  for(uint8_t column=0; column < 8; ++column) {	\
				  if(sprite & (128 >> column)) {			\
					  if (display[y+row][(x+column)/8] & (128 >> (x+column)%8)) V[0xF] = 1;	\
					  set_pixel(x1+column, y1+row);						\
				  }														\
				  if (x1+column == 63) break;							\
			  }															\
			  if (y1+row == 31) break;									\
		  }																\
	      update_screen();)





void ins() {
	uint16_t opcode = (memory[pc]<<8) | memory[pc+1];
	pc += 2;

	uint8_t x = (opcode & 0x0F00) >> 8;
	uint8_t y = (opcode & 0x00F0) >> 4;
	uint8_t n = opcode & 0x000F;
	uint8_t nn = opcode & 0x00FF;
	uint16_t nnn = opcode & 0x0FFF;

#define o(name, mask, code, f) if ((opcode & 0x##mask) == 0x##code) {f;}
	LIST_INSTRUCTIONS(o)
#undef o
}

int main(int argc, char **argv)
{
	if (init_sdl() < 0) return -1;

	if (argc != 2) return -1;

	FILE *ptr = fopen(argv[1], "rb");
	fread(memory+0x200, 1, 0x1000-0x200, ptr);
	fclose(ptr);

	running = 1;
	while (running)
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT) running = 0;
		}

		ins();
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
