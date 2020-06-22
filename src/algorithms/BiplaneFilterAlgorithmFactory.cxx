#include "BiplaneFilterAlgorithmFactory.h"
using namespace std;

vector<string> BiplaneFilterAlgorithmFactory::filterTypes;
string BiplaneFilterAlgorithmFactory::filterTypeLastCreated;

#include <iostream>

// --> include all filter headers here
#include "BiplaneAlgorithm.h"
#include "SimpleBiplaneFilter.h"


void BiplaneFilterAlgorithmFactory::generateFilterTypesList()
{
	filterTypes.clear();

	// --> insert all filter names here
	filterTypes.push_back("Crosscorrelation Biplane Filter");

}

BiplaneAlgorithm* BiplaneFilterAlgorithmFactory::createDefaultAlgorithm()
{
	filterTypeLastCreated = "Crosscorrelation Biplane Filter";
	return BiplaneAlgorithm::New();
}

BiplaneAlgorithm* BiplaneFilterAlgorithmFactory::createAlgorithm(string type) {
	filterTypeLastCreated = type;	
	
	// --> insert statement for each filter type here
	if(type == "Crosscorrelation Biplane Filter") return SimpleBiplaneFilter::New();

	cout << "BiplaneFilterAlgorithmFactory: The default filter was created instead of the unavailable requested type '" << type << "'!" << endl;
	return BiplaneFilterAlgorithmFactory::createDefaultAlgorithm();
}

vector<string> BiplaneFilterAlgorithmFactory::getFilterTypes() {
	if(filterTypes.empty()) generateFilterTypesList();

	return filterTypes;
}

string BiplaneFilterAlgorithmFactory::getFilterTypeLastCreated()
{
	return filterTypeLastCreated;
}

bool BiplaneFilterAlgorithmFactory::hasFilterType(string type) {
	if(filterTypes.empty()) generateFilterTypesList();

	for(unsigned int i = 0; i < filterTypes.size(); ++i) {
		if(type == filterTypes[i]) return true;
	}

	return false;
}