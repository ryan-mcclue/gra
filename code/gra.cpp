// SPDX-License-Identifier: zlib-acknowledgement
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>
#include <cmath>
// TODO(Ryan): Investigate whether C++ STL features used for embedded
// e.g. std::vector.at() throws exception on out-of-bounds
// exceptions bloat the stack
#include <vector>
#include <string>
#include <cassert>

#define INTERNAL static
#define GLOBAL static

typedef unsigned int uint; 

GLOBAL float secs_this_frame = 0.0f;
GLOBAL uint render_width;
GLOBAL uint render_height;
GLOBAL SDL_Renderer *renderer;

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
    const char *msg = SDL_GetError();
    return;
  }
  #define SBP() sbp()
  INTERNAL inline void
  stbp(void)
  {
    const char *msg = TTF_GetError();
    return;
  }
  #define STBP() stbp()
#else
  #define BP()
  #define SBP()
  #define STBP()
#endif

int
get_text_width(TTF_Font *font, std::string text)
{
  int text_width = 0;
  int text_height = 0;
  if (TTF_SizeText(font, text.c_str(), &text_width, &text_height) < 0)
  {
    STBP();
  }
  return text_width;
}

// Offset font to background to make stand out
void
draw_text(TTF_Font *font, std::string text, float x, float y, SDL_Color color)
{
  // TTF_RenderUTF8_Blended(); TTF_SizeUTF8()
  SDL_Surface *text_surface = \
    TTF_RenderText_Solid(font, text.c_str(), color);
  if (text_surface == NULL)
  {
    STBP();
  }
  
  int text_width = 0;
  int text_height = 0;
  if (TTF_SizeText(font, text.c_str(), &text_width, &text_height) < 0)
  {
    STBP();
  }

  SDL_Texture *text_texture = \
    SDL_CreateTextureFromSurface(renderer, text_surface);

  int min_x = nearbyint(x);
  int min_y = nearbyint(y);
  SDL_Rect text_rect = {min_x, min_y, text_width, text_height};
  SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);

  SDL_FreeSurface(text_surface);
  SDL_DestroyTexture(text_texture);
}

void
draw_rect(float x0, float y0, float x1, float y1, SDL_Color color)
{
  int min_x = nearbyint(x0);
  int min_y = nearbyint(y0);
  int max_x = nearbyint(x1);
  int max_y = nearbyint(y1);

  SDL_Rect rect = {min_x, min_y, max_x - min_x, max_y - min_y};

  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderFillRect(renderer, &rect);
}

#define INPUT_BUFFER_MAX_SIZE 8000
struct
TextInput
{
  std::string text;
  bool entered;
  bool escaped;
  bool activate;

  char input_buffer[INPUT_BUFFER_MAX_SIZE];
  int input_buffer_count;

  void
  handle_event(SDL_Event *event)
  {
    // when a key is pressed, will get a 2 events
    // 1 for a key is pressed
    // 2 will convert that key to some number of characters based on
    // locale mappings
    if (event->type == SDL_TEXTINPUT)
    {
      char key = event->text.text[0];
      assert(key < 128);
      if (input_buffer_count < INPUT_BUFFER_MAX_SIZE)
      {
        input_buffer[input_buffer_count++] = key;
      }

      printf("Got %s\n", event->text.text);
    }
  }

  void
  draw(TTF_Font *font, float x, float y, SDL_Color color)
  {
    std::string text(input_buffer);
    draw_text(font, text, x, y, color);

    // may need to alter position based on font descender/ascender info
    int text_width = get_text_width(font, text);
    float cursor_x = x + text_width;
    // better to make width based on width of "M" character
    float cursor_width = TTF_FontHeight(font) * 0.5f;
    float cursor_height = TTF_FontHeight(font);

    // start_time = last_keypress_time; (this is so it does not flash when pressing key)
    // cos_val = cos((time - start_time)*/ val). division slows, multiplication increases
    // cos_val *= cos_val (could sqrt(). this is to make curve more dynamic) 
    // color = lerp_color(flash_color, start_color, cos_val);
    Uint8 cursor_alpha = fabs(255.0f * cos(SDL_GetTicks() / 200));
    SDL_Color cursor_color = {255, 255, 255, cursor_alpha};
    draw_rect(cursor_x, y, cursor_x + cursor_width, y + cursor_height,
              cursor_color);
  }
};

enum
CONSOLE_STATE
{
  CONSOLE_CLOSED,
  CONSOLE_OPENED_SMALL,
  CONSOLE_OPENED_BIG
};

struct
Console
{
  TextInput *text_input;

  TTF_Font *input_font = NULL;
  int input_font_height;
  SDL_Color input_bg_color;
  SDL_Color input_text_color;

  TTF_Font *output_font = NULL;
  int output_font_height;
  SDL_Color output_bg_color;
  SDL_Color output_text_color;

  // IMPORTANT(Ryan): Things that happen instantaneously can be
  // disorientating for the user
  float current_height = 0.0f;
  float desired_height = 0.0f;
  float open_speed = 1000.0f;

  std::vector<std::string> history;

  Console()
  {
    text_input = new TextInput();
    output_font = TTF_OpenFont("fonts/Raleway-Regular.ttf", 32);
    if (output_font == NULL)
    {
      STBP();
    }
    input_font = output_font;

    // pad = font_height * pad_val;
    output_font_height = TTF_FontHeight(output_font); 
    input_font_height = TTF_FontHeight(input_font); 

    // NOTE(Ryan): Colour selection gimp magic wand, hue+sat, colour picker
    output_text_color = {20, 40, 60, 255};
    output_bg_color = {0, 255, 0, 255};
    input_bg_color = {0, 0, 255, 255};

    history.push_back("hi there");
    history.push_back("wandering about");
    history.push_back("more text here");
  }

  void
  draw(void)
  {
    update_height();

    if (current_height > 0)
    {
      draw_rect(0, 0, render_width, current_height, output_bg_color);
      float input_font_height = 40.0f; //TTF_FontHeight(input_font);
      draw_rect(0, current_height, render_width, 
                current_height + input_font_height, input_bg_color);

      float text_y0 = current_height - output_font_height;
      int index = history.size() - 1;
      while (text_y0 > 0)
      {
        if (index < 0)
        {
          break;
        }

        std::string cur_text = history[index]; 
        draw_text(output_font, cur_text, 
                  output_font_height * 0.5f, 
                  text_y0, output_text_color);
        text_y0 -= output_font_height;

        index--;
      }
      // float baseline_height = 0.0f;
      text_input->draw(input_font, 0, current_height, input_text_color); 
    }
  }

  void
  update_height(void)
  {
    // IMPORTANT(Ryan): Even though it's cleaner to wrap in a clamp()
    // function, at a later stage we may want to be aware that a
    // transition state occurs, e.g. when finished closing, play a sound
    // effect
    float height_d = secs_this_frame * open_speed;
    if (current_height < desired_height)
    {
      current_height += height_d;
      if (current_height > desired_height)
      {
        current_height = desired_height;
      }
    }
    if (current_height > desired_height)
    {
      current_height -= height_d;
      if (current_height < 0)
      {
        current_height = 0;
      }
    }
  }

  bool
  is_open(void)
  {
    return (current_height > 0.0f);
  }

  void
  handle_event(SDL_Event *event)
  {
    switch (event->type)
    {
      case SDL_KEYUP:
      {
        switch (event->key.keysym.sym)
        {
          case SDLK_BACKQUOTE:
          {
            if (is_open())
            {
              set_state(CONSOLE_CLOSED); 
              break;
            }
            if (event->key.keysym.mod & KMOD_LSHIFT)
            {
              set_state(CONSOLE_OPENED_BIG);
            }
            else
            {
              set_state(CONSOLE_OPENED_SMALL);
            }
          } break;
        }
      } break;
    }
    if (is_open())
    {
      text_input->handle_event(event); 
    }
  }

  void
  set_state(CONSOLE_STATE state)
  {
    switch (state)
    {
      case CONSOLE_CLOSED:
      {
        desired_height = 0.0f * render_height;
      } break;
      case CONSOLE_OPENED_SMALL:
      {
        desired_height = 0.3f * render_height;
      } break;
      case CONSOLE_OPENED_BIG:
      {
        desired_height = 0.7f * render_height;
      } break;
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

  if (TTF_Init() < 0)
  {
    STBP();
  }

  int window_width = 1280;
  int window_height = 720;
  render_width = window_width;
  render_height = window_height;
  SDL_Window *window = SDL_CreateWindow("GRA", SDL_WINDOWPOS_CENTERED, 
                                        SDL_WINDOWPOS_CENTERED,
                                        window_width, window_height, 0);
  if (window == NULL)
  {
    SBP();
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL)
  {
    SBP();
  }
  if (SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND) < 0)
  {
    SBP();
  }

  Console *console = new Console();

  uint prev_ms = SDL_GetTicks();

  bool want_to_run = true;
  SDL_StartTextInput();
  while (want_to_run)
  {
    SDL_Event event = {};
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        want_to_run = false;
        break;
      }
      console->handle_event(&event);
      if (console->is_open())
      {
        break;
      }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    console->draw();

    SDL_RenderPresent(renderer);

    uint cur_ms = SDL_GetTicks();
    secs_this_frame = (cur_ms - prev_ms) / 1000.0f;
    prev_ms = cur_ms;
  }

  return 0;
}
