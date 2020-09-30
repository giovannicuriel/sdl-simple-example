#include <iostream>
#include <SDL.h>

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const int FPS = 24;

struct State {
  int vx;
  int vy;
};

struct Dimension {
  SDL_Point min;
  SDL_Point max;
};

bool operator<=(const SDL_Point & o, const Dimension & d) {
  return (o.x >= d.min.x) &&
    (o.x <= d.max.x) &&
    (o.y >= d.min.y) &&
    (o.y <= d.max.y);
}

bool operator<=(const SDL_Rect & o, const Dimension & d) {
  return (o.x >= d.min.x) &&
    (o.x + o.w <= d.max.x) &&
    (o.y >= d.min.y) &&
    (o.y + o.h <= d.max.y);
}

std::ostream & operator<<(std::ostream & out, const SDL_Rect & r) {
  out << "(" << r.x << "," << r.y << "-" << r.w << "," << r.h << ")";
  return out;
}

std::ostream & operator<<(std::ostream & out, const SDL_Point & r) {
  out << "(" << r.x << "," << r.y << ")";
  return out;
}

std::ostream & operator<<(std::ostream & out, const Dimension & d) {
  out << "<" << d.min << "," << d.max << ">";
  return out;
}

class WorldObject {
  SDL_Rect tangible;
  SDL_Rect nextTangible;
  State state;
public:
  WorldObject() : tangible(), state() {
    tangible.h = 100;
    tangible.w = 100;
    tangible.x = 100;
    tangible.y = 100;
    nextTangible = tangible;
  }
  const SDL_Rect & getTangible() { return tangible; }
  const SDL_Rect & getNextTangible() { return nextTangible; }

  void addSpeed(State s) {
    state.vx += s.vx;
    state.vy += s.vy;
    // std::cout << "New speed is:" << state.vx << "," << state.vy << "\n";
  }

  void flipSpeedWithDumping(const Dimension & d, float dump) {
    if ((nextTangible.x < d.min.x) || (nextTangible.x + nextTangible.w > d.max.x)) {
      state.vx *= -dump;
    }
    if ((nextTangible.y < d.min.y) || (nextTangible.y + nextTangible.h > d.max.y)) {
      state.vy *= -dump;
    }
  }
  void setSpeed(State s) {
    state.vx = s.vx;
    state.vy = s.vy;
    // std::cout << "Forced speed is:" << state.vx << "," << state.vy << "\n";
  }
  void setXSpeed(State s) {
    state.vx = s.vx;
    // std::cout << "Forced speed is:" << state.vx << "," << state.vy << "\n";
  }
  void setYSpeed(State s) {
    state.vy = s.vy;
    // std::cout << "Forced speed is:" << state.vx << "," << state.vy << "\n";
  }
  void computeNextState() {
    nextTangible.x += state.vx / FPS;
    nextTangible.y += state.vy / FPS;

    // std::cout << "Next tangible is " << nextTangible << "\n";
  }
  void commitNextState() {
    tangible = nextTangible;
  }
  void rollbackNextState() {
    nextTangible = tangible;
  }
};

class Gravity {
  State state;
public:
  Gravity() {
    state.vy = 30;
    state.vx = 0;
  }
  void actuate(WorldObject & obj) {
    obj.addSpeed(state);
  }
};

class Floor {
  Dimension * d;
public:
  Floor(Dimension * d) {
    this->d = d;
  }
  void actuate(WorldObject & obj) {
    if (!(obj.getNextTangible() <= (*d))) {
      obj.flipSpeedWithDumping(*d, 0.8);
    }
  }
};
class Window {
  SDL_Window * window;
  SDL_Surface * screenSurface;
  bool * shouldExit;
  WorldObject obj;
  Gravity g;
  Floor f;
  Dimension d;
  //The window renderer
  SDL_Renderer* gRenderer;
public:
  Window(bool * shouldExit): f(&d) {
    this->shouldExit = shouldExit;
    this->window = nullptr;
    this->screenSurface = nullptr;
    d.min.x = 0;
    d.min.y = 0;
    gRenderer = nullptr;
  }

  void init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      std::cerr << "Could not initialize video: " << SDL_GetError() << "\n";
      return;
    }

    this->window = SDL_CreateWindow("SDL Tutorial",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      SCREEN_WIDTH,
      SCREEN_HEIGHT,
      SDL_WINDOW_SHOWN);

    if (this->window == nullptr) {
      std::cerr << "Could not create window: " << SDL_GetError() << "\n";
      return;
    }

    // this->screenSurface = SDL_GetWindowSurface(this->window);
    SDL_GetWindowSize(this->window, &d.max.x, &d.max.y);
    gRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
  }

  void processKeyboardEvent(SDL_Event & ev) {
    State s = State();
    switch (ev.key.keysym.scancode) {
      case SDL_SCANCODE_UP:
        s.vx = 0;
        s.vy = -400;
        obj.setYSpeed(s);
      break;
      case SDL_SCANCODE_LEFT:
        s.vx = -400;
        s.vy = 0;
        obj.setXSpeed(s);
      break;
      case SDL_SCANCODE_RIGHT:
        s.vx = 400;
        s.vy = 0;
        obj.setXSpeed(s);
      break;
      default:
      break;
    }
  }

  void loop() {
    while (!(*shouldExit)) {
      this->update();
      // SDL_Delay(1000 / FPS);
      std::cout << "Next frame\n";
    }
  }
  void update() {
    // std::cout << "Window dimensions: " << d << "\n";
    g.actuate(obj);
    obj.computeNextState();
    f.actuate(obj);
    if (obj.getNextTangible() <= d) {
      obj.commitNextState();
    } else {
      obj.rollbackNextState();
    }

    // Clear screen
    SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xff, 0xFF );  
    SDL_RenderClear( gRenderer );
    SDL_SetRenderDrawColor( gRenderer, 0xFF, 0x00, 0x00, 0xFF );
    SDL_RenderFillRect( gRenderer, &obj.getTangible() );

    // Update screen
    SDL_RenderPresent( gRenderer );
  }

  ~Window() {

    SDL_DestroyWindow(this->window);
    SDL_DestroyRenderer( gRenderer );
    SDL_Quit();
  }
};

int mainLoop(void * data) {
  Window * window = (Window *) data;
  window->loop();
  return 0;
};

int main(void) {
  bool shouldExit = false;
  Window window = Window(&shouldExit);
  window.init();
  SDL_Thread * mainThread = SDL_CreateThread(mainLoop, "MainWorldLoop", (void *)&window);

  SDL_Event event;
  while (!shouldExit) {
    while (SDL_PollEvent(&event) != 0) {
      // std::cout << "Received an event: " << event.type << "\n";
      switch (event.type) {
        case SDL_QUIT: 
          shouldExit = true;
          break;
        case SDL_KEYUP:
          window.processKeyboardEvent(event);
          break;
      }
    }
  }
	return 0;
}
