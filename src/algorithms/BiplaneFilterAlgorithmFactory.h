#ifndef BIPLANE_FILTER_ALGORITHM_FACTORY_H
#define BIPLANE_FILTER_ALGORITHM_FACTORY_H

#include <vector>
#include <string>
//using namespace std;

class BiplaneAlgorithm;

/** This class creates a BiplaneAlgorithm subclass of the requested type.
    If the type is not available, a default BiplaneAlgorithm will be returned.
  */
class BiplaneFilterAlgorithmFactory {
public:
	static BiplaneAlgorithm* createAlgorithm(std::string type);

	static BiplaneAlgorithm* createDefaultAlgorithm();

	static std::vector<std::string> getFilterTypes();

	static std::string getFilterTypeLastCreated();

	static bool hasFilterType(std::string type);

private:
	static std::vector<std::string> filterTypes;
	static std::string filterTypeLastCreated;

	static void generateFilterTypesList();

};

#endif //BIPLANE_FILTER_ALGORITHM_FACTORY_H