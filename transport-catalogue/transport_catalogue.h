#pragma once
#include "domain.h"
#include <string_view>
#include <unordered_map>
#include <deque>
#include <set>

namespace catalogue{

class TransportCatalogue {
public:
	void AddStop(const std::string& stop_name, geo::Coordinates coordinates);
	const detail::Stop* FindStop(std::string_view stop_name)const;
	detail::Stop* FindStop(std::string_view stop_name);
	void AddBus(const std::string& bus_name,const std::vector<std::string_view>& stops);
	const detail::Bus* FindBus(std::string_view bus_name)const;
	detail::Bus* FindBus(std::string_view bus_name);
	void AddStopDistances(std::string_view stop_name, std::unordered_map<std::string_view, int> distances);
	int DistanceBetweenStops(std::string_view from, std::string_view to) const;
	const std::deque<detail::Bus> GetAllBuses() const;
	std::tuple<int, int, double , double > GetBusInfo(std::string_view bus_name)const;
	std::set<std::string_view> GetStopInfo(std::string_view stop_name) const;
private:
	std::unordered_map<std::string_view, detail::Stop*> stopname_to_stop_;
	std::deque<detail::Stop> stops_;
	std::unordered_map<std::string_view, detail::Bus*> busname_to_bus_;
	std::deque<detail::Bus> buses_;
	std::unordered_map<std::string_view, std::vector<std::string_view>> buses_for_stop;
	std::unordered_map<std::pair<detail::Stop*, detail::Stop*>, int,detail::StopsPairHasher> stop_ptr_pair;
};
}