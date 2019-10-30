
#pragma once 




namespace HvyDXBase
{


    struct HvyGeodesic
    {
    public:
        HvyGeodesic(); 

        HvyGeodesic(bool p_infinite_radius, double p_radius, HvyDXBase::HvyPlex p_center, D2D1_SWEEP_DIRECTION p_sweep); 

        HvyGeodesic(HvyDXBase::HvyGeodesic const& g1); 

        ~HvyGeodesic();

    public:
        bool                            e_infinite_radius;
        double                          e_radius;
        std::complex<double>            e_center;
        D2D1_SWEEP_DIRECTION            eo_sweepDirection;
    };



    double CircumradiusFromSchlafliCOSH(int schlafP, int schlafQ);


    double ApothemFromSchlafli(int schlafP, int schlafQ);


    HvyGeodesic MakeGeodesicFromSchlafli(unsigned int schlafliP, unsigned int schlafliQ);


    std::complex<double> hyper_translate(std::complex<double> p_amount, std::complex<double> p_z);

}
//  Closes namespace HvyDXBase;




