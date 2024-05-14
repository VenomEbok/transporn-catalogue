
#include <unordered_set>
#include "transport_catalogue.h"
namespace catalogue{
using namespace detail;
void TransportCatalogue::AddStop(const std::string& stop_name, geo::Coordinates coordinates) {
	Stop* stop = &stops_.emplace_back(Stop(stop_name, std::move(coordinates.lat), std::move(coordinates.lng)));
	stopname_to_stop_[stop->name] = stop;
	buses_for_stop[stop->name] = {};
}

Stop* TransportCatalogue::FindStop(std::string_view stop_name) {
	return stopname_to_stop_.at(std::string{ stop_name });
}

const Stop* TransportCatalogue::FindStop(std::string_view stop_name) const {
	if (stopname_to_stop_.count(std::string{ stop_name })) {
		return stopname_to_stop_.at(std::string{ stop_name });
	}
	return NULL;
}

void TransportCatalogue::AddBus(const std::string& bus_name,const std::vector<std::string_view>& stops) {
 	std::vector<Stop*> stops_for_bus;
	std::string name = bus_name;
	for (auto& stop : stops) {
		stops_for_bus.push_back(FindStop(stop));
	}
	Bus* bus = &buses_.emplace_back(Bus(bus_name, std::move(stops_for_bus)));
	busname_to_bus_[bus->name] = bus;
	for (auto& stop : stops) {
		buses_for_stop.at(stop).push_back(bus->name);
	}
}

Bus* TransportCatalogue::FindBus(std::string_view bus_name) {
	return busname_to_bus_[std::string{bus_name}];
}

const Bus* TransportCatalogue::FindBus(std::string_view bus_name)const {
	if (busname_to_bus_.count(std::string{ bus_name })) {
		return busname_to_bus_.at(std::string{ bus_name });
	}
	return NULL;
}

void TransportCatalogue::AddStopDistances(std::string_view stop_name, std::unordered_map<std::string_view, int> distances) {
	for (auto dist : distances) {
		stop_ptr_pair.insert_or_assign({FindStop(stop_name),FindStop(dist.first)}, dist.second);
	}
}

int TransportCatalogue::DistanceBetweenStops(std::string_view stop1,std::string_view stop2) const {
	const Stop* from = FindStop(stop1);
	const Stop* to = FindStop(stop2);
	if (stop_ptr_pair.count({ const_cast<Stop*>(from),const_cast<Stop*>(to) })) {
		return stop_ptr_pair.at({ const_cast<Stop*>(from),const_cast<Stop*>(to) });
	}
	else if(stop_ptr_pair.count({ const_cast<Stop*>(to),const_cast<Stop*>(from) })){
		return stop_ptr_pair.at({ const_cast<Stop*>(to),const_cast<Stop*>(from) });
	}
	return 0;
}

const std::deque<detail::Bus> TransportCatalogue::GetAllBuses() const {
	return buses_;
}

 std::tuple<int, int, double,double> TransportCatalogue::GetBusInfo(std::string_view bus_name)const {
	const Bus* bus = FindBus(bus_name);
	if (!bus) {
		return { 0, 0, 0 ,0};
	}
	int unique_stops;
	int all_stops = static_cast<int>(bus->stops.size());
	double geographical_distance=0;
	int actual_distance = 0;
	std::unordered_set<Stop*> unique;
	unique.insert(bus->stops.begin(), bus->stops.end());
	unique_stops = static_cast<int>(unique.size());
	for (int i = 0; i < static_cast<int>(all_stops) - 1; ++i) {
		geographical_distance += geo::ComputeDistance({ bus->stops[i]->latitude,bus->stops[i]->longitude }, { bus->stops[i + 1]->latitude,bus->stops[i + 1]->longitude });
		actual_distance += DistanceBetweenStops(bus->stops[i]->name, bus->stops[i + 1]->name);
	}
	return { all_stops,unique_stops,actual_distance, actual_distance/geographical_distance };
}

 std::set<std::string_view> TransportCatalogue::GetStopInfo(std::string_view stop_name)const {
	 std::set<std::string_view> stop_info;
	 if (buses_for_stop.at(stop_name).empty()) {
		 return stop_info;
	 }
	 stop_info.insert(buses_for_stop.at(stop_name).begin(), buses_for_stop.at(stop_name).end());
	 return stop_info;
 }
 }