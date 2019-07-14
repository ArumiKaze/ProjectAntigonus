#include "VRPickupObject.h"
#include "MotionControllerComponent.h"
#include "Components/StaticMeshComponent.h"

//---Constructor---//
AVRPickupObject::AVRPickupObject()
{
}

//--- Pick up---//
void AVRPickupObject::Pickup(UMotionControllerComponent * parent)
{
	if (GetWorld() && parent && GetStaticMeshComponent())
	{
		GetStaticMeshComponent()->SetSimulatePhysics(false);
		FAttachmentTransformRules rules{ EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false };
		GetStaticMeshComponent()->AttachToComponent(parent, rules, FName(TEXT("")));
	}
}

//---Drop---//
void AVRPickupObject::Drop()
{
	if (GetWorld() && GetStaticMeshComponent())
	{
		GetStaticMeshComponent()->SetSimulatePhysics(true);
		FDetachmentTransformRules rules{ EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, false };
		DetachFromActor(rules);
	}
}

