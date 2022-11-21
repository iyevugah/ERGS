#include "PorousFlowOrthotropicEmbeddedFracturePermeability.h"

registerMooseObject("PorousFlowApp", PorousFlowOrthotropicEmbeddedFracturePermeability);

InputParameters
PorousFlowOrthotropicEmbeddedFracturePermeability::validParams()
{
  InputParameters params = PorousFlowPermeabilityBase::validParams();
  params.addClassDescription(
    " This permeability material calculates the permeability tensor based on attributes of the "
    " Orthotropic Embedded Fractures in Zill et. al.(2021): Hydro-mechanical continuum modelling of "
    " fluid percolation through rock. The permeability is given as follows: "
    " k = k_m*I +  [b_i/a_i * (\frac{b_{i}^2}{12} - k_m)*(I-M_i)]. where i=summation Over number of "
    " fracture planes/surfaces. b is the fracture aperture given by: b_{i0} + /Delta{b_i} "
    " /Delta{b_i} depends on the strain (/epsilon) as follows "
    " /Delta{b_i} = a_i * 〈/epsilon :M_i - /epsilon_{0i}〉. Here, /epsilon_{0i} is a threshold strain of "
    " the material in each of the fracture normal vector direction. /epsilon is total strain."
    " a_i and b_i are fracture distance and fracture aperture respectively in each fracture "
    " normal vector direction. K_m is the matrix/intrinsic permeability. I_{ij} is the identity tensor"
    " and M_{ij} is a structure tensor given as n_i⊗n_i. n_i is a vector normal to each fracture plane.");

    params.addRequiredParam<std::vector<double>>("a", "Mean fracture distance value in all 3 directions");
    params.addRequiredParam<std::vector<double>>("e0", "threshold strain");
    params.addRequiredParam<Real>("km", "matrix/intrinsic permeability");
    params.addRequiredParam<Real>("b0", "initial fracture aperture");
    params.addRequiredParam<Real>("rad_xy", "fracture rotation angle in radians");
    params.addRequiredParam<Real>("rad_yz", "fracture rotation angle in radians");
    params.addParam<RealTensorValue>("n",
                           "normal vector wrt to fracture surface");
    params.addParam<std::string>("base_name",
                             "Optional parameter that allows the user to define "
                             "multiple mechanics material systems on the same "
                             "block, i.e. for multiple phases");
    params.addParam<bool>("normal_vector_to_fracture_is_constant",
                      false,
                      "Whether the normal vector wrt to fracture surface is constant/known or not.");
  return params;
}

PorousFlowOrthotropicEmbeddedFracturePermeability::PorousFlowOrthotropicEmbeddedFracturePermeability(
    const InputParameters & parameters)
  : PorousFlowPermeabilityBase(parameters),
    _a(getParam<std::vector<double>>("a")),
    _e0(getParam<std::vector<double>>("e0")),
    _b0(getParam<Real>("b0")),
    _km(getParam<Real>("km")),
    _nVec(getParam<RealTensor>("n")),
    _identity_two(RankTwoTensor::initIdentity),
    _base_name(isParamValid("base_name") ? getParam<std::string>("base_name") + "_" : ""),
    _n_const(parameters.get<bool>("normal_vector_to_fracture_is_constant")),
    _stress(getMaterialProperty<RankTwoTensor>(_base_name + "stress")),
    _rad_xy(getParam<Real>("rad_xy")),
    _rad_yz(getParam<Real>("rad_yz")),
    _strain(getMaterialProperty<RankTwoTensor>("creep_strain"))
  //  _dvol_strain_qp_dvar(_mechanical ? &getMaterialProperty<std::vector<RealGradient>>(
  //                              "dPorousFlow_total_volumetric_strain_qp_dvar")
  //                                                   : nullptr),
{

// should be included if the derivatives/jacobian of this material is computed as well.
 //  _dictator.usePermDerivs(true);

for (int j = 0; j < 3; j++)
{
  if (_a[j] = 0.0)
    mooseError("PorousFlowOrthotropicEmbeddedFracturePermeability: Mean fracture distance value"
    "in any of the 3 directions has to be greater than 0.");
}

}

void
PorousFlowOrthotropicEmbeddedFracturePermeability::computeQpProperties()
{
// This code block describes how the 'normal vector' (n) wrt each (of the 3) fracture face
// should be computed. if the components of n is known (e.g., sigma_xx, tau_xy and tau_zx),
// then it should be specify in the input file. Otherwise, n is computed as the
// direction (eigenvector) of the all the three principal stresses. The assumption here is
// that the three fracture planes lie within the principal stresses.

   if (_n_const)
       {
          RealTensorValue _n =_nVec;
       }
 // Eigenvectors were derived from the total stress obtained from the tensor mech. action.
 // Then, the eigenvector in each column, corresponding to the direction of each principal
 // stress vector was computed.

      RankTwoTensor eigvec;
      std::vector<Real> eigvals;
      _stress[_qp].symmetricEigenvaluesEigenvectors(eigvals, eigvec);
      RealTensorValue _n = eigvec;

  // To rotatate the fracture normal vectors around the Z-axis (X-Y plane) during random
  // rotation of the material,the Z-unit axis is rotated first. Hence rotating the material
  // by a magnitude. Then the magnitude of rotation is multiplied by the fracture normal
  // vector. (See Zill et al. for why material is randomly rotated)

    RankTwoTensor I = _identity_two;
    RankTwoTensor _transformation_matrix;

    _transformation_matrix(0, 0) = std::cos(_rad_xy);
    _transformation_matrix(0, 1) = std::sin(_rad_xy);
    _transformation_matrix(0, 2) = 0;
    _transformation_matrix(1, 0) = -std::sin(_rad_xy);
    _transformation_matrix(1, 1) = std::cos(_rad_xy);
    _transformation_matrix(1, 2) = 0;
    _transformation_matrix(2, 0) = 0;
    _transformation_matrix(2, 1) = 0;
    _transformation_matrix(2, 2) = 1;

    RealTensorValue rotMat_xy = _transformation_matrix * I;
  // RealTensorValue rotMat_xy = I.rotateXyPlane(_rad_xy);

  // Similarly, the magnitude of rotation is necessary to rotate the fracture normal vector
  // around  the X-axis (Y-Z plane)

   _transformation_matrix(0, 0) = 1;
   _transformation_matrix(0, 1) = 0;
   _transformation_matrix(0, 2) = 0;
   _transformation_matrix(1, 0) = 0;
   _transformation_matrix(1, 1) = std::cos(_rad_yz);
   _transformation_matrix(1, 2) = std::sin(_rad_yz);
   _transformation_matrix(2, 0) = 0;
   _transformation_matrix(2, 1) = -std::sin(_rad_yz);
   _transformation_matrix(2, 2) = std::cos(_rad_yz);

     RealTensorValue rotMat_yz = _transformation_matrix * I;
  // RealTensorValue rotMat_yz = I.rotateYzPlane(rad_yz);

 // Now, the tensor containing the fracture normal vectors is rotated using the magnitudes of rotation
    RankTwoTensor n_r = rotMat_xy * rotMat_yz * _n;

 // Finally, the permeability is computed by first initializing it.
   _permeability_qp[_qp] = _km*I;

 // Note that each column of the n_r tensor corresponds to the rotated fracture normal vector in
 // each principal stress direction. The final/total permeability is the summation over each permeability
 // corresponding to each rotated fracture normal vector:

 for (int i = 0; i < 3; i++)
 {
  // strain in the each fracture normal vector direction
   Real e_n = (_strain[_qp] * n_r.column(i))*(n_r.column(i));

  // H_de implements the macaulay-bracket in Zill et al.
   Real H_de = (e_n > _e0[i]) ? 1.0 : 0.0;

  // initial fracture aperture is sqrt(12 * k_m) in the literature
   Real _b0 = std::sqrt(12. * _km);

  // change in fracture aperture
   Real b_f = _b0 + (H_de * _a[i] * (e_n - _e0[i]));

   Real coeff = H_de * (b_f / _a[i]) * ((b_f * b_f / 12.0) - _km);

   RankTwoTensor I = _identity_two;
   auto _M = RankTwoTensor::selfOuterProduct(n_r.column(i));

   _permeability_qp[_qp] += coeff * (I - _M);
 }

 // This permeability material model does not include its derivatives (jacobian)
}