#include "lib.h"
#include "perlin.h"
#include "svg.h"
#include <algorithm>
#include <cmath>
#include <list>
#include <map>
#include <vector>

const long long HEIGHT = 1024 * 8;
const long long WIDTH = 1024 * 8;
const long long MIN_RADIUS = 16;
const long long MAX_RADIUS = 64;

std::vector<SVG_Shape*> bonus_draw;
std::list<SVG_Def*> _defs;

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
    friend double vec_angle(const Point* origin, const Point* dir) {
        return atan2(dir->y - origin->y, dir->x - origin->x) + M_PI_2;
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
    return {{base_x + b * dy, base_y + b * -dx},
            {base_x + b * -dy, base_y + b * dx}};
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
    inline double angle() const { return vec_angle(a, b); }
    friend double interior_angle(const ExposedEdge& first,
                                 const ExposedEdge& second) {
        if (first.b != second.a)
            throw std::invalid_argument("first.b must equal second.a");
        return normalize_rad(second.angle() - first.angle() + M_PI);
    }
};
struct Triangle {
    Point* a;
    Point* b;
    Point* c;
    Triangle(Point* a, Point* b, Point* c) : a(a), b(b), c(c) {}

    SVG_Polygon to_poly() const {
        static ColorMap colorMap;
        SVG_Polygon poly;
        poly.points = {{a->x, a->y}, {b->x, b->y}, {c->x, c->y}};
#ifdef SIMPLE_COLOR
        double mx = (a->x + b->x + c->x) / 3 / (MAX_RADIUS * 4);
        double my = (a->y + b->y + c->y) / 3 / (MAX_RADIUS * 4);

        poly.color = colorMap(mx, my);
#else
        double mx = (a->x + b->x + c->x) / 3 / (MAX_RADIUS * 4);
        double my = (a->y + b->y + c->y) / 3 / (MAX_RADIUS * 4);

        double min_x = std::min({a->x, b->x, c->x}) / (MAX_RADIUS * 4);
        double max_x = std::max({a->x, b->x, c->x}) / (MAX_RADIUS * 4);
        double min_y = std::min({a->y, b->y, c->y}) / (MAX_RADIUS * 4);
        double max_y = std::max({a->y, b->y, c->y}) / (MAX_RADIUS * 4);

        double m_radius = ((max_x - min_x) + (max_y - min_y)) / 2 / 4;
        double c_angle = frandrange(0, 2 * M_PI);

        std::string color1 = colorMap(mx + m_radius * std::cos(c_angle),
                                      my + m_radius * std::sin(c_angle));
        std::string color2 = colorMap(mx + m_radius * std::cos(c_angle + M_PI),
                                      my + m_radius * std::sin(c_angle + M_PI));
        static size_t gradient_num = 0;
        std::string gradientID = "G" + std::to_string(gradient_num++);
        _defs.push_back(
            new SVG_LinearGradient(gradientID, 100, 0, 0, 100, color1, color2));
        poly.color = "url(#" + gradientID + ')';
#endif
        return poly;
    }
};

struct Space {
    typedef std::list<ExposedEdge> Path;
    typedef std::list<ExposedEdge> EdgeList;
    typedef std::map<Point*, EdgeList> EdgeMap;

    double width;
    double height;
    size_t cell_width;
    size_t cell_height;
    std::vector<Point*> all;
    std::vector<std::vector<std::list<Point*>>> arr;

    Space(double width, double height)
        : width(width), height(height),
          cell_width((size_t)(width / MAX_RADIUS)),
          cell_height((size_t)(height / MAX_RADIUS)),
          arr(cell_width, std::vector<std::list<Point*>>(cell_height,
                                                         std::list<Point*>())) {
    }
    inline size_t get_cell_x(double x) const {
        return cap_range<long long>(x / MAX_RADIUS, 0, cell_width - 1);
    }
    inline size_t get_cell_y(double y) const {
        return cap_range<long long>(y / MAX_RADIUS, 0, cell_height - 1);
    }

    std::list<Point*> get_neighbors(size_t cell_x, size_t cell_y,
                                    size_t range = 1) {
        std::list<Point*> out;
        for (size_t x = std::max(cell_x, range) - range;
             x <= std::min(cell_x + range, cell_width - 1); ++x) {
            for (size_t y = std::max(cell_y, range) - range;
                 y <= std::min(cell_y + range, cell_height - 1); ++y) {
                // Copy all from other
                out.insert(out.end(), arr[x][y].begin(), arr[x][y].end());
            }
        }
        return out;
    }
    inline std::list<Point*> get_neighbors(const Coord& c, size_t range = 1) {
        return get_neighbors(get_cell_x(c.first), get_cell_y(c.second), range);
    }
    inline std::list<Point*> get_neighbors(const Point* p, size_t range = 1) {
        return get_neighbors(get_cell_x(p->x), get_cell_y(p->y), range);
    }

    void add(double x, double y, double radius) {
        all.push_back(new Point(x, y, radius));
        arr[get_cell_x(x)][get_cell_y(y)].push_back(all.back());
    }

    std::vector<Triangle> populate() {
        std::vector<Triangle> out;
        EdgeList edges;
        EdgeList dead_edges;
        // Add first point in middle
        double first_radius = frandrange(MIN_RADIUS, MAX_RADIUS);
        add(width / 2, height / 2, first_radius);
        // Add second point around first point
        double second_radius = frandrange(MIN_RADIUS, MAX_RADIUS);
        double second_angle = frandrange(0, M_PI * 2);
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
            for (Point* p : get_neighbors(potential)) {
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
                            // Add a new edge
                            edges.remove(ExposedEdge(p, edges.front().b));
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
                            // Add a new edge
                            edges.remove(ExposedEdge(edges.front().a, p));
                            edges.emplace_back(edges.front().a, p);
                        }
                        goto endloop;
                    }
                }
            }
            // Check if this overlaps with anything
            for (const Point* p : get_neighbors(potential, 2)) {
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
                for (Point* p : get_neighbors(potential, 3)) {
                    if (!in_range(width, height, p->x, p->y))
                        continue; // Don't make edges with points out of range
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
        EdgeMap edge_map;
        while (!dead_edges.empty()) {
            edge_map.emplace(dead_edges.front().a, EdgeList{})
                .first->second.push_back(dead_edges.front());
            dead_edges.pop_front();
        }
        // Then try to find loops and fill them
        std::list<Path> loops;
        for (auto& point : edge_map) {
            // Setup initial options
            std::list<Path> paths;
            for (const ExposedEdge& edge : point.second)
                paths.push_back({edge});
            // Build paths
            while (!paths.empty()) {
                Path& _path = paths.front();
                EdgeMap::iterator options = edge_map.find(_path.back().b);
                if (options == edge_map.end()) {
                    paths.pop_front();
                    continue;
                }
                // Check all possible next options
                for (ExposedEdge& next : options->second) {
                    // See if we've found the loop
                    if (next.b == point.first) {
                        if (_path.size() == 1)
                            goto loops_next_option; // don't form 2-point loops
                        _path.push_back(next);
                        // Check to make sure it's interior angles
                        double angleSum = 0;
                        for (Path::iterator itr = _path.begin();
                             itr != _path.end(); ++itr) {
                            angleSum +=
                                interior_angle(*itr, *loop_next(_path, itr));
                        }
                        if (fabs(angleSum - (_path.size() * M_PI - 2 * M_PI)) >
                            0.001) {
                            _path.pop_back();
                            goto loops_next_option;
                        }
                        // Ok this is a loop!
                        loops.push_back(_path);
                        // Delete all the ExposedEdges
                        for (const ExposedEdge& edge : _path)
                            edge_map[edge.a].remove(edge);
                        goto loops_next_point;
                    }
                    // See if it's crossed over our existing path
                    for (const ExposedEdge& edge : _path) {
                        if (next.b == edge.a)
                            goto loops_next_option;
                    }
                    // Otherwise add it as an option
                    paths.push_back(_path);
                    paths.back().push_back(next);
                loops_next_option:;
                }
                paths.pop_front();
            }
        loops_next_point:;
        }
#ifdef DEBUG
        for (Path& l : loops) {
            std::string color = to_hsl(rand(), 100, 60);
            double angleSum = 0;
            double x_sum = 0;
            double y_sum = 0;
            for (auto e = l.begin(); e != l.end(); ++e) {
                angleSum += interior_angle(*e, *loop_next(l, e));
                x_sum += e->a->x;
                y_sum += e->a->y;
                SVG_Line* line =
                    new SVG_Line(e->a->x, e->a->y, e->b->x, e->b->y);
                line->color = color;
                line->width = 2;
                bonus_draw.push_back(line);
            }
            bonus_draw.push_back(
                new SVG_Text(x_sum / l.size(), y_sum / l.size(),
                             std::to_string(angleSum * 180 / M_PI)));
        }
#endif
        // Clean up the loops
        for (Path& loop : loops) {
            if (loop.size() < 3)
                throw std::runtime_error(
                    "Loop should not be less than size 3!");
            while (loop.size() > 3) {
                // Find the closest two
                auto closest = loop.end();
                double closest_dist2 = INFINITY;
                auto itr = loop.begin();
                for (; itr != loop.end(); ++itr) {
                    auto next = loop_next(loop, itr);
                    double dist = itr->a->dist2(next->b);
                    if (dist < closest_dist2) {
                        // Check to avoid convexity
                        if (normalize_rad(next->angle() - itr->angle()) < M_PI)
                            continue;
                        closest = itr;
                        closest_dist2 = dist;
                    }
                }
                // Make sure there's no funny business
                if (closest == loop.end())
                    throw std::runtime_error(
                        "No drawable links in a loop larger than 3! This "
                        "shouldn't happen.");
                // Draw triangle
                auto closest_next = loop_next(loop, closest);
                out.emplace_back(closest->a, closest->b, closest_next->b);
                establish_links(closest->a, closest_next->b);
                increment_links(closest->a, closest->b, closest_next->b);
                // Shrink loop
                loop.emplace(closest, closest->a, closest_next->b);
                loop.erase(closest);
                loop.erase(closest_next);
            }
            out.emplace_back(loop.front().a, loop.front().b, loop.back().a);
            increment_links(loop.front().a, loop.front().b, loop.back().a);
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
    svg.defs = _defs;

#ifdef DEBUG
    // Overlay circles
    for (const Point* point : space.all)
        svg.shapes.push_back(new SVG_Circle(point->to_circle()));
    // Overlay bonus_draw
    for (SVG_Shape* bonus : bonus_draw)
        svg.shapes.push_back(bonus);
#endif

    std::ofstream file("out.svg");
    file << "<!DOCTYPE svg>\n" << svg << std::endl;
    file.close();
}