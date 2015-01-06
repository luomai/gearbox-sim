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

#ifndef JOB_H_
#define JOB_H_

#include "Config.h"
#include "ConfigReader.h"

/* The Job class is used to describe the deadline-based and non-deadline-based jobs,
 * It is not necessary to inherit from the cObject if the Job is used in an independent test-bed.
 * NOTE: All the functions are assumed to be linear functions. */
class Job : public cObject
{
    public:
        client_t clientId; // The ID of the client who proposes this job.
        request_t requestId; // The ID of the request that proposes this job.

        job_t jobId; // The ID of this job. This should be unique inside the resource allocation engine.
        token_t numOfTokens; // The number of currently owned tokens

        Function* returnFunc;
        Function* budgetFunc;
        double jobSize;
        double jobSizeCompleted;

//        double _progress; // The current progress of this job, e.g., progress 49% means that 49% amount of the work of this job has been achieved.
        SimTime enterTime; // The time this job entered the Cloud. SimTime type is provided by the OMNET++ library.
        SimTime predictedCompletionTime; // This member is used by the Arbitrator for determining the which job is the earliest one to finish.

        /* This queue memories the allocation of tokens over time
         * For example, a queue <19, 20, 24> means this job is given 19 tokens during the first scheduling period, 20 during the second, and 24 during the third.
         * It is expected to finish at the end of third scheduling period. */
        list<token_t> tokenList;

        // 13-LADIS
        boost::variate_generator<boost::mt19937&, boost::normal_distribution<> >* var_nor;
        double efficiency;
        // 13-LADIS

        Job();
        Job(client_t c, request_t r, job_t j, Function* rf, Function* bf, double js, double std) :
                clientId(c), requestId(r), jobId(j), returnFunc(rf), budgetFunc(bf), jobSize(js)
        {
            numOfTokens = 0;
            jobSizeCompleted = 0;
            enterTime = simTime();
            predictedCompletionTime = 0.0;

            // 13-LADIS
            // Use the jobId to be the seed initialing the generator.
            double mean = atof(ConfigReader::getInstance().getVal("token_perf_mean").c_str());
            var_nor = new boost::variate_generator<boost::mt19937&, boost::normal_distribution<> >(
                    (*new boost::mt19937(jobId)), (*new boost::normal_distribution<>(mean, std)));
            efficiency = (*var_nor)();
            // 13-LADIS
        }

        Job(client_t c, request_t r, job_t j, Function* rf, Function* bf, double js) :
                clientId(c), requestId(r), jobId(j), returnFunc(rf), budgetFunc(bf), jobSize(js)
        {
            numOfTokens = 0;
            jobSizeCompleted = 0;
            enterTime = simTime();
            predictedCompletionTime = 0.0;

            // 13-LADIS
            // Use the jobId to be the seed initialing the generator.
            double mean = atof(ConfigReader::getInstance().getVal("token_perf_mean").c_str());
            double std = atof(ConfigReader::getInstance().getVal("token_perf_std").c_str());
            var_nor = new boost::variate_generator<boost::mt19937&, boost::normal_distribution<> >(
                    (*new boost::mt19937(jobId)), (*new boost::normal_distribution<>(mean, std)));
            efficiency = 1.0;
            // 13-LADIS
        }

        // Returns the number of tokens to catch the minimum requirement for performance metrics or deadline.
        token_t returnMinNumOfTokens();

        // Returns the number of tokens to catch the maximum requirement for performance metrics or deadline.
        token_t returnMaxNumOfTokens();

        /* This function returns the expected completion time given a certain number of tokens in next scheduling period.
         * This function is mainly used by the event-based simulator which needs the information to compute which nearest event will happen then.
         * NOTE: this function assumed that the number of tokens is not changed.
         * Cannot be used by the OptComplexPolicy. */
        double expectedJobLifetimeWithFixedTokenAllocation(token_t numOfTokens);

        /*
         * This function returns the expected completion time of a job given time-varied token allocations.
         * This should be used by the OptComplexPolicy.
         * */
        double expectedJobLifetimeWithVariedTokenAllocation();

        double estimateRemainingTimeWithVariedTokenAllocation();

        // Used by the OptSimplePolicy for updating each job's return function.
        void updateMultiplicativeInverseReturnFunction();

        virtual ~Job();
};

#endif /* JOB_H_ */
