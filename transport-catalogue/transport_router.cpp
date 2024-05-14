#include "transport_router.h"
#include <unordered_map>
// Вставьте сюда решние из предыдущего спринта

void TransportRouter::SetWaitTime(int wait_time){
	wait_time_ = wait_time;
}

void TransportRouter::SetVelocity(double velocity){
	velocity_ = velocity;
}

int TransportRouter::GetWaitTime() const
{
	return wait_time_;
}

double TransportRouter::GetVelocity() const
{
	return velocity_;
}


void TransportRouter::ConstructGraph(catalogue::TransportCatalogue& catalogue, const json::Array& stops){
	size_t k = 0;

	graph::DirectedWeightedGraph<double> graph(stops.size()*2);
	for (const json::Node& stop : stops) {
		stop_edge[stop.AsString()] = {k,k + 1};
		graph.AddEdge(graph::Edge<double>{ k, k + 1, wait_time_ * 1.0, "", stop.AsString(), 0 });
		k += 2;
	}
	for (const auto& bubu : catalogue.GetAllBuses()) {
		for (int i = 0; i < bubu.stops.size() - 1; ++i) {
			int span_count=0;
			double road_distance = 0.0;
			for (int j = i + 1; j < bubu.stops.size(); ++j) {
				road_distance += (catalogue.DistanceBetweenStops(bubu.stops[j-1]->name, bubu.stops[j]->name)) * 1.0;
				graph.AddEdge(graph::Edge<double>{stop_edge[bubu.stops[i]->name].second, stop_edge[bubu.stops[j]->name].first, (road_distance) / (velocity_ * 100 / 6), bubu.name,"",++span_count});
			}
		}
	}
	graph_ = graph;
	router_ = std::make_unique<graph::Router<double>>(graph_);

}

std::unordered_map<std::string, std::pair<size_t, size_t>> TransportRouter::GetStopEdges()
{
	return stop_edge;
}

const graph::DirectedWeightedGraph<double>& TransportRouter::GetGraph()
{
	return graph_;
}

graph::Router<double>* TransportRouter::GetRouter()
{
		return router_.get();
}
