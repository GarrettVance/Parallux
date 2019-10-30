


#include "pch.h"

#include <complex>
#include "HyperbolicMethods.h"



using namespace HvyDXBase; 
using namespace Windows::Foundation;





double const HvyDXBase::konst_pi = 3.1415926535897932384626433;








double HvyDXBase::CircumradiusFromSchlafliCOSH(int schlafP, int schlafQ)
{
    //  Obtain circumradius from a Schlafli Symbol {p, q}. 
    //       
    //  Example: Schlafli {7, 3} has circumradius 0.300 742 618 746...
    //      
    //  The problem at hand is to solve a hyperbolic right triangle 
    //  in the Poincare disk model given only the three angle measures. 
    //  Solution is possible because similarity (AAA) implies congruence; 
    //          
    //  Start from the Hyperbolic Law of Cosines: 
    //  
    //  cos C = -cosA cosB + sinA sinB cosh(cHyp). 
    //          
    //  The triangle is fully determined by its Schlafli symbol, 
    //  and its three angles are pi/p,  pi/q  and pi/2. 
    //      
    //  In the formula above, "cHyp" is the hypotenuse (and also
    //  the circumradius of the tessellation's polygon) 
    //  and angle "C" is pi/2, for which the cosine is zero. 
    //      
    //      
    //              cos pi/2 + cos pi/p * cos pi/q 
    //  cosh cHyp = --------------------------------
    //                  sin pi/p * sin pi/q
    //          
    //      
    //      
    //                zero + cos pi/p * cos pi/q 
    //  cosh cHyp = --------------------------------
    //                    sin pi/p * sin pi/q
    //          
    //      
    //      
    //                           1 * 1
    //  cosh cHyp = --------------------------------
    //                    tan pi/p * tan pi/q
    //          
    //      
    //      
    //                           1
    //  cosh cHyp = --------------------------------
    //                       tanProduct
    //          
    //  where
    //      

    double tanProduct =
        std::tan(konst_pi / (double)schlafP) *
        std::tan(konst_pi / (double)schlafQ);


    double cHyp = std::acosh(1.00 / tanProduct);

    //  
    //  Hazard: cHyp is the circumradius in hyperbolic measure. 
    //          
    //  Convert from hyperbolic distance to Euclidean distance: 
    //  ======================================================= 
    //  Use Hvidsten Corollary 17.15 to convert
    //  from hyperbolic distance to Euclidean: 
    //      
    //      
    //  cEuc aka circumradius in Euclidean measure 
    //      = (exp(cHyp) - 1) / (exp(cHyp) + 1);
    //      
    //      
    //  Finally recognize that this expression for cEuc 
    //  looks a lot like the definition of tanh(). 
    //      

    double cEuc = std::tanh( cHyp / 2.000 );

    return cEuc;
}
    










    
double HvyDXBase::ApothemFromSchlafli(int schlafliP, int schlafliQ)
{
    //  This is based on the following identity 
    //  for hyperbolic right triangles: 
    //  
    //  cosh(adjacent) = cos(pi/q) / sin(pi/p)
    //      

    double cosPiQ = cos(konst_pi / (double)schlafliQ);

    double sinPiP = sin(konst_pi / (double)schlafliP);

    double apothemHyp = std::acosh(cosPiQ / sinPiP); 

    //  
    //  Convert from hyperbolic measure to Euclidean measure: 
    //  

    double apothem_Euc = std::tanh(apothemHyp / 2.000); 

    return apothem_Euc;
}










HvyGeodesic HvyDXBase::MakeGeodesicFromSchlafli(unsigned int schlafliP, unsigned int schlafliQ)
{
    double tmpCosQ = cos(konst_pi / (double)schlafliQ);
    double tmpSinP = sin(konst_pi / (double)schlafliP);

    double bHyp = std::acosh(tmpCosQ / tmpSinP); 

    double bEuc = std::tanh(bHyp / 2.000); 


    //  
    //  I think bEuc is aka apothem (in Euclidean units of measure). 
    // 



    //      
    //      ...continue the proof until in the form of...
    //      
    //   cCenter.x = sqrt(cosq2 / (cosq2 - sinp2));
    //   cCenter.y = 0.0;
    //   cRadius = sqrt(sinp2 / (cosq2 - sinp2));


    double tCenterX = (1.0 + bEuc * bEuc) / (2.0 * bEuc);
    double tCenterY = 0.000;
    double tRadius = (1.0 - bEuc * bEuc) / (2.0 * bEuc);


    //  
    //  TODO:    test:   want bEuc + cRadius = cCenter.x
    //  
    // double lhs = bEuc + cRadius;
    // double rhs = cCenter.x;
    //  
    //  throw if not precise...
    //  


    HvyDXBase::HvyGeodesic ggG;  

    ggG.e_center = std::complex<double>(tCenterX, tCenterY); // TODO:  wrong!!!! use braced initialization and include sweep direction;
    ggG.e_infinite_radius = false;
    ggG.e_radius = tRadius;

    return ggG;
}




std::complex<double> HvyDXBase::hyper_translate(std::complex<double> p_amount, std::complex<double> z)
{
    //  cf. hyperbolic translation, translation axis, translation length; 

    return (z + p_amount) / (std::complex<double>(1.00, 0.00) + z * std::conj(p_amount));
}





HvyDXBase::HvyGeodesic::HvyGeodesic()
    :
    e_infinite_radius(false),
    e_radius(1.0),
    e_center(HvyPlex(1.0, 0.0)),
    eo_sweepDirection(D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE)
{

}



HvyDXBase::HvyGeodesic::HvyGeodesic(
    bool p_infinite_radius,
    double p_radius,
    HvyDXBase::HvyPlex p_center,
    D2D1_SWEEP_DIRECTION p_sweep)
    :
    e_infinite_radius(p_infinite_radius),
    e_radius(p_radius),
    e_center(p_center),
    eo_sweepDirection(p_sweep)
{

}






HvyDXBase::HvyGeodesic::HvyGeodesic(HvyDXBase::HvyGeodesic const& g1)
{
    //  copy ctor

    this->e_infinite_radius = g1.e_infinite_radius;

    this->e_radius = g1.e_radius;

    this->e_center = g1.e_center;

    this->eo_sweepDirection = g1.eo_sweepDirection;
}




HvyDXBase::HvyGeodesic::~HvyGeodesic()
{
    ;
}













