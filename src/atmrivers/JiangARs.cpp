///////////////////////////////////////////////////////////////////////////////
///
///	\file    JiangARs.cpp
///	\author  Paul Ullrich
///	\version September 11th, 2016
///
///	<remarks>
///		Copyright 2000-2014 Paul Ullrich
///
///		This file is distributed as part of the Tempest source code package.
///		Permission is granted to use, copy, modify and distribute this
///		source code and its documentation under the terms of the GNU General
///		Public License.  This software is provided "as is" without express
///		or implied warranty.
///	</remarks>

#include "CommandLine.h"
#include "Exception.h"
#include "Announce.h"

#include "DataArray1D.h"
#include "DataArray2D.h"

#include "netcdfcpp.h"
#include "NetCDFUtilities.h"

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {

	NcError error(NcError::silent_nonfatal);

try {
	// Input file
	std::string strInputFile;

	// Output file
	std::string strOutputFile;

	// Input integrated water vapor variable name
	std::string strIWVVariable;

	// Output variable name
	std::string strOutputVariable;

	// Minimum absolute latitude
	double dMinAbsLat;

	// Maximum absolute latitude of equatorial band
	double dEqBandMaxLat;

	// Minimum pointwise integrated water vapor
	double dMinIWV;

	// Minimum area
	double dMinArea;

	// Maximal areal fraction
	double dMaxArealFraction;

	// Zonal mean weight
	double dZonalMeanWeight;

	// Zonal max weight
	double dZonalMaxWeight;

	// Meridional mean weight
	double dMeridMeanWeight;

	// Meridional max weight
	double dMeridMaxWeight;

	// Parse the command line
	BeginCommandLine()
		CommandLineString(strInputFile, "in", "");
		//CommandLineString(strInputFileList, "inlist", "");
		CommandLineString(strOutputFile, "out", "");
		CommandLineString(strIWVVariable, "iwvvar", "");
		CommandLineString(strOutputVariable, "outvar", "");
		CommandLineDouble(dMinAbsLat, "minabslat", 15.0);
		CommandLineDouble(dEqBandMaxLat, "eqbandmaxlat", 15.0);
		CommandLineDouble(dMinIWV, "miniwv", 20.0);
		CommandLineDouble(dZonalMeanWeight, "zonalmeanwt", 0.7);
		CommandLineDouble(dZonalMaxWeight, "zonalmaxwt", 0.3);
		CommandLineDouble(dMeridMeanWeight, "meridmeanwt", 0.9);
		CommandLineDouble(dMeridMaxWeight, "meridmaxwt", 0.1);
		//CommandLineBool(fRegional, "regional");

		ParseCommandLine(argc, argv);
	EndCommandLine(argv)

	AnnounceBanner();

	AnnounceStartBlock("Loading data");

	// Check input
	if (strInputFile == "") {
		_EXCEPTIONT("No input file (--in) specified");
	}

	// Check output
	if (strOutputFile == "") {
		_EXCEPTIONT("No output file (--out) specified");
	}

	// Check variable
	if (strIWVVariable == "") {
		_EXCEPTIONT("No IWV variable name (--iwvvar) specified");
	}

	// Check output variable
	if (strOutputVariable.length() == 0) {
		strOutputVariable = strIWVVariable + "tag";
	}

	// Open the NetCDF input file
	NcFile ncInput(strInputFile.c_str());

	if (!ncInput.is_valid()) {
		_EXCEPTION1("Unable to open NetCDF file \"%s\" for reading",
			strInputFile.c_str());
	}


	// Get the time dimension
	NcDim * dimTime = ncInput.get_dim("time");
	//if (dimTime == NULL) {
	//	_EXCEPTIONT("Error accessing dimension \"time\"");
	//}

	// Get the longitude dimension
	NcDim * dimLon = ncInput.get_dim("lon");
	if (dimLon == NULL) {
		_EXCEPTIONT("Error accessing dimension \"lon\"");
	}

	// Get the longitude variable
	NcVar * varLon = ncInput.get_var("lon");
	if (varLon == NULL) {
		_EXCEPTIONT("Error accessing variable \"lon\"");
	}

	DataArray1D<double> dLonDeg(dimLon->size());
	varLon->get(&(dLonDeg[0]), dimLon->size());

	// Get the latitude dimension
	NcDim * dimLat = ncInput.get_dim("lat");
	if (dimLat == NULL) {
		_EXCEPTIONT("Error accessing dimension \"lat\"");
	}

	// Get the latitude variable
	NcVar * varLat = ncInput.get_var("lat");
	if (varLat == NULL) {
		_EXCEPTIONT("Error accessing variable \"lat\"");
	}

	DataArray1D<double> dLatDeg(dimLat->size());
	varLat->get(&(dLatDeg[0]), dimLat->size());

	// Get the integrated water vapor variable
	NcVar * varIWV = ncInput.get_var(strIWVVariable.c_str());
	if (varIWV == NULL) {
		_EXCEPTION1("Error accessing variable \"%s\"",
			strIWVVariable.c_str());
	}

	DataArray2D<float> dIWV(dimLat->size(), dimLon->size());

	// Open the NetCDF output file
	NcFile ncOutput(strOutputFile.c_str(), NcFile::Replace);
	if (!ncOutput.is_valid()) {
		_EXCEPTION1("Unable to open NetCDF file \"%s\" for writing",
			strOutputFile.c_str());
	}

	// Copy over latitude, longitude and time variables to output file
	NcDim * dimTimeOut = NULL;
	if (dimTime != NULL) {
		CopyNcVar(ncInput, ncOutput, "time", true);
		dimTimeOut = ncOutput.get_dim("time");
		if (dimTimeOut == NULL) {
			_EXCEPTIONT("Error copying variable \"time\" to output file");
		}
	}

	CopyNcVar(ncInput, ncOutput, "lat", true);
	CopyNcVar(ncInput, ncOutput, "lon", true);

	NcDim * dimLonOut = ncOutput.get_dim("lon");
	if (dimLonOut == NULL) {
		_EXCEPTIONT("Error copying variable \"lon\" to output file");
	}
	NcDim * dimLatOut = ncOutput.get_dim("lat");
	if (dimLatOut == NULL) {
		_EXCEPTIONT("Error copying variable \"lat\" to output file");
	}

	NcVar * varIWVtag = NULL;
	if (dimTime != NULL) {
		varIWVtag = ncOutput.add_var(
			"ar_binary_tag",
			ncByte,
			dimTimeOut,
			dimLatOut,
			dimLonOut);

	} else {
		varIWVtag = ncOutput.add_var(
			"ar_binary_tag",
			ncByte,
			dimLatOut,
			dimLonOut);
	}

	varIWVtag->add_att("description", "binary indicator of atmospheric river");
	varIWVtag->add_att("scheme", "Jiang");
	varIWVtag->add_att("version", "1.0");

	AnnounceEndBlock("Done");

	// Tagged cell array
	DataArray2D<ncbyte> dIWVtag(dimLat->size(), dimLon->size());

	// Loop through all times
	int nTimes = 1;
	if (dimTime != NULL) {
		nTimes = dimTime->size();
	}
	for (int t = 0; t < nTimes; t++) {

		char szBuffer[20];
		sprintf(szBuffer, "Time %i", t);
		AnnounceStartBlock(szBuffer);

		// Get the IWV array
		if (dimTime != NULL) {
			varIWV->set_cur(t, 0, 0);
			varIWV->get(&(dIWV[0][0]), 1, dimLat->size(), dimLon->size());
		} else {
			varIWV->set_cur(0, 0);
			varIWV->get(&(dIWV[0][0]), dimLat->size(), dimLon->size());
		}

		AnnounceStartBlock("Compute zonal/meridional thresholds");

		// Compute zonal threshold
		DataArray1D<float> dZonalThreshold(dimLat->size());
		for (int j = 0; j < dimLat->size(); j++) {
			float dMaxZonalIWV = dIWV[j][0];
			for (int i = 0; i < dimLon->size(); i++) {
				dZonalThreshold[j] += dIWV[j][i];
				if (dIWV[j][i] > dMaxZonalIWV) {
					dMaxZonalIWV = dIWV[j][i];
				}
			}
			dZonalThreshold[j] /= static_cast<float>(dimLon->size());

			dZonalThreshold[j] =
				dZonalMeanWeight * dZonalThreshold[j]
				+ dZonalMaxWeight * dMaxZonalIWV;
		}

		// Compute meridional threshold
		DataArray1D<float> dMeridThreshold(dimLon->size());
		for (int i = 0; i < dimLon->size(); i++) {
			float dMaxMeridIWV = dIWV[0][i];
			for (int j = 0; j < dimLat->size(); j++) {
				dMeridThreshold[i] += dIWV[j][i];
				if (dIWV[j][i] > dMaxMeridIWV) {
					dMaxMeridIWV = dIWV[j][i];
				}
			}
			dMeridThreshold[i] /= static_cast<float>(dimLon->size());

			dMeridThreshold[i] =
				dMeridMeanWeight * dMeridThreshold[i]
				+ dMeridMaxWeight * dMaxMeridIWV;
		}

		// Announce
		AnnounceEndBlock("Done");

		AnnounceStartBlock("Build tagged cell array");

		// Build tagged cell array
		dIWVtag.Zero();

		for (int j = 0; j < dimLat->size(); j++) {
		for (int i = 0; i < dimLon->size(); i++) {
			if (fabs(dLatDeg[j]) < dMinAbsLat) {
				continue;
			}
			if (dIWV[j][i] < dMinIWV) {
				continue;
			}
			if (dIWV[j][i] < dZonalThreshold[j]) {
				continue;
			}
			if (dIWV[j][i] < dMeridThreshold[i]) {
				continue;
			}

			dIWVtag[j][i] = 1;
		}
		}

		// Remove points connected with equatorial moisture band
		for (int i = 0; i < dimLon->size(); i++) {
			bool fSouthDone = false;
			bool fNorthDone = false;

			for (int j = 0; j < dimLat->size(); j++) {
				if ((!fSouthDone) && (dLatDeg[j] > -dMinAbsLat)) {
					for (int k = j-1; k > 0; k--) {
						if (dIWVtag[k][i] == 0) {
							break;
						}
						if (dLatDeg[k] < -dEqBandMaxLat) {
							break;
						}
						dIWVtag[k][i] = 0;
					}
					fSouthDone = true;
				}
				if ((!fNorthDone) && (dLatDeg[j] > dMinAbsLat)) {
					for (int k = j-1; k < dimLat->size(); k++) {
						if (dIWVtag[k][i] == 0) {
							break;
						}
						if (dLatDeg[k] > dEqBandMaxLat) {
							break;
						}
						dIWVtag[k][i] = 0;
					}
					fNorthDone = true;
				}
			}
		}

		AnnounceEndBlock("Done");

		AnnounceStartBlock("Writing results");

		// Output tagged cell array
		if (dimTime != NULL) {
			varIWVtag->set_cur(t, 0, 0);
			varIWVtag->put(&(dIWVtag[0][0]), 1, dimLatOut->size(), dimLonOut->size());
		} else {
			varIWVtag->set_cur(0, 0);
			varIWVtag->put(&(dIWVtag[0][0]), dimLatOut->size(), dimLonOut->size());
		}

		AnnounceEndBlock("Done");

		AnnounceEndBlock(NULL);
	}

	AnnounceEndBlock("Done");

	AnnounceBanner();

} catch(Exception & e) {
	Announce(e.ToString().c_str());
}
}

///////////////////////////////////////////////////////////////////////////////

