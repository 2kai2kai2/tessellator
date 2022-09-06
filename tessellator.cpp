#include "svg.h"
#include <cmath>
#include <list>
#include <random>
#include <vector>

const long long HEIGHT = 1024;
const long long WIDTH = 1024;
const long long EDGE_MIN = 200;
const long long EDGE_VAR = 100;

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

struct ExposedEdge {
    Point* a;
    Point* b;

    /* Always placing triangle in the positive rotation with p1 as origin */
    ExposedEdge(Point* p1, Point* p2) : a(p1), b(p2) {}
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

int main() {
    srand(time(NULL));
    std::vector<Point*> points;
    std::vector<Triangle> triangles;
    std::list<ExposedEdge> edges;

    // Place starting points
    points.push_back(new Point(WIDTH / 2, HEIGHT / 2));
    size_t _start_len = EDGE_MIN + rand() % EDGE_VAR;
    double _start_angle = remainder(rand() / 1000000.0, M_2_PI);
    points.push_back(new Point(WIDTH / 2 + _start_len * cos(_start_angle),
                               HEIGHT / 2 + _start_len * sin(_start_angle)));
    establish_links(points[0], points[1]);
    edges.emplace_back(points[0], points[1]);
    edges.emplace_back(points[1], points[0]);

    for (int j = 0; j < 10 && !edges.empty(); ++j) {
        double dx = edges.front().b->x - edges.front().a->x;
        double dy = edges.front().b->y - edges.front().a->y;
        double edge_angle = atan2(dy, dx);
        Point* new_point = nullptr;

        for (auto itr = edges.begin()++; itr != edges.end(); ++itr) {
            if (itr->b == edges.front().a && itr->a != edges.front().b) {
                // This means that there is an edge facing the opposite
                // direction attached to a. check if it's close!
                double p_dx = itr->a->x - edges.front().a->x;
                double p_dy = itr->a->y - edges.front().a->y;
                double p_angle = atan2(p_dy, p_dx);
                if (fabs(edge_angle - p_angle) < M_PI / 3) {
                    new_point = itr->a;
                    edges.erase(itr);
                    break;
                }
            }
        }
        if (!new_point) {
            double angle = fabs(remainder(rand() / 1000000.0, M_PI / 3));
            std::cout << edge_angle << ' ' << angle << std::endl;

            size_t new_length = rand() % EDGE_VAR + EDGE_MIN;
            long long new_x = edges.front().a->x +
                              new_length * cos(edge_angle + M_PI_2 + angle);
            long long new_y = edges.front().a->y +
                              new_length * sin(edge_angle + M_PI_2 + angle);
            new_point = new Point(new_x, new_y);

            points.push_back(new_point);
            if (new_x >= 0 && new_y >= 0 && new_x < WIDTH && new_y < HEIGHT) {
                edges.emplace_back(edges.front().a, new_point);
                edges.emplace_back(new_point, edges.front().b);
            }
        }
        triangles.emplace_back(edges.front().a, edges.front().b, new_point);
        establish_links(edges.front().a, new_point);
        establish_links(edges.front().b, new_point);

        edges.pop_front();
    }

    SVG svg(HEIGHT, WIDTH);
    for (const Triangle& tri : triangles)
        svg.polygons.push_back(tri.to_poly());

    std::ofstream file("out.svg");
    file << "<!DOCTYPE svg>\n" << svg << std::endl;
    file.close();
}