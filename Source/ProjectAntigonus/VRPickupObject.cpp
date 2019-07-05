#include "VRPickupObject.h"
#include "MotionControllerComponent.h"
#include "Components/StaticMeshComponent.h"

AVRPickupObject::AVRPickupObject()
{
	mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
}

void AVRPickupObject::Pickup(UMotionControllerComponent *& parent)
{
	mesh->SetSimulatePhysics(false);
	FAttachmentTransformRules rules{ EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false };
	RootComponent->AttachToComponent(parent, rules, FName(TEXT("")));
}

void AVRPickupObject::Drop()
{
	mesh->SetSimulatePhysics(true);
	FDetachmentTransformRules rules{ EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, false };
	DetachFromActor(rules);
}

