#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_image.h>

typedef SDL_FPoint Vector2;
typedef Uint64 u64;
typedef Uint32 u32;
typedef Uint16 u16;
typedef Uint8  u8;
typedef Sint64 s64;
typedef Sint32 s32;
typedef Sint16 s16;
typedef Sint8  s8;
typedef float  f32;
typedef double f64;

#define PI 3.14159265358979323846f
#define DEG2RAD (PI/180.0f)
#define RAD2DEG (180.0f/PI)

f32 Wrap(f32 value, f32 min, f32 max) {
  f32 result = value - (max - min) * floorf((value - min)/(max - min));
  return result;
}

static SDL_Window *window     = NULL;
static SDL_Renderer *renderer = NULL;
 
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720
#define HALF_WINDOW_WIDTH  (WINDOW_WIDTH  / 2)
#define HALF_WINDOW_HEIGHT (WINDOW_HEIGHT / 2)

#define MAX_SHIPS 4
SDL_Texture* ship_textures[MAX_SHIPS] = {0};

Vector2 ship_acceleration = {300, 300};
u32 rotation_speed = 500;
u32 ship_radius = 10;

typedef struct {
  f32 rotation;
  Vector2 position;
  Vector2 velocity;
} Ship;

Ship ships[MAX_SHIPS] = {0};

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

  for(u32 i = 0; i < MAX_SHIPS; i++) {
    char* filename = NULL;
    SDL_asprintf(&filename, "../assets/ship_%d.png", i);
    ship_textures[i] = IMG_LoadTexture(renderer, filename);
    SDL_free(filename);

    f32 x = (HALF_WINDOW_WIDTH  - (ship_textures[i]->w / 2));
    f32 y = (HALF_WINDOW_HEIGHT - (ship_textures[i]->h / 2));

    if(i == 0) { x *= 0.25; y *= 0.25; }
    if(i == 1) { x *= 1.75; y *= 0.25; }
    if(i == 2) { x *= 1.75; y *= 1.75; }
    if(i == 3) { x *= 0.25; y *= 1.75; }

    ships[i].position.x = x;
    ships[i].position.y = y;
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

    f32 dt = 0.013f;
    if(key_code == SDL_SCANCODE_A) {
      ships[0].rotation -= rotation_speed * dt;
    }

    if(key_code == SDL_SCANCODE_D) {
      ships[0].rotation += rotation_speed * dt;
    }

    if(key_code == SDL_SCANCODE_W) {
      f32 radians = ships[0].rotation * DEG2RAD;
      Vector2 direction = {(f32)sin(radians), (f32)-cos(radians)};
      ships[0].velocity.x += direction.x * ship_acceleration.x * dt;
      ships[0].velocity.y += direction.y * ship_acceleration.y * dt;
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

  ships[0].position.x += ships[0].velocity.x * delta_time;
  ships[0].position.y += ships[0].velocity.y * delta_time;
  ships[0].position.x = Wrap(ships[0].position.x, 0, WINDOW_WIDTH);
  ships[0].position.y = Wrap(ships[0].position.y, 0, WINDOW_HEIGHT);

  /// Renderer /////////////////////////////////////////////////
  SDL_SetRenderDrawColor(renderer, 30, 30, 30, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);

  float x, y;
  SDL_GetMouseState(&x, &y);
  Vector2 screen_mouse  = {x, y};

  draw_circle(screen_mouse, 30, (SDL_Color){20, 100, 10});

  for(u8 i = 0; i < MAX_SHIPS; i++) {
    SDL_Texture *texture = ship_textures[i];
    f32 texture_width  = texture->w;
    f32 texture_height = texture->h;
    SDL_FRect dst_rect;
    dst_rect.x = ships[i].position.x;
    dst_rect.y = ships[i].position.y;
    dst_rect.w = texture_width;
    dst_rect.h = texture_height;
    Vector2 center;
    center.x = texture_width  / 2.0f;
    center.y = texture_height / 2.0f;
    SDL_RenderTextureRotated(renderer, texture, NULL, &dst_rect, ships[i].rotation, &center, SDL_FLIP_NONE);
  }

  
  SDL_SetRenderScale(renderer, 4.0f, 4.0f);
  SDL_SetRenderDrawColor(renderer, 30, 230, 30, SDL_ALPHA_OPAQUE);
  f32 fps = 1.0f / delta_time; 
  // SDL_RenderDebugTextFormat(renderer, 10, 10, "FPS: %.2f", fps);
  // SDL_RenderDebugTextFormat(renderer, 10, 10, "IMG Version: %d", SDL_IMAGE_MAJOR_VERSION);
  SDL_SetRenderScale(renderer, 1.0f, 1.0f);

  SDL_SetRenderDrawColor(renderer, 200, 20, 20, SDL_ALPHA_OPAQUE);
  SDL_RenderLine(renderer, 0, HALF_WINDOW_HEIGHT, WINDOW_WIDTH, HALF_WINDOW_HEIGHT);
  SDL_RenderLine(renderer, HALF_WINDOW_WIDTH, 0, HALF_WINDOW_WIDTH, WINDOW_HEIGHT);
  
  SDL_RenderPresent(renderer);

  last_time = now;

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {}
