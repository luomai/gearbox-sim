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

#ifndef CONFIG_H_
#define CONFIG_H_

#include <omnetpp.h>
#include <iostream>
#include <algorithm>
#include <list>
#include <queue>
#include <assert.h>
#include <sys/time.h>
#include <math.h>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/normal_distribution.hpp>

#include "Function.h"

typedef int client_t;
typedef int job_t;
typedef long token_t;
typedef double money_t;
typedef short msg_t;
typedef long request_t;
typedef double megaByte_t;
typedef short policy_t;
typedef short function_t;

class Config
{
    public:
//        static const bool RANDOM_MODE = true;

        static client_t clientCount;

        static const msg_t GENERATE_REQ_EVENT_MSG = 1;
        static const msg_t REQ_MSG = 2;
        static const msg_t NEXT_JOB_COMPLETION_EVENT = 3;
        static const msg_t JOB_COMLETION_FEEDBACK_MSG = 4;
        static const msg_t RECORD_UTILIZATION_EVENT = 5;
        static const msg_t UPDATE_TOKEN_ALLOCATION_EVENT = 6;
        static const msg_t CLIENT_STATS_EVENT = 7;
        static const msg_t TOKEN_PERF_UPDATE_EVENT = 8;

        // different resource allocation policies
        static const policy_t HARD = 1;
        static const policy_t SOFT = 2;
        static const policy_t RAND = 3;
        static const policy_t EDF = 4; // Earliest Deadline First
        static const policy_t aLP_CPLEX = 5; // Solve the linear program.
        static const policy_t MIP_CPLEX = 6; // Solve the mixed integer program.
        static const policy_t SP_CPLEX = 7;
//        static const policy_t ALLOCATION_POLICY = MIP_CPLEX;

        // different admission policies.
        static const policy_t MARGINAL = 1;
        static const policy_t TOTAL = 2;
        static const policy_t AVG_JOB_RETURN = 3;
        static const policy_t NO_ADMISSION_CONTROL = 4;
//        static const policy_t ADMISSION_CONTROL_POLICY = TOTAL;

        static const double MIP_BETA = 0.875;
        static const bool UPDATE_RETURN_FUNCTION = true;
        static const short BATCH_PROCESSING = 1; // focus on MapReduce-like jobs

//        static const double LOAD = 1.1;
        static const double STATISTIC_SAMPLING_INTERVAL = 1.0;
        static const double TOKEN_PERF_UPDTAE_INTERVAL = 1.0;
        static const double JOBSIZE_ERROR = 1;

        /**
         * Job setting
         * job_size = base * (1 + scale) where scale is subject to a HALF-normal distribution.
         */
//        static const double JOBSIZE_BASE = 10;
//        static const double JOBSIZE_SCLAE_MEAN = 0;
//        static const double JOBSIZE_SCALE_STD = 50;
        static const token_t TOTAL_NUM_OF_TOKENS = 1000; // the resource constraint
        static const request_t REQUEST_COUNT = 1000;
//        static const double SIMULATION_TIME_LENGTH = 2000;
        static const double TIME_UNIT = 1.0; // the scale of time units in the complex model.

        Config();
        virtual ~Config();
};

#endif /* CONFIG_H_ */
