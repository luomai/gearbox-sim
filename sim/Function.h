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

#ifndef FUNCTION_H_
#define FUNCTION_H_

#include <ilcplex/ilocplex.h>
using namespace std;
ILOSTLBEGIN

class Function
{
    public:
        double minX;
        double maxX;
        Function();
        Function(double a, double e);
        bool testLegality(double x)
        {
            return ((x > minX) && (x < maxX)) ? true : false;
        }
        virtual ~Function();
        virtual double returnY(double x) = 0;
        virtual double returnX(double y) = 0;
        virtual IloExpr& returnExpr(IloEnv& env, IloNumVar& x) = 0;
        virtual IloExpr& returnExpr(IloEnv& env, IloExpr& x) = 0;
        virtual Function* clone() = 0;
};

/**
 * y = _slope * x + _intercept
 */
class LinearFunction : public Function
{
    public:
        double intercept;
        double slope;
        LinearFunction()
        {
        }
        LinearFunction(double b, double e, double i, double s) :
                Function(b, e), intercept(i), slope(s)
        {
        }
        double returnY(double x)
        {
            return (x * slope + intercept);
        }
        double returnX(double y)
        {
            return (y - intercept) / slope;
        }

        IloExpr& returnExpr(IloEnv& env, IloNumVar& x)
        {
            assert(slope < 0);

            IloExpr* expr = new IloExpr(env);
            (*expr) += slope * x + intercept;
            return (*expr);
        }

        IloExpr& returnExpr(IloEnv& env, IloExpr& x)
        {
            assert(slope < 0);

            IloExpr* expr = new IloExpr(env);
            (*expr) += slope * x + intercept;
            return (*expr);
        }

        Function* clone()
        {
            return new LinearFunction(minX, maxX, intercept, slope);
        }

        virtual ~LinearFunction()
        {
        }
};

/**
 *  y = alpha / x + beta
 */
class MultiplicativeInverseFunction : public Function
{
    public:
        double alpha;
        double beta;
        MultiplicativeInverseFunction(double begin, double end, double a, double b) :
                Function(begin, end), alpha(a), beta(b)
        {
            assert(alpha < 0);
        }

        double returnY(double x)
        {
            return (alpha / x + beta);
        }

        double returnX(double y)
        {
            return alpha / (y - beta);
        }

        IloExpr& returnExpr(IloEnv& env, IloNumVar& x)
        {
            assert(alpha < 0);

            IloExpr* expr = new IloExpr(env);
            (*expr) += alpha / x + beta;
            return (*expr);
        }

        IloExpr& returnExpr(IloEnv& env, IloExpr& x)
        {
            assert(alpha < 0);

            IloExpr* expr = new IloExpr(env);
            (*expr) += alpha / x + beta;
            return (*expr);
        }

        Function* clone()
        {
            return new MultiplicativeInverseFunction(minX, maxX, alpha, beta);
        }

        virtual ~MultiplicativeInverseFunction()
        {
        }
};

#endif /* FUNCTION_H_ */
