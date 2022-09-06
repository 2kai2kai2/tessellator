#include <fstream>
#include <iostream>
#include <set>
#include <list>

struct SVG_Polygon {
    std::list<std::pair<long long, long long>> points;
    std::string color;

    friend std::ostream& operator<<(std::ostream& a, const SVG_Polygon& b) {
        a << "<polygon points=\"";
        for (const std::pair<long long, long long>& p : b.points)
            a << p.first << ',' << p.second << ' ';
        a << "\" style=\"";
        if (!b.color.empty())
            a << "fill:" << b.color << ";";
        return a << "\" />";
    }
};

struct SVG {
    size_t height;
    size_t width;
    std::list<SVG_Polygon> polygons;

    SVG(size_t height, size_t width) : height(height), width(width) {}

    friend std::ostream& operator<<(std::ostream& a, const SVG& b) {
        a << "<svg xmlns=\"http://www.w3.org/2000/svg\" height=\"" << b.height << "\" width=\"" << b.width << "\">\n";
        for (const SVG_Polygon& poly : b.polygons)
            a << poly << '\n';
        return a << "</svg>";
    }
};