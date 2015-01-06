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

#include "OptLPPolicy.h"

OptLPPolicy::OptLPPolicy()
{
    // TODO Auto-generated constructor stub
    _currentTotalReturn = 0;
}

OptLPPolicy::~OptLPPolicy()
{
    // TODO Auto-generated destructor stub
}

IloExpr& returnApproximatedReturnFunction(IloEnv& env, IloNumVar& x, Function* func)
{
    IloExpr profit(env);
    IloNum minToken = func->minX;
    IloNum maxToken = func->maxX;
    IloNum stepSize = (maxToken - minToken) / 3;
    if (stepSize == 0)
        stepSize += 0.001;
    IloNum slope1 = (func->returnY(minToken + stepSize) - func->returnY(minToken)) / stepSize;
    assert(slope1 >= 0);
    IloNum slope2 = (func->returnY(minToken + 2 * stepSize) - func->returnY(minToken + stepSize)) / stepSize;
    assert(slope2 >= 0);
    IloNum slope3 = (func->returnY(minToken + 3 * stepSize) - func->returnY(minToken + 2 * stepSize)) / stepSize;
    assert(slope3 >= 0);
    IloNum step1 = func->returnY(minToken);
    profit = IloPiecewiseLinear(x,
            IloNumArray(env, 5, minToken, minToken, minToken + stepSize, minToken + stepSize * 2, maxToken),
            IloNumArray(env, 6, 0.0, step1, slope1, slope2, slope3, 0.0), 0.0, 0.0);
    return profit;
}

bool OptLPPolicy::allocateTokensToJobs(std::list<Job*> &jobPtrList, token_t totalNumOfTokens)
{
    token_t numOfTokenUsed = 0;
    for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
    {
        if (Config::UPDATE_RETURN_FUNCTION == true)
            (*itr)->updateMultiplicativeInverseReturnFunction();
        numOfTokenUsed += (*itr)->returnFunc->minX;
    }
    if (numOfTokenUsed > Config::TOTAL_NUM_OF_TOKENS)
        return false;

    IloEnv env;
    try
    {
        IloModel model(env);
        IloNumVarArray var(env);
        for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
        {
            IloNumVar x(env, (*itr)->returnFunc->minX, (*itr)->returnFunc->maxX, ILOINT);
            var.add(x);
        }
        model.add(var);

        IloExpr expr1(env);
        IloInt i = 0;
        for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
        {
//            expr1 += (*itr)->returnFunc->returnExpr(env, var[i++]);
            expr1 += returnApproximatedReturnFunction(env, var[i++], (*itr)->returnFunc);
        }

        IloObjective obj = IloMaximize(env, expr1);
        model.add(obj);

        IloExpr expr2(env);
        for (i = 0; i < var.getSize(); i++)
        {
            expr2 += var[i];
        }
        model.add(expr2 <= Config::TOTAL_NUM_OF_TOKENS);

        IloCplex cplex(model);

        // Optimize the problem and obtain solution.
        if (!cplex.solve())
        {
            std::cout << "Failed to optimize LP" << endl;
            throw(-1);
        }

        bool decision = false;
        switch (atoi(ConfigReader::getInstance().getVal("admission_id").c_str()))
        {
            case Config::TOTAL: {
                decision = (cplex.getObjValue() > this->_currentTotalReturn) ? true : false;
                break;
            }
            case Config::AVG_JOB_RETURN: {
                double avgJobReturn = 0.0;
                if (jobPtrList.size() > 1)
                    avgJobReturn = this->_currentTotalReturn / (jobPtrList.size() - 1);
                double newJobReturn = cplex.getObjValue() - this->_currentTotalReturn;
                decision = (newJobReturn > (avgJobReturn * Config::MIP_BETA)) ? true : false;
//                cout << "newJobReturn: " << newJobReturn << endl;
//                cout << "avgJobReturn: " << avgJobReturn << endl;
//                int tmp;
//                cin >> tmp;
                break;
            }
            case Config::MARGINAL: {
                cout << "ERROR: The MARGINAL cannot be used in the LP." << endl;
                exit(0);
            }
            case Config::NO_ADMISSION_CONTROL: {
                decision = true;
                break;
            }
            default: {
                cout << "ERROR: The admission control policy is undefined." << endl;
                exit(0);
            }
        }

        if (decision)
        {
            //env.out() << "Solution status = " << cplex.getStatus() << endl;
            // update the current earning
            this->_currentTotalReturn = cplex.getObjValue();
            //env.out() << "Solution value  = " << cplex.getObjValue() << endl;

            // update the token allocation
            IloNumArray vals(env);
            cplex.getValues(vals, var);
            //env.out() << "Values        = " << vals << endl;
            IloIntArray IloVals = vals.toIntArray();
            i = 0;
            for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
            {
                //assert(IloVals[i] >= (*itr)->returnMinNumOfTokens() && IloVals[i] <= (*itr)->returnMaxNumOfTokens());
                (*itr)->numOfTokens = IloVals[i++];
            }

            env.end();
            return true;
        }
        else
        {
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

void OptLPPolicy::reallocateTokensToJobs(std::list<Job*> &jobPtrList, token_t totalNumOfTokens)
{
    IloEnv env;
    try
    {
        IloModel model(env);
        IloNumVarArray var(env);
        for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
        {
            if (Config::UPDATE_RETURN_FUNCTION == true)
                (*itr)->updateMultiplicativeInverseReturnFunction();
            IloNumVar x(env, (*itr)->returnFunc->minX, (*itr)->returnFunc->maxX, ILOINT);
            var.add(x);
        }
        model.add(var);

        IloExpr expr1(env);
        IloInt i = 0;
        for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
        {
//            expr1 += (*itr)->returnFunc->returnExpr(env, var[i++]);
            expr1 += returnApproximatedReturnFunction(env, var[i++], (*itr)->returnFunc);
        }

        IloObjective obj = IloMaximize(env, expr1);
        model.add(obj);

        IloExpr expr2(env);
        for (i = 0; i < var.getSize(); i++)
        {
            expr2 += var[i];
        }
        model.add(expr2 <= Config::TOTAL_NUM_OF_TOKENS);

        IloCplex cplex(model);

        // Optimize the problem and obtain solution.
        if (!cplex.solve())
        {
            std::cout << "Failed to optimize LP" << endl;
            throw(-1);
        }

        // update the current earning
        this->_currentTotalReturn = cplex.getObjValue();

        // update the token allocation
        IloNumArray vals(env);
        cplex.getValues(vals, var);
        IloIntArray IloVals = vals.toIntArray();
        i = 0;
        for (std::list<Job*>::iterator itr = jobPtrList.begin(); itr != jobPtrList.end(); itr++)
        {
            (*itr)->numOfTokens = IloVals[i++];
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
}
