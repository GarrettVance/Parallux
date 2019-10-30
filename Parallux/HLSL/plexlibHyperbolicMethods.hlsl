//      
//      
//  ghv 20190409:  HLSL methods for complex numbers and hyperbolic geometry;
//      
//      
 
 
static const float konst_pi = 3.1415926535897932384626433;


float2 plexlibComplexConjugate(float2 a)
{
    return float2(
        a.x,
        -a.y
    );
}


float plexlibSquaredNorm(float2 a)
{ 
    return a.x * a.x + a.y * a.y;
}


float plexlibAbs(float2 a)
{
    return sqrt(plexlibSquaredNorm(a)); 
}


float plexlibArg(float2 a)
{
    // ghv: use atan2() rather than plain atan();

    return atan2(a.y, a.x); 
}


float2 plexlibMulComplex(float2 a, float2 b) 
{
    return float2(
        a.x * b.x - a.y * b.y, 
        a.x * b.y + a.y * b.x
    );
}


float2 plexlibMulBisComplex(float2 a, float2 b) 
{ 
    //  return the product  a times (conjugate of b) (Arnaud Cheritat);

    return float2(
         a.x * b.x + a.y * b.y, 
        -a.x * b.y + a.y * b.x
    );
}


float2 plexlibDivComplex(float2 a, float2 b) 
{ 
    //  Return the quotient  a / b of two complex numbers. 
    //    
    //  NYI: check for divide-by-zero;
    //  For the moment, compile in VisualStudio 
    //  with Floating Point Model set to "precise". 
    //  
    //  Refer to Visual Studio property pages for 
    //  C++ --> Code Generation --> Floating Point Model
    //  and use /fp:precise.  
    //  

    float d = plexlibSquaredNorm(b);  

    return float2(
        (a.x * b.x + a.y * b.y) / d, 
        (-a.x * b.y + a.y * b.x) / d
    );
}


float2 plexlibHyperTranslate(float2 p_amount, float2 p_z) 
{
    //  The hyperbolic translation: see Caroline Series
    //  re translation length and axis;

    return plexlibDivComplex(
        p_z + p_amount, 
        float2(1.0, 0.0) + plexlibMulBisComplex(p_z, p_amount)
    );
}


float2 plexlibPolar(float modulus, float phaseAngle)
{
    return float2(
        modulus * cos(phaseAngle), 
        modulus * sin(phaseAngle)
    );
}


float2 plexlibIsometryOfInversion(float p_apothem, float2 z)
{
    //  Reflect across the edge opposite vertex 0 at origin;

    float2 u = plexlibHyperTranslate(
        float2(-p_apothem, 0.0), 
        z
    );

    //  Reflect d0 across the imaginary axis: 
    //  (Note the signum).

    float2 v = -plexlibComplexConjugate(u);

    float2 reflected = plexlibHyperTranslate(
        float2(p_apothem, 0.0), 
        v
    );

    return reflected;
}


bool plexlibInsideFDTStrict(float p_apothem, uint schlafliP, float2 z)
{
    bool insideSector = (
        (plexlibArg(z) < konst_pi / schlafliP) &&
        (plexlibArg(z) > 0.00));

    float2 w = plexlibHyperTranslate(
        float2(-p_apothem, 0.0), 
        z
    );

    bool insideGeodesicArc = (w.x < 0.00);

    return insideSector && insideGeodesicArc;
}


float2 plexlibFloorSector(uint schlafliP, float2 z_input)
{
    float2 z_return = z_input;

    float s = plexlibArg(z_input) * schlafliP / (2.f * konst_pi);

    //  
    //  TODO:  attribution = Arnaud Cheritat. 
    //  ======================================
    //  Rotate z_input until it resides inside
    //  the sector -pi/p < theta < pi/p.
    //  Visualize this sector as straddling 
    //  the positive real axis and having 
    //  TOTAL included angle 2pi/p. 
    //      

    if (plexlibAbs(s) > 0.5)
    {
        int n = (int)floor(0.5 + s);

        float angle1 = -n * 2 * (float)konst_pi / schlafliP;

        z_return = plexlibMulComplex(
            z_input,
            plexlibPolar(1.f, angle1)
        );
    }

    return z_return;
}



bool plexlibInsideCircleNonStrict(float3 aCircle, float2 somePoint)
{
    //  Any circle can be stored in a float3 by encoding as: 

    float2 center = float2(aCircle.x, aCircle.y);   
    float radius = aCircle.z; 

    float q = plexlibAbs(somePoint - center);

    return (q <= radius);
}



