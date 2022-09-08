#include "lib.h"
#include "perlin.h"
#include "svg.h"
#include <algorithm>
#include <cmath>
#include <list>
#include <map>
#include <random>
#include <vector>

const long long HEIGHT = 1024;
const long long WIDTH = 1024;
const long long MIN_RADIUS = 16;
const long long MAX_RADIUS = 64;

typedef std::pair<double, double> Coord;

struct Point {
    double x;
    double y;
    double radius;
    std::map<Point*, unsigned char> links;
    Point(double x, double y, double radius) : x(x), y(y), radius(radius) {}
    Point(const Coord& loc, double radius)
        : x(loc.first), y(loc.second), radius(radius) {}

    friend void establish_links(Point* a, Point* b) {
        a->links.emplace(b, 0);
        b->links.emplace(a, 0);
    }
    friend void increment_links(Point* a, Point* b, Point* c) {
        ++(a->links[b]);
        ++(a->links[c]);
        ++(b->links[a]);
        ++(b->links[c]);
        ++(c->links[a]);
        ++(c->links[b]);
    }

    SVG_Circle to_circle() const {
        SVG_Circle out{x, y, radius};
        out.stroke = "black";
        out.stroke_width = 2;
        out.fill_opacity = 0;
        return out;
    }

    inline double dist2(const Point* other) const {
        double dx = x - other->x;
        double dy = y - other->y;
        return dx * dx + dy * dy;
    }
    inline double dist2(const Coord& other) const {
        double dx = x - other.first;
        double dy = y - other.second;
        return dx * dx + dy * dy;
    }
};

std::pair<Coord, Coord> intersects(const Point* p1, const Point* p2,
                                   double add_radius = 0.0) {
    // https://math.stackexchange.com/a/1367732
    double dx = p2->x - p1->x;
    double dy = p2->y - p1->y;
    double R2 = dx * dx + dy * dy;
    if (R2 == 0)
        return {};
    double r1 = p1->radius + add_radius;
    double r2 = p2->radius + add_radius;
    // a = (r_1^2 - r_2^2) / R2
    double a = (r1 * r1 - r2 * r2) / R2;
    double base_x = (p1->x + p2->x) / 2 + a / 2 * dx;
    double base_y = (p1->y + p2->y) / 2 + a / 2 * dy;
    // b = that complicated square root in the link including 1/2
    double b = sqrt(2 * (r1 * r1 + r2 * r2) / R2 - a * a - 1) / 2;
    return std::pair<std::pair<double, double>, std::pair<double, double>>(
        {base_x + b * dy, base_y + b * -dx},
        {base_x + b * -dy, base_y + b * dx});
}
struct ExposedEdge {
    Point* a;
    Point* b;
    unsigned char attempts;

    /* Always placing triangle in the negative rotation with p1 as origin
    This is 'right' in a normal coordinate grid, or 'left' on a computer
    canvas*/
    ExposedEdge(Point* p1, Point* p2) : a(p1), b(p2), attempts(10) {}

    bool operator==(const ExposedEdge& other) const {
        return a == other.a && b == other.b;
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
        double mx = (a->x + b->x + c->x) / 3 / WIDTH * 4;
        double my = (a->y + b->y + c->y) / 3 / HEIGHT * 4;
        poly.color = to_hsl(perlin(mx, my) * 180 + 180,
                            perlin(mx, my + HEIGHT) * 20 + 80,
                            perlin(mx + WIDTH, my) * 30 + 70);
        return poly;
    }
};

struct Space {
    double width;
    double height;
    size_t cell_width;
    size_t cell_height;
    std::vector<Point*> all;
    std::vector<std::vector<std::list<Point*>>> arr;

    Space(double width, double height)
        : width(width), height(height), cell_width(width / MAX_RADIUS),
          cell_height(height / MAX_RADIUS),
          arr(cell_width, std::vector<std::list<Point*>>(cell_height,
                                                         std::list<Point*>())) {
    }

    void add(double x, double y, double radius) {
        all.push_back(new Point(x, y, radius));

        size_t cell_x = cap_range<long long>(x / MAX_RADIUS, 0, cell_width - 1);
        size_t cell_y =
            cap_range<long long>(y / MAX_RADIUS, 0, cell_height - 1);
        arr[cell_x][cell_y].push_back(all.back());
    }

    std::vector<Triangle> populate() {
        std::vector<Triangle> out;
        std::list<ExposedEdge> edges;
        std::list<ExposedEdge> dead_edges;
        // Add first point in middle
        double first_radius = frandrange(MIN_RADIUS, MAX_RADIUS);
        add(width / 2, height / 2, first_radius);
        // Add second point around first point
        double second_radius = frandrange(MIN_RADIUS, MAX_RADIUS);
        double second_angle = frandrange(0, M_2_PI);
        add(width / 2 + (first_radius + second_radius) * cos(second_angle),
            height / 2 + (first_radius + second_radius) * sin(second_angle),
            second_radius);
        // Set initial link/edges between first and second points
        establish_links(all[0], all[1]);
        edges.emplace_back(all[0], all[1]);
        edges.emplace_back(all[1], all[0]);
        // Go!
        while (!edges.empty()) {
            double new_radius = frandrange(MIN_RADIUS, MAX_RADIUS);
            Coord potential =
                intersects(edges.front().a, edges.front().b, new_radius).first;
            // Now check if we can connect 3 in a triangle
            for (Point* p : all) {
                if (p->dist2(potential) < pow(MIN_RADIUS, 2)) {
                    // They are close
                    for (auto link : edges.front().a->links) {
                        if (link.second >= 2 || link.first != p)
                            continue;
                        // Epic! we can use this
                        establish_links(p, edges.front().b);
                        increment_links(p, edges.front().a, edges.front().b);
                        out.emplace_back(p, edges.front().a, edges.front().b);

                        if (in_range(width, height, p->x, p->y)) {
                            // Remove the interior edges if applicable
                            edges.remove(ExposedEdge(p, edges.front().a));
                            edges.remove(ExposedEdge(edges.front().b, p));
                            // Add a new edge if applicable
                            edges.emplace_back(p, edges.front().b);
                        }
                        goto endloop;
                    }
                    for (auto link : edges.front().b->links) {
                        if (link.second >= 2 || link.first != p)
                            continue;
                        // Epic! we can use this
                        establish_links(p, edges.front().a);
                        increment_links(p, edges.front().a, edges.front().b);
                        out.emplace_back(p, edges.front().a, edges.front().b);

                        if (in_range(width, height, p->x, p->y)) {
                            // Remove the interior edges
                            edges.remove(ExposedEdge(p, edges.front().a));
                            edges.remove(ExposedEdge(edges.front().b, p));
                            // Add a new edge if applicable
                            edges.emplace_back(edges.front().a, p);
                        }
                        goto endloop;
                    }
                }
            }
            // Check if this overlaps with anything
            for (const Point* p : all) {
                if (p->dist2(potential) >= pow(p->radius + new_radius - 2, 2))
                    continue;
                // Here an overlap has been found
                if (edges.front().attempts > 0) {
                    // Put it for later
                    edges.emplace_back(edges.front());
                    --edges.back().attempts;
                } else {
                    // Put it in dead edges to figure out later
                    dead_edges.emplace_back(edges.front());
                }
                goto endloop;
            }
            // If not, keep going
            add(potential.first, potential.second, new_radius);
            establish_links(all.back(), edges.front().a);
            establish_links(all.back(), edges.front().b);
            increment_links(all.back(), edges.front().a, edges.front().b);
            out.emplace_back(all.back(), edges.front().a, edges.front().b);
            if (in_range(width, height, all.back()->x, all.back()->y)) {
                edges.emplace_back(edges.front().a, all.back());
                edges.emplace_back(all.back(), edges.front().b);
                // Check if we can add any new edges
                for (Point* p : all) {
                    if (!in_range(width, height, p->x, p->y))
                        continue; // Don't make edges with poins out of range
                    else if (p == all.back() || all.back()->links.count(p))
                        continue; // Only if there isn't already something
                    else if (p->dist2(potential) <
                             pow(p->radius + new_radius + MIN_RADIUS, 2)) {
                        // These could have an edge
                        establish_links(p, all.back());
                        edges.emplace_back(p, all.back());
                        edges.emplace_back(all.back(), p);
                    }
                }
            }

        endloop:
            edges.pop_front();
        }
        // Now get the extra thingies
        // First, sort the edges by originating point
        std::map<Point*, std::list<ExposedEdge>> edge_map;
        while (!dead_edges.empty()) {
            edge_map.emplace(dead_edges.front().a, std::list<ExposedEdge>{})
                .first->second.push_back(dead_edges.front());
            dead_edges.pop_front();
        }
        // Then try to find loops and fill them
        // Currently only 3-point
        for (auto& point : edge_map) {
            auto itr_a = point.second.begin();
            for (; itr_a != point.second.end();
                 itr_a = point.second.erase(itr_a)) {
                // Find the second point
                auto find_b = edge_map.find(itr_a->b);
                if (find_b == edge_map.end())
                    continue;
                // Look through its edges
                auto itr_b = find_b->second.begin();
                for (; itr_b != find_b->second.end(); ++itr_b) {
                    // Find the third point
                    auto find_c = edge_map.find(itr_b->b);
                    if (find_c == edge_map.end())
                        continue;
                    // Look through its edges for the first point
                    auto itr_c = find_c->second.begin();
                    for (; itr_c != find_c->second.end(); ++itr_c) {
                        if (itr_c->b == itr_a->a) {
                            // Loop found!
                            out.emplace_back(itr_a->a, itr_a->b, itr_b->b);
                            increment_links(itr_a->a, itr_a->b, itr_b->b);
                            find_c->second.erase(itr_c);
                            find_b->second.erase(itr_b);
                            goto found_tri;
                        }
                    }
                }
            // If we have (or haven't) found a loop then this first edge is no
            // longer needed
            found_tri:;
            }
        }
        return out;
    }
};

int main() {
    srand(time(NULL));

    Space space(WIDTH, HEIGHT);
    std::vector<Triangle> triangles = space.populate();

    // Draw triangles
    SVG svg(HEIGHT, WIDTH);
    for (const Triangle& tri : triangles)
        svg.shapes.push_back(new SVG_Polygon(tri.to_poly()));

    // Overlay circles
    for (const Point* point : space.all)
        svg.shapes.push_back(new SVG_Circle(point->to_circle()));

    std::ofstream file("out.svg");
    file << "<!DOCTYPE svg>\n" << svg << std::endl;
    file.close();
}