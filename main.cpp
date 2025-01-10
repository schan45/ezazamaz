#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <windows.h>

using namespace std;

// Forgatási irányok térképe
map<string, string> rotationMap = {
    {"N", "E"}, {"E", "S"}, {"S", "W"}, {"W", "N"}
};

// Forrás struktúra
struct Source {
    string id;
    string color;
    int x, y;
    vector<string> outputs;
};

// Fogyasztó struktúra
struct Consumer {
    string id;
    string color;
    int x, y;
    vector<string> inputs;
    bool flooded = false;
};

// Cső struktúra
struct Pipe {
    string id;
    int x = -1, y = -1; // A cső kezdetben nincs a pályán
    vector<string> connections;

    // Forgatás 90 fokkal
    void rotate() {
        for (auto &dir : connections) {
            dir = rotationMap[dir];
        }
    }

    // Ellenőrzi, hogy a cső illeszkedik-e az adott irányokhoz
    bool fits(string from, string to) {
        return (connections[0] == from && connections[1] == to) ||
               (connections[0] == to && connections[1] == from);
    }
};

struct Cell {
    string color = "";      // A cella színe
    bool isFlooded = false; // Anyag áthalad-e a cellán
};

struct Board {
    int rows, cols;
    vector<vector<Cell>> grid; // 2D mátrix a pálya celláihoz

    // Konstruktor: inicializálja a mátrixot
    Board(int r, int c) : rows(r), cols(c), grid(r, vector<Cell>(c)) {}

    // Ellenőrzi, hogy egy cella a pályán belül van-e
    bool isValid(int x, int y) const {
        return x >= 0 && x < cols && y >= 0 && y < rows;
    }

    // Ellenőrzi, hogy egy cella szabad-e a megadott szín számára
    bool canFlow(int x, int y, const string &color) const {
        if (!isValid(x, y)) return false; // Határon kívüli cella
        const Cell &cell = grid[y][x];
        return cell.color == "" || cell.color == color; // Csak azonos szín engedett
    }

    // Beállítja az anyagot egy cellára
    void setFlow(int x, int y, const string &color) {
        if (isValid(x, y)) {
            Cell &cell = grid[y][x];
            cell.color = color;
            cell.isFlooded = true;
        }
    }
};


void simulateFlow(Board &board, const Source &source, const string &color, vector<Pipe> &pipes, int consumerX, int consumerY) {
    // Rekurzív segédfüggvény a mélységi kereséshez
    function<bool(int, int)> dfs = [&](int x, int y) {
        if (!board.isValid(x, y)) return false; // Határon kívül
        if (board.grid[y][x].color != color && board.grid[y][x].color != "") return false; // Színkeveredés
        if (x == consumerX && y == consumerY) return true; // Elértük a fogyasztót

        // Jelöljük a cellát elárasztottnak
        board.setFlow(x, y, color);

        // Négy irányú keresés
        for (const auto &pipe : pipes) {
            if (pipe.x == x && pipe.y == y) {
                for (const string &dir : pipe.connections) {
                    int nx = x, ny = y;
                    if (dir == "N") ny--;
                    else if (dir == "S") ny++;
                    else if (dir == "E") nx++;
                    else if (dir == "W") nx--;

                    // Rekurzív hívás az új cellára
                    if (dfs(nx, ny)) return true;
                }
            }
        }
        return false;
    };

    // Induljunk a forrásból
    if (!dfs(source.x, source.y + 1)) {
        cerr << "Szivárgás vagy szétkapcsolt pálya! A fogyasztó nem érhető el!\n";
    }
}



// Beolvasási funkciók
void readBoardSize(ifstream &file, int &rows, int &cols) {
    file >> rows >> cols; // Kiolvassuk a sorok és oszlopok számát
    cout << "Pályaméret: " << rows << "x" << cols << "\n";
}


void readSources(ifstream &file, vector<Source> &sources) {
    int count;
    file >> count;
    for (int i = 0; i < count; ++i) {
        Source source;
        file >> source.id >> source.color >> source.x >> source.y;
        int outputCount;
        file >> outputCount;
        for (int j = 0; j < outputCount; ++j) {
            string output;
            file >> output;
            source.outputs.push_back(output);
        }
        sources.push_back(source);
    }
}

void readConsumers(ifstream &file, vector<Consumer> &consumers) {
    int count;
    file >> count;
    for (int i = 0; i < count; ++i) {
        Consumer consumer;
        file >> consumer.id >> consumer.color >> consumer.x >> consumer.y;
        int inputCount;
        file >> inputCount;
        for (int j = 0; j < inputCount; ++j) {
            string input;
            file >> input;
            consumer.inputs.push_back(input);
        }
        consumers.push_back(consumer);
    }
}

void readPipes(ifstream &file, vector<Pipe> &pipes) {
    int count;
    file >> count;
    for (int i = 0; i < count; ++i) {
        Pipe pipe;
        file >> pipe.id;
        int connectionCount;
        file >> connectionCount;
        for (int j = 0; j < connectionCount; ++j) {
            string conn;
            file >> conn;
            pipe.connections.push_back(conn);
        }
        pipes.push_back(pipe);
    }
}

void readPhases(ifstream &file, vector<string> &phases) {
    int count;
    file >> count;
    for (int i = 0; i < count; ++i) {
        string phase;
        file >> phase;
        phases.push_back(phase);
    }
}

void generateOutput(const Board &board, const vector<Source> &sources, const vector<Consumer> &consumers,
                    const vector<Pipe> &pipes, const vector<string> &phases, const vector<string> &tapStates) {
    ofstream outFile("output.txt");
    if (!outFile.is_open()) {
        cerr << "Nem sikerült megnyitni az output fájlt!\n";
        return;
    }

    // Időmök elhelyezése
    outFile << "[Placement]\n";
    for (const auto &pipe : pipes) {
        if (pipe.x != -1 && pipe.y != -1) { // Csak elhelyezett csöveket írjunk ki
            outFile << pipe.id << " " << pipe.x << " " << pipe.y;
            for (const auto &dir : pipe.connections) {
                outFile << " " << dir;
            }
            outFile << "\n";
        }
    }

    // Fázisok eredményei
    outFile << "\n[Phases]\n";
    for (size_t i = 0; i < phases.size(); ++i) {
        outFile << "Phase " << i + 1 << ": " << phases[i] << "\n";
        outFile << "Taps: " << tapStates[i] << "\n";
        outFile << "Flooded consumers: ";
        for (const auto &consumer : consumers) {
            if (consumer.flooded) {
                outFile << consumer.id << " ";
            }
        }
        outFile << "\n\n";
    }

    outFile.close();
    cout << "Output generálva az 'output.txt' fájlba.\n";
}




// Megoldási algoritmus
void solve(Board &board, vector<Source> &sources, vector<Consumer> &consumers, vector<Pipe> &pipes,
           vector<string> &phases) {
    vector<string> tapStates; // Csapok állapota fázisonként

    // Minden fázist feldolgozunk
    for (const auto &phase : phases) {
        cout << "Fázis: " << phase << "\n";
            string tapState = "";

        // Aktív források feldolgozása
        for (auto &source : sources) {
            if (source.color == phase) {
                for (auto &consumer : consumers) {
                    if (consumer.color == phase) {
                        cout << "Forrás (" << source.x << ", " << source.y << ") összekötése "
                             << "fogyasztóval (" << consumer.x << ", " << consumer.y << ")\n";

                            // Csövek elhelyezése
                            for (auto &pipe : pipes) {
                            if (pipe.x == -1 && pipe.y == -1) { // Csak nem használt csöveket helyezünk el
                                // Első cső
                                pipe.x = source.x;
                                pipe.y = source.y + 1; // Jobbra lépünk
                                while (!pipe.fits("E", "S")) {
                                    pipe.rotate();
                                }
                                board.setFlow(pipe.x, pipe.y, phase); // Anyag beállítása
                                cout << "Első cső elhelyezve (" << pipe.x << ", " << pipe.y
                                     << ") irányok: " << pipe.connections[0] << ", " << pipe.connections[1] << "\n";
                                    break;
                            }
                        }

                        for (auto &pipe : pipes) {
                            if (pipe.x == -1 && pipe.y == -1) { // Második cső
                                pipe.x = source.x + 1;
                                pipe.y = source.y + 1; // Le és jobbra lépünk
                                while (!pipe.fits("N", "E")) {
                                    pipe.rotate();
                                }
                                board.setFlow(pipe.x, pipe.y, phase); // Anyag beállítása
                                cout << "Második cső elhelyezve (" << pipe.x << ", " << pipe.y
                                     << ") irányok: " << pipe.connections[0] << ", " << pipe.connections[1] << "\n";
                                    break;
                            }
                        }

                        for (auto &pipe : pipes) {
                            if (pipe.x == -1 && pipe.y == -1) { // Harmadik cső
                                pipe.x = source.x + 2;
                                pipe.y = source.y + 1; // Még egyet le lépünk
                                while (!pipe.fits("W", "S")) {
                                    pipe.rotate();
                                }
                                board.setFlow(pipe.x, pipe.y, phase); // Anyag beállítása
                                cout << "Harmadik cső elhelyezve (" << pipe.x << ", " << pipe.y
                                     << ") irányok: " << pipe.connections[0] << ", " << pipe.connections[1] << "\n";
                                    break;
                            }
                        }

                        // Anyag áramlásának ellenőrzése
                        simulateFlow(board, source, phase, pipes, consumer.x, consumer.y);

                        // Ellenőrizzük, hogy a fogyasztót sikerült-e elárasztani
                        if (board.grid[consumer.y][consumer.x].isFlooded) {
                            consumer.flooded = true;
                            cout << "Fogyasztó elárasztva (" << consumer.x << ", " << consumer.y << ")\n";
                        } else {
                            consumer.flooded = false;
                            cerr << "Hiba: Fogyasztó nem érhető el a (" << consumer.x << ", " << consumer.y << ") pozícióban!\n";
                        }
                    }
                }
            }
        }
        tapStates.push_back(tapState);
    }

    // Output generálása
    generateOutput(board, sources, consumers, pipes, phases, tapStates);
}






int main() {

    // Állítsd a konzol kódolását UTF-8-ra
    SetConsoleOutputCP(CP_UTF8);

    // Ha szükséges, a bemeneti kódolást is állítsd (pl. fájlbeolvasáshoz)
    SetConsoleCP(CP_UTF8);

    ifstream inputFile("csovek.txt");
    if (!inputFile.is_open()) {
        cerr << "Nem sikerült megnyitni az input fájlt!\n";
        return 1;
    }

    int rows = 0, cols = 0;
    vector<Source> sources;
    vector<Consumer> consumers;
    vector<Pipe> pipes;
    vector<string> phases;

    string section;
    while (inputFile >> section) {
        if (section == "[Board]") {
            readBoardSize(inputFile, rows, cols); // Pályaméret beolvasása
        } else if (section == "[Sources]") {
            readSources(inputFile, sources);
        } else if (section == "[Consumers]") {
            readConsumers(inputFile, consumers);
        } else if (section == "[Pipes]") {
            readPipes(inputFile, pipes);
        } else if (section == "[Phases]") {
            readPhases(inputFile, phases);
        }
    }
    inputFile.close();

    // Board inicializálása a beolvasott méretekkel
    Board board(rows, cols);

    // Megoldás futtatása
    solve(board, sources, consumers, pipes, phases);

    return 0;
}
