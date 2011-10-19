#ifndef BINARY_KERNEL_HPP_
#define BINARY_KERNEL_HPP_

namespace rjmcmc {

    /*

    class unary_kernel
    {
        double m_p;
    public:
        enum { size = 1 };
        unary_kernel(double p=1.) : m_p(p) {}
        inline unsigned int kernel_id() const { return 0; }
        inline void probability(double p) { m_p = p; }
        inline double probability() const { return m_p; }
    };
*/


    // a binary_kernel encodes a reversible jump
    // for complex problems, a reversible jump typically only modifies a portion of the whole configuration.
    // eg for mpp problems, a reversible jump typically only adds, removes or modifies a few objects.
    // a view denotes the part of the configuration modified by the jump, other aspects of the configuration remaining constant.
    /*
    //

        // Kernel0 provides a between a subview of the configuration of type Input, and the possibly additional input random variates
    // Kernel1 provides a between a subview of the configuration of type Output, and the possibly additional output random variates
    // trivial problems do not require Views: types View0, View1 and Configuration are equal, and Kernels return the whole configuration
    // for mpp problems, a Kernel may return a 0, 1 or more objects that are to be killed, modified, switched, split...

    // Transforms maps
    // Kernels are also responsible

*/

    /*
    template<typename Kernel, typename Transform,
             typename View = Transform::first_argument_type,
             > class unary_kernel
*/

    class null_view
    {
    public:
        enum { dimension = 0 };
        template<typename Configuration, typename Modification, typename OutputIterator>
        inline double operator()(Configuration& c, Modification& modif, OutputIterator it) const {
            return 1.;
        }
        template<typename Configuration, typename Modification, typename InputIterator>
        inline double inverse_pdf(Configuration& c, Modification& modif, InputIterator it) const {
            return 1.;
        }
    };


    template<unsigned int N>
    class variate
    {
        typedef boost::variate_generator<rjmcmc::mt19937_generator&, boost::uniform_real<> > die_type;
        mutable die_type m_die;
    public:
        enum { dimension = N };
        template<typename Configuration, typename Modification, typename OutputIterator>
        inline double operator()(Configuration& c, Modification& modif, OutputIterator it) const {
            for(unsigned int i=0; i<N; ++i) *it++ = m_die();
            return 1.;
        }
        template<typename Configuration, typename Modification, typename InputIterator>
        inline double inverse_pdf(Configuration& c, Modification& modif, InputIterator it) const {
            // check support
            return 1.;
        }
        template<typename InputIterator>
        inline double pdf(InputIterator it) const {
            for(unsigned int i=0; i<N; ++i, ++it) if(*it<0 || *it>1) return 0;
            return 1.;
        }
        variate() : m_die(rjmcmc::random(), boost::uniform_real<>(0,1)) {}
    };

    template<typename View0, typename View1, typename Variate0, typename Variate1, typename Transform> class kernel
    {
    double m_p, m_p01, m_p10;
    View0 m_view0;
    View1 m_view1;
    Variate0 m_variate0;
    Variate1 m_variate1;
    Transform m_transform;
    mutable unsigned int m_kernel_id;
    public:
    enum { size = 2 };
    inline unsigned int kernel_id() const { return m_kernel_id; }
    inline const char* name(unsigned int i) const { return "kernel"; }//(i)?m_kernel1.name():m_kernel0.name(); }

    kernel(const View0& v0, const View1& v1, const Variate0& x0, const Variate1& x1, const Transform& t, double p01=0.5, double p10=0.5) :
            m_view0(v0), m_view1(v1), m_variate0(x0), m_variate1(x1), m_transform(t), m_p(p01+p10), m_p01(p01), m_p10(p10)
    {
    }
    kernel(double p01=0.5, double p10=0.5) :
            m_view0(), m_view1(), m_variate0(), m_variate1(), m_transform(), m_p(p01+p10), m_p01(p01), m_p10(p10)
    {
    }
    inline double probability() const { return m_p; }

    // p is uniform between 0 and probability()
    template<typename Configuration, typename Modification>
    double operator()(double p, Configuration& c, Modification& modif) const
    {
        modif.clear();
        double val0[Transform::dimension];
        double val1[Transform::dimension];
        if(p<m_p01) { // branch probability : m_p01
            m_kernel_id = 0;
            double *var0 = val0 + View0::dimension;
            double *var1 = val1 + View1::dimension;
            double J01  = m_view0   (c,modif,val0);             // returns the discrete probability that samples the portion of the configuration that is being modified (stored in the modif input)
            double phi01= m_variate0(c,modif,var0);             // returns the continuous probability that samples the completion variates
            m_transform.apply(val0,val1);                       // computes val1 from val0
            double phi10= m_variate1.inverse_pdf(c,modif,var1); // returns the continuous probability of the inverse variate sampling, arguments are constant
            double J10  = m_view1   .inverse_pdf(c,modif,val1); // returns the discrete probability of the inverse view sampling, arguments are constant except val1 that is encoded in modif
            return m_transform.abs_jacobian(val0)*(m_p10*J10*phi10)/(m_p01*J01*phi01);
        } else { // branch probability : m_p10
            m_kernel_id = 1;
            double *var1 = val1 + View1::dimension;
            double *var0 = val0 + View0::dimension;
            double J10  = m_view1   (c,modif,val1);      // returns the discrete probability that samples the portion of the configuration that is being modified (stored in the modif input)
            double phi10= m_variate1(c,modif,var1);      // returns the continuous probability that samples the completion variates
            m_transform.inverse(val1,val0);                       // computes val0 from val1
            double J01  = m_view0   .inverse_pdf(c,modif,var0);  // returns the continuous probability of the inverse variate sampling, arguments are constant
            double phi01= m_variate0.inverse_pdf(c,modif,val0);  // returns the discrete probability of the inverse view sampling, arguments are constant except val0 that is encoded in modif
            return m_transform.abs_jacobian(val1)*(m_p01*J01*phi01)/(m_p10*J10*phi10);
        }
    }
};


}; // namespace rjmcmc

#endif // BINARY_KERNEL_HPP_
