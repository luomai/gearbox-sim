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

#ifndef RANDPOLICY_H_
#define RANDPOLICY_H_

#include "Policy.h"

class RandPolicy: public Policy {
public:
	RandPolicy();
	virtual ~RandPolicy();
	virtual bool allocateTokensToJobs(std::list<Job*> &jobPtrList, token_t totalNumOfTokens);
	virtual void reallocateTokensToJobs(std::list<Job*> &jobPtrList, token_t totalNumOfTokens);
};

#endif /* RANDPOLICY_H_ */
