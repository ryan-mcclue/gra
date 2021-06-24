// SPDX-License-Identifier: zlib-acknowledgement
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// 2:56:00 drop-down-console

#include <iostream>
#include <cmath>
// TODO(Ryan): Investigate whether C++ STL features used for embedded
// e.g. std::vector.at() throws exception on out-of-bounds
// exceptions bloat the stack
#include <vector>
#include <fstream>
#include <iterator>
#include <string>
#include <cassert>
#include <map>

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

// TODO(Ryan): Investigate references?
// TODO(Ryan): Iterators seem pervasive
std::vector<std::string> 
read_entire_file_as_lines(std::string file_name)
{
  std::vector<std::string> lines;
  std::fstream file(file_name);
  if (file.is_open())
  {
    std::string line;
    while (std::getline(file, line))
    {
      lines.push_back(line);
    }
  }
  return lines;
}

union
Variable
{
  // doesn't handle overflow
  int i_val; 
  float f_val; 
};

struct
VariableContainer
{
  std::map<std::string, Variable> variables;

  add_value(std::string name, Variable value)
  {

  }
};

map<string, map<string, Variable>> variable_containers;

VariableContainer player = VariableContainer("Player");
player.add_value("i_x", 10);
player.add_value("b_fast", false);

variable_containers["Player"]["fast"]

// could just store in one giant struct

void
init_variables(void)
{
  add_value_holder(player_hotload); 

  std::vector<std::string> file_lines = \
    read_entire_file_as_lines("All.variables");  

  int line_number = 1;
  for (std::string line: file_lines)
  {
    if (line.empty()) continue;
   
    int space_counter = 0;
    for (char c: line)
    {
      if (c != ' ') break;
      space_counter++;
    }
    line.erase(0, space_counter);
    if (line.empty()) continue;

    // Parsing choose which is more encompassing: 
    // identifier character or length
    const char *line_at = line.c_str();
    if (line_at[0] == ':')
    {
      if (line.length() < 2) {
        printf("Error at line: %d. Line starting with ':' must have '/' and a name after it.\n", line_number);
      }
      else
      {
        if (line_at[1] != '/')
        {
          printf("Error at line: %d. Expected a '/' after ':'\n", line_number);
        }
        else
        {
          line_at += 2;
          printf("Folder name: %s\n", line_at);
          if (cant_find_variable) print_error();
          current_variable = variable;
        }
      }
    }
    else if (line_at[0] == '#')
    {
      printf("Comment: %s\n", line_at);
    }
    else
    {
      const char *rhs = line_at;
      while (true)
      {
        if (rhs[0] == '\0') break;
        if (rhs[0] == ' ') break;
        rhs++;
      }

      if (rhs[0] == '\0')
      {
        printf("Error at line number %d! Expected a space after variable name\n", line_number);
      }
      else
      {
        const char *name = substr(line, 0, (rhs - line));
        rhs = consume_spaces(rhs);
        if (current_holder == nullptr)
        {
          // error 
        }
        else
        {
          // set value in holder
          if (value_name_not_in_current_holder) error;
          poke_value(current_holder, value_name, value);
          int --> atoi(str);
          float --> atof(str);
          bool --> if at[0] == 'T/t/F/f'
        }
      }
    }
    line_number++;
  }
}

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
  if (text.empty())
  {
    return;
  }
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

struct
TextInput
{
  bool entered;
  bool escaped;
  bool activate;

  std::vector<char> input_buffer;

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
      input_buffer.push_back(key);
    }
    if (event->type == SDL_KEYUP)
    {
      if (event->key.keysym.sym == SDLK_RETURN)
      {
        entered = true;
      }
      if (event->key.keysym.sym == SDLK_BACKSPACE)
      {
        escaped = true;
      }
    }
  }

  void
  draw(TTF_Font *font, float x, float y, SDL_Color color)
  {
    std::string text(input_buffer.data(), input_buffer.size());
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
      if (text_input->entered)
      {
        std::string hist_string(text_input->input_buffer.data(), 
                                text_input->input_buffer.size());
        history.push_back(hist_string);
        text_input->entered = false;
        text_input->input_buffer.clear();
      }
      if (text_input->escaped)
      {
        text_input->input_buffer.pop_back();
        text_input->escaped = false;
      }
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

  init_variables();

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
