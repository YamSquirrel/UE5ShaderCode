/*
//++[NEXT] 2 layer blend landscape material [6/17/2020 windywang]

#include "Materials/MaterialExpressionLandscapeTwoLayerBlend.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "MaterialCompiler.h"
#if WITH_EDITOR
#include "MaterialGraph/MaterialGraphNode.h"
#endif

#define LOCTEXT_NAMESPACE "Landscape"

UMaterialExpressionLandscapeTwoLayerBlend::UMaterialExpressionLandscapeTwoLayerBlend(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	struct FConstructorStatics
	{
		FText NAME_Landscape;
		FConstructorStatics()
			: NAME_Landscape(LOCTEXT("Landscape", "Landscape"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

#if WITH_EDITORONLY_DATA

	MenuCategories.Add(ConstructorStatics.NAME_Landscape);

	Outputs.Reset();
	Outputs.Add(FExpressionOutput(TEXT("Two Layer")));
	Outputs.Add(FExpressionOutput(TEXT("Total Weight")));

	bShowOutputNameOnPin = true;

#endif
}

void UMaterialExpressionLandscapeTwoLayerBlend::GetAllParameterInfo(TArray<FMaterialParameterInfo> &OutParameterInfo, TArray<FGuid> &OutParameterIds, const FMaterialParameterInfo& InBaseParameterInfo) const
{
	for (int32 LayerIdx = 0; LayerIdx < Layers.Num(); LayerIdx++)
	{
		int32 CurrentSize = OutParameterInfo.Num();
		FMaterialParameterInfo NewParameter(Layers[LayerIdx], InBaseParameterInfo.Association, InBaseParameterInfo.Index);
		OutParameterInfo.AddUnique(NewParameter);

		if (CurrentSize != OutParameterInfo.Num())
		{
			OutParameterIds.Add(ExpressionGUID);
		}
	}
}

#if WITH_EDITOR

void UMaterialExpressionLandscapeTwoLayerBlend::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty)
	{
		const FName PropertyName = PropertyChangedEvent.MemberProperty->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UMaterialExpressionLandscapeTwoLayerBlend, Layers))
		{
			LayerWeightInputs.SetNum(Layers.Num());

			if (UMaterialGraphNode* MatGraphNode = Cast<UMaterialGraphNode>(GraphNode))
			{
				MatGraphNode->RecreateAndLinkNode();
			}
		}
	}
}

void UMaterialExpressionLandscapeTwoLayerBlend::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(FString(TEXT("2 Layer Blend")));
}

uint32 UMaterialExpressionLandscapeTwoLayerBlend::GetInputType(int32 InputIndex)
{
	if (LayerWeightInputs.IsValidIndex(InputIndex))
	{
		return MCT_Float;
	}
	return MCT_Unknown;
}

FName UMaterialExpressionLandscapeTwoLayerBlend::GetInputName(int32 InputIndex) const
{
	if (LayerWeightInputs.IsValidIndex(InputIndex) && Layers.IsValidIndex(InputIndex))
	{
		return Layers[InputIndex];
	}
	return NAME_None;
}

const TArray<FExpressionInput*> UMaterialExpressionLandscapeTwoLayerBlend::GetInputs()
{
	TArray<FExpressionInput*> OutInputs;
	for (int32 i = 0; i < LayerWeightInputs.Num(); i++)
	{
		OutInputs.Add(&LayerWeightInputs[i]);
	}
	return OutInputs;
}

FExpressionInput* UMaterialExpressionLandscapeTwoLayerBlend::GetInput(int32 InputIndex)
{
	if (LayerWeightInputs.IsValidIndex(InputIndex))
	{
		return &LayerWeightInputs[InputIndex];
	}
	return nullptr;
}

uint32 UMaterialExpressionLandscapeTwoLayerBlend::GetOutputType(int32 OutputIndex)
{
	switch (OutputIndex)
	{
	case 0:
		return MCT_Float4;
	case 1:
		return MCT_Float;
	default:
		return MCT_Unknown;
	}
}

int32 UMaterialExpressionLandscapeTwoLayerBlend::Compile(FMaterialCompiler* Compiler, int32 OutputIndex)
{
	struct LayerWeightData
	{
		int32 Index;
		FName LayerName;
		int32 WeightCode;
	};
	TArray<LayerWeightData> ActivedLayerWeights;
	TSet<FName> ActivedLayers;

	for (int32 LayerIdx = 0; LayerIdx < Layers.Num(); LayerIdx++)
	{
		FName LayerName = Layers[LayerIdx];
		if (!ActivedLayers.Contains(LayerName))
		{
			int32 WeightCode = INDEX_NONE;
			if (LayerWeightInputs[LayerIdx].Expression)
			{
				WeightCode = LayerWeightInputs[LayerIdx].Compile(Compiler);
			}
			if (WeightCode == INDEX_NONE)
			{
				WeightCode = Compiler->StaticTerrainLayerWeight(LayerName, INDEX_NONE);
			}
			if (WeightCode != INDEX_NONE)
			{
				ActivedLayerWeights.Add(LayerWeightData{ LayerIdx, LayerName, WeightCode });
				ActivedLayers.Add(LayerName);
			}
		}
	}

	if (OutputIndex == 0)
	{
		int32 LayerAIndexCode = INDEX_NONE;
		int32 LayerAWeightCode = INDEX_NONE;
		int32 LayerBIndexCode = INDEX_NONE;
		int32 LayerBWeightCode = INDEX_NONE;

		if (ActivedLayers.Num() == 0)
		{
			LayerAIndexCode = Compiler->Constant(0.0f);
			LayerAWeightCode = Compiler->Constant(1.0f);
			LayerBIndexCode = Compiler->Constant(0.0f);
			LayerBWeightCode = Compiler->Constant(0.0f);
		}
		else if (ActivedLayers.Num() == 1)
		{
			LayerAIndexCode = Compiler->Constant(ActivedLayerWeights[0].Index);
			LayerAWeightCode = ActivedLayerWeights[0].WeightCode;
			LayerBIndexCode = Compiler->Constant(ActivedLayerWeights[0].Index);
			LayerBWeightCode = Compiler->Constant(0.0f);
		}
		else if (ActivedLayers.Num() == 2)
		{
			LayerAIndexCode = Compiler->Constant(ActivedLayerWeights[0].Index);
			LayerAWeightCode = ActivedLayerWeights[0].WeightCode;
			LayerBIndexCode = Compiler->Constant(ActivedLayerWeights[1].Index);
			LayerBWeightCode = ActivedLayerWeights[1].WeightCode;
		}
		else
		{
			//Create Sort Function
			{
				FString InputParamsDeclList;
				for (int32 Idx = 0; Idx < ActivedLayers.Num(); Idx++)
				{
					if (Idx > 0)
					{
						InputParamsDeclList += TEXT(", ");
					}
					InputParamsDeclList += FString::Printf(TEXT("MaterialFloat w%d, MaterialFloat i%d"), Idx, Idx);
				}

				FString FuncBody;
				FuncBody += TEXT("MaterialFloat4 r = w1 > w0 ? MaterialFloat4(w1, i1, w0, i0) : MaterialFloat4(w0, i0, w1, i1);\r\n");

				for (int32 Idx = 2; Idx < ActivedLayerWeights.Num() - 1; Idx++)
				{
					FuncBody += FString::Printf(TEXT("r.zw = w%d > r.z ? MaterialFloat2(w%d, i%d) : r.zw;\r\n"), Idx, Idx, Idx);
					FuncBody += TEXT("r = r.z > r.x ? r.zwxy : r;\r\n\r\n");
				}
				// keep the sample order by index to fix seam of texture linear sample
				int32 LastIndex = ActivedLayerWeights.Num() - 1;
				FuncBody += FString::Printf(TEXT("r.zw = w%d > r.z ? MaterialFloat2(w%d, i%d) : r.zw;\r\n"), LastIndex, LastIndex, LastIndex);
				FuncBody += TEXT("r = r.y > r.w ? r.zwxy : r;\r\n\r\n");

				FuncBody += TEXT("return r;\r\n");

				FString FuncCode = FString::Printf(TEXT("MaterialFloat4 SortBlendLayers%d(%s)\r\n{\r\n%s\r\n}\r\n"), ActivedLayers.Num(), *InputParamsDeclList, *FuncBody);

				bool IsImplemented = false;
				TArray<FString>& CustomExpressionImplementations = Compiler->GetCustomExpressionImplementations();
				for (int32 CodeIdx = 0; CodeIdx < CustomExpressionImplementations.Num(); CodeIdx++)
				{
					if (CustomExpressionImplementations[CodeIdx] == FuncCode)
					{
						IsImplemented = true;
						break;
					}
				}
				if (!IsImplemented)
				{
					CustomExpressionImplementations.Add(FuncCode);
				}
			}

			FString CallInputParamList;
			for (int32 Idx = 0; Idx < ActivedLayerWeights.Num(); Idx++)
			{
				if (Idx > 0)
				{
					CallInputParamList += TEXT(", ");
				}
				CallInputParamList += FString::Printf(TEXT("%s, %d.0"),
					*Compiler->GetParameterCode(ActivedLayerWeights[Idx].WeightCode),
					ActivedLayerWeights[Idx].Index);
			}

			return Compiler->AddCodeChunk(
				MCT_Float4,
				*FString::Printf(TEXT("SortBlendLayers%d(%s)"),
					ActivedLayerWeights.Num(),
					*CallInputParamList));
		}

		int32 ResultCode = Compiler->AddCodeChunk(
			MCT_Float4,
			*FString::Printf(TEXT("MaterialFloat4(%s, %s, %s, %s)"),
				*Compiler->GetParameterCode(LayerAWeightCode),
				*Compiler->GetParameterCode(LayerAIndexCode),
				*Compiler->GetParameterCode(LayerBWeightCode),
				*Compiler->GetParameterCode(LayerBIndexCode)));

		return ResultCode;
	}
	else if (OutputIndex == 1)
	{
		if (ActivedLayers.Num() == 0)
		{
			return Compiler->Constant(1.0f);
		}

		int32 SumWeightCode = ActivedLayerWeights[0].WeightCode;
		for (int32 Idx = 1; Idx < ActivedLayerWeights.Num(); Idx++)
		{
			SumWeightCode = Compiler->Add(SumWeightCode, ActivedLayerWeights[Idx].WeightCode);
		}

		return SumWeightCode;
	}

	return INDEX_NONE;
}

#endif


#undef LOCTEXT_NAMESPACE

//--[NEXT]

*/