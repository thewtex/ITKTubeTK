/*=========================================================================

Library:   TubeTK

Copyright 2010 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

=========================================================================*/
#ifndef __itkDiffusiveRegistrationFilter_txx
#define __itkDiffusiveRegistrationFilter_txx

#include "itkDiffusiveRegistrationFilter.h"

#include "itkResampleImageFilter.h"
#include "itkVectorResampleImageFilter.h"

namespace itk
{

/**
 * Constructor
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
DiffusiveRegistrationFilter
< TFixedImage, TMovingImage, TDeformationField >
::DiffusiveRegistrationFilter()
{
  m_UpdateBuffer = UpdateBufferType::New();

  // We are using our own regularization, so don't use the implementation
  // provided by the PDERegistration framework.  We also want to use the image
  // spacing to calculate derivatives in physical space
  this->SmoothDeformationFieldOff();
  this->SmoothUpdateFieldOff();
  this->UseImageSpacingOn();

  // Create the registration function
  this->CreateRegistrationFunction();

  m_HighResolutionTemplate                      = 0;

  m_CurrentLevel                                = 0;
  m_IntensityDistanceWeightings.push_back( 1.0 );
  m_RegularizationWeightings.push_back( 1.0 );
}

/**
 * PrintSelf
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::PrintSelf( std::ostream& os, Indent indent ) const
{
  Superclass::PrintSelf( os, indent );

  os << indent << "Diffusion tensor images:" << std::endl;
  for( int i = 0; i < this->GetNumberOfTerms(); i++ )
    {
    if( m_DiffusionTensorImages[i] )
      {
      m_DiffusionTensorImages[i]->Print( os, indent );
      }
    }
  os << indent << "Diffusion tensor derivative images:" << std::endl;
  for( int i = 0; i < this->GetNumberOfTerms(); i++ )
    {
    if( m_DiffusionTensorDerivativeImages[i] )
      {
      m_DiffusionTensorDerivativeImages[i]->Print( os, indent );
      }
    }
  os << indent << "Deformation field component images:" << std::endl;
  for( int i = 0; i < this->GetNumberOfTerms(); i++ )
    {
    if( m_DeformationComponentImages[i] )
      {
      m_DeformationComponentImages[i]->Print( os, indent );
      }
    }
  os << indent << "Multiplication vector images:" << std::endl;
  for( int i = 0; i < this->GetNumberOfTerms(); i++ )
    {
    if( m_MultiplicationVectorImageArrays[i].Length != 0 )
      {
      for( unsigned int j = 0; j < ImageDimension; j++ )
        {
        if( m_MultiplicationVectorImageArrays[i][j] )
          {
          m_MultiplicationVectorImageArrays[i][j]->Print( os, indent );
          }
        }
      }
    }
  if( m_HighResolutionTemplate )
    {
    os << indent << "Image attribute template:" << std::endl;
    m_HighResolutionTemplate->Print( os, indent );
    }
  std::cout << indent << "Current level: " << m_CurrentLevel << std::endl;
  std::cout << indent << "Intensity distance weightings: ";
  for( unsigned int i = 0; i < m_IntensityDistanceWeightings.size(); i++ )
    {
    std::cout << m_IntensityDistanceWeightings[i] << " ";
    }
  std::cout << std::endl;
  std::cout << indent << "Regulraization weightings: ";
  for( unsigned int i = 0; i < m_RegularizationWeightings.size(); i++ )
    {
    std::cout << m_RegularizationWeightings[i] << " ";
    }
  std::cout << std::endl;
}

/**
 * Create the registration function
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::CreateRegistrationFunction()
{
  typename RegistrationFunctionType::Pointer registrationFunction
      = RegistrationFunctionType::New();
  registrationFunction->SetComputeRegularizationTerm( true );
  registrationFunction->SetComputeIntensityDistanceTerm( true );
  this->SetDifferenceFunction( static_cast<FiniteDifferenceFunctionType *>(
      registrationFunction.GetPointer() ) );
}

/**
 * Get the registration function pointer
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
typename DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::RegistrationFunctionType *
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::GetRegistrationFunctionPointer() const
{
  RegistrationFunctionType * df = dynamic_cast< RegistrationFunctionType * >
       ( this->GetDifferenceFunction().GetPointer() );
  return df;
}

/**
 * Helper function to allocate space for an image given a template image
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
  template < class UnallocatedImagePointer, class TemplateImagePointer >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::AllocateSpaceForImage( UnallocatedImagePointer& image,
                         const TemplateImagePointer& templateImage ) const
{
  assert( image );
  assert( templateImage );
  image->SetOrigin( templateImage->GetOrigin() );
  image->SetSpacing( templateImage->GetSpacing() );
  image->SetDirection( templateImage->GetDirection() );
  image->SetLargestPossibleRegion( templateImage->GetLargestPossibleRegion() );
  image->SetRequestedRegion( templateImage->GetRequestedRegion() );
  image->SetBufferedRegion( templateImage->GetBufferedRegion() );
  image->Allocate();
}

/**
 * Helper function to check whether the attributes of an image matches template
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
template < class CheckedImageType, class TemplateImageType >
bool
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::CompareImageAttributes( const CheckedImageType * image,
                          const TemplateImageType * templateImage ) const
{
  assert( image );
  assert( templateImage );
  return image->GetOrigin() == templateImage->GetOrigin()
      && image->GetSpacing() == templateImage->GetSpacing()
      && image->GetDirection() == templateImage->GetDirection()
      && image->GetLargestPossibleRegion()
          == templateImage->GetLargestPossibleRegion()
      && image->GetLargestPossibleRegion().GetIndex()
          == templateImage->GetLargestPossibleRegion().GetIndex()
      && image->GetLargestPossibleRegion().GetSize()
          == templateImage->GetLargestPossibleRegion().GetSize();
}

/**
 * Resample an image to match a template
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
template< class ResampleImagePointer, class TemplateImagePointer >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::ResampleImageNearestNeighbor(
    const ResampleImagePointer & highResolutionImage,
    const TemplateImagePointer & templateImage,
    ResampleImagePointer & resampledImage ) const
{
  // We have to implement nearest neighbors by hand, since we are dealing with
  // pixel types that do not have Numeric Traits
  typedef typename ResampleImagePointer::ObjectType ResampleImageType;

  // Create the resized resampled image
  resampledImage = ResampleImageType::New();
  this->AllocateSpaceForImage( resampledImage, templateImage );

  // Do NN interpolation
  typedef itk::ImageRegionIteratorWithIndex< ResampleImageType >
      ResampleImageRegionType;
  ResampleImageRegionType resampledImageIt = ResampleImageRegionType(
      resampledImage, resampledImage->GetLargestPossibleRegion() );

  typename ResampleImageType::PointType physicalPoint;
  physicalPoint.Fill( 0.0 );
  typename ResampleImageType::IndexType highResolutionIndex;
  highResolutionIndex.Fill( 0.0 );
  typename ResampleImageType::PixelType pixelValue;

  for( resampledImageIt.GoToBegin();
       !resampledImageIt.IsAtEnd();
       ++resampledImageIt )
    {
    resampledImage->TransformIndexToPhysicalPoint(
        resampledImageIt.GetIndex(), physicalPoint );
    highResolutionImage->TransformPhysicalPointToIndex(
        physicalPoint, highResolutionIndex );
    pixelValue = highResolutionImage->GetPixel( highResolutionIndex );
    resampledImageIt.Set( pixelValue );
    }
}

/**
 * Resample an image to match a template
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
template< class ResampleImagePointer, class TemplateImagePointer >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::ResampleImageLinear( const ResampleImagePointer & highResolutionImage,
                       const TemplateImagePointer & templateImage,
                       ResampleImagePointer & resampledImage ) const
{
  // Do linear interpolation
  typedef itk::ResampleImageFilter
      < typename ResampleImagePointer::ObjectType,
        typename ResampleImagePointer::ObjectType > ResampleFilterType;
  typename ResampleFilterType::Pointer resampler = ResampleFilterType::New();
  resampler->SetInput( highResolutionImage );
  resampler->SetOutputParametersFromImage( templateImage );
  resampler->Update();
  resampledImage = resampler->GetOutput();
}

/**
 * Resample a vector image to match a template
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
template< class VectorResampleImagePointer, class TemplateImagePointer >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::VectorResampleImageLinear(
    const VectorResampleImagePointer & highResolutionImage,
    const TemplateImagePointer & templateImage,
    VectorResampleImagePointer & resampledImage,
    bool normalize ) const
{
  // Do linear interpolation
  typedef itk::VectorResampleImageFilter
      < typename VectorResampleImagePointer::ObjectType,
        typename VectorResampleImagePointer::ObjectType > ResampleFilterType;
  typename ResampleFilterType::Pointer resampler = ResampleFilterType::New();
  resampler->SetInput( highResolutionImage );
  resampler->SetOutputOrigin( templateImage->GetOrigin() );
  resampler->SetOutputSpacing( templateImage->GetSpacing() );
  resampler->SetOutputDirection( templateImage->GetDirection() );
  resampler->SetOutputStartIndex(
      templateImage->GetLargestPossibleRegion().GetIndex() );
  resampler->SetSize( templateImage->GetLargestPossibleRegion().GetSize() );
  resampler->Update();
  resampledImage = resampler->GetOutput();

  if( normalize )
    {
    this->NormalizeVectorField( resampledImage );
    }
}

/**
 * Normalizes a vector field to ensure each vector has length 1
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
template< class VectorImagePointer >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::NormalizeVectorField( VectorImagePointer & image ) const
{
  DeformationVectorImageRegionType vectorIt(
      image, image->GetLargestPossibleRegion() );
  for( vectorIt.GoToBegin(); !vectorIt.IsAtEnd(); ++vectorIt )
    {
    vectorIt.Value().Normalize();
    }
}

/**
 * Allocate space for the update buffer
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::AllocateUpdateBuffer()
{
  // The update buffer looks just like the output and holds the voxel changes
  typename OutputImageType::Pointer output = this->GetOutput();
  assert( output );
  this->AllocateSpaceForImage( m_UpdateBuffer, output );
}

/**
 * All other initialization done before the initialize iteration / calculate
 * change / apply update loop
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::Initialize()
{
  Superclass::Initialize();

  // Ensure we have a fixed image and moving image
  if( !this->GetFixedImage() || !this->GetMovingImage() )
    {
    itkExceptionMacro( << "Fixed image and/or moving image not set" );
    }

  // Ensure we have good intensity distance and regularization weightings
  unsigned int intensityWeightingsSize = m_IntensityDistanceWeightings.size();
  unsigned int regularizationWeightingsSize = m_RegularizationWeightings.size();
  if( intensityWeightingsSize == 0 || regularizationWeightingsSize == 0 )
    {
    itkExceptionMacro( << "Intensity distance and/or regularization weightings "
                       << "not set" );
    }
  if( intensityWeightingsSize != regularizationWeightingsSize )
    {
    itkWarningMacro( << "Number of intensity distance weightings does not "
                     << "equal number of regularization weightings" );
    }

  // Update the current multiresolution level (when registering, level is 1..N)
  m_CurrentLevel++;

  // Check the timestep for stability if we are using the diffusive or
  // anisotropic diffusive regularization terms
  RegistrationFunctionType * df = this->GetRegistrationFunctionPointer();
  assert( df );
  df->CheckTimeStepStability( this->GetInput(), this->GetUseImageSpacing() );

  // Assert that we have a deformation field, and that its image attributes
  // match the fixed image
  assert( this->GetDeformationField() );
  if( !this->CompareImageAttributes( this->GetDeformationField(),
                                     this->GetFixedImage() ) )
    {
    itkExceptionMacro( << "Deformation field attributes do not match fixed "
                       << "image" );
    }

  // Update the registration function's deformation field
  df->SetDeformationField( this->GetDeformationField() );

  // Set the intensity distance and regularization weightings to the
  // registration function.  If we are past the end of the vectors, use the
  // last element.
  if( m_CurrentLevel <= intensityWeightingsSize )
    {
    df->SetIntensityDistanceWeighting(
        m_IntensityDistanceWeightings[m_CurrentLevel - 1] );
    }
  else
    {
    df->SetIntensityDistanceWeighting( m_IntensityDistanceWeightings.back() );
    }
  if( m_CurrentLevel <= regularizationWeightingsSize )
    {
    df->SetRegularizationWeighting(
        m_RegularizationWeightings[m_CurrentLevel - 1] );
    }
  else
    {
    df->SetRegularizationWeighting( m_RegularizationWeightings.back() );
    }

  // Allocate and initialize the images we will use to store data computed
  // during the registration (or set pointers to 0 if they are not being used).
  this->AllocateImageMembers();

  // Compute the diffusion tensors and their derivatives
  if( this->GetComputeRegularizationTerm() )
    {
    this->InitializeDeformationComponentAndDerivativeImages();
    this->ComputeDiffusionTensorImages();
    this->ComputeDiffusionTensorDerivativeImages();
    this->ComputeMultiplicationVectorImages();
    }
}

/**
 * Allocate the images we will use to store data computed during the
 * registration
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::AllocateImageMembers()
{
  assert( this->GetOutput() );

  // The output will be used as the template to allocate the images we will
  // use to store data computed before/during the registration
  typename OutputImageType::Pointer output = this->GetOutput();

  int numTerms = this->GetNumberOfTerms();

  // Allocate the diffusion tensor images and their derivatives
  // If we are not computing a regularization term, the image arrays will be
  // filled with '0' pointers
  DiffusionTensorImagePointer diffusionTensorPointer = 0;
  TensorDerivativeImagePointer tensorDerivativePointer = 0;
  for( int i = 0; i < numTerms; i++ )
    {
    if( this->GetComputeRegularizationTerm() )
      {
      diffusionTensorPointer = DiffusionTensorImageType::New();
      this->AllocateSpaceForImage( diffusionTensorPointer, output );
      tensorDerivativePointer = TensorDerivativeImageType::New();
      this->AllocateSpaceForImage( tensorDerivativePointer, output );
      }
    if( (int) m_DiffusionTensorImages.size() < numTerms )
      {
      m_DiffusionTensorImages.push_back( diffusionTensorPointer );
      m_DiffusionTensorDerivativeImages.push_back( tensorDerivativePointer );
      }
    else
      {
      m_DiffusionTensorImages[i] = diffusionTensorPointer;
      m_DiffusionTensorDerivativeImages[i] = tensorDerivativePointer;
      }
    }

  // Initialize image pointers that may or may not be allocated by individual
  // filters later on, namely deformation derivatives and multiplication vectors
  for( int i = 0; i < numTerms; i++ )
    {
    if( (int) m_DeformationComponentImages.size() < numTerms )
      {
      m_DeformationComponentImages.push_back( 0 );
      }
    else
      {
      m_DeformationComponentImages[i] = 0;
      }

    ScalarDerivativeImageArrayType deformationComponentFirstArray;
    TensorDerivativeImageArrayType deformationComponentSecondArray;
    DeformationVectorImageArrayType multiplicationVectorArray;
    for( int j = 0; j < ImageDimension; j++ )
      {
      deformationComponentFirstArray[j] = 0;
      deformationComponentSecondArray[j] = 0;
      multiplicationVectorArray[j] = 0;
      }
    if( (int) m_DeformationComponentFirstOrderDerivativeArrays.size()
      < numTerms )
      {
      m_DeformationComponentFirstOrderDerivativeArrays.push_back(
          deformationComponentFirstArray );
      }
    else
      {
      m_DeformationComponentFirstOrderDerivativeArrays[i]
          = deformationComponentFirstArray;
      }
    if( (int) m_DeformationComponentSecondOrderDerivativeArrays.size()
      < numTerms )
      {
      m_DeformationComponentSecondOrderDerivativeArrays.push_back(
          deformationComponentSecondArray );
      }
    else
      {
      m_DeformationComponentSecondOrderDerivativeArrays[i]
          = deformationComponentSecondArray;
      }
    if( (int) m_MultiplicationVectorImageArrays.size() < numTerms )
      {
      m_MultiplicationVectorImageArrays.push_back( multiplicationVectorArray );
      }
    else
      {
      m_MultiplicationVectorImageArrays[i] = multiplicationVectorArray;
      }
    }
}

/**
 * Initialize the deformation component images and their derivatives
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::InitializeDeformationComponentAndDerivativeImages()
{
  assert( this->GetOutput() );
  assert( this->GetComputeRegularizationTerm() );

  // The output will be used as the template to allocate the images we will
  // use to store data computed before/during the registration
  typename OutputImageType::Pointer output = this->GetOutput();

  // Setup pointer to the deformation component image - we have only one
  // component, which is the entire deformation field
  m_DeformationComponentImages[GAUSSIAN] = output;

  // Setup the first and second order deformation component images
  for( int i = 0; i < ImageDimension; i++ )
    {
    m_DeformationComponentFirstOrderDerivativeArrays[GAUSSIAN][i]
        = ScalarDerivativeImageType::New();
    this->AllocateSpaceForImage(
        m_DeformationComponentFirstOrderDerivativeArrays[GAUSSIAN][i], output );

    m_DeformationComponentSecondOrderDerivativeArrays[GAUSSIAN][i]
        = TensorDerivativeImageType::New();
    this->AllocateSpaceForImage(
        m_DeformationComponentSecondOrderDerivativeArrays[GAUSSIAN][i],
        output );
    }
}

/**
 * Updates the diffusion tensor image before each run of the registration
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::ComputeDiffusionTensorImages()
{
  assert( this->GetComputeRegularizationTerm() );
  assert( m_DiffusionTensorImages[GAUSSIAN] );

  // For the Gaussian regularization, we only need to set the
  // diffusion tensors to the identity
  typename DiffusionTensorImageType::PixelType identityTensor;
  identityTensor.SetIdentity();
  m_DiffusionTensorImages[GAUSSIAN]->FillBuffer( identityTensor );
}

/**
 * Updates the diffusion tensor image derivatives before each run of the
 * registration
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::ComputeDiffusionTensorDerivativeImages()
{
  assert( this->GetComputeRegularizationTerm() );
  assert( this->GetOutput() );

  // Get the spacing and radius
  SpacingType spacing = this->GetOutput()->GetSpacing();
  const RegistrationFunctionType * df = this->GetRegistrationFunctionPointer();
  const typename OutputImageType::SizeType radius = df->GetRadius();

  // Compute the diffusion tensor derivative images
  for( int i = 0; i < this->GetNumberOfTerms(); i++ )
    {
    this->ComputeDiffusionTensorDerivativeImageHelper(
        m_DiffusionTensorImages[i], i, spacing, radius );
    }
}

/**
 * Actually computes the diffusion tensor derivative images
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::ComputeDiffusionTensorDerivativeImageHelper(
    const DiffusionTensorImagePointer & tensorImage,
    int term,
    const SpacingType & spacing,
    const typename OutputImageType::SizeType & radius )
{
  assert( tensorImage );

  TensorDerivativeImagePointer tensorDerivativeImage
      = m_DiffusionTensorDerivativeImages[term];
  assert( tensorDerivativeImage );

  // Get the FiniteDifferenceFunction to use in calculations.
  const RegistrationFunctionType * df = this->GetRegistrationFunctionPointer();
  assert( df );
  typename RegularizationFunctionType::ConstPointer reg
      = df->GetRegularizationFunctionPointer();
  assert( reg );

  // Setup the structs for the face calculations, the face iterators, and the
  // iterators over the current face
  FaceStruct< DiffusionTensorImagePointer > tensorStruct(
      tensorImage, tensorImage->GetLargestPossibleRegion(), radius );
  DiffusionTensorNeighborhoodType tensorNeighborhood;

  FaceStruct< TensorDerivativeImagePointer > tensorDerivativeStruct(
      tensorDerivativeImage,
      tensorDerivativeImage->GetLargestPossibleRegion(),
      radius );
  TensorDerivativeImageRegionType tensorDerivativeRegion;

  for( tensorStruct.GoToBegin(), tensorDerivativeStruct.GoToBegin();
       !tensorDerivativeStruct.IsAtEnd();
       tensorStruct.Increment(), tensorDerivativeStruct.Increment() )
    {
    // Set the neighborhood iterators to the current face
    tensorStruct.SetIteratorToCurrentFace(
        tensorNeighborhood, tensorImage, radius );
    tensorDerivativeStruct.SetIteratorToCurrentFace(
        tensorDerivativeRegion, tensorDerivativeImage );

    // Iterate through the neighborhood for this face and compute derivatives
    for( tensorNeighborhood.GoToBegin(), tensorDerivativeRegion.GoToBegin();
         !tensorNeighborhood.IsAtEnd();
         ++tensorNeighborhood, ++tensorDerivativeRegion )
      {
      reg->ComputeDiffusionTensorFirstOrderPartialDerivatives(
          tensorNeighborhood, tensorDerivativeRegion, spacing );
      }
    }
}

/**
 * Update x, y, z components of a deformation field
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::ExtractXYZComponentsFromDeformationField(
    const OutputImageType * deformationField,
    DeformationComponentImageArrayType& deformationComponentImages ) const
{
  assert( deformationField );

  typename VectorIndexSelectionFilterType::Pointer indexSelector;
  for( unsigned int i = 0; i < ImageDimension; i++ )
    {
    indexSelector = VectorIndexSelectionFilterType::New();
    indexSelector->SetInput( deformationField );
    indexSelector->SetIndex( i );
    deformationComponentImages[i] = indexSelector->GetOutput();
    indexSelector->Update();
    }
}

/**
 * Calculates the derivatives of the deformation vector derivatives after
 * each iteration.
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::ComputeDeformationComponentDerivativeImages()
{
  assert( this->GetComputeRegularizationTerm() );
  assert( this->GetOutput() );

  // Get the spacing and the radius
  SpacingType spacing = this->GetOutput()->GetSpacing();
  const RegistrationFunctionType * df = this->GetRegistrationFunctionPointer();
  typename OutputImageType::SizeType radius = df->GetRadius();

  // By default, calculate the derivatives for each of the deformation
  // components
  DeformationComponentImageArrayType deformationComponentImageArray;
  deformationComponentImageArray.Fill( 0 );

  for( int i = 0; i < this->GetNumberOfTerms(); i++ )
    {
    this->ExtractXYZComponentsFromDeformationField(
        m_DeformationComponentImages[i], deformationComponentImageArray );

    for( int j = 0; j < ImageDimension; j++ )
      {
      this->ComputeDeformationComponentDerivativeImageHelper(
          deformationComponentImageArray[j], i, j, spacing, radius );
      }
    }
}

/**
 * Actually computes the deformation component image derivatives.
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::ComputeDeformationComponentDerivativeImageHelper(
    DeformationVectorComponentImagePointer & deformationComponentImage,
    int term,
    int dimension,
    SpacingType & spacing,
    typename OutputImageType::SizeType & radius )
{
  assert( deformationComponentImage );

  // Set up for multithreaded processing.
  ComputeDeformationComponentDerivativeImageHelperThreadStruct str;
  str.Filter = this;
  str.DeformationComponentImage = deformationComponentImage;
  str.Term = term;
  str.Dimension = dimension;
  str.Spacing = spacing;
  str.Radius = radius;

  // Multithread the execution
  this->GetMultiThreader()->SetNumberOfThreads( this->GetNumberOfThreads() );
  this->GetMultiThreader()->SetSingleMethod(
      this->ComputeDeformationComponentDerivativeImageHelperThreaderCallback,
      & str );
  this->GetMultiThreader()->SingleMethodExecute();

  // Explicitly call Modified on the first- and second-order partial derviative
  // image affected here, since
  // ThreadedComputeDeformationComponentDerivativeImageHelper changes them
  // through iterators which do not increment their timestamp
  m_DeformationComponentFirstOrderDerivativeArrays[term][dimension]->Modified();
  m_DeformationComponentSecondOrderDerivativeArrays[term][dimension]
      ->Modified();
}

/**
 * Calls ThreadedComputeDeformationComponentDerivativeImageHelper for processing
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
ITK_THREAD_RETURN_TYPE
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::ComputeDeformationComponentDerivativeImageHelperThreaderCallback( void *arg )
{
  int threadId = ((MultiThreader::ThreadInfoStruct *)(arg))->ThreadID;
  int threadCount = ((MultiThreader::ThreadInfoStruct *)(arg))->NumberOfThreads;

  ComputeDeformationComponentDerivativeImageHelperThreadStruct * str
      = (ComputeDeformationComponentDerivativeImageHelperThreadStruct *)
            (((MultiThreader::ThreadInfoStruct *)(arg))->UserData);

  // Execute the actual method with appropriate output region
  // first find out how many pieces extent can be split into.
  // Using the SplitRequestedRegion method from itk::ImageSource.
  ThreadDeformationVectorComponentImageRegionType
      splitDeformationVectorComponentRegion;
  int total = str->Filter->SplitRequestedRegion(
      threadId, threadCount, splitDeformationVectorComponentRegion );

  ThreadScalarDerivativeImageRegionType splitScalarDerivativeRegion;
  int scalarTotal = str->Filter->SplitRequestedRegion(
      threadId, threadCount, splitScalarDerivativeRegion );

  ThreadTensorDerivativeImageRegionType splitTensorDerivativeRegion;
  int tensorTotal = str->Filter->SplitRequestedRegion(
      threadId, threadCount, splitTensorDerivativeRegion );

  // Make sure we could split all of the images equally
  assert( total == scalarTotal );
  assert( total == tensorTotal );

  if( threadId < total )
    {
    str->Filter->ThreadedComputeDeformationComponentDerivativeImageHelper(
        str->DeformationComponentImage,
        splitDeformationVectorComponentRegion,
        splitScalarDerivativeRegion,
        splitTensorDerivativeRegion,
        str->Term,
        str->Dimension,
        str->Spacing,
        str->Radius );
    }

  return ITK_THREAD_RETURN_VALUE;
}

/**
 * Does the actual work of computing the deformation component image derivatives
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::ThreadedComputeDeformationComponentDerivativeImageHelper(
    const DeformationVectorComponentImagePointer & deformationComponentImage,
    const ThreadDeformationVectorComponentImageRegionType
      & deformationVectorComponentRegionToProcess,
    const ThreadScalarDerivativeImageRegionType
      & scalarDerivativeRegionToProcess,
    const ThreadTensorDerivativeImageRegionType
      & tensorDerivativeRegionToProcess,
    int term,
    int dimension,
    const SpacingType & spacing,
    const typename OutputImageType::SizeType & radius ) const
{
  ScalarDerivativeImagePointer firstOrderDerivativeImage
      = m_DeformationComponentFirstOrderDerivativeArrays[term][dimension];
  assert( firstOrderDerivativeImage );
  TensorDerivativeImagePointer secondOrderDerivativeImage
      = m_DeformationComponentSecondOrderDerivativeArrays[term][dimension];
  assert( secondOrderDerivativeImage );

  // Get the FiniteDifferenceFunction to use in calculations.
  const RegistrationFunctionType * df = this->GetRegistrationFunctionPointer();
  assert( df );
  typename RegularizationFunctionType::ConstPointer reg
      = df->GetRegularizationFunctionPointer();
  assert( reg );

  // Setup the structs for the face calculations, the face iterators, and the
  // iterators over the current face
  FaceStruct< DeformationVectorComponentImagePointer >
      deformationComponentStruct (
          deformationComponentImage,
          deformationVectorComponentRegionToProcess,
          radius );
  DeformationVectorComponentNeighborhoodType deformationComponentNeighborhood;

  FaceStruct< ScalarDerivativeImagePointer > firstOrderStruct (
      firstOrderDerivativeImage, scalarDerivativeRegionToProcess, radius );
  ScalarDerivativeImageRegionType firstOrderRegion;

  FaceStruct< TensorDerivativeImagePointer > secondOrderStruct (
      secondOrderDerivativeImage, tensorDerivativeRegionToProcess, radius );
  TensorDerivativeImageRegionType secondOrderRegion;

  for( deformationComponentStruct.GoToBegin(), firstOrderStruct.GoToBegin(),
       secondOrderStruct.GoToBegin();
       !deformationComponentStruct.IsAtEnd();
       deformationComponentStruct.Increment(), firstOrderStruct.Increment(),
       secondOrderStruct.Increment() )
    {
    // Set the neighborhood iterators to the current face
    deformationComponentStruct.SetIteratorToCurrentFace(
        deformationComponentNeighborhood, deformationComponentImage, radius );
    firstOrderStruct.SetIteratorToCurrentFace(
        firstOrderRegion, firstOrderDerivativeImage );
    secondOrderStruct.SetIteratorToCurrentFace(
        secondOrderRegion, secondOrderDerivativeImage );

    // Iterate through the neighborhood for this face and compute derivatives
    for( deformationComponentNeighborhood.GoToBegin(),
         firstOrderRegion.GoToBegin(), secondOrderRegion.GoToBegin();
         !deformationComponentNeighborhood.IsAtEnd();
         ++deformationComponentNeighborhood, ++firstOrderRegion,
         ++secondOrderRegion )
      {
      reg->ComputeIntensityFirstAndSecondOrderPartialDerivatives(
          deformationComponentNeighborhood,
          firstOrderRegion,
          secondOrderRegion,
          spacing );
      }
    }
}

/**
 * Initialize the state of the filter and equation before each iteration.
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::InitializeIteration()
{
  assert( this->GetOutput() );
  Superclass::InitializeIteration();

  // Update the deformation field component images
  // Since the components depend on the current deformation field, they must be
  // computed on every registration iteration
  if( this->GetComputeRegularizationTerm() )
    {
    this->UpdateDeformationComponentImages();
    this->ComputeDeformationComponentDerivativeImages();
    }
}

/**
 * Populates the update buffer
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
typename DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::TimeStepType
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::CalculateChange()
{
  // Set up for multithreaded processing.
  DenseFDThreadStruct str;
  str.Filter = this;
  // Not used during the calculate change step
  str.TimeStep = NumericTraits< TimeStepType >::Zero;
  this->GetMultiThreader()->SetNumberOfThreads( this->GetNumberOfThreads() );
  this->GetMultiThreader()->SetSingleMethod(
      this->CalculateChangeThreaderCallback, & str );

  // Initialize the list of time step values that will be generated by the
  // various threads.  There is one distinct slot for each possible thread,
  // so this data structure is thread-safe.
  int threadCount = this->GetMultiThreader()->GetNumberOfThreads();

  str.TimeStepList = new TimeStepType[threadCount];
  str.ValidTimeStepList = new bool[threadCount];
  for ( int i = 0; i < threadCount; ++i )
    {
    str.ValidTimeStepList[i] = false;
    }

  // Multithread the execution
  this->GetMultiThreader()->SingleMethodExecute();

  // Resolve the single value time step to return
  TimeStepType dt = this->ResolveTimeStep( str.TimeStepList,
                                           str.ValidTimeStepList,
                                           threadCount);
  delete [] str.TimeStepList;
  delete [] str.ValidTimeStepList;

  // Explicitly call Modified on m_UpdateBuffer here, since
  // ThreadedCalculateChange changes this buffer through iterators which do not
  // increment the update buffer timestamp
  this->m_UpdateBuffer->Modified();

  return dt;
}

/**
 * Calls ThreadedCalculateChange for processing
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
ITK_THREAD_RETURN_TYPE
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::CalculateChangeThreaderCallback( void * arg )
{
  int threadId = ((MultiThreader::ThreadInfoStruct *)(arg))->ThreadID;
  int threadCount = ((MultiThreader::ThreadInfoStruct *)(arg))->NumberOfThreads;

  DenseFDThreadStruct * str = (DenseFDThreadStruct *)
            (((MultiThreader::ThreadInfoStruct *)(arg))->UserData);

  // Execute the actual method with appropriate output region
  // first find out how many pieces extent can be split into.
  // Using the SplitRequestedRegion method from itk::ImageSource.
  ThreadRegionType splitRegion;
  int total = str->Filter->SplitRequestedRegion( threadId,
                                                 threadCount,
                                                 splitRegion );

  ThreadDiffusionTensorImageRegionType splitTensorRegion;
  int tensorTotal = str->Filter->SplitRequestedRegion( threadId,
                                                       threadCount,
                                                       splitTensorRegion );

  ThreadTensorDerivativeImageRegionType splitTensorDerivativeRegion;
  int tensorDerivativeTotal = str->Filter->SplitRequestedRegion(
      threadId, threadCount, splitTensorDerivativeRegion );

  ThreadScalarDerivativeImageRegionType splitScalarDerivativeRegion;
  int scalarDerivativeTotal = str->Filter->SplitRequestedRegion(
      threadId, threadCount, splitScalarDerivativeRegion );

  // Make sure we could split all of the images equally
  assert( total == tensorTotal );
  assert( total == tensorDerivativeTotal );
  assert( total == scalarDerivativeTotal );

  if (threadId < total)
    {
    str->TimeStepList[threadId] = str->Filter->ThreadedCalculateChange(
      splitRegion,
      splitTensorRegion,
      splitTensorDerivativeRegion,
      splitScalarDerivativeRegion,
      threadId);
    str->ValidTimeStepList[threadId] = true;
    }

  return ITK_THREAD_RETURN_VALUE;
}

/**
 * Inherited from superclass - do not call
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
typename DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::TimeStepType
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::ThreadedCalculateChange( const ThreadRegionType &, int)
{
  // This function should never be called!
  itkExceptionMacro( << "ThreadedCalculateChange(regionToProcess, threadId) "
                     << "should never be called.  Use the other "
                     << "ThreadedCalculateChange function instead" );
}

/**
 * Does the actual work of calculating change over a region supplied by the
 * multithreading mechanism
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
typename DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::TimeStepType
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::ThreadedCalculateChange(
    const ThreadRegionType & regionToProcess,
    const ThreadDiffusionTensorImageRegionType & tensorRegionToProcess,
    const ThreadTensorDerivativeImageRegionType &
      tensorDerivativeRegionToProcess,
    const ThreadScalarDerivativeImageRegionType &
      scalarDerivativeRegionToProcess,
    int)
{
  // Get the FiniteDifferenceFunction to use in calculations.
  RegistrationFunctionType * df = this->GetRegistrationFunctionPointer();
  assert( df );
  // Ask the function object for a pointer to a data structure it
  // will use to manage any global values it needs.  We'll pass this
  // back to the function object at each calculation and then
  // again so that the function object can use it to determine a
  // time step for this iteration.
  void * globalData = df->GetGlobalDataPointer();

  // Get the radius and output
  const typename OutputImageType::SizeType radius = df->GetRadius();
  OutputImagePointer output = this->GetOutput();

  // Break the input into a series of regions.  The first region is free
  // of boundary conditions, the rest with boundary conditions.  We operate
  // on the output region because the input has been copied to the output.

  // Setup the types of structs for the face calculations
  // (Struct handles the case where the image pointer doesn't exist)
  FaceStruct< OutputImagePointer > outputStruct(
      output, regionToProcess, radius );
  NeighborhoodType outputNeighborhood;

  UpdateBufferRegionType updateRegion;

  FaceStruct< DiffusionTensorImagePointer > tensorStruct(
      m_DiffusionTensorImages, tensorRegionToProcess, radius );
  DiffusionTensorNeighborhoodVectorType tensorNeighborhoods;

  FaceStruct< ScalarDerivativeImagePointer >
      deformationComponentFirstOrderStruct(
          m_DeformationComponentFirstOrderDerivativeArrays,
          scalarDerivativeRegionToProcess,
          radius );
  ScalarDerivativeImageRegionArrayVectorType
      deformationComponentFirstOrderRegionArrays;

  FaceStruct< TensorDerivativeImagePointer >
      deformationComponentSecondOrderStruct(
          m_DeformationComponentSecondOrderDerivativeArrays,
          tensorDerivativeRegionToProcess,
          radius );
  TensorDerivativeImageRegionArrayVectorType
      deformationComponentSecondOrderRegionArrays;

  FaceStruct< TensorDerivativeImagePointer > tensorDerivativeStruct(
      m_DiffusionTensorDerivativeImages,
      tensorDerivativeRegionToProcess,
      radius );
  TensorDerivativeImageRegionVectorType tensorDerivativeRegions;

  FaceStruct< DeformationFieldPointer > multiplicationVectorStruct(
      m_MultiplicationVectorImageArrays, regionToProcess, radius );
  DeformationVectorImageRegionArrayVectorType multiplicationVectorRegionArrays;

  // Get the type of registration
  bool computeRegularization = this->GetComputeRegularizationTerm();

  // Go to the first face
  outputStruct.GoToBegin();
  if( computeRegularization )
    {
    tensorStruct.GoToBegin();
    deformationComponentFirstOrderStruct.GoToBegin();
    deformationComponentSecondOrderStruct.GoToBegin();
    tensorDerivativeStruct.GoToBegin();
    multiplicationVectorStruct.GoToBegin();
    }

  // Iterate over each face
  while( !outputStruct.IsAtEnd() )
    {
    // Set the neighborhood iterators to the current face
    outputStruct.SetIteratorToCurrentFace( outputNeighborhood, output, radius );
    outputStruct.SetIteratorToCurrentFace( updateRegion, m_UpdateBuffer );
    if( computeRegularization )
      {
      tensorStruct.SetIteratorToCurrentFace(
          tensorNeighborhoods, m_DiffusionTensorImages, radius );
      deformationComponentFirstOrderStruct.SetIteratorToCurrentFace(
          deformationComponentFirstOrderRegionArrays,
          m_DeformationComponentFirstOrderDerivativeArrays );
      deformationComponentSecondOrderStruct.SetIteratorToCurrentFace(
          deformationComponentSecondOrderRegionArrays,
          m_DeformationComponentSecondOrderDerivativeArrays );
      tensorDerivativeStruct.SetIteratorToCurrentFace(
          tensorDerivativeRegions, m_DiffusionTensorDerivativeImages );
      multiplicationVectorStruct.SetIteratorToCurrentFace(
          multiplicationVectorRegionArrays,
          m_MultiplicationVectorImageArrays );
      }

    // Go to the beginning of the neighborhood for this face
    outputNeighborhood.GoToBegin();
    updateRegion.GoToBegin();
    if( computeRegularization )
      {
      for( int i = 0; i < this->GetNumberOfTerms(); i++ )
        {
        tensorNeighborhoods[i].GoToBegin();
        tensorDerivativeRegions[i].GoToBegin();
        for( unsigned int j = 0; j < ImageDimension; j++ )
          {
          deformationComponentFirstOrderRegionArrays[i][j].GoToBegin();
          deformationComponentSecondOrderRegionArrays[i][j].GoToBegin();
          multiplicationVectorRegionArrays[i][j].GoToBegin();
          }
        }
      }

    // Iterate through the neighborhood for this face and compute updates
    while( !outputNeighborhood.IsAtEnd() )
      {
      // Compute updates
      updateRegion.Value() = df->ComputeUpdate(
          outputNeighborhood,
          tensorNeighborhoods,
          deformationComponentFirstOrderRegionArrays,
          deformationComponentSecondOrderRegionArrays,
          tensorDerivativeRegions,
          multiplicationVectorRegionArrays,
          globalData );

      // Go to the next neighborhood
      ++outputNeighborhood;
      ++updateRegion;
      if( computeRegularization )
        {
        for( int i = 0; i < this->GetNumberOfTerms(); i++ )
          {
          ++tensorNeighborhoods[i];
          ++tensorDerivativeRegions[i];
          for( unsigned int j = 0; j < ImageDimension; j++ )
            {
            ++deformationComponentFirstOrderRegionArrays[i][j];
            ++deformationComponentSecondOrderRegionArrays[i][j];
            if( multiplicationVectorRegionArrays[i][j].GetImage() )
              {
              ++multiplicationVectorRegionArrays[i][j];
              }
            }
          }
        }
      }

    // Go to the next face
    outputStruct.Increment();
    if( computeRegularization )
      {
      tensorStruct.Increment();
      tensorDerivativeStruct.Increment();
      deformationComponentFirstOrderStruct.Increment();
      deformationComponentSecondOrderStruct.Increment();
      multiplicationVectorStruct.Increment();
      }
    }

  // Ask the finite difference function to compute the time step for
  // this iteration.  We give it the global data pointer to use, then
  // ask it to free the global data memory.
  TimeStepType timeStep = df->ComputeGlobalTimeStep(globalData);
  df->ReleaseGlobalDataPointer(globalData);
  return timeStep;
}

/**
 * Applies changes from the update buffer to the output
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::ApplyUpdate(TimeStepType dt)
{
  // Set up for multithreaded processing.
  DenseFDThreadStruct str;
  str.Filter = this;
  str.TimeStep = dt;
  this->GetMultiThreader()->SetNumberOfThreads( this->GetNumberOfThreads() );
  this->GetMultiThreader()->SetSingleMethod(
    this->ApplyUpdateThreaderCallback, & str);

  // Multithread the execution
  this->GetMultiThreader()->SingleMethodExecute();

  // Explicitely call Modified on GetOutput here, since ThreadedApplyUpdate
  // changes this buffer through iterators which do not increment the
  // output timestamp
  this->GetOutput()->Modified();

  RegistrationFunctionType * df = this->GetRegistrationFunctionPointer();
  assert( df );
  this->SetRMSChange( df->GetRMSChange() );
}

/**
 * Calls ThreadedApplyUpdate, need to reimplement here to also split the
 * diffusion tensor image
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
ITK_THREAD_RETURN_TYPE
DiffusiveRegistrationFilter
  < TFixedImage, TMovingImage, TDeformationField >
::ApplyUpdateThreaderCallback( void * arg )
{
  int threadId = ((MultiThreader::ThreadInfoStruct *)(arg))->ThreadID;
  int threadCount = ((MultiThreader::ThreadInfoStruct *)(arg))->NumberOfThreads;

  DenseFDThreadStruct * str = (DenseFDThreadStruct *)
            (((MultiThreader::ThreadInfoStruct *)(arg))->UserData);

  // Execute the actual method with appropriate output region
  // first find out how many pieces extent can be split into.
  // Using the SplitRequestedRegion method from itk::ImageSource.
  int total;
  ThreadRegionType splitRegion;
  total = str->Filter->SplitRequestedRegion( threadId,
                                             threadCount,
                                             splitRegion );

  if (threadId < total)
    {
    str->Filter->ThreadedApplyUpdate(str->TimeStep, splitRegion, threadId );
    }

  return ITK_THREAD_RETURN_VALUE;
}

/**
 * Does the actual work of updating the output from the UpdateContainer
 * over an output region supplied by the multithreading mechanism.
 */
template < class TFixedImage, class TMovingImage, class TDeformationField >
void
DiffusiveRegistrationFilter
< TFixedImage, TMovingImage, TDeformationField >
::ThreadedApplyUpdate(TimeStepType dt,
  const ThreadRegionType &regionToProcess, int )
{
  UpdateBufferRegionType  u( m_UpdateBuffer, regionToProcess );
  OutputImageRegionType   o( this->GetOutput(), regionToProcess );

  for( u = u.Begin(), o = o.Begin();
       !u.IsAtEnd();
       ++o, ++u )
    {
    o.Value() += static_cast< DeformationVectorType >( u.Value() * dt );
    // no adaptor support here
    }
}

} // end namespace itk

#endif
