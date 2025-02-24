#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
 
typedef SDL_FPoint Vector2;
typedef Uint64 u64;
typedef Uint32 u32;
typedef Uint16 u16;
typedef Uint8  u8;
typedef Sint64 s64;
typedef Sint32 s32;
typedef Sint16 s16;
typedef Sint8  s8;


static SDL_Window *window     = NULL;
static SDL_Renderer *renderer = NULL;
 
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720
#define HALF_WINDOW_WIDTH  (WINDOW_WIDTH  / 2)
#define HALF_WINDOW_HEIGHT (WINDOW_HEIGHT / 2)

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  // SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "120");
  SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

  SDL_SetAppMetadata("Multiplayer Asteroids", "0.1", "com.douglasselias.multiplayer-asteroids");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("Multiplayer Asteroids", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  if(event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;
  }

  if(event->type == SDL_EVENT_KEY_DOWN) {
    SDL_Scancode key_code = event->key.scancode;
    if(key_code == SDL_SCANCODE_ESCAPE || key_code == SDL_SCANCODE_Q) {
      return SDL_APP_SUCCESS;
    }
  }

  return SDL_APP_CONTINUE;
}

void draw_circle(Vector2 center, float radius, SDL_Color c) {
  SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);

  for(int x = -radius; x <= radius; x++) {
    for(int y = -radius; y <= radius; y++) {
      if(x*x + y*y < radius*radius) {
        SDL_RenderPoint(renderer, center.x + x, center.y + y);
      }
    }
  }
}

Uint64 last_time = 0;
SDL_AppResult SDL_AppIterate(void *appstate) {
  Uint64 now = SDL_GetTicks();
  float delta_time = ((float) (now - last_time)) / 1000.0f;

  SDL_SetRenderDrawColor(renderer, 30, 30, 30, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);

  float x, y;
  SDL_GetMouseState(&x, &y);
  Vector2 screen_mouse  = {x, y};

  draw_circle(screen_mouse, 30, (SDL_Color){20, 100, 10});
  
  SDL_SetRenderScale(renderer, 4.0f, 4.0f);
  SDL_SetRenderDrawColor(renderer, 30, 230, 30, SDL_ALPHA_OPAQUE);
  float fps = 1.0f / delta_time; 
  SDL_RenderDebugTextFormat(renderer, 10, 10, "FPS: %.2f", fps);
  SDL_SetRenderScale(renderer, 1.0f, 1.0f);
  
  SDL_RenderPresent(renderer);

  last_time = now;

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {}
