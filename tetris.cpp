#include <iostream>
#include <string>
#include <fstream>
#include <signal.h>
#include <thread>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <vector>
#include <algorithm>
#include <limits>
#include <chrono>
#include <atomic>

using namespace std;
using namespace std::chrono;

struct ScoreEntry {
    string name;
    int score;
};

vector<ScoreEntry> leaderboard;

void loadLeaderboard() {
    ifstream file("leaderboard.txt");
    if (file.is_open()) {
        ScoreEntry entry;
        while (file >> entry.name >> entry.score) {
            leaderboard.push_back(entry);
        }
        file.close();
    }
}

void saveLeaderboard() {
    ofstream file("leaderboard.txt");
    for (const auto &entry : leaderboard) {
        file << entry.name << " " << entry.score << endl;
    }
    file.close();
}

void displayLeaderboard() {
    cout << "\n==== Leaderboard ====\n";
    for (const auto &entry : leaderboard) {
        cout << entry.name << " - " << entry.score << "\n";
    }
    cout << "====================\n";
}

void updateLeaderboard(string playerName, int score) {
    leaderboard.push_back({playerName, score});
    sort(leaderboard.begin(), leaderboard.end(), [](ScoreEntry a, ScoreEntry b) {
        return a.score > b.score;
    });
    if (leaderboard.size() > 5) leaderboard.pop_back();
    saveLeaderboard();
}

void clearInputBuffer() {
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void gameMenu() {
    int choice;
    while (true) {
        cout << "\n=== TETRIS GAME MENU ===";
        cout << "\n1. Start Game";
        cout << "\n2. View Leaderboard";
        cout << "\n3. Exit";
        cout << "\nChoose an option: ";
        cin >> choice;

        if (cin.fail()) {
            cout << "Invalid input. Please enter a number.\n";
            clearInputBuffer();
            continue;
        }

        switch (choice) {
            case 1:
                return;
            case 2:
                displayLeaderboard();
                break;
            case 3:
                exit(0);
            default:
                cout << "Invalid choice, please select again.\n";
        }
    }
}

bool draw = true;

string drawbuffer = "";
char boardColorIDs[10][20];
char boardColorIDsToDraw[10][20];

bool tetrominoShapes[7][4][4] = {
    {{1, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},  // O
    {{0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}},  // I
    {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},  // S
    {{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},  // Z
    {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}},  // L
    {{0, 0, 1, 0}, {0, 0, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}},  // J
    {{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}   // T
};

// Assign colors based on the given image
int tetrominoColors[7] = {6, 3, 1, 2, 4, 5, 7}; // O = Yellow, I = Cyan, S = Red, Z = Green, L = Orange, J = Pink, T = Purple

bool centersOfRotationX[7] = {0, 1, 1, 1, 1, 1, 1};
bool centersOfRotationY[7] = {0, 1, 1, 1, 1, 1, 1};

int X = 3;
int Y = 0;
bool currentTetromino[4][4];
bool nextTetromino[4][4];
char currentColor = 1;
char nextColor = 1;
int tetrominoID = 0;
int nextTetrominoID = 0;
int score = 0;
bool spawnNewTetromino = true;
bool is_GameOver = false;
bool is_Paused = false;

int centerOfRotationX = 0;
int centerOfRotationY = 0;

int highscore = 0;
string topPadding = "";
string leftPadding = "";

char input_raw[3] = {0};
const string highscoreFile = string(getpwuid(getuid())->pw_dir) + "/.local/share/Tetris highscore";
struct termios old;
atomic<bool> fall(false);

void newTetromino() {
    X = 3; 
    Y = 0;

    if (!is_GameOver) {
        currentColor = tetrominoColors[nextTetrominoID]; // Use predefined color
        tetrominoID = nextTetrominoID;
        nextTetrominoID = (nextTetrominoID + 1) % 7; // Sequential selection instead of random
        nextColor = tetrominoColors[nextTetrominoID]; // Assign fixed color based on ID
        
        centerOfRotationX = centersOfRotationX[tetrominoID];
        centerOfRotationY = centersOfRotationY[tetrominoID];

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                currentTetromino[i][j] = nextTetromino[i][j];
                nextTetromino[i][j] = tetrominoShapes[nextTetrominoID][j][i];
            }
        }
    }
    spawnNewTetromino = false;
    draw = true;
}

void Input() {
    while (true) {
        char tmp_input[3] = {0};
        read(0, &tmp_input, 3);
        if (tmp_input[0] == '\e') {
            if (tmp_input[1] == '[') {
                if (64 < tmp_input[2] && tmp_input[2] < 69) {
                    tmp_input[0] = tmp_input[2] - 48;
                } else { tmp_input[0] = 0; }
            }
        }
        input_raw[0] = tmp_input[0];
        input_raw[1] = tmp_input[1];
        input_raw[2] = tmp_input[2];
    }
}

void Solidify() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (currentTetromino[j][i]) { boardColorIDs[j + X][i + Y] = currentColor; }
        }
    }

    char NewColor[10][20];
    int FirstY = 0;
    for (int i = Y; i < 20; i++) {
        bool Full = true;
        for (int j = 0; j < 10; j++) { if (boardColorIDs[j][i] == 0) { Full = false; } }

        if (Full) {
            for (int j = 0; j < 10; j++) { NewColor[j][FirstY] = 0; }
            for (int j = 0; j < 10; j++) {
                for (int k = 0; k < i; k++) {
                    NewColor[j][k + 1] = boardColorIDs[j][k];
                    boardColorIDs[j][k] = NewColor[j][k];
                }
                boardColorIDs[j][i] = NewColor[j][i];
            }
            FirstY++;
            score++;
        }
    }
}

bool CheckCollision(int x, int y) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int jx = j + x;
            int iy = i + y;
            if (currentTetromino[j][i] && (jx > 9 || jx < 0 || iy > 19 || iy < 0 || boardColorIDs[j + x][i + y] != 0)) { return true; }
        }
    }
    return false;
}

void Fall() {
    while (true) {
        this_thread::sleep_for(milliseconds(1000 - min(score * 20, 900)));
        if (!is_GameOver && !is_Paused) { fall = true; }
    }
}

void resizeHandler(int sig) {
    system("clear");
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    topPadding = string((w.ws_row - 22) / 2, '\n');
    leftPadding = string((w.ws_col - 22) / 2, ' ');
    draw = true;
}

void signalHandler(int signum) {
    tcsetattr(0, TCSANOW, &old);
    system("clear");
    cout << "\e[?25h" << flush;
    exit(signum);
}

int main(int argc, const char *argv[]) {
    loadLeaderboard();
    gameMenu();

    string playerName;
    cout << "Enter your name: ";
    cin >> playerName;

    int playerScore = 0;
    updateLeaderboard(playerName, playerScore);

    if (argc != 1) {
        if (argc > 2) { cout << "Too many arguments" << endl; return 0; }
        else if (argv[1] == "-h" || argv[1] == "--help") {
            cout << "Simple Tetris Game\n"
                 << "  Controls:\n"
                 << "   h/j/l, Arrow Left/Down/Right, n/e/o - Move left/down/right\n"
                 << "   p,ESC - is_Paused/Unpause\n"
                 << "   SPACE - Unpause/Restart game after game over" << endl;
            return 0;
        } else if (argc != 1) { cout << "Unknown argument: " + string(argv[1]) << endl; return 0; }
    }
    srand(time(0));

    ifstream Tmp(highscoreFile);
    Tmp >> highscore;
    Tmp.close();

    signal(SIGWINCH, resizeHandler);
    signal(SIGINT, signalHandler);

    resizeHandler(SIGWINCH);

    nextColor = tetrominoColors[nextTetrominoID]; // Use fixed colors
	currentColor = tetrominoColors[tetrominoID]; // Use fixed colors
    tetrominoID = rand() % 7;
    nextTetrominoID = rand() % 7;
    centerOfRotationX = centersOfRotationX[tetrominoID];
    centerOfRotationY = centersOfRotationY[tetrominoID];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            currentTetromino[i][j] = tetrominoShapes[tetrominoID][j][i];
            nextTetromino[i][j] = tetrominoShapes[nextTetrominoID][j][i];
        }
    }

    struct termios current;
    tcgetattr(0, &old);
    current = old;
    current.c_lflag &= ~ICANON;
    current.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &current);

    thread(Fall).detach();
    thread(Input).detach();
    system("clear");
    while (true) {
        char input[3] = {input_raw[0], input_raw[1], input_raw[2]};
        input_raw[0] = 0;
        if (input[0] == '\e' || input[0] == 'p') { is_Paused = !is_Paused; draw = true; }
        bool S = false;

        if (!is_GameOver && !is_Paused) {
            if (fall) {
                if (CheckCollision(X, Y + 1)) {
                    S = true;
                    spawnNewTetromino = true;
                } else {
                    Y++;
                }
                draw = true;
                fall = false;
            }

            if (input[0] == 'h' || input[0] == 'n' || input[0] == 20) {
                if (!CheckCollision(X - 1, Y)) { X--; draw = true; }
            } else if (input[0] == 'j' || input[0] == 'e' || input[0] == 18) {
                if (CheckCollision(X, Y + 1)) {
                    S = true;
                    spawnNewTetromino = true;
                    draw = true;
                } else { Y++; draw = true; }
            } else if (input[0] == 'k' || input[0] == 'i' || input[0] == 17) {
                if (tetrominoID != 0) {
                    int cX = centerOfRotationX;
                    int cY = centerOfRotationY;
                    int x = X;
                    int y = Y;
                    bool TmpT[4][4];
                    bool Safe = true;
                    cX = 3 - centerOfRotationY; x -= cX - centerOfRotationX;
                    cY = centerOfRotationX; y -= cY - centerOfRotationY;
                    for (int i = 0; i < 4; i++) {
                        for (int j = 0; j < 4; j++) {
                            int jx = j + x;
                            int iy = i + y;
                            TmpT[j][i] = currentTetromino[i][3 - j];
                            if (TmpT[j][i] && (jx > 9 || jx < 0 || iy > 19 || iy < 0 || boardColorIDs[jx][iy] != 0)) { Safe = false; }
                        }
                    }
                    if (Safe) {
                        for (int i = 0; i < 4; i++) {
                            for (int j = 0; j < 4; j++) { currentTetromino[i][j] = TmpT[i][j]; }
                        }
                        centerOfRotationX = cX; centerOfRotationY = cY; X = x; Y = y;
                        draw = true;
                    }
                }
            } else if (input[0] == 'l' || input[0] == 'o' || input[0] == 19) {
                if (!CheckCollision(X + 1, Y)) { X++; draw = true; }
            } else if (input[0] == ' ') {
                for (int y = Y + 1; y < 21; y++) { if (CheckCollision(X, y)) { Y = y - 1; break; } } S = true; spawnNewTetromino = true; draw = true;
            }

            if (S) {
                Solidify();
                for (int x = 0; x < 10; x++) {
                    if (boardColorIDs[x][0] != 0) {
                        is_GameOver = true;
                        break;
                    }
                }
                S = false;
            }
            if (spawnNewTetromino && !is_GameOver) {
                newTetromino();
            }
            if (is_GameOver) {
                if (score > highscore) {
                    ofstream Tmp(highscoreFile);
                    highscore = score;
                    Tmp << highscore;
                    Tmp.close();
                }
                score = 0;
                break;
            }

        } else if (!is_Paused) {
            if (input[0] == ' ') {
                X = 3; Y = 0;
                for (int i = 0; i < 20; i++) {
                    for (int j = 0; j < 10; j++) { boardColorIDs[j][i] = 0; }
                }
                nextColor = tetrominoColors[rand() % 7];
                tetrominoID = rand() % 7;
                nextTetrominoID = rand() % 7;
                currentColor = tetrominoColors[tetrominoID];
                centerOfRotationX = centersOfRotationX[tetrominoID];
                centerOfRotationY = centersOfRotationY[tetrominoID];
                for (int i = 0; i < 4; i++) {
                    for (int j = 0; j < 4; j++) {
                        currentTetromino[i][j] = tetrominoShapes[tetrominoID][j][i];
                        nextTetromino[i][j] = tetrominoShapes[nextTetrominoID][j][i];
                    }
                }
                S = false; is_GameOver = false;
            }
        }
        if (input[0] == ' ') { is_Paused = false; draw = true; }

        if (draw) {
            for (int j = 0; j < 10; j++) {
                for (int i = 0; i < 20; i++) { boardColorIDsToDraw[j][i] = boardColorIDs[j][i]; }
            }
            if (!is_GameOver) {
                for (int i = 0; i < 4; i++) {
                    for (int j = 0; j < 4; j++) {
                        if (currentTetromino[j][i]) { boardColorIDsToDraw[j + X][i + Y] = currentColor; }
                    }
                }
            }

            drawbuffer = "\e[1H\e[?25l" + topPadding + leftPadding + "┌────────────────────┬────────────┐\n";
            for (int i = 0; i < 20; i++) {
                drawbuffer += leftPadding + "\e[0m│";
                if (is_GameOver || is_Paused) {
                    switch (i) {
                        case 9: drawbuffer += "\e[40;37m▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄"; break;
                        case 10:
                            if (is_GameOver) { drawbuffer += "\e[47;30m    GAME OVER      "; }
                            else if (is_Paused) { drawbuffer += "\e[47;30m    GAME PAUSED      "; }
                            break;
                        case 11: drawbuffer += "\e[40;37m▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀"; break;
                        default:
                            for (int j = 0; j < 10; j++) {
                                if (boardColorIDsToDraw[j][i] == 0) { drawbuffer += "- "; }
                                else { drawbuffer += "\e[4" + to_string(boardColorIDsToDraw[j][i]) + "m  \e[0m"; }
                            }
                            break;
                    }
                } else {
                    for (int j = 0; j < 10; j++) {
                        if (boardColorIDsToDraw[j][i] == 0) { drawbuffer += "- "; }
                        else { drawbuffer += "\e[4" + to_string(boardColorIDsToDraw[j][i]) + "m  \e[0m"; }
                    }
                }
                drawbuffer += "\e[0m";

                if (i + 1 < 6) {
                    drawbuffer += "│  ";
                    for (int j = 0; j < 4; j++) {
                        if (nextTetromino[j][i - 1]) { drawbuffer += "\e[4" + to_string(nextColor) + "m  \e[0m"; }
                        else { drawbuffer += "- "; }
                    }
                    drawbuffer += "\e[0m  │\n"; continue;
                }

                switch (i) {
                    case 0: drawbuffer += "│          │"; break;
                    case 5: drawbuffer += "├────────────┘"; break;
                    case 7: drawbuffer += "│ score: " + to_string(score); break;
                    case 8: drawbuffer += "│ High score: " + to_string(highscore); break;
                    default: drawbuffer += "│"; break;
                }
                drawbuffer += '\n';
            }
            if (is_GameOver) {
                drawbuffer += leftPadding + "└────────────────────┘" + topPadding;
                cout << drawbuffer;
            } else {
                drawbuffer += leftPadding + "└────────────────────┘" + topPadding;
                cout << drawbuffer;
            }
            draw = false;
        }
        this_thread::sleep_for(milliseconds(1));
    }

    tcsetattr(0, TCSANOW, &old);
    system("clear");
    cout << "\e[?25h" << flush;
    return 0;
}