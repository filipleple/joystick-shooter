#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>


using namespace std;

#define SERIAL_PORT_PATH "/dev/ttyACM0"

void countTime(int &TIME){
    while (true){
        TIME+=20;
        this_thread::sleep_for(chrono::milliseconds(20));        
    }
}

enum ControlModes{
    KEYBOARD,
    CONTROLLER
};

enum ProgramModes{
    MAIN_MENU,
    MAIN_GAME,
    PAUSE_MENU,
    SCOREBOARD
};


const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;


//take these out into MainMenu variables
const int WALL_GAP_HEIGHT = 220;
const int WALL_WIDTH = 30;
const int ENV_SPEED = 1;
const int BULLET_SPEED = 5;


class Walls{
    
    static const int gapHeight = WALL_GAP_HEIGHT;
    static const int width = WALL_WIDTH;
    static const int wallSpeed = ENV_SPEED;
    
    int gapTopYpos;
    int heightTop;
    int heightBott;

    public: // how about making it all private and have the MainGame as friend clasS?
    SDL_Rect topRect;
    SDL_Rect bottRect;

    Walls(){
        gapTopYpos = rand()%(SCREEN_HEIGHT-gapHeight);
        heightTop = gapTopYpos;        
        heightBott = SCREEN_HEIGHT - (gapTopYpos+gapHeight);

        topRect = {SCREEN_WIDTH-1, 0, width, heightTop};
        bottRect = {SCREEN_WIDTH-1, gapTopYpos+gapHeight, width, heightBott};
    }

    void render(SDL_Renderer* renderer){
        SDL_SetRenderDrawColor( renderer, 0x00, 0x00, 0x00, 0xFF );
        SDL_RenderFillRect(renderer, &topRect);
        SDL_RenderFillRect(renderer, &bottRect);

    }

    void move(){
        topRect.x -= wallSpeed;
        bottRect.x -= wallSpeed; 
    }

};

class Enemy{    
    static const int speed = 1;
    static const int Xspeed = ENV_SPEED;

    int dir = 1;    
    int topLimit, bottLimit;

    public:
    SDL_Rect rect;

    Enemy(int top, int bot){
        rect = {SCREEN_WIDTH-1, 200, 20, 20};
        rect.y = top;
        topLimit = top;
        bottLimit = bot - rect.h;
    }

    void render(SDL_Renderer* renderer){
        SDL_SetRenderDrawColor( renderer, 0xFF, 0x00, 0x00, 0xFF );
        SDL_RenderFillRect(renderer, &rect);
    }

    void move(){
        rect.y += speed*dir;
        if (rect.y < topLimit || rect.y > bottLimit){
            dir = -dir;
        }
        rect.x -= Xspeed;
    }
};


class Bullet{    
    int width = 5;
    int height = 5;
    int speed = BULLET_SPEED;
    int dir;

    public:

    SDL_Rect rect;

    Bullet(int player_x, int player_y, int dir_given){
        rect = {player_x, player_y, width, height};
        dir = dir_given;
    }

    void render(SDL_Renderer* renderer){
        SDL_SetRenderDrawColor( renderer, 0x00, 0x00, 0x00, 0xFF );
        SDL_RenderFillRect(renderer, &rect);
    } 

    void move(){
        rect.x += dir*speed;
    }   
};

class Player{    
    ControlModes controlMode;
    

    public:
    SDL_Rect rect;

    Player(ControlModes controlMode, int &controllerY, int xPos, int yPos, int W, int H){
        this->controlMode = controlMode;
        

        rect.x = xPos;
        rect.y = yPos;
        rect.w = W;
        rect.h = H;        

    }

    void move(int &y_tmp){
        if(controlMode == KEYBOARD){
            //controlled via maingame::handleEvent
        }

        else if (controlMode == CONTROLLER){
            rect.y = y_tmp;
        }
    }

    void render(SDL_Renderer* renderer){
        SDL_SetRenderDrawColor( renderer, 0x00, 0x00, 0x00, 0xFF );
        SDL_RenderFillRect(renderer, &rect);
    }

    void fire(int &TIME, int &lastFireRequestTime, vector<Bullet> &bullets, int xpos, int ypos){
        if (abs(TIME%1000 - lastFireRequestTime) > 60){
            lastFireRequestTime = TIME;
            cout << "F!!" << endl;        
            bullets.push_back({xpos, ypos, 1});
        }
    }
};

class Text{
    TTF_Font *font;
    SDL_Color color {0, 0, 100};
    SDL_Surface* surface = new SDL_Surface;
    SDL_Texture* texture = NULL;
    
    
    int texW = 0;
    int texH = 0;
    string text;

    public:
    int x, y;
    SDL_Rect dstrect;

    Text(int fontsize, int x, int y, string text){
        font = TTF_OpenFont("arial.ttf", fontsize);
        this->x = x;
        this->y = y;

        this->text = text;

    }

    void getText(string text){
        this->text = text;
    }

    void render(SDL_Renderer* renderer){
        char* cstr = new char[text.length()+1];
        strcpy(cstr, text.c_str());

        surface = TTF_RenderText_Solid(font, cstr, color);
        texture = SDL_CreateTextureFromSurface(renderer, surface);

        

        SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);

        dstrect = { x, y, texW, texH };

        if(x==SCREEN_WIDTH/2){
            dstrect.x -= texW/2;
            
        }
        if(y==SCREEN_HEIGHT/2){
            dstrect.y -= texH/2;            
        }

        

        SDL_RenderCopy(renderer, texture, NULL, &dstrect);

        delete cstr;
    }
};

class MainGame{    
    //make it increase with time!
    float wallDensityModifier;
    int enemyDensityModifier = 1;

    int lastFireRequestTime = 0;
    int TIME = 0;

    int y_tmp = 0;

    Text* gameHUD;

    int score = 0;

    vector <Enemy> enemies;
    vector <Walls> walls;
    vector <Bullet> bullets;

    Player* player = nullptr;

    SDL_Renderer* renderer;

    ControlModes controlMode;
    int controllerY;

    ProgramModes nextMode = MAIN_GAME;

    public:

    void reset(ControlModes controlMode, float wallDensityModifier, int &TIME ){
        score = 0;
        TIME = 0;
        enemies.erase(enemies.begin(), enemies.end());
        walls.erase(walls.begin(), walls.end());
        nextMode = MAIN_GAME;

        walls.push_back(Walls());
        enemies.push_back( Enemy(walls[0].topRect.h, walls[0].bottRect.y ) );
    }

    MainGame(SDL_Renderer* renderer, ControlModes controlMode, float wallDensityModifier){
        this->renderer = renderer;
        this->controlMode = controlMode;
        this->wallDensityModifier = wallDensityModifier;

        walls.push_back(Walls());
        enemies.push_back( Enemy(walls[0].topRect.h, walls[0].bottRect.y ) );

        player = new Player(controlMode, controllerY, 20, SCREEN_HEIGHT/2, 10, 10);

        gameHUD = new Text(30, 20, 20, "SCORE: ");
    }

    void render(){
        player->render(renderer);

        for(int i = 0; i < walls.size(); i++){
            walls[i].render(renderer);
        }

        for(int i = 0; i < enemies.size(); i++){
            enemies[i].render(renderer);
        }

        for (int i = 0; i < bullets.size(); i++){
            bullets[i].render(renderer);
        }

        gameHUD->getText("SCORE: "+to_string(score));
        gameHUD->render(renderer);
    }

    int gameLoop(ProgramModes &currentProgramMode, int TIME, int y_tmp){
        this->TIME = TIME;
        this->y_tmp = y_tmp;
        moveEntities();
        collisionDetection();
        render();
        generateObstacles();

        currentProgramMode = nextMode;

        

        return score;
    }

    void handleEvents(SDL_Event e){
        if(e.type == SDL_KEYDOWN){
                switch (e.key.keysym.sym){
                    case SDLK_UP:
                        player->rect.y-=20;
                        break;
                    case SDLK_DOWN:
                        player->rect.y+=20;
                        break;
                    case SDLK_x:
                        player->fire(TIME, lastFireRequestTime, bullets, player->rect.x, player->rect.y);
                        break;
                    case SDLK_ESCAPE:
                        //quit = true;
                        break;
                }
        }
    }

    void collisionDetection(){
        for(int i = 0; i < walls.size(); i++){
            if (SDL_HasIntersection(&player->rect, &walls[i].topRect)||
                SDL_HasIntersection(&player->rect, &walls[i].bottRect) ){
                    cout << "ouch!" << endl;
                    nextMode = SCOREBOARD;
                    break;
                }

            for (int j = 0; j < bullets.size(); j++){
                if(SDL_HasIntersection(&walls[i].topRect, &bullets[j].rect) ||
                   SDL_HasIntersection(&walls[i].bottRect, &bullets[j].rect)  ){
                    bullets.erase(bullets.begin() + j);
                }
            }
        }

        for(int i = 0; i < enemies.size(); i++){
            if (SDL_HasIntersection(&player->rect, &enemies[i].rect)){
                    cout << "ouch!" << endl;
                    nextMode = SCOREBOARD;
                    break;
                }

            
            for (int j = 0; j < bullets.size(); j++){
                if(SDL_HasIntersection(&enemies[i].rect, &bullets[j].rect)){
                    enemies.erase(enemies.begin()+i);
                    score += 20;
                }
            }
            
        }
    }

    void moveEntities(){
        player->move(y_tmp);
        
        for (int i = 0; i < walls.size(); i++){
            if (walls[i].topRect.x == player->rect.x){
                score += 10;
                if(score % 50==0){
                    wallDensityModifier+=0.05;
                }                
            }

            walls[i].move();
        }

        for (int i = 0; i < enemies.size(); i++){
            enemies[i].move();
        }

        for (int i = 0; i < bullets.size(); i++){
            bullets[i].move();
        }

    }

    void generateObstacles(){

        if(walls.back().topRect.x < wallDensityModifier*SCREEN_WIDTH ){
            walls.push_back(Walls());

            if(walls.size()%enemyDensityModifier == 0)
                enemies.push_back(Enemy( walls.back().topRect.h, walls.back().bottRect.y ));
        }

        if(walls[0].topRect.x <= -20){
            walls.erase(walls.begin());
            
        }

        if (enemies[0].rect.x <= -20 ){
            enemies.erase(enemies.begin());
        }
    }
};

class Scoreboard{
    SDL_Renderer* renderer;
    Text* scoreHUD = nullptr;
    Text* restart = nullptr;
    Text* gotoMainMenu = nullptr;

    ProgramModes nextProgramMode = SCOREBOARD;

    string text;
    int score;

    public:
    Scoreboard(SDL_Renderer* renderer, int score){
        
        scoreHUD = new Text(50, SCREEN_WIDTH/2, SCREEN_HEIGHT/2, "SCORE: "+to_string(score) );
        restart = new Text(20, SCREEN_WIDTH/2, SCREEN_HEIGHT/2, "press enter to retry");
        gotoMainMenu = new Text(20, SCREEN_WIDTH/2, SCREEN_HEIGHT/2, "press esc to go to main" );
        
        this->renderer=renderer;
        this->score = score;
    }

    void render(){
        scoreHUD->getText("SCORE: "+to_string(score));
        scoreHUD->render(renderer);

        restart->y = scoreHUD->dstrect.y + scoreHUD->dstrect.h-10;
        restart->render(renderer);

        gotoMainMenu->y = restart->dstrect.y + restart->dstrect.h-10;
        gotoMainMenu->render(renderer);
    }

    void getScore(int score){
        nextProgramMode = SCOREBOARD;
        this->score = score;
    }

    void handleEvents(SDL_Event e){
        switch(e.key.keysym.sym){
            case SDLK_RETURN:
                nextProgramMode = MAIN_GAME;
                break;
        }
    }

    void loop(ProgramModes &currentProgramMode){
        render();
        currentProgramMode = nextProgramMode;
    }
};

class TimerGUI{
    Text* timerTEXT;

    public:

    TimerGUI(){
        timerTEXT = new Text(20, SCREEN_WIDTH/2, 20, "TIME: ");
    }

    void render(SDL_Renderer* renderer, int TIME){
        timerTEXT->getText("TIME: "+ to_string(TIME/1000));
        timerTEXT->render(renderer);
    }
};

class Controller{
    struct termios g_tty;
    int g_fd;

    char* napis;
    uint8_t l_len_buff;

    public:

    Controller(){
        g_fd = open(SERIAL_PORT_PATH, O_RDWR | O_NONBLOCK);
        
        if(g_fd < 0) {
            printf("Something went wrong while opening the port...\r\n");
            exit(EXIT_FAILURE);
        }

        if(tcgetattr(g_fd, &g_tty)) {
            printf("Something went wrong while getting port attributes...\r\n");
            exit(EXIT_FAILURE);
        }

        cfsetispeed(&g_tty,B9600);
        cfsetospeed(&g_tty,B9600);

        cfmakeraw(&g_tty);

        if(tcsetattr(g_fd,TCSANOW,&g_tty)) {
            printf("Something went wrong while setting port attributes...\r\n");
            exit(EXIT_FAILURE);
        }   
        
        sleep(2);
        usleep(10000);
        
    }

    int checkPot(int &y_tmp){
    
        while (1){
            read(g_fd, napis, l_len_buff);

            int x = y_tmp;
            string napisS(napis);

            size_t foundposX, foundposY;

            char wartosc[] = "99999";

            if(napisS.find("f")!=string::npos){
                cout << "F!!" << endl;
                
            }

            foundposX = napisS.find("x");
            foundposY = napisS.find(".", foundposX);

            if (foundposX!=string::npos && foundposY!=string::npos && foundposY > foundposX){
                for (int i = 0, j = foundposX+1; j < foundposY, i < foundposY-foundposX; i++, j++){
                    wartosc[i] = napisS[j];
                }

                wartosc[6] = '\0';

                
                
                x = stoi(wartosc);
                if (x == 0) x = y_tmp;
            }

            usleep(15000);

            memset(napis, 0, l_len_buff);

            y_tmp = x;
                    
            cout << y_tmp << endl;

            //return y_tmp;
        }
    }


    
        

};



int main() {
    SDL_Init( SDL_INIT_VIDEO );
    TTF_Init();

    int TIME = 0;

    SDL_Window* window = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
    SDL_Renderer* renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );

    ProgramModes currentProgramMode = MAIN_GAME;
    ControlModes controlMode = KEYBOARD;

    float wallDensityModifier = 0.5;
    MainGame mainGame(renderer, controlMode, wallDensityModifier);
    int score;

    Scoreboard scoreBoard(renderer, score);

    int y_tmp = 300;
    if(controlMode == CONTROLLER) {    
        Controller controller;
        thread contrThread( [&controller, &y_tmp](){
            controller.checkPot(y_tmp);
            } 
        );

        contrThread.detach();
    }

    thread timerThread (countTime, ref(TIME));
    timerThread.detach();
    TimerGUI timerGUI;

    bool quit = false;
    SDL_Event e;

    while(!quit){
        while(SDL_PollEvent(&e) != 0){
            if( e.type == SDL_QUIT ){
                quit = true;
            }

            switch(currentProgramMode){
                case MAIN_GAME:
                    mainGame.handleEvents(e);
                    break;
                case SCOREBOARD:
                    scoreBoard.handleEvents(e);                    
                    break;    
            }
            
        }
        //clear screen
        SDL_SetRenderDrawColor( renderer, 0xFF, 0xFF, 0xFF, 0xFF );
        SDL_RenderClear( renderer );      
        
        switch(currentProgramMode){
            case MAIN_MENU:
                break;

            case MAIN_GAME:
                score = mainGame.gameLoop(currentProgramMode, TIME, y_tmp);
                scoreBoard.getScore(score);
                break;

            case PAUSE_MENU:
                break;

            case SCOREBOARD:
                scoreBoard.loop(currentProgramMode);
                if (currentProgramMode==MAIN_GAME){
                    mainGame.reset(controlMode, wallDensityModifier, TIME);
                }
                break;
        }

        timerGUI.render(renderer, TIME);


        SDL_Delay(5);
    


        //Update screen
        SDL_RenderPresent( renderer );

    }

    TTF_Quit();
    SDL_Quit();


    return 0;
}
