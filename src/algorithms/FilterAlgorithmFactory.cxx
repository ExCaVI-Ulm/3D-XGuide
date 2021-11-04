#include "FilterAlgorithmFactory.h"
using namespace std;

vector<string> FilterAlgorithmFactory::filterTypes;
string FilterAlgorithmFactory::filterTypeLastCreated;

#include <iostream>

// --> include all filter headers here
#include "CrosscorrelationMotionCorrectionFilter.h"
#include "DeepLearningMotionCorrectionFilter.h"


void FilterAlgorithmFactory::generateFilterTypesList()
{
	filterTypes.clear();

	// --> insert all filter names here
	filterTypes.push_back("None");
	filterTypes.push_back("Crosscorrelation Motion Correction Filter");
	filterTypes.push_back("Deep Learning-based Motion Correction Filter");


}

MotionCorrectionFilter* FilterAlgorithmFactory::createDefaultAlgorithm()
{

	filterTypeLastCreated = "Crosscorrelation Motion Correction Filter";
	return CrosscorrelationMotionCorrectionFilter::New();
}

MotionCorrectionFilter* FilterAlgorithmFactory::createAlgorithm(string type) {
	filterTypeLastCreated = type;

	if (type == "None") return MotionCorrectionFilter::New();
	// --> insert statement for each filter type here
	if(type == "Crosscorrelation Motion Correction Filter") return CrosscorrelationMotionCorrectionFilter::New();
	if (type == "Deep Learning-based Motion Correction Filter") return DeepLearningMotionCorrectionFilter::New();

	cout << "FilterAlgorithmFactory: The default filter was created instead of the unavailable requested type '" << type << "'!" << endl;
	return FilterAlgorithmFactory::createDefaultAlgorithm();
}

vector<string> FilterAlgorithmFactory::getFilterTypes() {
	if(filterTypes.empty()) generateFilterTypesList();

	return filterTypes;
}

string FilterAlgorithmFactory::getFilterTypeLastCreated()
{
	return filterTypeLastCreated;
}

bool FilterAlgorithmFactory::hasFilterType(string type) {
	if(filterTypes.empty()) generateFilterTypesList();

	for(unsigned int i = 0; i < filterTypes.size(); ++i) {
		if(type == filterTypes[i]) return true;
	}

	return false;
}