///////////////////////////////////////////////////////////////////////////////
///
///	\file    NodeFileUtilities.cpp
///	\author  Paul Ullrich
///	\version October 4, 2018
///
///	<remarks>
///		Copyright 2000-2018 Paul Ullrich
///
///		This file is distributed as part of the Tempest source code package.
///		Permission is granted to use, copy, modify and distribute this
///		source code and its documentation under the terms of the GNU General
///		Public License.  This software is provided "as is" without express
///		or implied warranty.
///	</remarks>

#include "NodeFileUtilities.h"

#include <iostream>

#include "SimpleGrid.h"
#include "AutoCurator.h"

///////////////////////////////////////////////////////////////////////////////

void NodeFile::Read(
	const std::string & strNodeFile,
	NodeFile::PathType ePathType,
	const ColumnDataHeader & cdh,
	const SimpleGrid & grid,
	Time::CalendarType caltype
) {
	// Check that this NodeFile
	if ((m_cdh.size() != 0) || (m_pathvec.size() != 0)) {
		_EXCEPTIONT("Attempting to Read() on an initialized NodeFile");
	}

	// Store the PathType
	m_ePathType = ePathType;

	// Store the ColumnDataHeader data
	m_cdh = cdh;

	// String buffer
	std::string strBuffer;

	// Only support grids of dimension 1 or 2
	if ((grid.m_nGridDim.size() < 1) || (grid.m_nGridDim.size() > 2)) {
		_EXCEPTIONT("Grid dimension out of range:  Only grids of dimension 1 or 2 supported");
	}

	// Coordinate buffer
	std::vector<int> coord;
	coord.resize(grid.m_nGridDim.size());

	// Clear the PathVector
	m_pathvec.clear();

	// Clear the TimeToPathNodeMap
	m_mapTimeToPathNode.clear();

	// Open the file as an input stream
	std::ifstream ifInput(strNodeFile);
	if (!ifInput.is_open()) {
		_EXCEPTION1("Unable to open input file \"%s\"", strNodeFile.c_str());
	}

	// Loop through all lines
	int iLine = 1;
	for (;;) {

		int nCount = 0;
		Time time;

		// Read header lines
		{
			getline(ifInput, strBuffer);
			if (ifInput.eof()) {
				break;
			}

			std::istringstream iss(strBuffer);

			// DetectCyclonesUnstructured output
			if (ePathType == PathTypeDCU) {
				int iYear;
				int iMonth;
				int iDay;
				int iHour;

				iss >> iYear;
				iss >> iMonth;
				iss >> iDay;
				iss >> nCount;

				if (!iss.good()) {
					_EXCEPTION2("Format error on line %i of \"%s\"",
						iLine, strNodeFile.c_str());
				}

				iss >> iHour;

				if (!iss.eof()) {
					_EXCEPTION2("Format error on line %i of \"%s\"",
						iLine, strNodeFile.c_str());
				}

				time = Time(
					iYear,
					iMonth-1,
					iDay-1,
					3600 * iHour,
					0,
					caltype);

			// StitchNodes output
			} else if (ePathType == PathTypeSN) {
				std::string strStart;
				int iYear;
				int iMonth;
				int iDay;
				int iHour;

				iss >> strStart;
				iss >> nCount;
				iss >> iYear;
				iss >> iMonth;
				iss >> iDay;

				if (!iss.good()) {
					_EXCEPTION2("Format error on line %i of \"%s\"",
						iLine, strNodeFile.c_str());
				}

				iss >> iHour;

				if (strStart != "start") {
					_EXCEPTION2("Format error on line %i of \"%s\"",
						iLine, strNodeFile.c_str());
				}
				if (!iss.eof()) {
					_EXCEPTION2("Format error on line %i of \"%s\"",
						iLine, strNodeFile.c_str());
				}

				m_pathvec.resize(m_pathvec.size() + 1);
				m_pathvec[m_pathvec.size()-1].m_timeStart =
					Time(
						iYear,
						iMonth-1,
						iDay-1,
						3600 * iHour,
						0,
						caltype);

				m_pathvec[m_pathvec.size()-1].m_vecPathNodes.resize(nCount);
			}

			iLine++;
		}

		// Read contents under each header line
		for (int i = 0; i < nCount; i++) {

			getline(ifInput, strBuffer);
			if (ifInput.eof()) {
				break;
			}

			std::istringstream iss(strBuffer);

			for (int n = 0; n < grid.m_nGridDim.size(); n++) {
				iss >> coord[n];
			}

			// Note that for 2D grids the coordinate indices are swapped
			if (coord.size() == 1) {
				if ((coord[0] < 0) || (coord[0] >= grid.m_nGridDim[0])) {
					_EXCEPTION4("Coordinate index out of range on line %i of \"%s\""
						" (%i/%i) (%i/%i)",
						iLine, strNodeFile.c_str(),
						coord[0], grid.m_nGridDim[0]);
				}
			} else if (coord.size() == 2) {
				if ((coord[0] < 0) || (coord[0] >= grid.m_nGridDim[1]) ||
				    (coord[1] < 0) || (coord[1] >= grid.m_nGridDim[0])
				) {
					_EXCEPTION6("Coordinate index out of range on line %i of \"%s\""
						" (%i/%i) (%i/%i)",
						iLine, strNodeFile.c_str(),
						coord[0], grid.m_nGridDim[1],
						coord[1], grid.m_nGridDim[0]);
				}
			} else {
				_EXCEPTION();
			}

			if (iss.eof()) {
				_EXCEPTION2("Format error on line %i of \"%s\"",
					iLine, strNodeFile.c_str());
			}

			std::string strBuf;
			std::vector<std::string> vecDelimitedOutput;
			for (;;) {
				iss >> strBuf;
				vecDelimitedOutput.push_back(strBuf);
				if ((iss.eof()) || (strBuf.length() == 0)) {
					break;
				}
			}

			int nOutputSize = vecDelimitedOutput.size();

			if (cdh.size() != nOutputSize-4) {
				_EXCEPTION3("Mismatch between column header size specified in format (%i)"
					" and node file columns on line %i of \"%s\"",
					static_cast<int>(cdh.size()), iLine, strNodeFile.c_str());
			}

			// StitchNodes format input
			if (ePathType == PathTypeSN) {
				PathNode & pathnode =
					m_pathvec[m_pathvec.size()-1].m_vecPathNodes[i];

				if (nOutputSize < 4) {
					_EXCEPTION2("Format error on line %i of \"%s\"",
						iLine, strNodeFile.c_str());
				}

				// Store time
				int iYear = std::stoi(vecDelimitedOutput[nOutputSize-4]);
				int iMonth = std::stoi(vecDelimitedOutput[nOutputSize-3]);
				int iDay = std::stoi(vecDelimitedOutput[nOutputSize-2]);
				int iHour = std::stoi(vecDelimitedOutput[nOutputSize-1]);

				time = Time(
					iYear,
					iMonth-1,
					iDay-1,
					3600 * iHour,
					0,
					caltype);

				pathnode.m_time = time;

				// Store coordinate
				if (coord.size() == 1) {
					pathnode.m_gridix = coord[0];
				} else if (coord.size() == 2) {
					pathnode.m_gridix = coord[0] + grid.m_nGridDim[1] * coord[1];
				} else {
					_EXCEPTIONT("Undefined behavior for SimpleGrid dimensionality > 2");
				}

				if ((pathnode.m_gridix < 0) || (pathnode.m_gridix > grid.GetSize())) {
					_EXCEPTION2("Coordinate index out of range on line %i of \"%s\"",
						iLine, strNodeFile.c_str());
				}

				// Store all other data as strings
				for (int i = 0; i < nOutputSize-4; i++) {
					pathnode.PushColumnDataString(
						vecDelimitedOutput[i]);
				}

				// Because nodes in StitchNodes format output are not ordered in
				// time, efficient data I/O requires us to reorganize the input
				// lines by time.
				TimeToPathNodeMap::iterator iter =
					m_mapTimeToPathNode.find(time);
				if (iter == m_mapTimeToPathNode.end()) {
					iter = m_mapTimeToPathNode.insert(
						TimeToPathNodeMap::value_type(
							time, std::vector< std::pair<int,int> >())).first;
				}
				iter->second.push_back(
					std::pair<int,int>(
						static_cast<int>(m_pathvec.size()-1),i));
			}

			iLine++;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void NodeFile::Write(
	const std::string & strNodeFile,
	const SimpleGrid * pgrid,
	const std::vector<int> * pvecColumnDataOutIx,
	FileFormat eFileFormat,
	bool fIncludeHeader
) {
	FILE * fpOutput = fopen(strNodeFile.c_str(),"w");
	if (fpOutput == NULL) {
		_EXCEPTION1("Error opening file \"%s\" for writing",
			strNodeFile.c_str());
	}

	// Output StitchNodes format file
	if (m_ePathType == PathTypeSN) {

		// GFDL file format
		if (eFileFormat == FileFormatGFDL) {
			for (int p = 0; p < m_pathvec.size(); p++) {
				Path & path = m_pathvec[p];
				if (m_pathvec[p].m_vecPathNodes.size() == 0) {
					_EXCEPTIONT("Zero length Path found");
				}
				fprintf(fpOutput, "start\t%i\t%i\t%i\t%i\t%i\n",
					static_cast<int>(path.m_vecPathNodes.size()),
					path.m_timeStart.GetYear(),
					path.m_timeStart.GetMonth(),
					path.m_timeStart.GetDay(),
					path.m_timeStart.GetSecond() / 3600);

				for (int i = 0; i < m_pathvec[p].m_vecPathNodes.size(); i++) {
					PathNode & pathnode = path.m_vecPathNodes[i];

					if (pgrid != NULL) {
						if (pgrid->m_nGridDim.size() == 1) {
							fprintf(fpOutput, "\t%lu", pathnode.m_gridix);
						} else if (pgrid->m_nGridDim.size() == 2) {
							fprintf(fpOutput, "\t%lu\t%lu",
								pathnode.m_gridix % pgrid->m_nGridDim[1],
								pathnode.m_gridix / pgrid->m_nGridDim[1]);
						}
					}

					if (pvecColumnDataOutIx == NULL) {
						for (int j = 0; j < pathnode.m_vecColumnData.size(); j++) {
							const ColumnData * pcd = pathnode.m_vecColumnData[j];
							fprintf(fpOutput, "\t%s", pcd->ToString().c_str());
						}

					} else {
						for (int j = 0; j < pvecColumnDataOutIx->size(); j++) {
							const ColumnData * pcd =
								pathnode.m_vecColumnData[(*pvecColumnDataOutIx)[j]];
							fprintf(fpOutput, "\t%s", pcd->ToString().c_str());
						}
					}

					fprintf(fpOutput, "\t%i\t%i\t%i\t%i\n",
						pathnode.m_time.GetYear(),
						pathnode.m_time.GetMonth(),
						pathnode.m_time.GetDay(),
						pathnode.m_time.GetSecond() / 3600);
				}
			}

		// CSV file format
		} else if (eFileFormat == FileFormatCSV) {

			if (fIncludeHeader) {
				fprintf(fpOutput, "track_id, year, month, day, hour");
				if (pgrid != NULL) {
					if (pgrid->m_nGridDim.size() == 1) {
						fprintf(fpOutput, ", i");
					} else {
						fprintf(fpOutput, ", i, j");
					}
				}
				if (pvecColumnDataOutIx == NULL) {
					for (int i = 0; i < m_cdh.size(); i++) {
						fprintf(fpOutput, ", %s", m_cdh[i].c_str());
					}

				} else {
					for (int j = 0; j < pvecColumnDataOutIx->size(); j++) {
						fprintf(fpOutput, ", %s", m_cdh[(*pvecColumnDataOutIx)[j]].c_str());
					}
				}
				fprintf(fpOutput,"\n");
			}

			for (int p = 0; p < m_pathvec.size(); p++) {
				Path & path = m_pathvec[p];
				if (m_pathvec[p].m_vecPathNodes.size() == 0) {
					_EXCEPTIONT("Zero length Path found");
				}

				for (int i = 0; i < m_pathvec[p].m_vecPathNodes.size(); i++) {
					PathNode & pathnode = path.m_vecPathNodes[i];

					fprintf(fpOutput, "%i, %i, %i, %i, %i",
						p,
						pathnode.m_time.GetYear(),
						pathnode.m_time.GetMonth(),
						pathnode.m_time.GetDay(),
						pathnode.m_time.GetSecond() / 3600);

					if (pgrid != NULL) {
						if (pgrid->m_nGridDim.size() == 1) {
							fprintf(fpOutput, ", %lu", pathnode.m_gridix);
						} else if (pgrid->m_nGridDim.size() == 2) {
							fprintf(fpOutput, ", %lu, %lu",
								pathnode.m_gridix % pgrid->m_nGridDim[1],
								pathnode.m_gridix / pgrid->m_nGridDim[1]);
						}
					}

					if (pvecColumnDataOutIx == NULL) {
						for (int j = 0; j < pathnode.m_vecColumnData.size(); j++) {
							const ColumnData * pcd = pathnode.m_vecColumnData[j];
							fprintf(fpOutput, ", %s", pcd->ToString().c_str());
						}

					} else {
						for (int j = 0; j < pvecColumnDataOutIx->size(); j++) {
							const ColumnData * pcd =
								pathnode.m_vecColumnData[(*pvecColumnDataOutIx)[j]];
							fprintf(fpOutput, ", %s", pcd->ToString().c_str());
						}
					}
					fprintf(fpOutput,"\n");
				}
			}

		} else {
			_EXCEPTIONT("Sorry, not yet implemented!");
		}
	
	} else {
		_EXCEPTIONT("Sorry, not yet implemented!");
	}

}

///////////////////////////////////////////////////////////////////////////////

