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

#ifndef ARBITRATOR_H_
#define ARBITRATOR_H_

#include "Config.h"
#include "Request.h"
#include "Job.h"
#include "ConfigReader.h"

#include "MinPolicy.h"
#include "MaxPolicy.h"
#include "RandPolicy.h"
#include "OptLPPolicy.h"
#include "OptMIPPolicy.h"
#include "EDFPolicy.h"
#include "OptSPPolicy.h"

class Arbitrator : public cSimpleModule
{
    public:
        job_t jobCount;
        std::list<Job*> jobPtrList;
        Policy* _policy;

        token_t numOfTokens;
        SimTime lastExeTime;
        cMessage* nextJobCompletionEventMsg;

        std::vector<long> runTimeVec;
        std::vector<int> utilVec;
        cMessage* recordUtilizationEventMsg;
        cMessage* updateTokenAllocationMsg; // only used by the complex policy
        cMessage* updateTokenPerfMsg;

        // 13-LADIS
        boost::variate_generator<boost::mt19937&, boost::normal_distribution<> >* var_nor;

        Arbitrator();
        virtual ~Arbitrator();

        Function* generateReturnFunction(Request*);
        Function* returnLinearReturnFunction(Request*);
        Function* returnMultiplicativeInverseReturnFunction(Request*);

        //bool isAccepted(std::list<Job*>&);
        //bool isCapacityExceeded();

        void updateJobStatus();
        void updateTokenPerf();
        void updateNextJobCompletionEventMsg();
        void updateJobCompletionTimePrediction();

        void handleCompletedJobs();
        void handleFailedJobs();

        void sendFeedback(Job*);

        token_t returnNumOfUsedTokens();

        void outputVec(vector<int>& vec);
    protected:
        virtual void initialize();
        virtual void handleMessage(cMessage *msg);
        virtual void finish();
};

#endif /* ARBITRATOR_H_ */
