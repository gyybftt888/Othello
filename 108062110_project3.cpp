#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <climits>

using namespace std;

struct Point {
    int x, y;
    Point() : Point(0, 0) {}
    Point(float x, float y) : x(x), y(y) {}
    bool operator==(const Point& rhs) const {
        return x == rhs.x && y == rhs.y;
    }
    bool operator!=(const Point& rhs) const {
        return !operator==(rhs);
    }
    Point operator+(const Point& rhs) const {
        return Point(x + rhs.x, y + rhs.y);
    }
    Point operator-(const Point& rhs) const {
        return Point(x - rhs.x, y - rhs.y);
    }
};
enum SPOT_STATE {
    EMPTY = 0, BLACK = 1, WHITE = 2
};
const Point directions[8]{
    Point(-1, -1), Point(-1, 0), Point(-1, 1),
    Point(0, -1), /*{0, 0}, */ Point(0, 1),
    Point(1, -1), Point(1, 0), Point(1, 1)
};
int player;
const int SIZE = 8;
std::array<std::array<int, SIZE>, SIZE> board;
int iboard[8][8]{ 0 };
std::vector<Point> next_valid_spots;
bool maxmin = true;
double coef[8][8]{ 
    {16.16,-3.58,1.16,0.53,0.53,1.16,-3.58,16.16},
    {-3.58,-1.81,-0.06,-2.23,-2.23,-0.06,-1.81,-3.58},
    {1.16,0.06,0.51,0.02,0.02,0.51,0.06,1.16},
    {0.53,-2.23,0.02,-0.01,-0.01,0.02,-2.23,0.53},
    {0.53,-2.23,0.02,-0.01,-0.01,0.02,-2.23,0.53},
    {1.16,0.06,0.51,0.02,0.02,0.51,0.06,1.16},
    {-3.58,-1.81,-0.06,-2.23,-2.23,-0.06,-1.81,-3.58},
    {16.16,-3.58,1.16,0.53,0.53,1.16,-3.58,16.16} };

class state {
public:
    double val;
    int fboard[8][8];
    int ox;     //player
    Point point;
    int depth;
    array<int, 3> disccount;

    //  估值
    void eval();
    //  得到可下的位置存成Point的vector
    vector<Point>getNVS();
    bool isspotvalid(Point);
    //  更新未來棋盤
    void update();
    //void getNVS(int[][8], int, vector<Point>);
    //vector<Point>NVS;

    state(Point p, int player, int b[8][8], int d) {
        point.x = p.x;
        point.y = p.y;
        depth = d;
        val = 0.0;
        update();
        ox = 3 - ox;
    }
};
double minimax(state, double, double, bool);
void state::eval() {
    val = coef[point.x][point.y];
    /*for(int i=0;i<SIZE;i++)
        for (int j = 0; j < SIZE; j++) {
            if (fboard[i][j] == ox)
                val += coef[i][j];
            else if (fboard[i][j] == 3 - ox)
                val -= coef[i][j];
        }*/
}
vector<Point> state::getNVS() {
    vector<Point> valid_spots;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            Point p = Point(i, j);
            if (fboard[i][j] != EMPTY)
                continue;
            if (isspotvalid(p))
                valid_spots.push_back(p);
        }
    }
    return valid_spots;
}
bool state::isspotvalid(Point center) {
    if (fboard[center.x][center.y] != EMPTY)
        return false;
    for (Point dir : directions) {
        // Move along the direction while testing.
        Point p = center + dir;
        if (0 > p.x || p.x >= SIZE || 0 > p.y || p.y >= SIZE || fboard[p.x][p.y] != 3 - ox)
            continue;
        p = p + dir;
        while (0 <= p.x && p.x < SIZE && 0 <= p.y && p.y < SIZE && fboard[p.x][p.y] != EMPTY) {
            if (fboard[p.x][p.y] == ox)
                return true;
            p = p + dir;
        }
    }
    return false;
}
void state::update() {
    if (0 > point.x || point.x >= SIZE || 0 > point.y || point.y >= SIZE || fboard[point.x][point.y] != EMPTY)
        return;
    fboard[point.x][point.y] = ox;
    for (Point dir : directions) {
        Point p = point + dir;
        if (0 > p.x || p.x >= SIZE || 0 > p.y || p.y >= SIZE || fboard[p.x][p.y] != 3 - ox)
            continue;
        p = p + dir;
        while (0 <= p.x && p.x < SIZE && 0 <= p.y && p.y < SIZE && fboard[p.x][p.y] != EMPTY) {
            if (fboard[p.x][p.y] == ox) {
                Point tmp = p;
                while (tmp != point) {
                    tmp = tmp - dir;
                    fboard[tmp.x][tmp.y] = ox;
                }
            }
            p = p + dir;
        }
    }
}
void read_board(std::ifstream& fin) {
    fin >> player;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            fin >> board[i][j];
            iboard[i][j] = board[i][j];
        }
    }
}
void read_valid_spots(std::ifstream& fin) {
    int n_valid_spots;
    fin >> n_valid_spots;
    int x, y;
    for (int i = 0; i < n_valid_spots; i++) {
        fin >> x >> y;
        next_valid_spots.push_back({x, y});
    }
}

double minimax(state present, double a, double b, bool maxmin) {
    vector<Point> NVS = present.getNVS();
    int l = NVS.size();
    if (!present.depth) {
        present.eval();
        return present.val;
    }
    //  (maxmin == true) -> max
    if (maxmin) {
        double max = -INT_MAX;
        for (int i = 0; i < l; i++) {
            double eval = minimax(state(NVS[i], present.ox, present.fboard, present.depth - 1), a, b, false);
            max = max > eval ? max : eval;
            a = a > max ? a : max;
            if (a >= b)
                break;
            present.val = max;
        }
    }
    else {
        double min = INT_MAX;
        for (int i = 0; i < l; i++) {   
            double eval = minimax(state(NVS[i], present.ox, present.fboard, present.depth - 1), a, b, true);
            min = min < eval ? min : eval;
            b = b < min ? b : min;
            if (b <= a)
                break;
            present.val = min;
        }
    }
}

void write_valid_spot(std::ofstream& fout) {
    int n_valid_spots = next_valid_spots.size();
    Point p;
    double max = -INT_MAX;
    for (int i = 0; i < next_valid_spots.size(); i++) {
        //int mini = eval(next_valid_spots[i]);
        double mini = minimax(state(next_valid_spots[i], player, iboard, 4), -INT_MAX, INT_MAX, true);;
        if (max < mini) {
            max = mini;
            p = next_valid_spots[i];
        }
    }
    // Remember to flush the output to ensure the last action is written to file.
    fout << p.x << " " << p.y << std::endl;
    fout.flush();
}
int main(int, char** argv) {
    std::ifstream fin(argv[1]);
    std::ofstream fout(argv[2]);
    read_board(fin);
    read_valid_spots(fin);
    write_valid_spot(fout);
    fin.close();
    fout.close();
    return 0;
}