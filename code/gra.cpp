// SPDX-License-Identifier: zlib-acknowledgement
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define INTERNAL static
#define GLOBAL static

typedef unsigned int uint; 

GLOBAL float secs_this_frame = 0.0f;

// NOTE(Ryan): Breakpoint macros
#if defined(DEV_BUILD)
  INTERNAL inline void
  bp(void)
  {
    return;
  }
  #define BP() bp()
  INTERNAL inline void
  sbp(void)
  {
    char *msg = SDL_GetError();
    return;
  }
  #define SBP() sbp()
  INTERNAL inline void
  stbp(void)
  {
    char *msg = TTF_GetError();
    return;
  }
  #define STBP() stbp()
#else
  #define BP()
  #define SBP()
  #define STBP()
#endif

enum
ConsoleState
{
  CONSOLE_CLOSED,
  CONSOLE_OPEN_SMALL,
  CONSOLE_OPEN_BIG
};

struct
Console
{
  TTF_Font *input_font = NULL;
  TTF_Font *report_font = NULL;

  float openness_max  = 0.7f; // fraction of screen space?
  float open_t        = 0.0f; // how much is it open
  float open_t_target = 0.0f; // how much it wants to be open
  float open_dt       = 7.0f; // speed

  void
  draw(void)
  {
    update_openness();

    // current dimensions of console
    float x0, x1, y0, y1;

    float input_font_height = TTF_FontHeight(input_font); 
    float input_tab_space = input_font_height;   
  }

  void
  open_or_close(ConsoleState extent)
  {
    if (extent == CONSOLE_CLOSED)
    {
      open_t_target = 0.0f;
    }
    if (extent == CONSOLE_OPEN_SMALL)
    {
      open_t_target = 1.0f;
    }
    if (extent == CONSOLE_OPEN_BIG)
    {
      open_t_target = 3.0f;
    }
  }

  void
  update_openness(void)
  {
    float dopen = secs_this_frame * open_dt;
    if (open_t < open_t_target)
    {
      open_t += dopen;
      if (open_t > open_t_target)
      {
        open_t = open_t_target;
      }
    }
    if (open_t > open_t_target)
    {
      open_t -= dopen;
      if (open_t < 0)
      {
        open_t = 0;
      }
    }
  }
};

int
main(int argc, char *argv[])
{
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
  {
    SBP();
  }

  int window_width = 1280;
  int window_height = 720;
  SDL_Window *window = SDL_CreateWindow("GRA", SDL_WINDOWPOS_CENTERED, 
                                        SDL_WINDOWPOS_CENTERED,
                                        window_width, window_height, 0);
  if (window == NULL)
  {
    SBP();
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
  if (renderer == NULL)
  {
    SBP();
  }

  TTF_Font *font = TTF_OpenFont("fonts/Raleway-Regular.ttf", 24);
  if (font == NULL)
  {
    STBP();
  }

  //SDL_Color white = {255, 255, 255};
  //SDL_Surface *text = TTF_RenderText_Solid(font, "my text", white);
  //SDL_Texture *text_texture = \
  //  SDL_CreateTextureFromSurface(renderer, text);
  //SDL_RenderCopy(renderer, text_texture, NULL, &rect);

  //u8 *SDL_GetKeyboardState(NULL);

  uint prev_ms = SDL_GetTicks();

  bool want_to_run = true;
  while (want_to_run)
  {
    SDL_Event event = {};
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_QUIT:
        {
          want_to_run = false;
        } break;
      }
    }

    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_RenderPresent(renderer);

    uint cur_ms = SDL_GetTicks();
    secs_this_frame = (cur_ms - prev_ms) / 1000.0f;
    prev_ms = cur_ms;
  }

  return 0;
}
