// Copyright Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionLandscapeTwoLayerCoords.generated.h"

UENUM()
enum ETerrainTwoCoordMappingType : int
{
	TCMT_TwoAuto,
	TCMT_TwoXY,
	TCMT_TwoXZ,
	TCMT_TwoYZ,
	TCMT_TwoMAX,
};

UENUM()
enum ELandscapeCustomizedTwoCoordType : int
{
	/** Don't use customized UV, just use original UV. */
	LCCT_TwoNone,
	LCCT_TwoCustomUV0,
	LCCT_TwoCustomUV1,
	LCCT_TwoCustomUV2,
	/** Use original WeightMapUV, which could not be customized. */
	LCCT_TwoWeightMapUV,
	LCCT_TwoMAX,
};

UCLASS(collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionLandscapeTwoLayerCoords : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	/** Determines the mapping place to use on the terrain. */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionLandscapeTwoLayerCoords)
	TEnumAsByte<enum ETerrainTwoCoordMappingType> MappingType;

	/** Determines the mapping place to use on the terrain. */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionLandscapeTwoLayerCoords)
	TEnumAsByte<enum ELandscapeCustomizedTwoCoordType> CustomUVType;

	/** Uniform scale to apply to the mapping. */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionLandscapeTwoLayerCoords)
	float MappingScale;

	/** Rotation to apply to the mapping. */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionLandscapeTwoLayerCoords)
	float MappingRotation;

	/** Offset to apply to the mapping along U. */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionLandscapeTwoLayerCoords)
	float MappingPanU;

	/** Offset to apply to the mapping along V. */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionLandscapeTwoLayerCoords)
	float MappingPanV;

	//~ Begin UMaterialExpression Interface
#if WITH_EDITOR
	LANDSCAPE_API virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	LANDSCAPE_API virtual void GetCaption(TArray<FString>& OutCaptions) const override;

	virtual bool GenerateHLSLExpression(FMaterialHLSLGenerator& Generator, UE::HLSLTree::FScope& Scope, int32 OutputIndex, UE::HLSLTree::FExpression const*& OutExpression) const override;
#endif
	//~ End UMaterialExpression Interface
};



