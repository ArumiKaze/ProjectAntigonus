#include "VRPickupObject.h"
#include "MotionControllerComponent.h"
#include "Components/StaticMeshComponent.h"

AVRPickupObject::AVRPickupObject()
{
	//mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
}

void AVRPickupObject::Pickup(UMotionControllerComponent * parent)
{
	if (GetWorld() && parent && GetStaticMeshComponent())
	{
		GetStaticMeshComponent()->SetSimulatePhysics(false);
		FAttachmentTransformRules rules{ EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false };
		GetStaticMeshComponent()->AttachToComponent(parent, rules, FName(TEXT("")));
	}
	/*
	UWorld* world{ GetWorld() };
	if (world != nullptr)
	{
		if (parent != nullptr)
		{
			UStaticMeshComponent* component{ GetStaticMeshComponent() };
			if (component != nullptr)
			{
				GetStaticMeshComponent()->SetSimulatePhysics(false);
				FAttachmentTransformRules rules{ EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false };
				GetStaticMeshComponent()->AttachToComponent(parent, rules, FName(TEXT("")));
			}
		}
	}
	*/
}

void AVRPickupObject::Drop()
{
	if (GetWorld() && GetStaticMeshComponent())
	{
		GetStaticMeshComponent()->SetSimulatePhysics(true);
		FDetachmentTransformRules rules{ EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, false };
		DetachFromActor(rules);
	}
}

