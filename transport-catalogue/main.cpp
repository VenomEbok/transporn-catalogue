#include "json_reader.h"
#include <fstream>
using namespace std;

int main() {
    catalogue::TransportCatalogue catalogue;
    JSONReader reader(catalogue);
    reader.BaseRequest(catalogue, cin, cout);
}