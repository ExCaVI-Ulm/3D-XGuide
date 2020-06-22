#ifndef FILTER_ALGORITHM_FACTORY_H
#define FILTER_ALGORITHM_FACTORY_H

#include <vector>
#include <string>
//using namespace std;

class MotionCorrectionFilter;

/** This class creates a MotionCorrectionFilter subclass of the requested type.
    If the type is not available, a default MotionCorrectionFilter will be returned.
  */
class FilterAlgorithmFactory {
public:
	static MotionCorrectionFilter* createAlgorithm(std::string type);

	static MotionCorrectionFilter* createDefaultAlgorithm();

	static std::vector<std::string> getFilterTypes();

	static std::string getFilterTypeLastCreated();

	static bool hasFilterType(std::string type);

private:
	static std::vector<std::string> filterTypes;
	static std::string filterTypeLastCreated;

	static void generateFilterTypesList();

};

#endif //FILTER_ALGORITHM_FACTORY_H