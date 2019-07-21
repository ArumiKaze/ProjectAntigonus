#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "VRPickupObject.generated.h"

UCLASS()
class PROJECTANTIGONUS_API AVRPickupObject : public AStaticMeshActor
{
	GENERATED_BODY()
	
public:

	//---Constructor---//
	AVRPickupObject();

	//--- Pick up---//
	void Pickup(class UMotionControllerComponent *& parent);

	//---Drop---//
	void Drop();
};
