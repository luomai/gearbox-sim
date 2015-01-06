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

#include "Arbitrator.h"

Define_Module(Arbitrator)

Arbitrator::Arbitrator()
{
    // TODO Auto-generated constructor stub
}

Arbitrator::~Arbitrator()
{
    double total = 0;
    for (unsigned int i = 0; i < utilVec.size(); i++)
        total += utilVec[i];
    double avgUtil = total / utilVec.size();
    // TODO Auto-generated destructor stub
    cout << "*********** Cloud Info ***************" << endl;
    cout << "AVG RESOURCE UTILIZATION: " << avgUtil << endl;
    cout << "************** END *******************" << endl;
}

void Arbitrator::initialize()
{
    jobCount = 0;
    lastExeTime = 0.0;
    numOfTokens = Config::TOTAL_NUM_OF_TOKENS;
    var_nor = new boost::variate_generator<boost::mt19937&, boost::normal_distribution<> >((*new boost::mt19937()),
            (*new boost::normal_distribution<>(0.1, 0.01)));

    int policy = atoi(ConfigReader::getInstance().getVal("policy_id").c_str());
    switch (policy)
    {
        case Config::HARD:
            _policy = new MinPolicy();
            break;
        case Config::SOFT:
            _policy = new MaxPolicy();
            break;
        case Config::RAND:
            _policy = new RandPolicy();
            break;
        case Config::aLP_CPLEX:
            _policy = new OptLPPolicy();
            break;
        case Config::MIP_CPLEX:
            _policy = new OptMIPPolicy();
            this->updateTokenAllocationMsg = new cMessage("An event msg for updating the token allocation",
                    Config::UPDATE_TOKEN_ALLOCATION_EVENT);
            break;
        case Config::EDF:
            _policy = new EDFPolicy();
            break;
        case Config::SP_CPLEX:
            _policy = new OptSPPolicy();
            this->updateTokenAllocationMsg = new cMessage("An event msg for updating the token allocation",
                    Config::UPDATE_TOKEN_ALLOCATION_EVENT);
            break;
        default:
            exit(1);
            break;
    }

    nextJobCompletionEventMsg = new cMessage("An event message indicating the next job completion",
            Config::NEXT_JOB_COMPLETION_EVENT);

    recordUtilizationEventMsg = new cMessage("An event message indicating the recording of utilization",
            Config::RECORD_UTILIZATION_EVENT);
    scheduleAt(simTime() + Config::STATISTIC_SAMPLING_INTERVAL, recordUtilizationEventMsg);

    updateTokenPerfMsg = new cMessage("An event message updating the token efficiency (performance)",
            Config::TOKEN_PERF_UPDATE_EVENT);
    int isRandMode = atoi(ConfigReader::getInstance().getVal("random_mode").c_str());
    if (isRandMode == 1)
        scheduleAt(simTime() + Config::TOKEN_PERF_UPDTAE_INTERVAL, updateTokenPerfMsg);

    for (int i = 0; i < 301; i++)
        runTimeVec.push_back(0);
}

void Arbitrator::updateJobStatus()
{
    if (!jobPtrList.empty())
    {
        SimTime timeInterval = simTime() - lastExeTime;
        assert(timeInterval.dbl() >= 0);
        for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
        {
            (*itr)->jobSizeCompleted += (*itr)->efficiency * (*itr)->numOfTokens * timeInterval.dbl();
        }
    }
    lastExeTime = simTime();
}

void Arbitrator::updateTokenPerf()
{
    int isRandMode = atoi(ConfigReader::getInstance().getVal("random_mode").c_str());
    if (isRandMode == 1)
    {
        if (!jobPtrList.empty())
        {
            for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
            {
                (*itr)->efficiency = (*(*itr)->var_nor)(); // update the token efficiency
            }
        }
    }
}

void Arbitrator::updateJobCompletionTimePrediction()
{
    // for each job, update its completion time prediction
    for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
    {
        double remainingJobSize = (*itr)->jobSize - (*itr)->jobSizeCompleted;
        assert(remainingJobSize >= 0);
        SimTime remainingTime;
        if ((*itr)->numOfTokens != 0)
        {
            int isRandMode = atoi(ConfigReader::getInstance().getVal("random_mode").c_str());
            if (isRandMode == 1)
            {
                remainingTime = remainingJobSize / (double((*itr)->numOfTokens) * (*itr)->efficiency);
            }
            else
            {
                remainingTime = remainingJobSize / double((*itr)->numOfTokens);
            }
        }
        else
        {
            remainingTime = 100000.0;
        }
        (*itr)->predictedCompletionTime = simTime() + remainingTime;
    }
}

bool jobCompletiomTimeComp(const Job* x, const Job* y)
{
    return x->predictedCompletionTime < y->predictedCompletionTime;
}

void Arbitrator::updateNextJobCompletionEventMsg()
{
    if (!jobPtrList.empty())
    {
        this->updateJobCompletionTimePrediction(); // update each job's completion time prediction
        std::list<Job*>::iterator min_itr = std::min_element(jobPtrList.begin(), jobPtrList.end(),
                jobCompletiomTimeComp); // find the earliest completed job
        SimTime nextCallTime = (*min_itr)->predictedCompletionTime;
        cancelEvent(nextJobCompletionEventMsg);
        scheduleAt(nextCallTime, nextJobCompletionEventMsg);
    }
}

void Arbitrator::sendFeedback(Job* job)
{
    cMessage* msg = new cMessage("JOB EXITS THE CLOUD.", Config::JOB_COMLETION_FEEDBACK_MSG);
    msg->addPar("_requestId");
    msg->par("_requestId").setLongValue(job->requestId);
    this->send(msg, "out");
}

void Arbitrator::handleCompletedJobs()
{
    for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end();)
    {
        double remainingJobSize = (*itr)->jobSize - (*itr)->jobSizeCompleted;
        double error = Config::JOBSIZE_ERROR;
        if (remainingJobSize <= error)
        {
            this->sendFeedback(*itr);
            itr = jobPtrList.erase(itr);
        }
        else
        {
            itr++;
        }
    }
}

void Arbitrator::handleFailedJobs()
{
    for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end();)
    {
        double maxDur = (*itr)->budgetFunc->maxX;
        double remainingTime = maxDur - (simTime() - (*itr)->enterTime).dbl();
        double remainingJobSize = (*itr)->jobSize - (*itr)->jobSizeCompleted;
        token_t minNumOfTokens = remainingJobSize / remainingTime;
        if (remainingJobSize < 0 || remainingTime < 0 || minNumOfTokens <= 0
                || minNumOfTokens > Config::TOTAL_NUM_OF_TOKENS)
        {
            this->sendFeedback(*itr);
            itr = jobPtrList.erase(itr);
        }
        else
        {
            itr++;
        }
    }
}

token_t Arbitrator::returnNumOfUsedTokens()
{
    token_t tokenCount = 0;
    for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
    {
        tokenCount += (*itr)->numOfTokens;
    }
    return tokenCount;
}

Function* Arbitrator::returnLinearReturnFunction(Request* req)
{
    double jobSize = req->jobSize;
    double minDur = req->budgetFunc->minX;
    double maxDur = req->budgetFunc->maxX;
    double maxPay = req->budgetFunc->returnY(minDur);
    double minPay = req->budgetFunc->returnY(maxDur);

    // #tokens = job size / duration
    double minToken = jobSize / maxDur;
    double maxToken = jobSize / minDur;

    assert(minToken <= maxToken);
    assert(maxToken <= Config::TOTAL_NUM_OF_TOKENS);
    assert(minToken >= 0);

    double slope = (maxPay - minPay) / (maxToken - minToken);
    return new LinearFunction(minToken, maxToken, slope, 0);
}

Function* Arbitrator::returnMultiplicativeInverseReturnFunction(Request* req)
{
    LinearFunction* budgetFunc = dynamic_cast<LinearFunction*>(req->budgetFunc);
    double alpha = budgetFunc->slope * req->jobSize;
    double beta = budgetFunc->intercept;

    double maxDur = budgetFunc->maxX;
    double minDur = budgetFunc->minX;
    double maxToken = req->jobSize / minDur;
    double minToken = req->jobSize / maxDur;

    return new MultiplicativeInverseFunction(minToken, maxToken, alpha, beta);

}

Function* Arbitrator::generateReturnFunction(Request* req)
{
    return returnMultiplicativeInverseReturnFunction(req);
}

void Arbitrator::handleMessage(cMessage *msg)
{
    double sim_len = atof(ConfigReader::getInstance().getVal("sim_len").c_str());
    if (simTime().dbl() > sim_len)
        return;

    this->updateJobStatus(); // Update the estimated remaining size of each job.
    this->handleCompletedJobs();
    this->handleFailedJobs();

    switch (msg->getKind())
    {
        // a new job is arriving
        case Config::REQ_MSG: {
            Request* req = (Request*) msg->getParList()[0];

            // Generate the return function.
            Function* returnFunc = this->generateReturnFunction(req);

            // Duplicate the budget function.
            Function* budgetFunc = req->budgetFunc->clone();

            // Create a new job representing the arriving request.
            Job* jobPtr = new Job(req->clientId, req->reqId, this->jobCount++, returnFunc, budgetFunc, req->jobSize);
//            Job* jobPtr = new Job(req->clientId, req->reqId, this->_jobCount++, returnFunc, budgetFunc, req->jobSize,
//                    (*var_nor)());

            jobPtrList.push_back(jobPtr);
            long start = time(NULL);
            bool decision = this->_policy->allocateTokensToJobs(this->jobPtrList, this->numOfTokens);
            long end = time(NULL);
            assert(end - start < long(runTimeVec.size()));
            runTimeVec[end - start] += 1;

            if (decision == false)
            {
                jobPtrList.pop_back(); // Reject the new job.
            }
            else
            {
                this->updateNextJobCompletionEventMsg(); // As the token allocation is changed, we need to update the job completion event.

                int policy = atoi(ConfigReader::getInstance().getVal("policy_id").c_str());
                if (policy == Config::MIP_CPLEX || policy == Config::SP_CPLEX)
                {
                    cancelEvent(updateTokenAllocationMsg);
                    scheduleAt(simTime().dbl() + Config::TIME_UNIT, updateTokenAllocationMsg);
                }
            }

            // Return the decision to the client.
            msg->addPar("decision");
            msg->par("decision").setBoolValue(decision);
            this->send(msg, "out"); // send a feedback to the client

            break;
        }

        case Config::NEXT_JOB_COMPLETION_EVENT: {

            // Re-schedule the resources
            this->_policy->reallocateTokensToJobs(this->jobPtrList, this->numOfTokens);

            this->updateNextJobCompletionEventMsg();
            break;
        }

            // Record the token utilization.
        case Config::RECORD_UTILIZATION_EVENT: {
            int utilization = (returnNumOfUsedTokens() * 100) / numOfTokens;
            assert(utilization <= 100);
            utilVec.push_back(utilization);
            scheduleAt(simTime() + Config::STATISTIC_SAMPLING_INTERVAL, msg);
            break;
        }

            // Update the token allocation (only used by the complex policy)
        case Config::UPDATE_TOKEN_ALLOCATION_EVENT: {
            if (jobPtrList.empty())
                break;

            this->updateJobStatus(); // update the job size estimation of each job
            for (list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
            {
                if (!(*itr)->tokenList.empty())
                {
                    (*itr)->numOfTokens = (*itr)->tokenList.front(); // Retrieve the next token allocation decision.
                    (*itr)->tokenList.pop_front();
                }
            }
            this->updateNextJobCompletionEventMsg();

            scheduleAt(simTime() + Config::TIME_UNIT, msg);
            break;
        }

        case Config::TOKEN_PERF_UPDATE_EVENT: {
            this->updateTokenPerf();
            scheduleAt(simTime() + Config::TOKEN_PERF_UPDTAE_INTERVAL, msg); // The token efficiency has been updated in the updateJobStatus() event;
            break;
        }

        default: {
            std::cout << "Undefined event message type!" << endl;
            exit(1);
            break;
        }
    }
}

void Arbitrator::outputVec(vector<int>& vec)
{
    std::ofstream f;
    f.open("./results/arbitrator/stats.csv");
    f << "util\n";
    for (vector<int>::iterator itr = vec.begin(); itr != vec.end(); itr++)
        f << (*itr) << "\n";
    f.close();

    f.open("./results/cplex/stats.csv");
    long sum = 0;
    for (unsigned int i = 0; i < runTimeVec.size(); i++)
    {
        sum += runTimeVec[i];
        f << i << "," << sum << "\n";
    }
    f.close();
}

void Arbitrator::finish()
{
    outputVec(utilVec);
}
