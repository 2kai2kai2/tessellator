#include <fstream>
#include <iostream>
#include <list>
#include <set>

struct SVG_Shape {
    virtual ~SVG_Shape() {}

    virtual std::ostream& print(std::ostream& ostr) const = 0;
    friend std::ostream& operator<<(std::ostream& a, const SVG_Shape& b) {
        return b.print(a);
    }
};

struct SVG_Polygon : SVG_Shape {
    std::list<std::pair<long long, long long>> points;
    std::string color;

    std::ostream& print(std::ostream& a) const override {
        a << "<polygon points=\"";
        for (const std::pair<long long, long long>& p : points)
            a << p.first << ',' << p.second << ' ';
        a << "\" style=\"";
        if (!color.empty())
            a << "fill:" << color << ";";
        return a << "\" />";
    }
};

struct SVG_Circle : SVG_Shape {
    double cx;
    double cy;
    double radius;
    std::string stroke;
    int stroke_width;
    double stroke_opacity;
    std::string fill;
    double fill_opacity;

    SVG_Circle()
        : cx(0), cy(0), radius(0), stroke_width(-1), stroke_opacity(1),
          fill_opacity(1) {}
    SVG_Circle(double cx, double cy, double radius)
        : cx(cx), cy(cy), radius(radius), stroke_width(-1), stroke_opacity(1),
          fill_opacity(1) {}

    std::ostream& print(std::ostream& a) const override {
        a << "<circle cx=\"" << cx << "\" cy=\"" << cy << "\" r=\"" << radius
          << "\" ";
        if (!stroke.empty())
            a << "stroke=\"" << stroke << "\" ";
        if (stroke_width >= 0)
            a << "stroke-width=\"" << stroke_width << "\" ";
        if (stroke_opacity < 1)
            a << "stroke-opacity=\"" << stroke_opacity << "\" ";
        if (!fill.empty())
            a << "fill=\"" << fill << "\" ";
        if (fill_opacity < 1)
            a << "fill-opacity=\"" << fill_opacity << "\" ";
        return a << "/>";
    }
};

struct SVG {
    size_t height;
    size_t width;
    std::list<SVG_Shape*> shapes;

    SVG(size_t height, size_t width) : height(height), width(width) {}
    ~SVG() {
        for (SVG_Shape* shape : shapes)
            delete shape;
    }

    friend std::ostream& operator<<(std::ostream& a, const SVG& b) {
        a << "<svg xmlns=\"http://www.w3.org/2000/svg\" height=\"" << b.height
          << "\" width=\"" << b.width << "\">\n";
        for (const SVG_Shape* shape : b.shapes)
            a << *shape << '\n';
        return a << "</svg>";
    }
};