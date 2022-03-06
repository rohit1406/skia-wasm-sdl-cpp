/* Author:
// Rohit Muneshwar: 06 March 2022
// This cpp file draws something on the screen using SDL and emscripten
*/

#include<stdio.h>
#include<SDL/SDL.h>

#ifdef __EMSCRIPTEN__
#include<emscripten.h>
#endif // !__EMSCRIPTEN__

extern "C" int main(int args, char** argv) {
	printf("Hello Emscripten!!!");

	// Create LOG file
	FILE *LOGGER = fopen("./log.txt", "w");
	if (!LOGGER) {
		printf("cannot open log file\n");
		return 1;
	}
	fprintf(LOGGER, "Log file opened");
	// Initialize SDL
	fprintf(LOGGER, "Initializing SDL");
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Surface* screen = SDL_SetVideoMode(256, 256, 32, SDL_SWSURFACE);
	fprintf(LOGGER, "SDL Initialzed");
	#ifdef TEST_SDL_LOCK_OPTS
		EM_ASM("SDL.defaults.copyOnLock = false; SDL.defaults.discardOnLock = true; SDL.defaults.opaqueFrontBuffer = false;");
	#endif

	if (SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);
	fprintf(LOGGER, "Drawing Square");
	// Draw Square
	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 256; j++) {
			#ifdef TEST_SDL_LOCK_OPTS
						// Alpha behaves like in the browser, so write proper opaque pixels.
						int alpha = 255;
			#else
						// To emulate native behavior with blitting to screen, alpha component is ignored. Test that it is so by outputting
						// data (and testing that it does get discarded)
						int alpha = (i + j) % 255;
			#endif
						* ((Uint32*)screen->pixels + i * 256 + j) = SDL_MapRGBA(screen->format, i, j, 255 - i, alpha);
		}
	}



	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	SDL_Flip(screen);

	printf("you should see a smoothly-colored square - no sharp lines but the square borders!\n");
	printf("and here is some text that should be HTML-friendly: amp: |&| double-quote: |\"| quote: |'| less-than, greater-than, html-like tags: |<cheez></cheez>|\nanother line.\n");


	// Quit SDL
	SDL_Quit();

	// Close Log File
	fprintf(LOGGER, "closing Log file");
	fclose(LOGGER);

	// Flushes the output to the canvas - this is required when using emscripten
	printf("\n");

	return 0;
}
