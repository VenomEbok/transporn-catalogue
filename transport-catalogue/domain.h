#pragma once
#include <string>
#include <vector>
#include "geo.h"
namespace catalogue {
namespace detail {
struct Stop {
	Stop(std::string n, double lat, double lng) :name{ n }, latitude{ lat }, longitude{ lng } {}
	std::string name;
	double latitude;
	double longitude;
};

struct Bus {
	Bus(std::string n, std::vector<Stop*> st) :name{ n }, stops{ st } {}
	std::string name;
	std::vector<Stop*> stops;
};

struct StopsPairHasher {
	size_t operator()(const std::pair<Stop*, Stop*>& pair_of_stops)const {
		return static_cast<size_t>(std::hash<const void*>()(pair_of_stops.first) * 1000 + std::hash<const void*>()(pair_of_stops.second) * 100000);
	}
};

}
}