#include "map_renderer.h"

namespace renderer {
inline const double EPSILON = 1e-6;
bool IsZero(double value) {
	return std::abs(value) < EPSILON;
}

svg::Color MapRenderer::FillColor(const json::Node& colors) {
	if (colors.IsArray()) {
		if (colors.AsArray().size() == 3) {
			return svg::Color(svg::Rgb(colors.AsArray()[0].AsInt(), colors.AsArray()[1].AsInt(), colors.AsArray()[2].AsInt()));
		}
		else {
			return svg::Color(svg::Rgba(colors.AsArray()[0].AsInt(), colors.AsArray()[1].AsInt(), colors.AsArray()[2].AsInt(), colors.AsArray()[3].AsDouble()));
		}
	}
	return svg::Color(colors.AsString());
}


void MapRenderer::FillRenderSettings(const json::Dict& setting) {
	render_settings_.width = setting.at("width").AsDouble();
	render_settings_.height = setting.at("height").AsDouble();
	render_settings_.padding = setting.at("padding").AsDouble();
	render_settings_.line_width = setting.at("line_width").AsDouble();
	render_settings_.stop_radius = setting.at("stop_radius").AsDouble();
	render_settings_.bus_label_font_size = setting.at("bus_label_font_size").AsInt();
	for (const auto& elem : setting.at("bus_label_offset").AsArray()) {
		render_settings_.bus_label_offset.push_back(elem.AsDouble());
	}
	render_settings_.stop_label_font_size = setting.at("stop_label_font_size").AsInt();
	for (const auto& elem : setting.at("stop_label_offset").AsArray()) {
		render_settings_.stop_label_offset.push_back(elem.AsDouble());
	}
	render_settings_.underlayer_color = FillColor(setting.at("underlayer_color"));
	render_settings_.underlayer_width = setting.at("underlayer_width").AsDouble();
	for (const auto& color : setting.at("color_palette").AsArray()) {
		render_settings_.color_palette.emplace_back(FillColor(color));
	}

}

RenderSettings MapRenderer::GetRenderSettings() {
	return render_settings_;
}


void MapRenderer::FillText(const geo::Coordinates& point, svg::Text& text, const renderer::SphereProjector& proj, const std::string& bus_name) {
	text.SetPosition(proj(point)).SetOffset(svg::Point(render_settings_.bus_label_offset[0], render_settings_.bus_label_offset[1])).SetFontSize(render_settings_.bus_label_font_size).SetFontFamily("Verdana").SetData(bus_name).SetFontWeight("bold").SetFillColor("black");

	}

void MapRenderer::FillStops(svg::Document& map, const renderer::SphereProjector& proj, const catalogue::detail::Stop* stop, std::vector<svg::Text>& stops) {
	svg::Circle circle;
	circle.SetCenter(proj(geo::Coordinates{stop->latitude,stop->longitude})).SetRadius(render_settings_.stop_radius).SetFillColor("white");
	map.Add(circle);

	svg::Text stop_label;
	stop_label.SetPosition(proj(geo::Coordinates{ stop->latitude,stop->longitude })).SetOffset(svg::Point(render_settings_.stop_label_offset[0], render_settings_.stop_label_offset[1])).SetFontSize(render_settings_.stop_label_font_size).SetFontFamily("Verdana").SetData(stop->name);
	svg::Text underlayer = stop_label;
	underlayer.SetFillColor(render_settings_.underlayer_color).SetStrokeColor(render_settings_.underlayer_color).SetStrokeWidth(render_settings_.underlayer_width).SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
	stop_label.SetFillColor("black");
	stops.push_back(underlayer);
	stops.push_back(stop_label);
}

void MapRenderer::FillMap(const catalogue::detail::Bus* bus, svg::Document& map, const renderer::SphereProjector& proj, int& number, bool is_roundtrip, std::vector<svg::Text>& buses) {
	if (bus->stops.size() == 0) {
		return;
	}

	svg::Polyline polyline;
	polyline.SetStrokeWidth(render_settings_.line_width);
	polyline.SetStrokeColor(render_settings_.color_palette[number % render_settings_.color_palette.size()]);
	polyline.SetFillColor(svg::Color());
	polyline.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
	polyline.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
	for (const auto& point : bus->stops) {
		polyline.AddPoint(proj(geo::Coordinates{point->latitude,point->longitude}));
	}
	map.Add(polyline);

	svg::Text underlayer;
	FillText(geo::Coordinates{bus->stops[0]->latitude,bus->stops[0]->longitude}, underlayer, proj, bus->name);
	underlayer.SetFillColor(render_settings_.underlayer_color).SetStrokeColor(render_settings_.underlayer_color).SetStrokeWidth(render_settings_.underlayer_width).SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
	buses.push_back(underlayer);

	svg::Text bus_label;
	FillText(geo::Coordinates{ bus->stops[0]->latitude,bus->stops[0]->longitude }, bus_label, proj, bus->name);
	bus_label.SetFillColor(render_settings_.color_palette[number % render_settings_.color_palette.size()]);
	buses.push_back(bus_label);

	if (!is_roundtrip && (geo::Coordinates{ bus->stops[0]->latitude,bus->stops[0]->longitude } != geo::Coordinates{ bus->stops[bus->stops.size() / 2]->latitude,bus->stops[bus->stops.size() / 2]->longitude })) {
		svg::Text underlayer_copy = underlayer;
		underlayer_copy.SetPosition(proj(geo::Coordinates{ bus->stops[bus->stops.size() / 2]->latitude,bus->stops[bus->stops.size() / 2]->longitude }));
		buses.push_back(underlayer_copy);
		svg::Text bus_label_copy = bus_label;
		bus_label_copy.SetPosition(proj(geo::Coordinates{ bus->stops[bus->stops.size() / 2]->latitude,bus->stops[bus->stops.size() / 2]->longitude }));
		buses.push_back(bus_label_copy);
	}
	number += 1;
}
}
