#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <time.h> 

//Highscore saving
#include <iostream>
#include <fstream>

#pragma once

using namespace std;

enum GameFaces{
	DEFAULT,
	WON,
	CONTINUE,
	LOST,
	KEY_TOTAL
};

//Main functions
bool init(void);
void close(void);
void update(void);
void updateRender(void);
void loadGeometry(void);
bool loadMedia(void);
void logDebug(string text);
void logDebug(const char* text);

void move(SDL_Keycode code);
void loss(void);
bool reset(void);
void setScore(int score);
void loadScore(void);
void saveScore(void);
void createFile(void);
void updateTopScore(void);



//util functions
SDL_Surface* loadSurface(string path);
SDL_Texture* loadTexture(string path);
void generateNewBlock(void);
void generateField(void);

//Screen dimension constants
const int SCREEN_WIDTH = 480;
const int SCREEN_HEIGHT = 680;

//Other stats
const char* GAMENAME = "2048 - The Game";
const bool DEBUG = false;
const int SIZE = 3;

int STARTY = SCREEN_HEIGHT / 5 * 2, x = 0, y = 0;
int smallest = SCREEN_WIDTH < SCREEN_HEIGHT - STARTY ? SCREEN_WIDTH : SCREEN_HEIGHT - STARTY;
int INBETWEEN = smallest/SIZE/8;
int BLOCKSIZE = (smallest - (SIZE+1)*INBETWEEN) / SIZE;
int STARTX = ((SCREEN_WIDTH - (INBETWEEN * (SIZE))) - BLOCKSIZE*SIZE) / 2;

int T_HEIGHT = SCREEN_WIDTH / 9;
int WIDTH = SCREEN_WIDTH / 5;

const int null = NULL;

SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;
SDL_Texture* gTexture = nullptr;

SDL_Texture* gameFaces[KEY_TOTAL];

//Globally used font
TTF_Font *gFont = nullptr;
SDL_Color black = {0,0,0};

//global stats
unsigned long long score = 0;
unsigned long long topScore = 0;
bool lost = false;
string FILENAME = "stats.txt";

class Color{
	public:
	virtual ~Color() {}

	int red,green,blue,alpha;
		Color(int r, int g, int b, int a) : red(r), green(g), blue(b), alpha(a){}
		virtual ostream& dump(ostream& o) {return o << "Color: red=" << red << " green="  << green << " blue="  << blue << " alpha="  << alpha << "; ";}
		SDL_Color sdl() {SDL_Color color = {red, green, blue};return color;}
};

class Drawable {
	public:
		Color* color;
		virtual ~Drawable() {}

		explicit Drawable(Color* c) : color(c){}
		virtual void draw() {SDL_SetRenderDrawColor(gRenderer, color->red, color->green, color->blue, color->alpha);}
		virtual ostream& dump(ostream& o) {return o << "Drawable: " << color->dump(o) << "; ";}
};

class NamedDraw {
	public:
		Drawable* drawable;
		string name;
		NamedDraw(Drawable* d, string n): name(n), drawable(d){};
};

/**#########  RECT  #########**/

class Rect : public Drawable {
	public:
		SDL_Rect* rectangle;
		Rect(SDL_Rect* rect, Color* c) : Drawable(c), rectangle(rect) {}
		void draw() override {Drawable::draw(); SDL_RenderFillRect( gRenderer, rectangle);};
		virtual ostream& dump(ostream& o) {return o << "Rect: h=" << rectangle->h << " w=" << rectangle->w << " x=" << rectangle->x << " y=" << rectangle->y << "; " << Drawable::dump(o);}
};

class Text : public Rect {
	public: 
		string message;
		Text(SDL_Rect* rect, Color* c, string text) : Rect(rect, c), message(text) {}
		void draw() override {
			SDL_Surface* textSurface = TTF_RenderText_Solid( gFont, message.c_str(), color->sdl());
			SDL_Texture* text = SDL_CreateTextureFromSurface(gRenderer, textSurface);
			SDL_RenderCopy(gRenderer, text, nullptr, rectangle);
		};
		virtual ostream& dump(ostream& o) {return o << "Text: text=" << message << "; " << Rect::dump(o);}
};

class Field : public Rect {
	public:
		bool active;
		int value;

		Field(SDL_Rect* rect, Color* c) : Rect(rect, c), value(1), active(false){}
		void draw() override {
			Rect::draw(); /*render text number*/
			
			if (active){
				SDL_Surface* textSurface = TTF_RenderText_Solid( gFont, to_string(value).c_str(), black);
				SDL_Texture* text = SDL_CreateTextureFromSurface(gRenderer, textSurface);
				SDL_RenderCopy(gRenderer, text, nullptr, rectangle);
			}
		};
		void reDouble() {
			if (value == 0) {
				value = 1;
			}
			if (value+value > INT_MAX || value+value < 1){ return;}
			int newscore = score + value;
			setScore(newscore); 
			value+=value; 
			color->red=(color->red+20)%255; color->green=(color->green+20)%255; color->blue=(color->blue+20)%255;

			};
		virtual ostream& dump(ostream& o) {return o << "Field: value=" << value << "; " << Rect::dump(o);}
};

class Button : public Rect {
	bool (*function)(void);
	Rect* rect;
public:
	Button(Rect* r, bool (* func)(void)) : Rect(r->rectangle, r->color), function(func), rect(r){}
	void draw() override {
		rect->draw();
	}
	bool check(SDL_MouseMotionEvent m) {
		return (rectangle->x <= m.x && rectangle->y <= m.y && rectangle->x+rectangle->w > m.x && rectangle->y+rectangle->h > m.y);
	}

	bool press() {
		logDebug("pressed button\n");
		return function();
	}
};

class PlayField{
	public:
	vector<vector<Drawable*> *> fields;
	PlayField() {fields.resize(0);}
		void set(int x, int y, Drawable* d) {
			checkVec(x); 
			vector<Drawable*> *field = fields.at(x);
			field->resize(y+1);
			field->at(y) = (d); 
		}

		Drawable* get(int x, int y) {
			return fields.at(x)->at(y);
		}
		
		void checkVec(int size) {
			int s = fields.size();
			if (size+1 > s){
				fields.resize(size+1);
				for (int i = s; i < size+1; i++){
					if (fields.at(i) == nullptr) {
						vector<Drawable*> *v = new vector<Drawable*>;
						v->resize(1);
						fields.at(i) = v;
					}
				}			
			}
		}

		void print() {
			if (!DEBUG) return;

			int x=0,y=0;
			while (x < fields.size()){
				vector<Drawable*> *v = fields.at(x);
				cout << x << " - ";
				if (v->empty()) {
					cout << "empty";
				} else { 
					while (y < v->size()) {
						Drawable *d = v->at(y);
						cout << y << "(" << d << ")";
						y++;
					}
				}
				x++; y=0; cout <<"\n";
			}
		}
};

NamedDraw* getDrawable(string name);

ostream& operator<<(ostream& o, Field& f) { return f.dump(o); }
ostream& operator<<(ostream& o, Color& f) { return f.dump(o); }
ostream& operator<<(ostream& o, Drawable& f) { return f.dump(o); }
ostream& operator<<(ostream& o, Rect& f) { return f.dump(o); }

//game variables
PlayField* field = new PlayField();
vector<NamedDraw*> rest;

/**#########  REST  #########**/

int main( int argc, char* args[] ){
	int ret = 0;

	if (!init()){
		ret = -1;
	} else if (!loadMedia()){
		ret = -1;
	} else{
		bool quit = false;

		SDL_Event e;
		gTexture = gameFaces[DEFAULT];
		updateRender();
		
		while(!quit){
			while (SDL_PollEvent(&e) != 0){
				if (e.type == SDL_QUIT){
					logDebug("Registered SDL quit event\n");
					quit = true;
					break;
				} else if (e.type == SDL_MOUSEBUTTONDOWN ) {
					vector<NamedDraw*> Drest = rest;
					for (NamedDraw* d : rest) {
						if (d == nullptr){break;}

						Button* b = dynamic_cast<Button*>(d->drawable);
						if(b == nullptr) continue;
						if (b->check(e.motion)) if (b->press()){break;};
					}
				} else if (e.type == SDL_KEYDOWN){
					logDebug("Registered SDL keypress event\n");
					switch(e.key.keysym.sym){
					case SDLK_ESCAPE:
						quit = true;
						break;
					case SDLK_UP:
					case SDLK_DOWN:
					case SDLK_LEFT:
					case SDLK_RIGHT:
						if (!lost){
							move(e.key.keysym.sym);
							generateNewBlock();
						}
						break;
					default:
						
						break;
					}

					updateRender();
				}
			}
		}
	}

	close();
	SDL_Delay( 2000 );
	return ret;
}

bool init(){
	bool good = true;
	srand(time(nullptr));
	cout << "Loading game... \n";
	if (SDL_Init(SDL_INIT_VIDEO ) < 0){
		printf("cannot init SDL error: %s", SDL_GetError());
		good = false;
	}else{
		logDebug("Initialized SDL\n");
		gWindow = SDL_CreateWindow(GAMENAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (gWindow == NULL){
			printf("cannot create SDL window error: %s", SDL_GetError());
			good = false;
		}else{
			logDebug("Initialized window\n");
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
			if (gRenderer == NULL){
				printf("cannot create SDL renderer error: %s", SDL_GetError());
				good = false;
			}else{
				logDebug("Initialized renderer\n");
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

				//Initialize PNG loading
				int imgFlags = IMG_INIT_PNG;
				if( !( IMG_Init( imgFlags ) & imgFlags ) ){
					printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
					good = false;
				} else {
					logDebug("Initialized image loading\n");

					//Initialize SDL_ttf
					if( TTF_Init() == -1 ){
						printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
						good = false;
					} else {
						logDebug("Initialized ttf text loading\n");
						
						gFont = TTF_OpenFont( "../fonts/arial.ttf", 40 );
						if( gFont == NULL ){
							printf( "Failed to load fast99 font! SDL_ttf Error: %s\n", TTF_GetError() );
							good = false;
						} else {
							logDebug("Loaded fast99 font\n");
						}
					}
				}
			}
		}

		loadScore();
		loadGeometry();
		cout << "Loading done!\n";;
	}

	return good;
}

void loadScore() {
	string line;
	ifstream file;
	file.open (FILENAME, ifstream::in);
	if (file.is_open()){
		logDebug("loaded stats file\n");
		while (!file.eof()){
			getline(file, line);
			int i = atoi(line.c_str());
			topScore = i;
			logDebug("topscore = " + to_string(topScore) + "\n");
		}
    } else {
	  logDebug("Unable to open stats file\n"); 
	}

    file.close();
}

void createFile() {
	ofstream file;
	file.open(FILENAME);
	if (!file) {
		logDebug("created file...\n");
		file.open(FILENAME, ios::out | ios::in);
		file << fflush;
		file.close();
	}
}

void saveScore() {
	ofstream file (FILENAME);
	string line;
	createFile();
	if (file.is_open()){
		file << to_string(topScore);
		logDebug("saving topscore... \n");
    } else {
		logDebug("Unable to open stats file\n"); 
	}

    //file << fflush;
    file.close();
}

void move(SDL_Keycode code) {
	vector<Drawable*>::iterator it;


	int x=0, y=0;
	while (x < field->fields.size()){
		vector<Drawable*> *v = field->fields.at(x);
		while(y < v->size()){
			Drawable *d = v->at(y);
			Field* f = static_cast<Field*>(field->get(x, y));
			if (f != NULL) {
				if (f->active)f->reDouble();
			}
			y++;
		}
		x++; y=0;
	}

	
}

void loadGeometry() {
	generateField();
	generateNewBlock();

	//square around squares
	Color* c = new Color(51, 51, 51, 0);
	SDL_Rect* rect = new SDL_Rect;
	int size = BLOCKSIZE*SIZE + INBETWEEN*(SIZE+1);
	rect->x = STARTX; rect->y = STARTY; rect->w = size; rect->h = size;

	Drawable* f = new Rect(rect, c);
	rest.push_back(new NamedDraw(f, "big_square"));

	//2 smaller squares for scores
	rect = new SDL_Rect;
	rect->x = STARTX + WIDTH*2.2; rect->y = STARTX; rect->w = WIDTH; rect->h = T_HEIGHT;
	f = new Rect(rect, c);
	rest.push_back(new NamedDraw(f, "score_square"));

	rect = new SDL_Rect;
	rect->x = STARTX +  WIDTH*2.2 + SCREEN_WIDTH / 4; rect->y = STARTX; rect->w = WIDTH; rect->h = T_HEIGHT;
	f = new Rect(rect, c);
	rest.push_back(new NamedDraw(f, "highscore_square"));

	//2 text in smaller square
	c = new Color(215, 199, 199, 0);
	rect = new SDL_Rect;
	rect->x = STARTX + WIDTH*2.4; rect->y = STARTX; rect->w = T_HEIGHT; rect->h = T_HEIGHT/2;
	rest.push_back(new NamedDraw(new Text(rect, c, "Score"), "scoreText"));

	rect = new SDL_Rect;
	rect->x = STARTX + WIDTH*2.4; rect->y = STARTX +T_HEIGHT/2; rect->w = T_HEIGHT; rect->h = T_HEIGHT/2;
	rest.push_back(new NamedDraw(new Text(rect, c, to_string(score)), "score"));

	rect = new SDL_Rect;
	rect->x = STARTX +  WIDTH*2.4 + SCREEN_WIDTH / 4; rect->y = STARTX; rect->w = T_HEIGHT; rect->h = T_HEIGHT/2;
	rest.push_back(new NamedDraw(new Text(rect, c, "Best"), "topscoreText"));

	rect = new SDL_Rect;
	rect->x = STARTX +  WIDTH*2.4 + SCREEN_WIDTH / 4; rect->y = STARTX + T_HEIGHT/2; rect->w = T_HEIGHT; rect->h = T_HEIGHT/2;
	rest.push_back(new NamedDraw(new Text(rect, c, topScore == 0 ? "-" : to_string(topScore)), "topscore"));

	//text
	c = new Color(225, 209, 209, 0);
	rect = new SDL_Rect;
	rect->x = STARTX; rect->y = STARTX; rect->w = WIDTH*2; rect->h = T_HEIGHT;
	rest.push_back(new NamedDraw(new Text(rect, c, "2048"), "title"));

	rect = new SDL_Rect;
	rect->x = STARTX; rect->y = STARTX + 55; rect->w = SCREEN_WIDTH/4; rect->h = 10;
	rest.push_back(new NamedDraw(new Text(rect, c, "Practice mode: "), "mode"));

	c = new Color(215, 199, 199, 0);
	rect = new SDL_Rect;
	rect->x = STARTX + SCREEN_WIDTH/4; rect->y = STARTX + 55; rect->w = SCREEN_WIDTH/2; rect->h = 10;
	rest.push_back(new NamedDraw(new Text(rect, c, "You have the option to undo your last move."), "mode_explained"));
}

void updateTopScore() {
	NamedDraw *topscore = getDrawable("topscore");
	if (topscore == nullptr) return;

	Text *drawable = dynamic_cast<Text*>(topscore->drawable);
	if (drawable == nullptr) return;

	int oldScore = atoi(drawable->message.c_str());
	if (oldScore >= topScore) return;

	logDebug("updated topscore\n");
	drawable->message = to_string(topScore);
	updateRender();
}

NamedDraw* getDrawable(string name) {
	for (NamedDraw* n : rest) {
		if (n->name == name) return n;
	}
	return nullptr;
}

void generateField() {
	int x=0,y=0;
	for (int w = 0; w<SIZE; w++) {
		for (int h = 0; h<SIZE; h++) {
			Color* c = new Color(113, 145, 162, 0);
			SDL_Rect* rect = new SDL_Rect;
			rect->x = STARTX + INBETWEEN + x*(BLOCKSIZE+INBETWEEN); rect->y = STARTY + INBETWEEN + y*(BLOCKSIZE+INBETWEEN); rect->w = BLOCKSIZE; rect->h = BLOCKSIZE;

			Field* f;
			f = new Field(rect, c);
			field->set(x, y, f);
			y++;
		}
		x++; y=0;
	}
}

void generateNewBlock() {
	srand(time(nullptr));
	int x = rand() % SIZE;
	int y = rand() % SIZE;
	Field* d = static_cast<Field*>(field->get(x, y));

	int tries = 0;
	while (d->active){
		if (tries > 100){ loss(); return;}
		x = rand() % SIZE;
		y = rand() % SIZE; 
		d = static_cast<Field*>(field->get(x, y));
		tries++;
	}

	d->active = true;
}

void loss() {
	lost = true;
	if (topScore < score) topScore = score;
	updateTopScore();

	Color *c = new Color(214, 32, 30, 0);
	SDL_Rect *rect = new SDL_Rect;
	rect->x = SCREEN_WIDTH/4; rect->y = SCREEN_HEIGHT/2; rect->w = SCREEN_WIDTH/2; rect->h = SCREEN_WIDTH/4;
	rest.push_back(new NamedDraw(new Text(rect, c, ("You LOST! Score " + to_string(score))), "lost"));

	c = new Color(30, 214, 66, 0);
	rect = new SDL_Rect;
	rect->x = SCREEN_WIDTH/4; rect->y = SCREEN_HEIGHT/2 + SCREEN_WIDTH/4; rect->w = SCREEN_WIDTH/2; rect->h = SCREEN_WIDTH/4;
	rest.push_back(new NamedDraw(new Button(new Text(rect, c, ("Try again? ")), reset), "tryagain"));
}

bool reset() {
	setScore(0);
	lost = false;
	
	rest.clear();
	loadGeometry();

	field->fields.clear();
	generateField();
	generateNewBlock();
	updateRender();
	
	return true;
}

void setScore(int newscore) {
	logDebug("score: " + to_string(newscore) + "\n");
	score = newscore; 

	//scoreText->message = to_string(newscore);
	NamedDraw *score = getDrawable("score");
	if (score == nullptr) return;

	Text *drawable = dynamic_cast<Text*>(score->drawable);
	if (drawable == nullptr) return;

	drawable->message = to_string(newscore);
	updateRender();
}

bool loadMedia(){
	bool good = true;
	logDebug("Loading media...\n");

	string image = "../images/image";
	GameFaces faces[] = {DEFAULT, WON, CONTINUE, LOST};

	string imagename = "";
	string logger = "Loaded media image ";

	int i=0;
	for each (GameFaces face in faces){
		imagename = image;
		imagename.append(to_string(i));
		imagename.append(".bmp");

		logDebug(logger+imagename+"\n");
		gameFaces[face] = loadTexture(imagename);
		if (gameFaces[face] == NULL){
			printf("Loading of image %s failed!\n", imagename.c_str());
			good = false;
		}

		i++;
	}
	logDebug("Loading media done!\n");
	return good;
}

void close(){
	cout << "Stopping game!";
	saveScore();

	SDL_DestroyTexture(gTexture);
	gTexture = nullptr;

	SDL_DestroyWindow(gWindow);
	gWindow = nullptr;

	 //Free global font
    TTF_CloseFont( gFont );
    gFont = nullptr;

	SDL_Quit();
	TTF_Quit();
    IMG_Quit();
}

void logDebug(string text){
	logDebug(text.c_str());
}
 
void logDebug(const char* text){
	if (DEBUG) cout << text;
}

void updateRender(){
	SDL_RenderClear(gRenderer);
	SDL_RenderCopy(gRenderer, gTexture, nullptr, nullptr);
	
	for (NamedDraw* n : rest) {
		n->drawable->draw();
	}

	if (!lost){
		int x=0, y=0;
		while (x < field->fields.size()){
			vector<Drawable*> *v = field->fields.at(x);
			while(y < v->size()){
				Drawable *d = v->at(y);
				if (d != nullptr) {
					d->draw();
				}
				y++;
			}
			x++; y=0;
		}
	}

	update();
	logDebug("updated renderer\n");
}

void update(){
	SDL_RenderPresent(gRenderer);
	logDebug("updated the screen\n");
}

SDL_Texture* loadTexture(string path){
	SDL_Texture* newTexture = nullptr;
	SDL_Surface* loadedSurface = loadSurface(path);
	
	if (loadedSurface != NULL){
		newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
		if (newTexture == NULL){
			printf("Unable to create Texture from %s! Error: %s\n", path.c_str(), SDL_GetError());
		} else {
			logDebug(string("loading of texture image ")+path.c_str()+" done\n");
		}

		SDL_FreeSurface(loadedSurface);
	} else {
		printf("loading of image %s failed\n", path.c_str());
	}

	return newTexture;
}

SDL_Surface* loadSurface(string path){
	SDL_Surface* surface = IMG_Load(path.c_str());
	if (surface == NULL){
		printf("Unable to load image %s, reason: %s\n", path.c_str(), SDL_GetError());
	} else{
		logDebug(string("loading of surface image ")+path.c_str()+" done\n");
	}

	return surface;
}