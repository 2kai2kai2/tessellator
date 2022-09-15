#pragma once

#include <fstream>
#include <iostream>
#include <list>
#include <set>
#include <utility>

struct SVG_Tag {
    virtual ~SVG_Tag() = default;

    virtual std::ostream& print(std::ostream& ostr) const = 0;
    friend std::ostream& operator<<(std::ostream& a, const SVG_Tag& b) {
        return b.print(a);
    }
};

struct SVG_Shape : SVG_Tag {};
struct SVG_Def : SVG_Tag {};

struct SVG_Line : SVG_Shape {
    double x1;
    double y1;
    double x2;
    double y2;
    std::string color;
    int width;

    SVG_Line(double x1, double y1, double x2, double y2)
        : x1(x1), y1(y1), x2(x2), y2(y2), width(-1) {}

    std::ostream& print(std::ostream& a) const override {
        a << "<line x1=\"" << x1 << "\" y1=\"" << y1 << "\" x2=\"" << x2
          << "\" y2=\"" << y2 << "\" style=\"";
        if (!color.empty())
            a << "stroke:" << color << ";";
        if (width >= 0)
            a << "stroke-width:" << width << ";";
        return a << "\" />";
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
struct SVG_Text : SVG_Shape {
    double x;
    double y;
    std::string text;
    std::string color;

    SVG_Text(double x, double y, std::string text, std::string color = "")
        : x(x), y(y), text(std::move(text)), color(std::move(color)) {}

    std::ostream& print(std::ostream& a) const override {
        a << "<text x=\"" << x << "\" y=\"" << y << '"';
        if (!color.empty())
            a << " fill=\"" << color << '"';
        return a << '>' << text << "</text>";
    }
};

struct SVG_LinearGradient : SVG_Def {
    std::string id;
    double x1;
    double y1;
    double x2;
    double y2;
    std::list<std::pair<double, std::string>> stops;

    SVG_LinearGradient(std::string id, double x1, double y1, double x2,
                       double y2,
                       std::list<std::pair<double, std::string>> stops)
        : id(std::move(id)), x1(x1), y1(y1), x2(x2), y2(y2),
          stops(std::move(stops)) {}
    SVG_LinearGradient(std::string id, double x1, double y1, double x2,
                       double y2, std::string color1, std::string color2)
        : id(std::move(id)), x1(x1), y1(y1), x2(x2), y2(y2),
          stops({{0.0, std::move(color1)}, {100.0, std::move(color2)}}) {}

    std::ostream& print(std::ostream& a) const override {
        a << "<linearGradient id=\"" << id << "\" x1=\"" << x1 << "%\" y1=\""
          << y1 << "%\" x2=\"" << x2 << "%\" y2=\"" << y2 << "%\">\n";
        for (const std::pair<double, std::string>& stop : stops) {
            a << "  <stop offset=\"" << stop.first << "%\" stop-color=\""
              << stop.second << "\" />\n";
        }
        return a << "</linearGradient>";
    }
};

struct SVG {
    size_t height;
    size_t width;
    std::list<SVG_Def*> defs;
    std::list<SVG_Shape*> shapes;

    SVG(size_t height, size_t width) : height(height), width(width) {}
    ~SVG() {
        for (SVG_Def* def : defs)
            delete def;
        for (SVG_Shape* shape : shapes)
            delete shape;
    }

    friend std::ostream& operator<<(std::ostream& a, const SVG& b) {
        a << "<svg xmlns=\"http://www.w3.org/2000/svg\" height=\"" << b.height
          << "\" width=\"" << b.width << "\">\n";
        if (!b.defs.empty()) {
            a << "<defs>\n";
            for (const SVG_Def* def : b.defs)
                a << *def << '\n';
            a << "</defs>\n";
        }
        for (const SVG_Shape* shape : b.shapes)
            a << *shape << '\n';
        return a << "</svg>";
    }
};