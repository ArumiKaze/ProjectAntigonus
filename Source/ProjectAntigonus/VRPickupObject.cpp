#include "VRPickupObject.h"
#include "MotionControllerComponent.h"
#include "Components/StaticMeshComponent.h"

AVRPickupObject::AVRPickupObject()
{
	//mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
}

void AVRPickupObject::Pickup(UMotionControllerComponent *& parent)
{
	if (parent && GetStaticMeshComponent())
	{
		GetStaticMeshComponent()->SetSimulatePhysics(false);
		FAttachmentTransformRules rules{ EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false };
		GetStaticMeshComponent()->AttachToComponent(parent, rules, FName(TEXT("")));
	}
}

void AVRPickupObject::Drop()
{
	GetStaticMeshComponent()->SetSimulatePhysics(true);
	FDetachmentTransformRules rules{ EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, false };
	DetachFromActor(rules);
}

