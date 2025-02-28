#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_image.h>

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include "src/types.c"
#include "src/ship.c"
#include "src/socket.c"

#define PI 3.14159265358979323846f
#define DEG2RAD (PI/180.0f)
#define RAD2DEG (180.0f/PI)

typedef SDL_FPoint Vector2;

SDL_Window *window     = NULL;
SDL_Renderer *renderer = NULL;
 
#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480
#define HALF_WINDOW_WIDTH  (WINDOW_WIDTH  / 2)
#define HALF_WINDOW_HEIGHT (WINDOW_HEIGHT / 2)

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
Ship last_ships[MAX_SHIPS] = {0};

typedef struct {
  Vector2 position;
  Vector2 velocity;
  f32 rotation;
} Bullet;

s32 bullet_radius = 10;
SDL_Texture* bullet_texture;
Vector2 bullet_texture_center;
#define MAX_BULLETS 1000
Bullet bullets[MAX_BULLETS] = {0};
s32 bullet_index = 0;

typedef struct {
  Vector2 position;
  Vector2 velocity;
} Asteroid;
u32 asteroid_radius = 45;
SDL_Texture* asteroid_texture;
Vector2 asteroid_texture_center;
#define MAX_ASTEROIDS 20
Asteroid asteroids[MAX_ASTEROIDS] = {0};

s32 GetRandomValue(s32 min, s32 max) {
  s32 value = 0;

  if (min > max) {
    s32 tmp = max;
    max = min;
    min = tmp;
  }

  value = (SDL_rand(100)%(abs(max - min) + 1) + min);
  return value;
}

void spawn_meteor(Asteroid* asteroid) {
  /// @note: padding to keep the location 'inside' the screen boundaries
  f32 spawn_padding = 30;
  /// @note: offset to keep the location 'outside' the screen boundaries
  f32 spawn_offset = 30;
  Vector2 meteor_spawn_locations[12] = {
    /// @note: top
    {spawn_padding, -spawn_offset},
    {HALF_WINDOW_WIDTH, -spawn_offset},
    {WINDOW_WIDTH - spawn_padding, -spawn_offset},
    /// @note: right
    {WINDOW_WIDTH + spawn_offset, spawn_padding},
    {WINDOW_WIDTH + spawn_offset, HALF_WINDOW_HEIGHT},
    {WINDOW_WIDTH + spawn_offset, WINDOW_HEIGHT - spawn_padding},
    /// @note: bottom
    {spawn_padding, WINDOW_HEIGHT + spawn_offset},
    {HALF_WINDOW_WIDTH, WINDOW_HEIGHT + spawn_offset},
    {WINDOW_WIDTH - spawn_padding, WINDOW_HEIGHT + spawn_offset},
    /// @note: left
    {-spawn_offset, spawn_padding},
    {-spawn_offset, HALF_WINDOW_HEIGHT},
    {-spawn_offset, WINDOW_HEIGHT - spawn_padding},
  };

  u32 location_index = GetRandomValue(0, 11);
  asteroid->position = meteor_spawn_locations[location_index];

  f32 angle = 0;
  switch(location_index) {
    /// @note: this logic follows the spawn locations order!
    /// hex numbers just to keep aligned :)
    /// top
    case 0x0: angle = GetRandomValue(270, 270 + 60); break;
    case 0x1: angle = GetRandomValue(190, 350);      break;
    case 0x2: angle = GetRandomValue(270 - 60, 270); break;
    /// right:
    case 0x3: angle = GetRandomValue(180, 180 + 60); break;
    case 0x4: angle = GetRandomValue(100, 170);      break;
    case 0x5: angle = GetRandomValue(180 - 60, 180); break;
    /// bottom:
    case 0x6: angle = GetRandomValue(90 - 60, 90);  break;
    case 0x7: angle = GetRandomValue(10, 170);      break;
    case 0x8: angle = GetRandomValue(90, 90  + 60); break;
    /// left:
    case 0x9: angle = GetRandomValue(360 - 60, 360);  break;
    case 0xa: angle = GetRandomValue(280, 270 + 160); break;
    case 0xb: angle = GetRandomValue(0, 60);          break;
  }
  angle *= DEG2RAD;
  f32 speed = GetRandomValue(50, 250);
  asteroid->velocity = (Vector2){
    (f32)cos(angle)  * speed,
    (f32)-sin(angle) * speed,
  };
}

bool is_out_of_bounds(Vector2 position) {
  float despawn_offset = 500;
  float top_despawn_zone    = -despawn_offset;
  float left_despawn_zone   = -despawn_offset;
  float right_despawn_zone  =  despawn_offset + WINDOW_WIDTH;
  float bottom_despawn_zone =  despawn_offset + WINDOW_HEIGHT;

  if(position.y < top_despawn_zone
  || position.x > right_despawn_zone
  || position.y > bottom_despawn_zone
  || position.x < left_despawn_zone) {
    return true;
  }

  return false;
}

void reset_bullet(u32 i) {
  bullets[i].position.x = -30;
  bullets[i].position.y = -30;
  bullets[i].velocity.x = 0;
  bullets[i].velocity.y = 0;
}

bool circles_are_colliding(Vector2 center1, f32 radius1, Vector2 center2, f32 radius2) {
  f32 dx = center2.x - center1.x;
  f32 dy = center2.y - center1.y;

  f32 distanceSquared = dx*dx + dy*dy;
  f32 radiusSum = radius1 + radius2;

  bool collision = (distanceSquared <= (radiusSum*radiusSum));
  return collision;
}

f32 Wrap(f32 value, f32 min, f32 max) {
  f32 result = value - (max - min) * floorf((value - min)/(max - min));
  return result;
}

SOCKET ConnectSocket;
u8 player_id;

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

  s32 displays_count;
  SDL_DisplayID* displays = SDL_GetDisplays(&displays_count);
  const SDL_DisplayMode* display_mode = SDL_GetCurrentDisplayMode(displays[0]);
  s32 w = display_mode->w, h = display_mode->h;

  char position = argv[1][0];
  switch(position) {
    case '0': SDL_SetWindowPosition(window, (w/2) - WINDOW_WIDTH, (h/2) - HALF_WINDOW_HEIGHT); break;
    case '1': SDL_SetWindowPosition(window, (w/2),                (h/2) - HALF_WINDOW_HEIGHT); break;
    case '2': SDL_SetWindowPosition(window, 0, 0); break;
    case '3': SDL_SetWindowPosition(window, 0, 0); break;
  }

  SDL_srand(0);

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

    last_ships[i].position.x = x;
    last_ships[i].position.y = y;
  }

  bullet_texture = IMG_LoadTexture(renderer, "../assets/bullet.png");
  bullet_texture_center.x = bullet_texture->w  / 2.0f;
  bullet_texture_center.y = bullet_texture->h  / 2.0f;

  for(u32 i = 0; i < MAX_BULLETS; i++) {
    bullets[i].position.x = -30;
    bullets[i].position.y = -30;
  }

  asteroid_texture = IMG_LoadTexture(renderer, "../assets/meteor0.png");
  asteroid_texture_center.x = asteroid_texture->w  / 2.0f;
  asteroid_texture_center.y = asteroid_texture->h  / 2.0f;

  for(u32 i = 0; i < MAX_ASTEROIDS; i++) {
    spawn_meteor(&asteroids[i]);
  }

  /********************* NETWORK *********************/
  WSADATA wsa_data;
  s32 iResult = WSAStartup(MAKEWORD(2,2), &wsa_data);

  struct addrinfo *result = NULL, *ptr = NULL, hints = {0};

  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  getaddrinfo("localhost", DEFAULT_PORT, &hints, &result);

  // Attempt to connect to an address until one succeeds
  for(ptr = result; ptr != NULL ; ptr = ptr->ai_next) {
    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if(ConnectSocket == INVALID_SOCKET) {
      SDL_Log("socket failed with error: %d\n", WSAGetLastError());
      return 1;
    }

    s32 connect_result = connect(ConnectSocket, ptr->ai_addr, (s32)ptr->ai_addrlen);
    if(connect_result != SOCKET_ERROR) {
      break;
    }
  }

  freeaddrinfo(result);

  if(ConnectSocket == INVALID_SOCKET) {
    SDL_Log("Unable to connect to server!\n");
    return 1;
  }

  char player_id_buffer[1];
  s32 recv_result = recv(ConnectSocket, player_id_buffer, 1, 0);
  if(recv_result > 0) {
    player_id = player_id_buffer[0];
  }

  DWORD non_blocking = 1;
  ioctlsocket(ConnectSocket, FIONBIO, &non_blocking);

  return SDL_APP_CONTINUE;
}

typedef enum {
  ACT_ROTATING_LEFT,
  ACT_ROTATING_RIGHT,
  ACT_NO_ROTATION,
} ActionRotate;
ActionRotate current_rotation_direction = ACT_NO_ROTATION;

bool moving_forward = false;
bool firing = false;

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

    if(key_code == SDL_SCANCODE_A) {
      current_rotation_direction = ACT_ROTATING_LEFT;
    } else if(key_code == SDL_SCANCODE_D) {
      current_rotation_direction = ACT_ROTATING_RIGHT;
    } else {
      current_rotation_direction = ACT_NO_ROTATION;
    }

    if(key_code == SDL_SCANCODE_W) {
      moving_forward = true;
    }

    if(key_code == SDL_SCANCODE_J) {
      firing = true;
    }
  }

  if(event->type == SDL_EVENT_KEY_UP) {
    SDL_Scancode key_code = event->key.scancode;

    if(key_code == SDL_SCANCODE_A) {
      current_rotation_direction = ACT_NO_ROTATION;
    } else if(key_code == SDL_SCANCODE_D) {
      current_rotation_direction = ACT_NO_ROTATION;
    }

    if(key_code == SDL_SCANCODE_W) {
      moving_forward = false;
    }

    if(key_code == SDL_SCANCODE_J) {
      firing = false;
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

f32 Lerp(f32 start, f32 end, f32 amount) {
  f32 result = start + amount*(end - start);
  return result;
}

u64 last_time = 0;
u64 frame_counter = 1;
u64 sync_frame = 2;

SDL_AppResult SDL_AppIterate(void *appstate) {
  u64 now = SDL_GetTicks();
  f32 delta_time = ((f32) (now - last_time)) / 1000.0f;

  if((frame_counter % (sync_frame)) == 0) {
    char received_position_buffer[MESSAGE_BUFFER_SIZE];
    s32 iResult = recv(ConnectSocket, received_position_buffer, MESSAGE_BUFFER_SIZE, 0);
    if(iResult > 0) {
      u8 remote_player_id = received_position_buffer[0];
      memcpy(&(ships[remote_player_id].position.x), received_position_buffer + 1, sizeof(f32));
      memcpy(&(ships[remote_player_id].position.y), received_position_buffer + 5, sizeof(f32));
      memcpy(&(ships[remote_player_id].rotation),   received_position_buffer + 9, sizeof(f32));
      // SDL_Log("Receive data: %.2f, %.2f\n", ships[remote_player_id].position.x, ships[remote_player_id].position.y);
    }
  } else {
    for(u8 i = 0; i < MAX_SHIPS; i++) {
      if(i == player_id) continue;
      u8 remote_player_id = i;
      f32 last_x = last_ships[remote_player_id].position.x;
      f32 last_y = last_ships[remote_player_id].position.y;
      f32 current_x = ships[remote_player_id].position.x;
      f32 current_y = ships[remote_player_id].position.y;

      last_ships[remote_player_id].position.x = current_x;
      last_ships[remote_player_id].position.y = current_y;
      last_ships[remote_player_id].rotation   = ships[remote_player_id].rotation;

      // if(current_x < last_x
      // || current_y < last_y) {
      //   last_ships[remote_player_id].position.x = current_x;
      //   last_ships[remote_player_id].position.y = current_y;
      // } else {
      //   last_ships[remote_player_id].position.x = Lerp(last_x, current_x, 0.1f);
      //   last_ships[remote_player_id].position.y = Lerp(last_y, current_y, 0.1f);
      // }
      // last_ships[remote_player_id].rotation   = Lerp(last_ships[remote_player_id].rotation, ships[remote_player_id].rotation, 0.1f);

      // SDL_Log("SHIPS: %.2f, %.2f\n", ships[remote_player_id].position.x, ships[remote_player_id].position.y);
      // SDL_Log("LASTS: %.2f, %.2f\n", last_ships[remote_player_id].position.x, last_ships[remote_player_id].position.y);
    }
  }

  if(current_rotation_direction == ACT_ROTATING_LEFT) {
    ships[player_id].rotation -= rotation_speed * delta_time;
  } else if(current_rotation_direction == ACT_ROTATING_RIGHT) {
    ships[player_id].rotation += rotation_speed * delta_time;
  }

  if(moving_forward) {
    f32 radians = ships[player_id].rotation * DEG2RAD;
    Vector2 direction = {(f32)sin(radians), (f32)-cos(radians)};
    ships[player_id].velocity.x += direction.x * ship_acceleration.x * delta_time;
    ships[player_id].velocity.y += direction.y * ship_acceleration.y * delta_time;
  }

  if(firing) {
    firing = false;
    f32 rotation = ships[player_id].rotation;
    Vector2 position = ships[player_id].position;

    f32 radians = rotation * DEG2RAD;
    Vector2 direction = {(f32)sin(radians), (f32)-cos(radians)};
    bullets[bullet_index].rotation = rotation;
    bullets[bullet_index].position = position;
    bullets[bullet_index].velocity = (Vector2){direction.x * 100 * 5, direction.y * 100 * 5};
    bullet_index = bullet_index < MAX_BULLETS - 1 ? bullet_index + 1 : 0;
  }

  ships[player_id].position.x += ships[player_id].velocity.x * delta_time;
  ships[player_id].position.y += ships[player_id].velocity.y * delta_time;
  ships[player_id].position.x = Wrap(ships[player_id].position.x, 0, WINDOW_WIDTH);
  ships[player_id].position.y = Wrap(ships[player_id].position.y, 0, WINDOW_HEIGHT);

  for(u32 i = 0; i < MAX_BULLETS; i++) {
    bullets[i].position.x += bullets[i].velocity.x * delta_time;
    bullets[i].position.y += bullets[i].velocity.y * delta_time;

    if(is_out_of_bounds(bullets[i].position)) {
      reset_bullet(i);
    }
  }

  for(u32 i = 0; i < MAX_ASTEROIDS; i++) {
    asteroids[i].position.x += asteroids[i].velocity.x * delta_time;
    asteroids[i].position.y += asteroids[i].velocity.y * delta_time;

    if(is_out_of_bounds(asteroids[i].position)) {
      spawn_meteor(&asteroids[i]);
    }

    for(u32 j = 0; j < MAX_BULLETS; j++) {
      Vector2 asteroid_center = {
        asteroids[i].position.x + asteroid_texture_center.x,
        asteroids[i].position.y + asteroid_texture_center.y
      };
      Vector2 bullet_center = {
        bullets[j].position.x + bullet_texture_center.x,
        bullets[j].position.y + bullet_texture_center.y
      };
      if(circles_are_colliding(bullet_center, bullet_radius, asteroid_center, asteroid_radius)) {
        spawn_meteor(&asteroids[i]);
        reset_bullet(j);
      }
    }
  }

  if((frame_counter % (sync_frame)) == 0) {
    char player_state_buffer[MESSAGE_BUFFER_SIZE] = {0};
    player_state_buffer[0] = player_id;
    memcpy(player_state_buffer + 1, &(ships[player_id].position.x), sizeof(f32));
    memcpy(player_state_buffer + 5, &(ships[player_id].position.y), sizeof(f32));
    memcpy(player_state_buffer + 9, &(ships[player_id].rotation),   sizeof(f32));
    send(ConnectSocket, player_state_buffer, MESSAGE_BUFFER_SIZE, 0);
  }

  frame_counter++;

  /********************* RENDERER *********************/
  SDL_SetRenderDrawColor(renderer, 15, 15, 15, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);

  for(u32 i = 0; i < MAX_BULLETS; i++) {
    f32 texture_width  = bullet_texture->w;
    f32 texture_height = bullet_texture->h;
    SDL_FRect dst_rect;
    dst_rect.x = bullets[i].position.x;
    dst_rect.y = bullets[i].position.y;
    dst_rect.w = texture_width;
    dst_rect.h = texture_height;
    SDL_RenderTextureRotated(renderer, bullet_texture, NULL, &dst_rect, bullets[i].rotation, &bullet_texture_center, SDL_FLIP_NONE);
  }

  for(u32 i = 0; i < MAX_ASTEROIDS; i++) {
    f32 texture_width  = asteroid_texture->w;
    f32 texture_height = asteroid_texture->h;
    SDL_FRect dst_rect;
    dst_rect.x = asteroids[i].position.x;
    dst_rect.y = asteroids[i].position.y;
    dst_rect.w = texture_width;
    dst_rect.h = texture_height;
    SDL_RenderTextureRotated(renderer, asteroid_texture, NULL, &dst_rect, 0, &asteroid_texture_center, SDL_FLIP_NONE);
  }

  for(u8 i = 0; i < MAX_SHIPS; i++) {
    SDL_Texture *texture = ship_textures[i];
    f32 texture_width  = texture->w;
    f32 texture_height = texture->h;
    SDL_FRect dst_rect;
    f32 x, y, rotation;
    if(i == player_id) {
      x = ships[i].position.x;
      y = ships[i].position.y;
      rotation = ships[i].rotation;
    } else {
      x = last_ships[i].position.x;
      y = last_ships[i].position.y;
      rotation = last_ships[i].rotation;
    }
    dst_rect.x = x;
    dst_rect.y = y;
    dst_rect.w = texture_width;
    dst_rect.h = texture_height;
    Vector2 center;
    center.x = texture_width  / 2.0f;
    center.y = texture_height / 2.0f;
    SDL_RenderTextureRotated(renderer, texture, NULL, &dst_rect, rotation, &center, SDL_FLIP_NONE);
  }

  SDL_RenderPresent(renderer);

  last_time = now;

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {}
