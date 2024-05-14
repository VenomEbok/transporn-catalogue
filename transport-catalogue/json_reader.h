#pragma once
#include <sstream>
#include <unordered_set>
#include "transport_router.h"

class JSONReader {
public:
	JSONReader(catalogue::TransportCatalogue& catalogue);
	json::Document LoadJSON(const std::string& s);

	void BaseRequest(catalogue::TransportCatalogue& catalogue, std::istream& input, std::ostream& output);

	std::string Print(const json::Node& node);

	void ApplyCommands(json::Document& commands, catalogue::TransportCatalogue& catalogue);

	json::Dict PrintGraph(const json::Node& req);

	void ParseAndPrintStat(json::Document& commands, const catalogue::TransportCatalogue& catalogue, std::ostream& output);

private:
	std::unordered_map < std::string_view, int> ParseDistances(const json::Dict& stops);

	std::vector<std::string_view> ParseRoute(const json::Array& route, const bool& is_roundtrip);
	TransportRouter transport_router_;
	json::Array SetToArray(std::set<std::string_view> original);
	catalogue::TransportCatalogue& catalogue_;
};
