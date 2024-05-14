#include "router.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include <memory>
class TransportRouter {
public:

	explicit TransportRouter() = default;
	explicit TransportRouter(int wait_time, double velocity) : wait_time_(wait_time), velocity_(velocity) {
	}

	void SetWaitTime(int wait_time);
	void SetVelocity(double velocity);

	int GetWaitTime() const;
	double GetVelocity() const;

	void ConstructGraph(catalogue::TransportCatalogue& catalogue,const json::Array& stops);

	std::unordered_map<std::string, std::pair<size_t, size_t>> GetStopEdges();
	
	const graph::DirectedWeightedGraph<double>& GetGraph();

	graph::Router<double>* GetRouter();

private:
	int wait_time_ = 0;
	double velocity_ = 0.;
	graph::DirectedWeightedGraph<double> graph_;
	std::unique_ptr<graph::Router<double>> router_ = nullptr;
	std::unordered_map<std::string, std::pair<size_t, size_t>> stop_edge;
};