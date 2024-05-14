#include "json_reader.h"
#include <fstream>

JSONReader::JSONReader(catalogue::TransportCatalogue& catalogue):catalogue_(catalogue){
}

json::Document JSONReader::LoadJSON(const std::string& s) {
    std::istringstream strm(s);
    return json::Load(strm);
}

void JSONReader::BaseRequest(catalogue::TransportCatalogue& catalogue, std::istream& input, std::ostream& output) {
    json::Document command = json::Load(input);
    ApplyCommands(command, catalogue);
    ParseAndPrintStat(command, catalogue, output);
}

std::string JSONReader::Print(const json::Node& node) {
    std::ostringstream out;
    json::Print(json::Document{ node }, out);
    return out.str();
}


std::vector<geo::Coordinates> GetVectorOfCoordinates(const std::vector<catalogue::detail::Stop*> stops) {
    std::vector<geo::Coordinates> result;
    for (const auto& stop : stops) {
        result.push_back(geo::Coordinates{ stop->latitude,stop->longitude });
    }
    return result;
}

std::vector<geo::Coordinates> AllCoordinates(std::set<std::string> stops, const catalogue::TransportCatalogue& catalogue) {
    std::vector<geo::Coordinates> result;
    for (const auto& stop : stops) {
        if(!catalogue.GetStopInfo(stop).empty())
        result.push_back(geo::Coordinates{ catalogue.FindStop(stop)->latitude, catalogue.FindStop(stop)->longitude });
    }
    return result;
}

void ApplyRenderSettings(json::Document& commands,const catalogue::TransportCatalogue& catalogue, std::ostream& out) {
    renderer::MapRenderer map_renderer;
    map_renderer.FillRenderSettings(commands.GetRoot().AsMap().at("render_settings").AsMap());
    svg::Document map;
    std::vector<svg::Text> bus_label, stop_label;
    std::map<std::string, bool> buses;
    std::set<std::string> stops;
    const auto& com = commands.GetRoot().AsMap().at("base_requests").AsArray();
    for (const auto& info : com) {
        if (info.AsMap().at("type").AsString() == "Bus") {
            buses.insert({ info.AsMap().at("name").AsString(), info.AsMap().at("is_roundtrip").AsBool() });
        }
        else if (info.AsMap().at("type").AsString() == "Stop") {
            stops.insert(info.AsMap().at("name").AsString());
        }
    }
    std::vector<geo::Coordinates> coordinates=AllCoordinates(stops,catalogue);
    const renderer::SphereProjector proj{ coordinates.begin(),coordinates.end(), map_renderer.GetRenderSettings().width, map_renderer.GetRenderSettings().height, map_renderer.GetRenderSettings().padding};
    int counter = 0;
    for (auto& bus : buses) {
        map_renderer.FillMap(catalogue.FindBus(bus.first), map,proj, counter, bus.second, bus_label);
    }
    for (auto& bus : bus_label) {
        map.Add(bus);
    }

    for (auto& stop : stops) {
        if (!catalogue.GetStopInfo(stop).empty()) {
            map_renderer.FillStops(map, proj, catalogue.FindStop(stop), stop_label);
        }
    }
    for (auto& stop : stop_label) {
        map.Add(stop);
    }
    std::ofstream fout("hello.svg");
    map.Render(fout);
    fout.close();
    map.Render(out);
}



std::unordered_map < std::string_view, int> JSONReader::ParseDistances(const json::Dict& stops) {
    std::unordered_map<std::string_view, int> result;
    for (const auto& stop : stops) {
        result.emplace(stop.first, stop.second.AsInt());
    }
    return result;
}

std::vector<std::string_view> JSONReader::ParseRoute(const json::Array& route, const bool& is_roundtrip) {
    std::vector<std::string_view> result;
    for (const auto& stop : route) {
        result.push_back(stop.AsString());
    }
    if (!is_roundtrip) {
        for (size_t i = (route.size() - 2); i > 0; i--) {
            result.push_back(route[i].AsString());
        }
        result.push_back(route[0].AsString());
    }
    return result;
}

void JSONReader::ApplyCommands(json::Document& commands, catalogue::TransportCatalogue& catalogue) {
    if (!commands.GetRoot().IsMap()) {
        return;
    }
    std::unordered_map<std::string, json::Dict> stop_distances;
    json::Array buses;
    json::Array stops;
    const auto& com = commands.GetRoot().AsMap().at("base_requests").AsArray();
    for (const auto& info : com) {
        if (info.AsMap().at("type").AsString() == "Stop") {
            stop_distances[info.AsMap().at("name").AsString()] = info.AsMap().at("road_distances").AsMap();
            stops.push_back(info.AsMap().at("name").AsString());
            catalogue.AddStop(info.AsMap().at("name").AsString(), geo::Coordinates{ info.AsMap().at("latitude").AsDouble(), info.AsMap().at("longitude").AsDouble() });
        }
        else if (info.AsMap().at("type").AsString() == "Bus") {
            buses.push_back(info);
        }
    }

    for (const auto& stop : stop_distances) {
        catalogue.AddStopDistances(stop.first, ParseDistances(stop.second));
    }

    for (const auto& bus : buses) {
        catalogue.AddBus(bus.AsMap().at("name").AsString(), ParseRoute(bus.AsMap().at("stops").AsArray(), bus.AsMap().at("is_roundtrip").AsBool()));
    }

    const auto& rooting_settings = commands.GetRoot().AsMap().at("routing_settings").AsMap();
    transport_router_.SetVelocity(rooting_settings.at("bus_velocity").AsDouble());
    transport_router_.SetWaitTime(rooting_settings.at("bus_wait_time").AsInt());

    transport_router_.ConstructGraph(catalogue, stops);

}


json::Array JSONReader::SetToArray(std::set<std::string_view> original) {
    json::Array result;
    for (auto& element : original) {
        result.push_back(std::string(element));
    }
    return result;
}

json::Dict JSONReader::PrintGraph(const json::Node& req)
{
    using namespace std::literals;
    std::string from = req.AsMap().at("from").AsString();
    std::string to = req.AsMap().at("to").AsString();
    std::unordered_map<std::string, std::pair<size_t, size_t>> stops_edges = transport_router_.GetStopEdges();
    if (from == to) {
        return json::Builder{}.StartDict().Key("total_time").Value(0).Key("request_id").Value(req.AsMap().at("id").AsInt()).Key("items").StartArray().EndArray().EndDict().Build().AsMap();
    }
    else {
        const auto info = transport_router_.GetRouter()->BuildRoute(stops_edges.at(from).first, stops_edges.at(to).first);
        if (info.has_value()) {
            const std::vector<graph::EdgeId>& elem = info.value().edges;
            json::Array rout_arr;

            double total_time = 0.0;
            for (const graph::EdgeId& el : elem) {
                json::Dict item_map;
                const graph::Edge<double> edge = transport_router_.GetGraph().GetEdge(el);
                if (edge.bus.empty()) {
                    item_map["type"] = "Wait"s;
                    item_map["stop_name"] = edge.stop;
                    item_map["time"] = edge.weight;

                    rout_arr.push_back(item_map);
                }
                else {
                    item_map["type"] = "Bus"s;
                    item_map["bus"] = edge.bus;
                    item_map["span_count"] = edge.span_count;
                    item_map["time"] = edge.weight;
                    rout_arr.push_back(item_map);
                }
                total_time += edge.weight;
            }
            return json::Builder{}
                .StartDict()
                .Key("total_time").Value(total_time)
                .Key("request_id").Value(req.AsMap().at("id").AsInt()).Key("items").Value(rout_arr)
                .EndDict()
                .Build()
                .AsMap();
        }
        
    }
    return
        json::Builder{}
        .StartDict()
        .Key("request_id").Value(req.AsMap().at("id").AsInt())
        .Key("error_message").Value("not found")
        .EndDict()
        .Build()
        .AsMap();
}

void JSONReader::ParseAndPrintStat(json::Document& commands, const catalogue::TransportCatalogue& catalogue, std::ostream& output) {
    using namespace std::literals;
    if (!commands.GetRoot().IsMap()) {
        return;
    }
    const auto& requests = commands.GetRoot().AsMap().at("stat_requests").AsArray();
    json::Array all_stat;
    for (const auto& req : requests) {
        if (req.AsMap().at("type").AsString() == "Bus") {
            if (!catalogue.FindBus(req.AsMap().at("name").AsString())) {
                all_stat.push_back(json::Builder{}.StartDict().Key("request_id"s).Value(req.AsMap().at("id").AsInt()).Key("error_message"s).Value("not found"s).EndDict().Build());
            }
            else {
                auto [all_stops, unique_stops, actual_distance, curvature] = catalogue.GetBusInfo(req.AsMap().at("name").AsString());
                all_stat.push_back(json::Builder{}.StartDict().Key("curvature"s).Value(curvature).Key("request_id"s).Value(req.AsMap().at("id").AsInt()).Key("route_length"s)
                    .Value(actual_distance).Key("stop_count"s).Value(all_stops).Key("unique_stop_count"s).Value(unique_stops).EndDict().Build());
            }
        }
        else if (req.AsMap().at("type").AsString() == "Stop") {
            if (!catalogue.FindStop(req.AsMap().at("name").AsString())) {
                all_stat.push_back(json::Builder{}.StartDict().Key("request_id"s).Value(req.AsMap().at("id").AsInt()).Key("error_message"s).Value("not found"s).EndDict().Build());
            }
            else {
                std::set<std::string_view> buses_for_stop = catalogue.GetStopInfo(req.AsMap().at("name").AsString());
                json::Array ar = SetToArray(buses_for_stop);
                all_stat.push_back(json::Builder{}.StartDict().Key("buses"s).Value(ar).Key("request_id"s).Value(req.AsMap().at("id").AsInt()).EndDict().Build());
            }
        }
        else if (req.AsMap().at("type").AsString() == "Map") {
            std::ostringstream map;
            ApplyRenderSettings(commands, catalogue, map);
            all_stat.push_back(json::Builder{}.StartDict().Key("map"s).Value(map.str()).Key("request_id"s).Value(req.AsMap().at("id").AsInt()).EndDict().Build());
        }
        else if (req.AsMap().at("type").AsString() == "Route") {
            all_stat.push_back(PrintGraph(req));
        }
    }
    json::Print(json::Document{ json::Node{all_stat} }, output);
}
