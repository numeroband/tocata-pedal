#pragma once

#include "display_sim.h"

#include <SDL.h>

#include <iostream>
#include <array>
#include <bitset>
#include <chrono>
#include <os/log.h>

using namespace std::chrono_literals;

namespace tocata {

class Application
{
public:
  Application(DisplaySim& display_sim) : _display_sim(display_sim) {
    _window = SDL_CreateWindow("Tocata Pedal",
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                620, 400,
                                0);

    if(!_window)
    {
        std::cout << "Failed to create window\n";
        std::cout << "SDL2 Error: " << SDL_GetError() << "\n";
        return;
    }

    _window_renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if(!_window_renderer)
    {
        std::cout << "Failed to get window's surface\n";
        std::cout << "SDL2 Error: " << SDL_GetError() << "\n";
        return;
    }
    
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    _display_texture = SDL_CreateTexture(_window_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, kDisplayColumns, kDisplayRows);
    if (!_display_texture) {
      std::cout << "Failed to create screen texture\n";
      std::cout << "SDL2 Error: " << SDL_GetError() << "\n";
      return;
    }

    initSwitches();
  }      

  ~Application() {
    SDL_DestroyTexture(_display_texture);
    SDL_DestroyRenderer(_window_renderer);
    SDL_DestroyWindow(_window);
  }

  bool run() {
    std::this_thread::sleep_for(15ms);
    while(SDL_PollEvent(&_window_event) > 0)
    {
      switch(_window_event.type)
      {
        case SDL_QUIT:
          return false;
        case SDL_MOUSEBUTTONDOWN:
        {
          int switch_id = checkSwitches({_window_event.button.x, _window_event.button.y});
          if (switch_id >= 0) {
            switch (_window_event.button.button) {
              case SDL_BUTTON_RIGHT:
                _switches_state[(switch_id + (kSwitches / 2)) % kSwitches] = true;                
              case SDL_BUTTON_LEFT:
                _switches_state[switch_id] = true;
                _switches_changed = true;
                break;
              default:
                break;
            }
          } else {
            system("open 'http://localhost:9002/tocata-pedal?transport=ws'");
          }
          break;
        }
        case SDL_MOUSEBUTTONUP:
          if (_switches_state.any()) {
            _switches_state.reset();
            _switches_changed = true;
          }
          break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
          keyChanged(_window_event.key.keysym.sym, _window_event.type == SDL_KEYDOWN);
          break;
        default:
          break;
      }
    }

    draw();

    return true;
  }

  void keyChanged(SDL_Keycode code, bool state) {
    uint8_t switch_id;
    switch (code) {
      case SDLK_a:
        switch_id = 0;
        break;
      case SDLK_b:
        switch_id = 1;
        break;
      case SDLK_c:
        switch_id = 2;
        break;
      case SDLK_d:
        switch_id = 3;
        break;
      case SDLK_e:
        switch_id = 4;
        break;
      case SDLK_f:
        switch_id = 5;
        break;
      default:
        return;
    }

    if (_switches_state[switch_id] != state) {
      _switches_state[switch_id] = state;
      _switches_changed = true;
    }
  }

  bool switchesChanged() { return _switches_changed; }
  
  uint32_t switchesValue() { 
    _switches_changed = false;
    return static_cast<uint32_t>(_switches_state.to_ulong()); 
  }

  void setLedColor(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= kSwitches) {
      return;
    }
    static constexpr uint8_t mapping[] = {2, 1, 0, 3, 4, 5};
    const uint8_t mapped = mapping[index];
    _leds[mapped] = {r, g, b, 0xFF};
  }

private:
  static constexpr size_t kDisplayRows = 64;
  static constexpr size_t kDisplayColumns = 132;
  static constexpr size_t kSwitches = 6;

  void initSwitches() {
    constexpr int size = 35;
    SDL_Rect start{40, 40, 250, 280};
    constexpr int rows = 2;
    constexpr int cols = kSwitches / 2;
    for (int row = 0; row < rows; ++row) {
      for (int col = 0; col < cols; ++col) {
        int index = row * cols + col;
        auto& rect = _switches_rect[index];
        rect.x = start.x + (col * start.w);
        rect.y = start.y + (row * start.h);
        rect.w = size;
        rect.h = size;
        _leds[index] = {0, 0, 0, 0xFF};
      }
    }
  }

  int checkSwitches(const SDL_Point& point) {
    os_log(OS_LOG_DEFAULT, "Checking switches in (%u,%u)", point.x, point.y);
    for (int i = 0; i < kSwitches; ++i) {
      if (SDL_PointInRect(&point, &_switches_rect[i])) {
        return i;
      }
    }

    return -1;
  }

  void draw() {
    SDL_SetRenderDrawColor(_window_renderer, 0x55, 0x55, 0x55, 0xFF);
    SDL_RenderClear(_window_renderer);
    drawDisplay();
    drawSwitches();
    drawLeds();
    SDL_RenderPresent(_window_renderer);
  }

  void drawDisplay() {
    static uint32_t colors[] = {0x000011FF, 0xEEEEFFFF};
    _display_sim.refresh(_display_buffer, kDisplayColumns, colors);
    SDL_UpdateTexture(_display_texture, nullptr, _display_buffer, kDisplayColumns * sizeof(_display_buffer[0]));
    SDL_Rect rect{115, 160, kDisplayColumns, kDisplayRows};
    SDL_RenderCopy(_window_renderer, _display_texture, nullptr, &rect);
  }

  void drawSwitches() {
    for (int i = 0; i < kSwitches; ++i) {
      if (_switches_state[i]) {
        SDL_SetRenderDrawColor(_window_renderer, 0xAA, 0xAA, 0xAA, 0xAA);
      } else {
        SDL_SetRenderDrawColor(_window_renderer, 0xEE, 0xEE, 0xEE, 0xFF);
      }
      fillCircle(_switches_rect[i]);
    }
  }

  void drawLeds() {
    SDL_Rect rect{0, 0, 15, 15};
    SDL_Rect start{50, 90, 250, 200};
    constexpr int rows = 2;
    constexpr int cols = kSwitches / 2;
    for (int row = 0; row < rows; ++row) {
      for (int col = 0; col < cols; ++col) {
        auto& led = _leds[row * cols + col];
        SDL_SetRenderDrawColor(_window_renderer, led.r, led.g, led.b, led.a);
        rect.x = start.x + (col * start.w);
        rect.y = start.y + (row * start.h);
        fillCircle(rect);
      }
    }
  }

  void fillCircle(const SDL_Rect& rect) {
    int a = rect.w / 2;
    int b = rect.h / 2;
    int b2 = (b * b);
    int b2a2 = b2 / (a * a);
    for (int x = 0; x < a; ++x) {
      int y = sqrt(b2 - (x * x * b2a2));
      SDL_RenderDrawLine(_window_renderer, rect.x + a + x, rect.y + b - y, rect.x + a + x, rect.y + b + y);
      SDL_RenderDrawLine(_window_renderer, rect.x + a - x, rect.y + b - y, rect.x + a - x, rect.y + b + y);
    }
  }

  SDL_Window *_window;
  SDL_Renderer *_window_renderer;
  SDL_Event _window_event;
  SDL_Texture *_display_texture;
  uint32_t _display_buffer[kDisplayRows * kDisplayColumns]{};
  DisplaySim& _display_sim;
  std::array<SDL_Rect, kSwitches> _switches_rect;
  std::bitset<kSwitches> _switches_state{};
  bool _switches_changed = false;
  std::array<SDL_Color, kSwitches> _leds;
};

}