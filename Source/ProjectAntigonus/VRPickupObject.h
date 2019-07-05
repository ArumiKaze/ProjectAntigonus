#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "VRPickupObject.generated.h"

UCLASS()
class PROJECTANTIGONUS_API AVRPickupObject : public AStaticMeshActor
{
	GENERATED_BODY()

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* mesh;
	
public:

	AVRPickupObject();

	//--- Pick up---//
	void Pickup(class UMotionControllerComponent *& parent);

	//---Drop---//
	void Drop();
};
