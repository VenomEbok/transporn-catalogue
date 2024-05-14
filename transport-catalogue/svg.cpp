#define _USE_MATH_DEFINES
#include "svg.h"
#include <cmath>
namespace svg {

using namespace std::literals;

std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
    if (line_cap == StrokeLineCap::BUTT) {
        out << "butt";
    }
    else if (line_cap == StrokeLineCap::ROUND) {
        out << "round";
    }
    else if (line_cap == StrokeLineCap::SQUARE) {
        out << "square";
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
    if (line_join == StrokeLineJoin::ARCS) {
        out << "arcs";
    }
    else if (line_join == StrokeLineJoin::BEVEL) {
        out << "bevel";
    }
    else if (line_join == StrokeLineJoin::MITER) {
        out << "miter";
    }
    else if (line_join == StrokeLineJoin::MITER_CLIP) {
        out << "miter-clip";
    }
    else if (line_join == StrokeLineJoin::ROUND) {
        out << "round";
    }
    return out;
}

// ---------- Object ------------------

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\" "sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point) {
    points.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    bool is_first = true;
    for (const auto& point : points) {
        if (!is_first) {
            out << " ";
        }
        out << point.x << "," << point.y;
        is_first = false;
    }
    out << "\" "sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}

// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj){
    objects_.emplace_back(std::move(obj));
}

void Document::Render(std::ostream& out) const{
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
    RenderContext ctx(out, 2, 2);
    for (const auto& object : objects_) {
        object.get()->Render(ctx);
    }

    out << "</svg>"sv;
}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos)
{
    position_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset)
{
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size)
{
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family)
{
    font_family_ = font_family;
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight)
{
    font_weight_ = font_weight;
    return *this;
}

Text& Text::SetData(std::string data)
{
    data_ = data;
    return *this;
}

void Text::RenderObject(const RenderContext& context) const{
    auto& out = context.out;
    out << "<text"sv;
    RenderAttrs(context.out);
    out<<" x=\""sv << position_.x << "\" y=\"" << position_.y << "\" dx=\"" << offset_.x << "\" dy=\"" << offset_.y << "\" font-size=\"" << font_size_ << "\"";
    if (!font_family_.empty()) {
        out<< " font-family=\"" << font_family_ << "\"";
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\"" << font_weight_ << "\"";
    }
    out << ">";
    out << data_ << "</text>";
}

std::ostream& operator<<(std::ostream& out, Color color) {
    std::visit(OstreamColor{ out }, color);
    return out;
}

}  // namespace svg

namespace shapes {

// ---------- Triangle ------------------
Triangle::Triangle(svg::Point p1, svg::Point p2, svg::Point p3)
    : p1_(p1)
    , p2_(p2)
    , p3_(p3) {
}

void Triangle::Draw(svg::ObjectContainer& container) const {
    container.Add(svg::Polyline().AddPoint(p1_).AddPoint(p2_).AddPoint(p3_).AddPoint(p1_));
}

// ---------- Star ------------------

Star::Star(svg::Point center, double outer_radius, double inner_radius, int num_rays){
    for (int i = 0; i <= num_rays; ++i) {
        double angle = 2 * M_PI * (i % num_rays) / num_rays;
        star_.AddPoint({ center.x + outer_radius * sin(angle), center.y - outer_radius * cos(angle) });
        if (i == num_rays) {
            break;
        }
        angle += M_PI / num_rays;
        star_.AddPoint({ center.x + inner_radius * sin(angle), center.y - inner_radius * cos(angle) });
    }
    star_.SetStrokeColor("black");
    star_.SetFillColor("red");
}

void Star::Draw(svg::ObjectContainer& container) const{
    container.Add(star_);
}

Snowman::Snowman(svg::Point center, double radius){
    circles[2].SetRadius(radius).SetCenter(center);
    circles[1].SetRadius(1.5 * radius).SetCenter(svg::Point{ center.x,center.y + 2 * radius });
    circles[0].SetRadius(2 * radius).SetCenter(svg::Point{ center.x,center.y + 5 * radius });
    for (int i = 0; i < 3; ++i) {
        circles[i].SetFillColor(svg::Rgb{240,240,240});
        circles[i].SetStrokeColor("black");
    }
}

void Snowman::Draw(svg::ObjectContainer& container) const{
    container.Add(circles[0]);
    container.Add(circles[1]);
    container.Add(circles[2]);
}

}