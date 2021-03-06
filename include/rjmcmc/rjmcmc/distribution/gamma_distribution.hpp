/***********************************************************************
This file is part of the librjmcmc project source files.

Copyright : Institut Geographique National (2008-2012)
Contributors : Mathieu Brédif, Olivier Tournaire, Didier Boldo
email : librjmcmc@ign.fr

This software is a generic C++ library for stochastic optimization.

This software is governed by the CeCILL license under French law and
abiding by the rules of distribution of free software. You can use,
modify and/or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty and the software's author, the holder of the
economic rights, and the successive licensors have only limited liability.

In this respect, the user's attention is drawn to the risks associated
with loading, using, modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean that it is complicated to manipulate, and that also
therefore means that it is reserved for developers and experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and, more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL license and that you accept its terms.

***********************************************************************/

#ifndef __GAMMA_DISTRIBUTION_HPP__
#define __GAMMA_DISTRIBUTION_HPP__

#include <boost/random/gamma_distribution.hpp>
#include <boost/math/distributions/gamma.hpp>

namespace rjmcmc {

    // beta^alpha * x^(alpha-1) * exp(-beta * x) / Gamma(alpha)
    class gamma_distribution {
    public:
        typedef double real_type;
        typedef boost::gamma_distribution<real_type>          rand_distribution_type;
        typedef boost::math::gamma_distribution<real_type>    math_distribution_type;

        gamma_distribution(real_type alpha, real_type beta)
            : m_rand(alpha,beta)
            , m_math(alpha,1./beta)
        {}

        // new/old: (mean^(n1-n0) * n0! / n1!
        real_type pdf_ratio(real_type x0, real_type x1) const
        {
            return pow(x1/x0,m_rand.alpha()-1)*exp(m_rand.beta()*(x0-x1));
        }

        real_type pdf(real_type x) const
        {
            return boost::math::pdf(m_math, x);
        }

        template<typename Engine>
        inline real_type operator()(Engine& e) const { return m_rand(e); }

    private:
        mutable rand_distribution_type m_rand;
        math_distribution_type m_math;
    };

}; // namespace rjmcmc

#endif // __GAMMA_DISTRIBUTION_HPP__
