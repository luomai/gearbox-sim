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

#ifndef REQUEST_H_
#define REQUEST_H_

#include "Config.h"

class Request : public cObject
{
    public:
        request_t reqId;
        client_t clientId;
        Function* budgetFunc;
        double createdTime;
        megaByte_t jobSize;
        Request();
        Request(request_t r, client_t c, Function* f, megaByte_t j) :
                reqId(r), clientId(c), budgetFunc(f), jobSize(j)
        {
            createdTime = simTime().dbl();
        }
        virtual money_t returnProfit(double performanceMetric) = 0;
        virtual ~Request();

        class reqEq
        {
            public:
                reqEq(const Request* compare_to) :
                        compareTo(compare_to)
                {
                }
                bool operator()(Request* req) const
                {
                    return (req->clientId == compareTo->clientId) && (req->reqId == compareTo->reqId);
                }
            private:
                const Request* compareTo;
        };
};

class BatchProcessingRequest : public Request
{
    public:
        BatchProcessingRequest(request_t r, client_t o, Function* f, megaByte_t j) :
                Request(r, o, f, j)
        {
        }

        virtual money_t returnProfit(double jobDur)
        {
            assert(jobDur > 0);

            // Too late
            if (jobDur > budgetFunc->maxX)
                return 0; // minPay = 0

            // Too early
            if (jobDur < budgetFunc->minX)
                return budgetFunc->returnY(budgetFunc->minX); // maxPay

            // Normal case
            if (jobDur >= budgetFunc->minX && jobDur <= budgetFunc->maxX)
                return budgetFunc->returnY(jobDur);

            // Illegal case
            assert(false);
            return -1;
        }
};

#endif /* REQUEST_H_ */
