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

#include "EDFPolicy.h"

EDFPolicy::EDFPolicy()
{
    // TODO Auto-generated constructor stub
}

EDFPolicy::~EDFPolicy()
{
    // TODO Auto-generated destructor stub
}

bool EDFPolicy::allocateTokensToJobs(list<Job*>& jobPtrList, token_t totalNumOfTokens)
{
    // Check the feasibility of accepting the new job.
    token_t numOfTokensAvailable = totalNumOfTokens;
    for (list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
    {
        numOfTokensAvailable -= (*itr)->returnMinNumOfTokens();
    }

    if (numOfTokensAvailable < 0)
    {
        return false;
    }
    else
    {
        // Step 1: Allocate each job with the minimum number of tokens needed.
        for (list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
        {
            (*itr)->numOfTokens = (*itr)->returnMinNumOfTokens();
        }

        double deadlineOfLastAllocatedJob = 0;
        int jobNum = jobPtrList.size();
        // Step 2: Allocate spare tokens to the job having the earliest deadline.
        while (numOfTokensAvailable > 0 && jobNum > 0)
        {
            Job* job = (*jobPtrList.begin());
            double minDeadline = 100000.0;
            for (list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
            {
                double deadline = (*itr)->budgetFunc->maxX;
                // Any job has the deadline < deadlineOfLastJob has been allocated tokens.
                // NOTE: It doesn't consider the case two jobs have exactly the same deadline.
                if (deadline > deadlineOfLastAllocatedJob)
                {
                    if (deadline < minDeadline)
                    {
                        job = *itr;
                        minDeadline = deadline;
                    }
                }
            }

            // Allocate tokens to this job.
            token_t numOfTokensNeeded = job->returnMaxNumOfTokens() - job->numOfTokens;
            if (numOfTokensNeeded < numOfTokensAvailable)
            {
                job->numOfTokens += numOfTokensNeeded;
                numOfTokensAvailable -= numOfTokensNeeded;
            }
            else
            {
                job->numOfTokens += numOfTokensAvailable;
                numOfTokensAvailable -= numOfTokensAvailable;
            }
            deadlineOfLastAllocatedJob = minDeadline;
            jobNum--;
        }

        return true;
    }
}

void EDFPolicy::reallocateTokensToJobs(list<Job*>& jobPtrList, token_t totalNumOfTokens)
{
    this->allocateTokensToJobs(jobPtrList, totalNumOfTokens);
}
