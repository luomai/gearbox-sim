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

#include "Client.h"

Define_Module(Client)

Client::Client()
{
    // TODO Auto-generated constructor stub

}

Client::~Client()
{
    double workload = atof(ConfigReader::getInstance().getVal("workload").c_str());
    // This calculation is based on the entry of "half-normal distribution" on Wiki.
    double meanScale = atof(ConfigReader::getInstance().getVal("job_size_scale_std").c_str()) * 1.414 / 1.78;
    double meanJobSize = atof(ConfigReader::getInstance().getVal("job_size_base").c_str()) * (1 + meanScale);

    std::cout << "*********** CLIENT INFO ************" << endl;
    std::cout << "TOTAL RETURN: " << this->_cumulativePayment << endl;
    std::cout << "COMPLETE RATIO: "
            << 1.0 - ((double) this->_rejectedReqCount + _zeroPaymentReqCount) / (double) this->_reqCount << endl;
    std::cout << "ACCEPT RATIO: "<< _completedReqCount / (double)this->_reqCount << endl;
    std::cout << "FAIL RATIO: " << _zeroPaymentReqCount / (double)(this->_reqCount - this->_rejectedReqCount) << endl;
    std::cout << "POLICY: " << atoi(ConfigReader::getInstance().getVal("policy_id").c_str()) << endl;
    std::cout << "ADMISSION CONTROL: " << atoi(ConfigReader::getInstance().getVal("admission_id").c_str()) << endl;
    std::cout << "JOB INTERVAL: " << (meanJobSize / Config::TOTAL_NUM_OF_TOKENS) / workload << endl;
    std::cout << "CLOUD WORKLOAD: " << workload << endl;
    std::cout << "*********** END OF INFO ***********" << endl;

    std::ofstream f;
    f.open("./results_file");
    if (f.is_open())
    {
        f << "*********** CLIENT INFO ************" << endl;
        f << "TOTAL RETURN: " << this->_cumulativePayment << endl;
    	f << "COMPLETE RATIO: "
            << 1.0 - ((double) this->_rejectedReqCount + _zeroPaymentReqCount) / (double) this->_reqCount << endl;
    	f << "ACCEPT RATIO: "<< _completedReqCount / (double)this->_reqCount << endl;
    	f << "FAIL RATIO: " << _zeroPaymentReqCount / (double)(this->_reqCount - this->_rejectedReqCount) << endl;
        f << "POLICY: " << atoi(ConfigReader::getInstance().getVal("policy_id").c_str()) << endl;
        f << "ADMISSION CONTROL: " << atoi(ConfigReader::getInstance().getVal("admission_id").c_str()) << endl;
        f << "JOB INTERVAL: " << (meanJobSize / Config::TOTAL_NUM_OF_TOKENS) / workload << endl;
        f << "CLOUD WORKLOAD: " << workload << endl;
        f << "*********** END OF INFO ***********" << endl;
        f.flush();
        f.close();
    }

    writeStatsToFile();
}

void Client::writeStatsToFile()
{
    int nbOfSamples = statsVec[0].size();

    std::ofstream f;
    f.open("./results/client/stats.csv");
    f << "rej,payment,paymentPercents\n";
    for (int i = 0; i < nbOfSamples; i++)
    {
        f << statsVec[0][i] << "," << statsVec[1][i] << "," << statsVec[2][i] << "\n";
    }
    f.close();

    f.open("./results/client/delay.csv");
    f << "delayPecents\n";
    nbOfSamples = statsVec[3].size();
    int sum = 0;
    for (int i = 0; i < nbOfSamples; i++)
    {
        sum += statsVec[3][i];
        f << sum << "\n";
    }
    f.close();
}

// Declare the static variables.
client_t Config::clientCount = 0;

void Client::initialize()
{
    clientId = Config::clientCount++;

    _reqCount = 0;
    _rejectedReqCount = 0;
    _completedReqCount = 0;
    _zeroPaymentReqCount = 0;

    _rejectRateCount = 0;

    _cumulativePayment = 0.0;
    maxPayments = 1.0;

    double mean = atof(ConfigReader::getInstance().getVal("job_size_scale_mean").c_str());
    double std = atof(ConfigReader::getInstance().getVal("job_size_scale_std").c_str());
    var_nor = new boost::variate_generator<boost::mt19937&, boost::normal_distribution<> >((*new boost::mt19937()),
            (*new boost::normal_distribution<>(mean, std)));

    cMessage* reqTimer = new cMessage("An event msg to generate requests", Config::GENERATE_REQ_EVENT_MSG);
    scheduleAt(simTime(), reqTimer);

    ConfigReader& configReader = ConfigReader::getInstance();

    /**
     * Statistics related variables initialization.
     */
    this->statsEventMsg = new cMessage("Recording the stats of the client.", Config::CLIENT_STATS_EVENT);
    scheduleAt(simTime() + Config::STATISTIC_SAMPLING_INTERVAL, statsEventMsg);
    std::vector<int> rejRatiosVec;
    statsVec.push_back(rejRatiosVec);
    std::vector<int> paymentVec;
    statsVec.push_back(paymentVec);
    std::vector<int> paymentPercents;
    statsVec.push_back(paymentPercents);
    std::vector<int> delayPercents;
    for (int i = 0; i < 1000; i++)
        delayPercents.push_back(0);
    statsVec.push_back(delayPercents);
}

Request* Client::generateRequest()
{
    // Estimate the size of the job.
//    double jobSize = 2500.0 + 5000.0 * dblrand();
    // The job size is subject to a half normal distribution.
    double jobSize = atof(ConfigReader::getInstance().getVal("job_size_base").c_str()) * (1 + fabs((*var_nor)()));

    // Generate a budget function based on the job size.
    Function* func = generateBudgetFunction(jobSize);

    // Create a request object to represent the new request.
    Request* req = new BatchProcessingRequest(_reqCount++, clientId, func, jobSize);
    return req;
}

Function* Client::generateBudgetFunction(double jobSize)
{
    // Currently only support linear inverse budget functions.
    Function* func = generateLinearBudgetFunction(jobSize);

    return func;
}

Function* Client::generateLinearBudgetFunction(double jobSize)
{
    double K = Config::TOTAL_NUM_OF_TOKENS;
    double minDur = (jobSize / K) * (1 + 4 * dblrand()); // s/k * rnd(1:5)
    double maxDur = (minDur + 1) * (1 + 2 * dblrand()); // ds * rnd(1,3)

//    double minDur = (jobSize / K) * (1 + 9 * dblrand()); // s/k * rnd(1:10)
//    double maxDur = (minDur + 1) * (1 + 2 * dblrand()); // ds * rnd(1,3)

//    double minDur = (jobSize / K) * (1 + 4 * dblrand()); // s/k * rnd(1:5)
//    double maxDur = (minDur + 1) * (1 + 4 * dblrand()); // ds * rnd(1,5)

    double maxPay = jobSize / minDur;
    double minPay = jobSize / maxDur;

    double slope = (maxPay - minPay) / (minDur - maxDur);
    double intercept = maxPay - slope * minDur;

    // Function shape: payment = slope / duration + intercept
//    Function* func = new LinearFunction(minDur, maxDur, intercept, slope);

    // Paolo's flat price line code
    Function* func = new LinearFunction(minDur, maxDur, intercept, -0.01);
    // END of Paolo's code

    return func;
}

void Client::handleMessage(cMessage* msg)
{
    switch (msg->getKind())
    {
        case Config::GENERATE_REQ_EVENT_MSG: {
            // Stop generating new requests after a certain time duration.
            double sim_len = atof(ConfigReader::getInstance().getVal("sim_len").c_str());
            if (simTime().dbl() > sim_len)
                break;

            Request* req = generateRequest();
            _reqPtrList.push_back(req);

            cMessage* reqMsg = new cMessage("A message to carry the newly generated request", Config::REQ_MSG);
            reqMsg->addObject(req);

            this->send(reqMsg, "out"); // send this request to the arbitrator
            double meanScale = atof(ConfigReader::getInstance().getVal("job_size_scale_std").c_str()) * 1.414 / 1.78;
            double meanJobSize = atof(ConfigReader::getInstance().getVal("job_size_base").c_str()) * (1 + meanScale);
            double workload = atof(ConfigReader::getInstance().getVal("workload").c_str());
            scheduleAt(simTime() + exponential((meanJobSize / Config::TOTAL_NUM_OF_TOKENS) / workload), msg); // Poisson process
            break;
        }
        case Config::REQ_MSG: {
            // The decision if the request is accepted by the Cloud.
            bool decision = msg->par("decision").boolValue();

            Request* req = (Request*) msg->getParList()[0];

            if (decision == false)
            {
                std::cout << ">> REQUEST: " << req->reqId << " IS REJECTED!" << std::endl;
                std::list<Request*>::iterator itr = std::find_if(_reqPtrList.begin(), _reqPtrList.end(),
                        Request::reqEq(req));
                _reqPtrList.erase(itr);
                _rejectedReqCount++;
                _rejectRateCount++;
            }
            else
            {
                std::cout << ">> REQUEST: " << req->reqId << " IS ACCEPTED!" << std::endl;
            }

            msg->removeObject(req);
            delete msg;
            break;
        }
        case Config::JOB_COMLETION_FEEDBACK_MSG: {
            std::cout << "************ JOB EXIT EVENT ***********" << std::endl;
            request_t reqId = msg->par("_requestId").longValue();
//            if (reqId > Config::REQUEST_COUNT)
//            {
//				this->endSimulation();
//            }

            for (std::list<Request*>::iterator itr = _reqPtrList.begin(); itr != _reqPtrList.end(); itr++)
            {
                if ((*itr)->reqId == reqId)
                {
                    double jobDuration = simTime().dbl() - (*itr)->createdTime;
                    money_t payment = (*itr)->returnProfit(jobDuration);
                    if (payment <= 0.001)
                    {
                        _zeroPaymentReqCount++;
                    }

                    this->_cumulativePayment += payment;
                    _completedReqCount++;
                    std::cout << ">> REQUEST: " << (*itr)->reqId << " FINISHES WITH PAY: " << payment << ". MAX: "
                            << (*itr)->budgetFunc->returnY((*itr)->budgetFunc->minX) << endl;
                    maxPayments += (*itr)->budgetFunc->returnY((*itr)->budgetFunc->minX);

//                    double minDur = (*itr)->budgetFunc->minX;
//                    double delay = jobDuration - minDur;
//                    if (delay < 0)
//                        delay = 0;
//                    int delayPercents = 100 * (delay / minDur);
//                    if (delayPercents > int(statsVec[3].size())) {
//                        cout << delayPercents << " " << (*itr)->budgetFunc->minX << " " << (*itr)->budgetFunc->maxX << endl;
//                        cout << (*itr)->jobSize << " " << (*itr)->createdTime << " " << simTime().dbl() << " " << simTime() << endl;
//                    }
//                    assert(delayPercents < int(statsVec[3].size()));
//                    statsVec[3][delayPercents] += 1;

                    _reqPtrList.erase(itr);
                    break;
                }
            }

            delete msg;
            break;
        }
        case Config::CLIENT_STATS_EVENT: {
            double sim_len = atof(ConfigReader::getInstance().getVal("sim_len").c_str());
            if (simTime().dbl() > sim_len)
                break;

            statsVec[0].push_back(_rejectRateCount);
            _rejectRateCount = 0; // reset the counter
            statsVec[1].push_back(_cumulativePayment);
            assert((100 * _cumulativePayment) / maxPayments >= -0.001);
            statsVec[2].push_back(int((100 * _cumulativePayment) / maxPayments));

            scheduleAt(simTime() + Config::STATISTIC_SAMPLING_INTERVAL, msg);
            break;
        }
        default: {
            break;
        }
    }
}
