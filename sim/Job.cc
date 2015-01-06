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

#include "Job.h"

Job::Job()
{
    // TODO Auto-generated constructor stub

}

Job::~Job()
{
    // TODO Auto-generated destructor stub
    delete this->var_nor;
}

token_t Job::returnMinNumOfTokens()
{
    double maxDur = budgetFunc->maxX;
    double remainingTime = maxDur - (simTime() - enterTime).dbl();
    assert(remainingTime >= 0);

    double remainingJobSize = jobSize - jobSizeCompleted;
    assert(remainingJobSize > 0);

    // The ceiling of the tokens needed.
    token_t minNumOfTokens = (remainingJobSize / remainingTime) + 1;
    assert(minNumOfTokens >= 0);
    assert(minNumOfTokens <= Config::TOTAL_NUM_OF_TOKENS);
    return minNumOfTokens;
}

token_t Job::returnMaxNumOfTokens()
{
    double elapsedTime = (simTime() - this->enterTime).dbl();
    assert(elapsedTime >= 0);

    double minDur = this->budgetFunc->minX;
    if (elapsedTime < minDur)
    {
        double remainingTime = minDur - elapsedTime;
        double remainingJobSize = jobSize - jobSizeCompleted;
        assert(remainingJobSize > 0);

        token_t maxNumOfTokens = remainingJobSize / remainingTime;
        if (maxNumOfTokens <= Config::TOTAL_NUM_OF_TOKENS)
            return maxNumOfTokens;
        else
            return Config::TOTAL_NUM_OF_TOKENS;
    }
    else
    {
        return Config::TOTAL_NUM_OF_TOKENS;
    }
}

double Job::expectedJobLifetimeWithFixedTokenAllocation(token_t numOfTokens)
{
    double remainingJobSize = jobSize - jobSizeCompleted;
    double remainingTime = remainingJobSize / numOfTokens;
    return (simTime() - this->enterTime).dbl() + remainingTime;
}

double Job::estimateRemainingTimeWithVariedTokenAllocation()
{
    int timeUnitCount = tokenList.size();

    for (list<token_t>::reverse_iterator itr = tokenList.rbegin(); itr != tokenList.rend(); itr++)
    {
        if ((*itr) != 0)
        {
            break;
        }
        timeUnitCount--;
    }assert(timeUnitCount <= int(tokenList.size()));

    return double(timeUnitCount) * Config::TIME_UNIT;
}

double Job::expectedJobLifetimeWithVariedTokenAllocation()
{
    double remainingTime = this->estimateRemainingTimeWithVariedTokenAllocation(); // An rough estimation of the job lifetime.
    return (simTime() - this->enterTime).dbl() + remainingTime;
}

void Job::updateMultiplicativeInverseReturnFunction()
{
    token_t minToken = returnMinNumOfTokens();
    double maxDur = expectedJobLifetimeWithFixedTokenAllocation(minToken);
    money_t minPay = budgetFunc->returnY(maxDur);

    token_t maxToken = returnMaxNumOfTokens();
    double minDur = expectedJobLifetimeWithFixedTokenAllocation(maxToken);
    money_t maxPay = budgetFunc->returnY(minDur);

    assert(minToken <= maxToken);

    if (minDur == maxDur) maxDur += 0.001;
    if (minPay == maxPay) maxPay += 0.001;

    assert(minDur < maxDur);
    assert(minPay < maxPay);

    double slope = (maxPay - minPay) / (minDur - maxDur);
    double intercept = maxPay - minDur * slope;
    assert(slope < 0.0);
    assert(intercept > 0.0);

    MultiplicativeInverseFunction* reFunc = dynamic_cast<MultiplicativeInverseFunction*>(this->returnFunc);
    double remainingJobSize = jobSize - jobSizeCompleted;
    assert(remainingJobSize > 0);
    reFunc->alpha = slope * remainingJobSize;
    reFunc->beta = intercept;
    reFunc->minX = minToken;
    reFunc->maxX = maxToken;
}
