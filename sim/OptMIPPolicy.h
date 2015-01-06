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

#ifndef OPTMIPPOLICY_H_
#define OPTMIPPOLICY_H_

#include "Policy.h"
#include "ConfigReader.h"

class OptMIPPolicy : public Policy
{
    public:
        OptMIPPolicy();
        virtual ~OptMIPPolicy();
        virtual bool allocateTokensToJobs(std::list<Job*> &jobPtrList, token_t totalNumOfTokens);
        virtual void reallocateTokensToJobs(std::list<Job*> &jobPtrList, token_t totalNumOfTokens);
        money_t computeMarginalReturn(std::list<Job*> &jobPtrList);
        money_t computeMarginalReturn(std::list<Job*> &jobPtrList, IloCplex& cplex, IloArray<IloNumVarArray>& x,
                IloInt nbTimeUnits);
        money_t computeTotalReturn(std::list<Job*> &jobPtrList);
        money_t returnExpectedPayment(Function* budgetFunction, double duration);
};

#endif /* OPTMIPPOLICY_H_ */
