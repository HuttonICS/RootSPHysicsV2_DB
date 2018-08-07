#pragma once



#ifndef _GENCASEBIS_T_
#define _GENCASEBIS_T_

#include "JPartDataBi4.h"
#include "JXml.h"
#include "TypesDef.h"
#include "JSph.h"
#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

class GenCaseBis_T
{

private:
	JPartDataBi4 *jpd;
	boolean test = true;

public:
	GenCaseBis_T();
	~GenCaseBis_T();
	void GenCaseBis_T::Bridge(std::string runPath, std::string caseName);
private:
	int GenCaseBis_T::calculNbParticles(std::string csvPath);
	std::vector<std::string> GenCaseBis_T::split(std::string line, char delim);
	void GenCaseBis_T::loadCsv(std::string csvName, int np, int *idp, double *vol, tdouble3 *pos);
	float GenCaseBis_T::loadRhop0(std::string caseName);
	double GenCaseBis_T::computeRayMax(int np, double *vol);
	void GenCaseBis_T::computeMassP(int np, double *vol, float *mp, float *rhop, float rhop0);
	void GenCaseBis_T::researchCasePosMaxAndMin(tdouble3 *pos, int np, tdouble3 *posMax, tdouble3 *posMin);
	double GenCaseBis_T::computeBorddomain(int np, tdouble3 posMax, tdouble3 posMin);
	void GenCaseBis_T::updateXml(std::string caseName, int np, double rMax, double borddomain);
};
#endif

