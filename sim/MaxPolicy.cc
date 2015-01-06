//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "MaxPolicy.h"

MaxPolicy::MaxPolicy() {
	// TODO Auto-generated constructor stub
}

MaxPolicy::~MaxPolicy() {
	// TODO Auto-generated destructor stub
}

bool MaxPolicy::allocateTokensToJobs(std::list<Job*>& jobPtrList, token_t totalNumOfTokens) {
	std::list<Job*>::iterator lastJobPtr = --jobPtrList.end();
	if (Config::UPDATE_RETURN_FUNCTION == true)
		(*lastJobPtr)->updateMultiplicativeInverseReturnFunction();
	(*lastJobPtr)->numOfTokens = (*lastJobPtr)->returnFunc->maxX; // presumably allocating the maximum number of tokens to this new job

	token_t numOfTokensUsed = 0;
	for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++) {
		numOfTokensUsed += (*itr)->numOfTokens;
	}

	return (numOfTokensUsed <= totalNumOfTokens) ? true: false;
}

void MaxPolicy::reallocateTokensToJobs(std::list<Job*>& jobPtrList, token_t totalNumOfTokens) {
	// do nothing
}
