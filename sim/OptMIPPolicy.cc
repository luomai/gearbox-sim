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

#include "OptMIPPolicy.h"

OptMIPPolicy::OptMIPPolicy()
{
    // TODO Auto-generated constructor stub
}

OptMIPPolicy::~OptMIPPolicy()
{
    // TODO Auto-generated destructor stub
}

money_t OptMIPPolicy::returnExpectedPayment(Function* budgetFunction, double duration)
{
    if (duration > budgetFunction->maxX)
    {
        return budgetFunction->returnY(budgetFunction->maxX);
    }
    else if (duration < budgetFunction->minX)
    {
        return 0;
    }
    else
    {
        return budgetFunction->returnY(duration);
    }
}

money_t OptMIPPolicy::computeMarginalReturn(std::list<Job*> &jobPtrList)
{
    money_t marginalReturn = 0;
    for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
    {
        double remainingTime = (*itr)->estimateRemainingTimeWithVariedTokenAllocation();
        if (remainingTime > 0)
        {
            double expectedJobDuration = (*itr)->expectedJobLifetimeWithVariedTokenAllocation();
            double expectedPayment = returnExpectedPayment((*itr)->budgetFunc, expectedJobDuration);
//            double partialPayment = expectedPayment
//                    * ((100.0 - (*itr)->_progress)) * 0.01;

            double remainingTime = (*itr)->estimateRemainingTimeWithVariedTokenAllocation();
            marginalReturn += expectedPayment / remainingTime;
        }
        else
        {
            // the job hasn't been allocated any token yet. Treat it as 0 profit.
        }
    }

    return marginalReturn;
}

money_t OptMIPPolicy::computeMarginalReturn(std::list<Job*> &jobPtrList, IloCplex& cplex, IloArray<IloNumVarArray>& x,
        IloInt nbTimeUnits)
{
    money_t marginalReturn = 0;
    IloInt j = 0;
    for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++, j++)
    {
        // step 1: Calculate the remaining time of the job according to the solution given by cplex.
        IloInt timeUnitsCount = nbTimeUnits;
        for (IloInt t = nbTimeUnits - 1; t >= 0; t--)
        {
            if (cplex.getValue(x[j][t]) - 0.0 > 0.01)
                break;
            timeUnitsCount--;
        }
        double remainingTime = double(timeUnitsCount) * Config::TIME_UNIT;

        // step 2: Calculate the expected return from this job given the remaining time estimation.
        double expectedJobDuration = remainingTime + (simTime().dbl() - (*itr)->enterTime.dbl());
        if (remainingTime > 0)
        {
            double expectedPayment = returnExpectedPayment((*itr)->budgetFunc, expectedJobDuration);
//            double partialPayment = expectedPayment
//                    * ((100.0 - (*itr)->_progress)) * 0.01;

            marginalReturn += expectedPayment / remainingTime;
        }
        else
        {
            // the job hasn't been allocated any token yet. Treat it as 0 profit.
        }
    }
    return marginalReturn;
}

money_t OptMIPPolicy::computeTotalReturn(std::list<Job*> &jobPtrList)
{
    money_t totalReturn = 0;
    for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
    {
        double expectedJobDuration = (*itr)->expectedJobLifetimeWithVariedTokenAllocation();
        if (expectedJobDuration > 0)
        {
            double expectedPayment = returnExpectedPayment((*itr)->budgetFunc, expectedJobDuration);
//            double partialPayment = expectedPayment * ((100.0 - (*itr)->_progress)) * 0.01;
            totalReturn += expectedPayment;
        }
    }

    return totalReturn;
}

bool OptMIPPolicy::allocateTokensToJobs(std::list<Job*> &jobPtrList, token_t totalNumOfTokens)
{
    // Step 1: Calculate the return of the current allocation.
    money_t currentReturn = 0;

    // Presuming if the new job has not yet been inserted.
    double orginalTotalReturn;
    Job* job = jobPtrList.back();
    jobPtrList.pop_back();
    switch (atoi(ConfigReader::getInstance().getVal("admission_id").c_str()))
    {
        case Config::TOTAL: {
            currentReturn = computeTotalReturn(jobPtrList);
            break;
        }
        case Config::MARGINAL: {
            currentReturn = computeMarginalReturn(jobPtrList);
            break;
        }
        case Config::AVG_JOB_RETURN: {
            orginalTotalReturn = computeTotalReturn(jobPtrList);
            if (jobPtrList.size() > 0)
            {
                currentReturn = orginalTotalReturn / jobPtrList.size();
            }
            else
            {
                currentReturn = 0;
            }
            break;
        }
        case Config::NO_ADMISSION_CONTROL: {
            break;
        }
        default: {
            std::cout << "Undefined admission control policy in the OptComplexPolicy.";
            exit(1);
        }
    }

    // Step 2: Solve the mixed-integer program assuming the new job has been accepted.

    jobPtrList.push_back(job); // Insert the new job again.

    double currentTime = simTime().dbl();
    double maxRemainingTime = 0.0; // The 'T' in the optimization program.
    for (list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
    {
        double latestDeadline = (*itr)->enterTime.dbl() + (*itr)->budgetFunc->maxX;
        double remainingTime = latestDeadline - currentTime;
        assert(remainingTime > 0.0);
        if (remainingTime > maxRemainingTime)
            maxRemainingTime = remainingTime;
    }

    IloInt nbTimeUnits = IloInt(maxRemainingTime / Config::TIME_UNIT) + 1;
    assert(nbTimeUnits > 1);

    IloEnv env;
    try
    {
        IloModel model(env);
        IloInt nbJob = jobPtrList.size();
        IloInt nbToken = totalNumOfTokens;
        IloInt j, t;
        IloArray<IloNumVarArray> x(env, nbJob);
        IloArray<IloBoolVarArray> y(env, nbJob);
        IloNumVarArray z(env); // The profit of each job
        list<Job*>::iterator itr;
        IloInt t_min, t_max; // t_min represents the number of time units required at least to finish within the earliest deadline

        for (j = 0; j < nbJob; j++)
        {
            // create the token variables: x_jt means that the job j has x_jt tokens at time t.
            x[j] = IloNumVarArray(env, nbTimeUnits, 0, nbToken, ILOINT); // INT variable

            // create the decision variables. y_jt = 1 means that the job j is finished at time t.
            y[j] = IloBoolVarArray(env, nbTimeUnits); // binary variable
            IloExpr ySum(env);
            for (t = 0; t < nbTimeUnits; t++)
            {
                ySum += y[j][t];
            }
            model.add(ySum == 1);

            // create the variable z to represent the profit from each job
            IloNumVar p(env);
            z.add(p);
        }

        // Impose the constraint that the job must be finished within SLO: sum(y) = 1 for each job
        itr = jobPtrList.begin();
        for (j = 0; j < nbJob; j++)
        {
            double elapsedDur = currentTime - (*itr)->enterTime.dbl();

            // compute the t_max
            IloNum maxDur = (*itr)->budgetFunc->maxX;
            t_max = IloInt((maxDur - elapsedDur) / Config::TIME_UNIT);
            assert(t_max >= 0);

            // add the constraint that ensures the job must be completed earlier than its t_max (latest deadline).
            // NOTE: the job can be completed earlier than its t_min even though this won't bring a great deal of extra profit.
            IloExpr ySum(env);
            for (t = 0; t < t_max; t++)
            {
                ySum += y[j][t];
            }
            model.add(ySum == IloTrue);

            itr++; // handle the next job
        }

        // Impose the constraint that the job will not be allocated any token if it has already been finished.
//        IloInt M = 1000000; // a very large number
        itr = jobPtrList.begin(); // reset the iterator
        for (j = 0; j < nbJob; j++)
        {
            double elapsedDur = currentTime - (*itr)->enterTime.dbl();
            double maxDur = (*itr)->budgetFunc->maxX;
            t_max = IloInt((maxDur - elapsedDur) / Config::TIME_UNIT);
            for (t = 0; t < nbTimeUnits; t++)
            {
                IloExpr ySum(env);
                for (IloInt z = t; z < nbTimeUnits; z++)
                {
                    ySum += y[j][z];
                }
                IloNum K = Config::TOTAL_NUM_OF_TOKENS;
                model.add(x[j][t] <= K * ySum); // state the constraint 0 <= x[j][t] <= M * ySum
            }

            itr++; // handle the next job
        }

        // Impose the job completion constraint sum(x_jt * time_unit) = remaining_job_size
        itr = jobPtrList.begin();
        for (j = 0; j < nbJob; j++)
        {
            IloNum elapsedDur = currentTime - (*itr)->enterTime.dbl();

            // compute the t_max
            IloNum maxDur = (*itr)->budgetFunc->maxX;
            t_max = IloInt((maxDur - elapsedDur) / Config::TIME_UNIT);
            assert(t_max >= 0);

            IloExpr xaSum(env);
            IloNum remainingJobSize = (*itr)->jobSize - (*itr)->jobSizeCompleted;
            for (t = 0; t < t_max; t++)
            {
                xaSum += x[j][t] * Config::TIME_UNIT; // cumulative job size completed
            }

            // The cumulative completed job size equals the remaining job size.
//            IloNum error = Config::JOBSIZE_ERROR;
//            model.add(xaSum >= remainingJobSize - error);
//            model.add(xaSum <= remainingJobSize + error);
            model.add(xaSum >= remainingJobSize);

            itr++;
        }

        // Impose the resource constraint for each t
        for (t = 0; t < nbTimeUnits; t++)
        {
            IloExpr xSum(env);
            for (j = 0; j < nbJob; j++)
                xSum += x[j][t];
            model.add(xSum <= nbToken);
        }

        // Impose the constraint for the profit of each job
        itr = jobPtrList.begin();
        for (j = 0; j < nbJob; j++)
        {
            IloNum elapsedDur = currentTime - (*itr)->enterTime.dbl();

            // compute the t_max
            IloNum maxDur = (*itr)->budgetFunc->maxX;
            t_max = IloInt((maxDur - elapsedDur) / Config::TIME_UNIT);
            assert(t_max >= 0);

            IloExpr remainingTimeUnits(env);
            for (t = 0; t < t_max; t++)
                remainingTimeUnits += (t + 1) * y[j][t];

            IloExpr jobDur(env);
            IloExpr remainingDur(env);
            remainingDur = remainingTimeUnits * Config::TIME_UNIT;
            jobDur = elapsedDur + remainingDur;

            // using piecewise function to depict the profit function
            IloNum minDur = (*itr)->budgetFunc->minX;
            IloNum maxPay = (*itr)->budgetFunc->returnY(minDur);
            IloNum minPay = (*itr)->budgetFunc->returnY(maxDur);
            IloNum slope = (maxPay - minPay) / (minDur - maxDur);
            assert(slope < 0);

            IloExpr profit(env);
            IloNum step1 = maxPay;
            IloNum step2 = 0.0 - minPay;
            IloNum fakeSlope = -0.001; // for ensuring that jobs would be finished ASAP
            profit = IloPiecewiseLinear(jobDur, IloNumArray(env, 5, 0.0, 0.0, minDur, maxDur, maxDur),
                    IloNumArray(env, 6, 0.0, step1, fakeSlope, slope, step2, 0.0), -1.0, 0.0);

            model.add(z[j] <= profit);
            itr++;
        }

        // state the objective
        // the objective is composed of the profit from each job
        IloExpr totalProfit(env);
        for (j = 0; j < nbJob; j++)
            totalProfit += z[j];
        IloObjective obj = IloMaximize(env, totalProfit);
        model.add(obj);

        IloCplex cplex(model);
        cplex.setParam(IloCplex::EpGap, 0.05);
        cplex.setParam(IloCplex::TiLim, 30);

        // Optimize the problem and obtain solution.
        if (!cplex.solve())
        {
            cout << " ------------------- failed reason ---------------------" << endl;
            env.error() << "Failed to optimize MLP";
            throw(-1);
        }

        bool decision = NULL;
        switch (atoi(ConfigReader::getInstance().getVal("admission_id").c_str()))
        {
            case Config::TOTAL: {
                decision = (currentReturn < cplex.getObjValue()) ? true : false;
                break;
            }
            case Config::MARGINAL: {
                money_t marginalReturn = computeMarginalReturn(jobPtrList, cplex, x, nbTimeUnits);
                decision = (currentReturn < marginalReturn) ? true : false;

                // Testing block
                if (currentReturn >= marginalReturn)
                {
                    cout << "current return: " << currentReturn << endl;
                    cout << "marginalReturn: " << marginalReturn << endl;
                }
                // end of testing block.

                break;
            }
            case Config::AVG_JOB_RETURN: {
//            double avgPerJobReturn = cplex.getObjValue() / jobPtrList.size();
//            decision = ((currentReturn * 0.9) < avgPerJobReturn) ? true : false;
                double newJobReturn = cplex.getObjValue() - orginalTotalReturn;
                decision = (newJobReturn > (currentReturn * Config::MIP_BETA)) ? true : false;
                break;
            }
            case Config::NO_ADMISSION_CONTROL: {
                decision = true;
                break;
            }
            default: {
                cout << "Undefined admission control policy in the OptComplexPolicy.";
                exit(1);
            }
        }

        if (decision)
        {
            env.out() << "Solution status = " << cplex.getStatus();
            std::cout << endl;
            env.out() << "Solution value  = " << cplex.getObjValue();
            std::cout << endl;

            itr = jobPtrList.begin();
            for (j = 0; j < nbJob; j++)
            {
                // clear the original queue
                (*itr)->tokenList.clear();

                // allocate tokens
                (*itr)->numOfTokens = IloAbs(IloRound(cplex.getValue(x[j][0])));

                // store the future token schedule
                for (t = 1; t < nbTimeUnits; t++)
                {
                    (*itr)->tokenList.push_back(IloAbs(IloRound(cplex.getValue(x[j][t]))));
                }

                itr++;
            }

            env.end();
            return true;
        }
        else
        {
            cout << " ------------------- failed reason ---------------------" << endl;
            cout << "_currentEarning > cplex.getObjValue()" << endl;
            env.out() << "cplex.getObjValue() = " << cplex.getObjValue();
            std::cout << endl;
            cout << "nbJob = " << nbJob << endl;
            cout << "nbTimeUnits = " << nbTimeUnits << endl;
            cout << "current time = " << simTime().dbl() << endl;

//            itr = jobPtrList.begin();
//            j = 0;
//            while (itr != jobPtrList.end()) {
//                cout << "job[" << j << "]: progress = " << (*itr)->_progress
//                        << ", enter time = " << (*itr)->_enterTime << endl;
//                cout << "d1 = " << 100.0 / (*itr)->_budgetFunction->_end
//                        << ", d2 = " << 100.0 / (*itr)->_budgetFunction->_begin
//                        << endl;
//                cout << "====================================================="
//                        << endl;
//                itr++;
//            }
//
//            for (j = 0; j < nbJob; j++) {
//                cout << "x[j][t] = ";
//                for (t = 0; t < nbTimeUnits; t++)
//                    env.out() << IloAbs(IloRound(cplex.getValue(x[j][t])))
//                            << ", ";
//                cout << endl;
//
//                cout << "y[j][t] = ";
//                for (t = 0; t < nbTimeUnits; t++)
//                    env.out() << cplex.getValue(y[j][t]) << ", ";
//                cout << endl;
//
//                env.out() << "z[j] = " << cplex.getValue(z[j]);
//                cout << endl;
//            }

            env.end();
            return false;
        }
    }
    catch (IloException& e)
    {
        cerr << "Concert exception caught: " << e << endl;
    }
    catch (...)
    {
        cerr << "Unknown exception caught" << endl;
    }

    env.end();
    return false;
}

void OptMIPPolicy::reallocateTokensToJobs(std::list<Job*> &jobPtrList, token_t totalNumOfTokens)
{
    // do nothing
}
