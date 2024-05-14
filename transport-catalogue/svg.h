#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>
namespace svg {

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap);

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join);

struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }
    double x = 0;
    double y = 0;
};

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
public:
    virtual void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

// ---------- Rgb ------------------

class Rgb {
public:
    Rgb() = default;
    Rgb(uint8_t r, uint8_t g, uint8_t b) {
        red = r;
        green = g;
        blue = b;
    }
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

// ---------- Rgba ------------------

class Rgba {
public:
    Rgba() = default;
    Rgba(uint8_t r, uint8_t g, uint8_t b, double o) : red(r),
        green(g),
        blue(b),
        opacity(o){
    }
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.0;
};

struct OstreamColor {
    std::ostream& out;

    void operator()(std::monostate) const {
        out << "none";
    }

    void operator()(std::string color) const {
        out << color;
    }

    void operator()(Rgb color) {
        out << "rgb(" << std::to_string(color.red) << "," << std::to_string(color.green) << "," << std::to_string(color.blue) << ")";
    }

    void operator()(Rgba color) {
        out << "rgba(" << std::to_string(color.red) << "," << std::to_string(color.green) << "," << std::to_string(color.blue)<<","<<color.opacity << ")";
    }
};

using Color = std::variant<std::monostate, std::string, svg::Rgb, svg::Rgba>;
inline const std::string NoneColor{"none"};

std::ostream& operator<<(std::ostream& out, Color color);

// ---------- PathProps ------------------

template<typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }

    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }

    Owner& SetStrokeWidth(double width) {
        stroke_width_ = std::move(width);
        return AsOwner();
    }

    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
        stroke_line_cap_ = std::move(line_cap);
        return AsOwner();
    }

    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
        stroke_line_join_ = std::move(line_join);
        return AsOwner();
    }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const {
        using namespace std::literals;

       // if (!std::holds_alternative<std::monostate>(fill_color_)) {
            out << " fill=\""sv;
            std::visit(OstreamColor{ out }, fill_color_);
            out<< "\""sv;
       // }
        if (!std::holds_alternative<std::monostate>(stroke_color_)) {
            out << " stroke=\""sv;
            std::visit(OstreamColor{ out }, stroke_color_);
            out << "\""sv;
        }
        if (stroke_width_) {
            out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
        }
        if (stroke_line_cap_) {
            out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv;
        }
        if (stroke_line_join_) {
            out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
        }
    }

private:
    Owner& AsOwner() {
        return static_cast<Owner&>(*this);
    }

    Color fill_color_;
    Color stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_line_cap_;
    std::optional<StrokeLineJoin> stroke_line_join_;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */

class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline> {
public:
    // Добавляет очередную вершину к ломаной линии
    Polyline& AddPoint(Point point);

    /*
     * Прочие методы и данные, необходимые для реализации элемента <polyline>
     */
private:
    void RenderObject(const RenderContext& context) const override;
    std::vector<Point> points;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final :public Object, public PathProps<Text> {
public:
    // Задаёт координаты опорной точки (атрибуты x и y)
    Text& SetPosition(Point pos);

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& SetOffset(Point offset);

    // Задаёт размеры шрифта (атрибут font-size)
    Text& SetFontSize(uint32_t size);

    // Задаёт название шрифта (атрибут font-family)
    Text& SetFontFamily(std::string font_family);

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& SetFontWeight(std::string font_weight);

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& SetData(std::string data);

    // Прочие данные и методы, необходимые для реализации элемента <text>
private:
    void RenderObject(const RenderContext& context) const override;
    Point position_ = { 0,0 };
    Point offset_ = { 0,0 };
    std::string font_family_;
    std::string font_weight_;
    std::string data_="";
    uint32_t font_size_=1;
};

// ---------- ObjectContainer ------------------

class ObjectContainer{
public:
    template<typename Obj>
    void Add(Obj obj) {
        objects_.emplace_back(std::make_unique<Obj>(std::move(obj)));
    }
    virtual ~ObjectContainer() = default;
    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
protected:
    std::vector<std::unique_ptr<Object>> objects_;
};

// ---------- Drawable ------------------

class Drawable{
public:
    virtual void Draw(ObjectContainer& container) const = 0;
    virtual ~Drawable() = default;
};

// ---------- Document ------------------

class Document: public ObjectContainer {
public:
    // Добавляет в svg-документ объект-наследник svg::Object
    void AddPtr(std::unique_ptr<Object>&& obj) override;

    // Выводит в ostream svg-представление документа
    void Render(std::ostream& out) const;

};

}  // namespace svg

namespace shapes {

// ---------- Triangle ------------------

 class Triangle : public svg::Drawable {
 public:
     Triangle(svg::Point p1, svg::Point p2, svg::Point p3);

        // Реализует метод Draw интерфейса svg::Drawable
     void Draw(svg::ObjectContainer& container) const override;

 private:
     svg::Point p1_, p2_, p3_;
 };

// ---------- Star ------------------

class Star :public svg::Drawable {
public:
    Star(svg::Point center, double outer_radius, double inner_radius, int num_rays);
    void Draw(svg::ObjectContainer& container) const override;
private:
    svg::Polyline star_;
};

// ---------- Snowman ------------------

class Snowman :public svg::Drawable {
public:
    Snowman(svg::Point center, double radius);
    void Draw(svg::ObjectContainer& container) const override;
private:
    svg::Circle circles[3];
};

}