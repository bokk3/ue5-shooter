#include "FPSAnimGraphTools.h"

#include "AnimGraphNode_BlendListByBool.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_RotationOffsetBlendSpace.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_Slot.h"
#include "AnimationGraphSchema.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimMontage.h"
#include "Animation/BlendSpace.h"
#include "Animation/Skeleton.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"

namespace FPSAnimGraphToolsPrivate
{
	/** Every node the tool adds carries this comment, so a re-run can find and replace them. */
	const TCHAR* LayerTag = TEXT("NF_WeaponLayer");

	UEdGraph* FindAnimGraph(UAnimBlueprint* AnimBlueprint)
	{
		for (UEdGraph* Graph : AnimBlueprint->FunctionGraphs)
		{
			if (Graph && Graph->GetFName() == UEdGraphSchema_K2::GN_AnimGraph)
			{
				return Graph;
			}
		}
		return nullptr;
	}

	UAnimGraphNode_Root* FindRoot(UEdGraph* Graph)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UAnimGraphNode_Root* Root = Cast<UAnimGraphNode_Root>(Node))
			{
				return Root;
			}
		}
		return nullptr;
	}

	FString DescribePins(const UEdGraphNode* Node)
	{
		TArray<FString> Parts;
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			FString Links;
			for (const UEdGraphPin* Linked : Pin->LinkedTo)
			{
				Links += FString::Printf(TEXT(" ->%s.%s"), *Linked->GetOwningNode()->GetName(), *Linked->PinName.ToString());
			}
			Parts.Add(FString::Printf(TEXT("%s%s[%s]%s"), Pin->Direction == EGPD_Input ? TEXT("in:") : TEXT("out:"),
				*Pin->PinName.ToString(), *Pin->GetDisplayName().ToString(), *Links));
		}
		return FString::Join(Parts, TEXT(" | "));
	}

	/** Finds a pin by name or display name (case-insensitive); logs the node's pins when it isn't there. */
	UEdGraphPin* FindPin(UEdGraphNode* Node, std::initializer_list<const TCHAR*> Names, EEdGraphPinDirection Direction, FString& Log)
	{
		for (const TCHAR* Name : Names)
		{
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin->Direction == Direction &&
					(Pin->PinName.ToString().Equals(Name, ESearchCase::IgnoreCase) ||
					 Pin->GetDisplayName().ToString().Equals(Name, ESearchCase::IgnoreCase)))
				{
					return Pin;
				}
			}
		}
		FString Wanted;
		for (const TCHAR* Name : Names)
		{
			Wanted += Wanted.IsEmpty() ? FString(Name) : FString(TEXT("/")) + Name;
		}
		Log += FString::Printf(TEXT("  pin %s not found on %s; pins: %s\n"), *Wanted, *Node->GetName(), *DescribePins(Node));
		return nullptr;
	}

	UEdGraphPin* FindPoseOutput(UEdGraphNode* Node, FString& Log)
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction == EGPD_Output && UAnimationGraphSchema::IsPosePin(Pin->PinType))
			{
				return Pin;
			}
		}
		Log += FString::Printf(TEXT("  no pose output on %s; pins: %s\n"), *Node->GetName(), *DescribePins(Node));
		return nullptr;
	}

	template <typename TNode, typename TSetup>
	TNode* Spawn(UEdGraph* Graph, int32 X, int32 Y, TSetup&& Setup)
	{
		FGraphNodeCreator<TNode> Creator(*Graph);
		TNode* Node = Creator.CreateNode(false);
		Node->NodePosX = X;
		Node->NodePosY = Y;
		Node->NodeComment = LayerTag;
		Setup(Node);
		Creator.Finalize();
		return Node;
	}

	UK2Node_VariableGet* SpawnVariableGet(UEdGraph* Graph, UClass* OwnerClass, FName Variable, int32 X, int32 Y, FString& Log)
	{
		const FProperty* Property = FindFProperty<FProperty>(OwnerClass, Variable);
		if (!Property)
		{
			Log += FString::Printf(TEXT("  variable %s not found on %s\n"), *Variable.ToString(), *GetNameSafe(OwnerClass));
			return nullptr;
		}
		return Spawn<UK2Node_VariableGet>(Graph, X, Y, [&](UK2Node_VariableGet* Node)
		{
			Node->VariableReference.SetFromField<FProperty>(Property, true, OwnerClass);
		});
	}

	bool Connect(UEdGraph* Graph, UEdGraphPin* From, UEdGraphPin* To, FString& Log)
	{
		if (!From || !To)
		{
			return false;
		}
		if (!Graph->GetSchema()->TryCreateConnection(From, To))
		{
			Log += FString::Printf(TEXT("  could not connect %s.%s -> %s.%s\n"), *From->GetOwningNode()->GetName(), *From->PinName.ToString(),
				*To->GetOwningNode()->GetName(), *To->PinName.ToString());
			return false;
		}
		return true;
	}

	/** Removes a previously inserted layer and reconnects the original pose to the output. */
	void RemoveExistingLayer(UAnimBlueprint* AnimBlueprint, UEdGraph* Graph, UEdGraphPin* ResultPin, FString& Log)
	{
		TArray<UEdGraphNode*> Tagged;
		UEdGraphPin* OriginalPose = nullptr;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node && Node->NodeComment == LayerTag)
			{
				Tagged.Add(Node);
				if (UAnimGraphNode_LayeredBoneBlend* Blend = Cast<UAnimGraphNode_LayeredBoneBlend>(Node))
				{
					if (UEdGraphPin* Base = FindPin(Blend, {TEXT("BasePose"), TEXT("Base Pose")}, EGPD_Input, Log); Base && Base->LinkedTo.Num() > 0)
					{
						OriginalPose = Base->LinkedTo[0];
					}
				}
			}
		}
		if (Tagged.IsEmpty())
		{
			return;
		}
		for (UEdGraphNode* Node : Tagged)
		{
			FBlueprintEditorUtils::RemoveNode(AnimBlueprint, Node, true);
		}
		Log += FString::Printf(TEXT("removed %d nodes from a previous run\n"), Tagged.Num());
		if (OriginalPose)
		{
			Connect(Graph, OriginalPose, ResultPin, Log);
		}
	}

	/** Pose + aim offset: Sequence -> RotationOffsetBlendSpace(X = yaw, Y = pitch). Returns the aim offset's pose output. */
	UEdGraphPin* BuildAimedPose(UEdGraph* Graph, UClass* Owner, UAnimSequenceBase* Pose, UBlendSpace* AimOffset,
		const FFPSWeaponLayerSettings& Settings, int32 X, int32 Y, FString& Log)
	{
		UAnimGraphNode_SequencePlayer* Sequence = Spawn<UAnimGraphNode_SequencePlayer>(Graph, X, Y,
			[&](UAnimGraphNode_SequencePlayer* Node) { Node->SetAnimationAsset(Pose); });
		UEdGraphPin* Out = FindPoseOutput(Sequence, Log);
		if (!AimOffset)
		{
			return Out;
		}
		UAnimGraphNode_RotationOffsetBlendSpace* Offset = Spawn<UAnimGraphNode_RotationOffsetBlendSpace>(Graph, X + 400, Y,
			[&](UAnimGraphNode_RotationOffsetBlendSpace* Node) { Node->SetAnimationAsset(AimOffset); });
		Connect(Graph, Out, FindPin(Offset, {TEXT("BasePose"), TEXT("Base Pose")}, EGPD_Input, Log), Log);
		if (UK2Node_VariableGet* Yaw = SpawnVariableGet(Graph, Owner, Settings.AimYawVariable, X + 150, Y + 180, Log))
		{
			Connect(Graph, Yaw->FindPin(Settings.AimYawVariable), FindPin(Offset, {TEXT("X")}, EGPD_Input, Log), Log);
		}
		if (UK2Node_VariableGet* Pitch = SpawnVariableGet(Graph, Owner, Settings.AimPitchVariable, X + 150, Y + 260, Log))
		{
			Connect(Graph, Pitch->FindPin(Settings.AimPitchVariable), FindPin(Offset, {TEXT("Y")}, EGPD_Input, Log), Log);
		}
		return FindPoseOutput(Offset, Log);
	}
}

FString UFPSAnimGraphTools::DumpAnimGraph(UAnimBlueprint* AnimBlueprint)
{
	using namespace FPSAnimGraphToolsPrivate;
	if (!AnimBlueprint)
	{
		return TEXT("null anim blueprint");
	}
	FString Out = FString::Printf(TEXT("parent=%s skeleton=%s\n"), *GetPathNameSafe(AnimBlueprint->ParentClass),
		*GetPathNameSafe(AnimBlueprint->TargetSkeleton.Get()));
	for (UEdGraph* Graph : AnimBlueprint->FunctionGraphs)
	{
		Out += FString::Printf(TEXT("graph %s (%d nodes)\n"), *GetNameSafe(Graph), Graph ? Graph->Nodes.Num() : 0);
	}
	if (UEdGraph* Graph = FindAnimGraph(AnimBlueprint))
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			FString Extra;
			if (const UAnimGraphNode_Slot* Slot = Cast<UAnimGraphNode_Slot>(Node))
			{
				Extra = FString::Printf(TEXT(" slot=%s"), *Slot->Node.SlotName.ToString());
			}
			Out += FString::Printf(TEXT("  node %s <%s> '%s'%s @(%d,%d)\n    %s\n"), *Node->GetName(), *Node->GetClass()->GetName(),
				*Node->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Extra, Node->NodePosX, Node->NodePosY, *DescribePins(Node));
		}
	}
	return Out;
}

TArray<FName> UFPSAnimGraphTools::GetMontageSlotNames(UAnimMontage* Montage)
{
	TArray<FName> Names;
	if (Montage)
	{
		for (const FSlotAnimationTrack& Track : Montage->SlotAnimTracks)
		{
			Names.Add(Track.SlotName);
		}
	}
	return Names;
}

bool UFPSAnimGraphTools::AddWeaponUpperBodyLayer(UAnimBlueprint* AnimBlueprint, const FFPSWeaponLayerSettings& Settings, FString& OutLog)
{
	using namespace FPSAnimGraphToolsPrivate;
	OutLog.Reset();
	if (!AnimBlueprint || !Settings.RiflePose)
	{
		OutLog = TEXT("need an anim blueprint and at least RiflePose");
		return false;
	}
	UEdGraph* Graph = FindAnimGraph(AnimBlueprint);
	UAnimGraphNode_Root* Root = Graph ? FindRoot(Graph) : nullptr;
	UEdGraphPin* ResultPin = Root ? FindPin(Root, {TEXT("Result")}, EGPD_Input, OutLog) : nullptr;
	if (!ResultPin)
	{
		OutLog += TEXT("no AnimGraph / Output Pose found");
		return false;
	}

	RemoveExistingLayer(AnimBlueprint, Graph, ResultPin, OutLog);
	if (ResultPin->LinkedTo.Num() == 0)
	{
		OutLog += TEXT("Output Pose has no input to layer on top of");
		return false;
	}
	UEdGraphPin* OriginalPose = ResultPin->LinkedTo[0];
	UClass* Owner = AnimBlueprint->ParentClass;
	const int32 X = Root->NodePosX;
	const int32 Y = Root->NodePosY;

	// Weapon pose branch (rifle, or pistol when bUsePistolPose).
	UEdGraphPin* WeaponPose = BuildAimedPose(Graph, Owner, Settings.RiflePose, Settings.RifleAimOffset, Settings, X - 2200, Y + 400, OutLog);
	if (Settings.PistolPose)
	{
		UEdGraphPin* PistolPose = BuildAimedPose(Graph, Owner, Settings.PistolPose, Settings.PistolAimOffset, Settings, X - 2200, Y + 800, OutLog);
		UAnimGraphNode_BlendListByBool* ByWeapon = Spawn<UAnimGraphNode_BlendListByBool>(Graph, X - 1350, Y + 550, [](UAnimGraphNode_BlendListByBool*) {});
		Connect(Graph, PistolPose, FindPin(ByWeapon, {TEXT("True Pose"), TEXT("BlendPose_0")}, EGPD_Input, OutLog), OutLog);
		Connect(Graph, WeaponPose, FindPin(ByWeapon, {TEXT("False Pose"), TEXT("BlendPose_1")}, EGPD_Input, OutLog), OutLog);
		if (UK2Node_VariableGet* UsePistol = SpawnVariableGet(Graph, Owner, Settings.UsePistolPoseVariable, X - 1600, Y + 750, OutLog))
		{
			Connect(Graph, UsePistol->FindPin(Settings.UsePistolPoseVariable),
				FindPin(ByWeapon, {TEXT("bActiveValue"), TEXT("Active Value")}, EGPD_Input, OutLog), OutLog);
		}
		WeaponPose = FindPoseOutput(ByWeapon, OutLog);
	}

	// Weapon actions (fire, reload, equip) override the hold pose but stay on the upper body.
	UAnimGraphNode_Slot* UpperSlot = Spawn<UAnimGraphNode_Slot>(Graph, X - 1000, Y + 450,
		[&](UAnimGraphNode_Slot* Node) { Node->Node.SlotName = Settings.UpperBodySlot; });
	Connect(Graph, WeaponPose, FindPin(UpperSlot, {TEXT("Source")}, EGPD_Input, OutLog), OutLog);

	// Sample locomotion below BlendRootBone, weapon layer above it.
	UAnimGraphNode_LayeredBoneBlend* Layer = Spawn<UAnimGraphNode_LayeredBoneBlend>(Graph, X - 650, Y,
		[&](UAnimGraphNode_LayeredBoneBlend* Node)
		{
			Node->Node.bMeshSpaceRotationBlend = true;
			if (Node->Node.LayerSetup.Num() > 0)
			{
				FBranchFilter Filter;
				Filter.BoneName = Settings.BlendRootBone;
				Filter.BlendDepth = 0;
				Node->Node.LayerSetup[0].BranchFilters = {Filter};
			}
		});
	Connect(Graph, OriginalPose, FindPin(Layer, {TEXT("BasePose"), TEXT("Base Pose")}, EGPD_Input, OutLog), OutLog);
	Connect(Graph, FindPoseOutput(UpperSlot, OutLog), FindPin(Layer, {TEXT("BlendPoses_0"), TEXT("Blend Poses 0")}, EGPD_Input, OutLog), OutLog);
	if (UK2Node_VariableGet* Alpha = SpawnVariableGet(Graph, Owner, Settings.WeaponLayerAlphaVariable, X - 900, Y + 250, OutLog))
	{
		Connect(Graph, Alpha->FindPin(Settings.WeaponLayerAlphaVariable),
			FindPin(Layer, {TEXT("BlendWeights_0"), TEXT("Blend Weights 0")}, EGPD_Input, OutLog), OutLog);
	}

	// Full-body actions (hit reacts, deaths) after the layer, straight into the output.
	UAnimGraphNode_Slot* FullSlot = Spawn<UAnimGraphNode_Slot>(Graph, X - 300, Y,
		[&](UAnimGraphNode_Slot* Node) { Node->Node.SlotName = Settings.FullBodySlot; });
	Connect(Graph, FindPoseOutput(Layer, OutLog), FindPin(FullSlot, {TEXT("Source")}, EGPD_Input, OutLog), OutLog);
	Connect(Graph, FindPoseOutput(FullSlot, OutLog), ResultPin, OutLog);

	if (USkeleton* Skeleton = AnimBlueprint->TargetSkeleton)
	{
		Skeleton->RegisterSlotNode(Settings.UpperBodySlot);
		Skeleton->RegisterSlotNode(Settings.FullBodySlot);
		Skeleton->MarkPackageDirty();
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(AnimBlueprint, EBlueprintCompileOptions::None, &Results);
	for (const TSharedRef<FTokenizedMessage>& Message : Results.Messages)
	{
		if (Message->GetSeverity() <= EMessageSeverity::Warning)
		{
			OutLog += TEXT("  compile: ") + Message->ToText().ToString() + TEXT("\n");
		}
	}
	const bool bOk = AnimBlueprint->Status != BS_Error && !OutLog.Contains(TEXT("not found")) && !OutLog.Contains(TEXT("could not connect"));
	OutLog += FString::Printf(TEXT("weapon layer %s (blueprint status %d)\n"), bOk ? TEXT("added") : TEXT("FAILED"), int32(AnimBlueprint->Status));
	return bOk;
}
