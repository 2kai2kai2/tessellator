#include "svg.h"
#include <cmath>
#include <list>
#include <random>
#include <vector>

const long long HEIGHT = 1024;
const long long WIDTH = 1024;
const long long CELL_SIZE = 128;

struct Point {
    long long x;
    long long y;
    std::set<Point*> links;
    Point(long long x, long long y) : x(x), y(y) {}

    friend void establish_links(Point* a, Point* b) {
        a->links.insert(b);
        b->links.insert(a);
    }
};
struct Triangle {
    Point* a;
    Point* b;
    Point* c;
    Triangle(Point* a, Point* b, Point* c) : a(a), b(b), c(c) {}

    SVG_Polygon to_poly() const {
        SVG_Polygon poly;
        poly.points = {{a->x, a->y}, {b->x, b->y}, {c->x, c->y}};
        poly.color = "rgb(" + std::to_string(rand() % 256) + ',' +
                     std::to_string(rand() % 256) + ',' +
                     std::to_string(rand() % 256) + ')';
        return poly;
    }
};

struct Space {
    double width;
    double height;
    size_t cell_width;
    size_t cell_height;
    Point*** arr;

    Space(double width, double height)
        : width(width), height(height), cell_width(width / CELL_SIZE),
          cell_height(height / CELL_SIZE) {
        arr = new Point**[cell_width];
        for (size_t col = 0; col < cell_width; ++col)
            arr[col] = new Point* [cell_height] { nullptr };
    }
    ~Space() {
        for (size_t col = 0; col < cell_width; ++col) {
            for (size_t row = 0; row < cell_height; ++row) {
                delete arr[col][row];
            }
            delete[] arr[col];
        }
        delete[] arr;
    }

    Point* get(size_t cell_x, size_t cell_y) const {
        if (cell_x >= cell_width || cell_y >= cell_height)
            throw std::out_of_range("Invalid cell number (" +
                                    std::to_string(cell_x) + ", " +
                                    std::to_string(cell_y) + ") accessed.");
        return arr[cell_x][cell_y];
    }

    void populate() {
        for (size_t col = 0; col < cell_width; ++col) {
            for (size_t row = 0; row < cell_height; ++row) {
                arr[col][row] = new Point(col * CELL_SIZE + rand() % CELL_SIZE,
                                          row * CELL_SIZE + rand() % CELL_SIZE);
            }
        }
    }
};

int main() {
    srand(time(NULL));
    std::vector<Triangle> triangles;

    Space space(WIDTH, HEIGHT);
    space.populate();

    for (size_t col = 0; col < space.cell_width; ++col) {
        for (size_t row = 0; row < space.cell_height; ++row) {
            if (col + 1 < space.cell_width && row + 1 < space.cell_height)
                triangles.emplace_back(space.get(col, row), space.get(col + 1, row), space.get(col, row + 1));
            if (col >= 1 && row >= 1)
                triangles.emplace_back(space.get(col, row), space.get(col - 1, row), space.get(col, row - 1));
            
        }
    }

    // Draw triangles
    SVG svg(HEIGHT, WIDTH);
    for (const Triangle& tri : triangles)
        svg.polygons.push_back(tri.to_poly());

    std::ofstream file("out.svg");
    file << "<!DOCTYPE svg>\n" << svg << std::endl;
    file.close();
}